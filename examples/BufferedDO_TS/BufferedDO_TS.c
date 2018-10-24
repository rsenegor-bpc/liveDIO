/*****************************************************************************/
/*                    Buffered digital output example                        */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a digital      */  
/*  buffered update. Each channel is a 16-bit port on the board.             */
/*                                                                           */
/*  The buffered digital input only works with PD2-DIO boards.               */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2012 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"
#include "pd_dsp_ct.h"

#include "ParseParams.h"


//===========================================================================
//
//  Data format:
//
//  Every command sent to the board contains eight 
//  32-bit DWORDS, 8 most significant bits of every DWORD is 
//  always ignored, adjusting to the DSP56301 24-bit format.
//  8 DWORD-sized data block contains three parts :
//      - control data for the DSP Timer2 (see pd_ct_def.h for more details)
//      - output data for the DIO ports
//      - execution control flags
//
//  Execution algorithm:
//      - TS board executes one entry at the time
//      - when entry is loaded, TMR2 is programmed directly using first 2 DWORDS
//      - digital outputs updated at the same time with ~ 30nS delay from each other
//      - next entry executes at the time when TMR2 will finish counting
//        command just loaded 
//
//  0..............................................................................255
//  31  23   0 31 23   0 31   15 0 31   15 0 31   15 0 31   15 0 31    40 31 23    0
//  |**|MMMMMM|**|NNNNNN|****|DDDD|****|XXXX|****|KKKK|****|LLLL|*******F|**|EEEEEE| 
//
//  MMMMMM   - DSP Timer2 Compare Register (TCPR), 
//  NNNNNN   - DSP Timer2 Control/Status Register (TCSR),
//  DDDD     - output data for DIO Port0 (DIO lines 15..0)
//  XXXX     - output data for DIO Port1 (DIO lines 31..16)
//  KKKK     - output data for DIO Port2 (DIO lines 47..32)
//  LLLL     - output data for DIO Port3 (DIO lines 63..48)
//  F        - Time Sequencer Flags (bit0-bit3): 
//              a) bit0: - Time Sequencer Interrupt Enable (TSIE). 
//                         If this bit is set, a host interrupt is generated when the 
//                         entry's instructions have executed (50IRQs/second limitation)
//              b) bit1: - Last Entry (TSLE).   
//                         There is a Last Entry in a Time Sequence if this bit is set.
//                         The first entry will begin to run after the current entry(auto-regenerate N entries).
//                         This feature is make sense only in regenerate mode from on-board memory (2K or 64K).
//              c) bit2: - unused.
//              d) bit3: - unused.
//  EEEEEE   - entry Id - entry Id is used when interrupt is generated at
//             the end of the entry execution in order to determine 
//             which entry causes interrupt
//  x        - unused bits.
//
//  NOTE: Description of the DSP Timer found in the "DSP56301 User 
//        Manual. 24-Bit Digital Signal Processor" in Motorola web 
//        site  ( http://e-www.motorola.com/brdata/PDFDB/docs/DSP56301UM.pdf)
//
//===========================================================================
//1) Possible output modes:
//    - ACB with driver-driven autoregeneration of any number of entries.
//    - Regenerate from on-board memory. If up to 256 or 8192 entries
//      fit onto the on-board memory, an output pattern can be
//      regenerated continuously by the board without any host involvement. 
//      The last entry requires a Last Entry flag to be set. Note that
//      upon reaching the last entry, there is a short delay before the
//      the first entry begins to be replayed again.
//    - One-shot. Board outputs one or more time-sequencer entries and 
//      stops at last entry. User application may be notified if the 
//      interrupt request flag is set for that  last entry.
//2) Internal clock is supported (see flag: AOB_CVSTART0).
//   External clock is supported in combination with any mode.
//   When you use external clock for a Time sequence (see flag AOB_CVSTART1),
//   The clock should be connected to pin TMR2 on DIO-STP terminal.
//3) External start/stop trigger is supported in combination with any mode 
//   See flags: AOB_STARTTRIG0, AOB_STARTTRIG1, AOB_STOPTRIG0, AOB_STOPTRIG1.
//4) Both internal 2Kx24 (256 entries) with DMA read (see flag AOB_DMARD)
//   and 64Kx24 (8192 entries) external memory (AOB_EXTM) is supported.
//=========================================================================== 


typedef enum _state
{
    closed,
    unconfigured,
    configured,
    running
} tState;

typedef struct _bufferedDoData
{
    int abort;                    // set to TRUE to abort generation
    HANDLE handle;                // board handle


    int board;
    int adapterType;              // adapter type, should be atMF or atPD2AO
    int nbOfChannels;             // number of channels
    int nbOfPointsPerChannel;     // number of samples per channel
    int nbOfFrames;
    DWORD doCfg;                  // Digital Output configuration
    unsigned char outPorts;       // mask that selects the ports of 16 lines that
                                  // will be used for digital output
    double updateRate;            // sampling frequency on each channel

    int preScalerFreq;            // pre-scaler, used to scale the frequency down.
                                  // Increase the pre-scaler freq, if you want to 
                                  // increase the time between updates.
    
    tState state;                 // state of the acquisition session
    unsigned int *rawBuffer;      // address of the buffer allocated by the driver to store the data
} tBufferedDoData;


int InitBufferedDOTS(tBufferedDoData *pDoData);
int BufferedDOTS(tBufferedDoData *pDoData);
void CleanUpBufferedDOTS(tBufferedDoData *pDoData);


static tBufferedDoData G_DoData;
static int bLastEntryFlagDetected=FALSE;
static double dBASECL = 50000000.0;			// assuming 100MHz DSP


//---------------------------------------------------------------------------
//Calculate and write data in Timer Load Register (TCPR) and  Timer Control/Status Register (TCSR)   
//---------------------------------------------------------------------------
void CalcWriteVal(float chTimeVal, int preScalerFreq, int useExtClk, DWORD* pdwTCPR, DWORD *pdwTCSR)
{
    double dobVal;
    double dobVal1;
    
    
    *pdwTCPR = 0; 
    *pdwTCSR = 0;
    *pdwTCSR =  M_TE | M_TCIE | M_TC0 | M_TRM; 

    if (useExtClk == TRUE)
       *pdwTCSR |= M_TC1;                                  // Use External clock for DSP Counter/Timer2
    
    if (chTimeVal<(0xFFFFFF-1)/dBASECL)
    {                                                       // 0.58s for 66MHz DSP/0.335s for 100MHz DSP
        dobVal=chTimeVal/(((0xFFFFFF-1)/dBASECL)/0xFFFFFF); // 3.03e-8s for 66MHz DSP/1.99e-8s for 100MHz DSP      
    }
    else
    {   
        switch(preScalerFreq)
        {
        case 0:     printf("\nCalcWriteVal: Error. Time not in valid range.\n\n"); exit(2);
        case 1:     dobVal1=0.001   ; break;
        case 2:     dobVal1=0.0001  ; break;          
        case 3:     dobVal1=0.00001 ; break;
        case 4:     dobVal1=0.000001; break;
        default:    printf("\nError. \n\n"); exit(2);
        }        
        dobVal=chTimeVal/dobVal1;         
        *pdwTCSR |= M_PCE;         //  Use prescaler 
    }
    
    *pdwTCPR = (DWORD)dobVal-1; 
    if (*pdwTCPR > 0x00FFFFFF)
    {
      printf("\nCalcWriteVal: Error. Time not in valid range. Check DataFile.\n\n");
        exit(2);
    }   
}

//---------------------------------------------------------------------------
//Fill the buffer
//
// This function reads data from a text file
// The file contains one time sequence per line, the format of a time sequence
// is:
// index   sequence_duration_in_secs   port0_state   port1_state   port2_state   port3_state   IRQA_state   last_entry_flag
//---------------------------------------------------------------------------
void PutDataIntoBufferTSEQ(tBufferedDoData* pDoData, DWORD dwDOBufferSize)
{
    DWORD* pdwB = NULL;            
    DWORD j = 0; 
    int   iResult;  
    float f;
    float totalTime = 0.0;
    int   totalEntries = 0;
    DWORD dwIRQA=0;
    DWORD dwLastEntry=0;
    DWORD dwTCPR;
    DWORD dwTCSR;
    FILE* fp;
    int   useExtClk = ((pDoData->doCfg&AOB_CVSTART1)==AOB_CVSTART1);

    pdwB = (DWORD*)pDoData->rawBuffer;
    
    fp = fopen("DataFile.txt", "rt");   // Open file
    
    if(fp == NULL)
    {
        printf("Data file not found\n");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0L,SEEK_SET);             // Set pointer to beginning of file   
    
    // Reading file and putting data into the buffer      
    for (j=0;j < (dwDOBufferSize); j=j+8)
    {                
        iResult = fscanf(fp,"%d",(pdwB+j+7));  // # entry
        iResult = fscanf(fp,"%f",&f);          // Time interval
        CalcWriteVal (f, pDoData->preScalerFreq, useExtClk, &dwTCPR, &dwTCSR);      // Time interval
        *(pdwB+j+0)= dwTCPR;
        *(pdwB+j+1)= dwTCSR;                              
        iResult = fscanf(fp,"%x",(pdwB+j+2));  // Data for DIO Port0         
        iResult = fscanf(fp,"%x",(pdwB+j+3));  // Data for DIO Port1
        iResult = fscanf(fp,"%x",(pdwB+j+4));  // Data for DIO Port2
        iResult = fscanf(fp,"%x",(pdwB+j+5));  // Data for DIO Port3
        iResult = fscanf(fp,"%x",&dwIRQA);     // Read flag "Enable/Disable interrupt"
        iResult = fscanf(fp,"%x",&dwLastEntry);// Read flag "Last Entry"
        totalEntries += (iResult==-1 || bLastEntryFlagDetected)? 0:1;
        bLastEntryFlagDetected=bLastEntryFlagDetected | (BOOL)dwLastEntry;
        *(pdwB+j+6) = (dwLastEntry<<1) | dwIRQA;
        
        if (iResult==-1)
        {
            // The rest of the buffer is filled with constant values
            // (if file size < PowerDAQ buffer size).
            // Time Sequencing will not work if the buffer is not filled.
            *(pdwB+j+7)=0xFFFF;
			f = 0.001f;                 
            CalcWriteVal (f, pDoData->preScalerFreq, useExtClk, &dwTCPR, &dwTCSR);  // Time interval
            *(pdwB+j+0)= dwTCPR; 
            *(pdwB+j+1)= dwTCSR;                                          
            *(pdwB+j+2)=0xFFFF;                       // Data for DIO Port0
            *(pdwB+j+3)=0xFFFF;                       // Data for DIO Port1
            *(pdwB+j+4)=0xFFFF;                       // Data for DIO Port2
            *(pdwB+j+5)=0xFFFF;                       // Data for DIO Port3
            *(pdwB+j+6)=0;                            // enable interrupt
        }         

		totalTime = totalTime + f;
    }

   printf("Total time for this sequence is %f s. Total active entries is %d of %d.\n", totalTime, totalEntries, j/8);
    
    fclose(fp); // Close file
    
    return;    
}


int InitBufferedDOTS(tBufferedDoData *pDoData)
{
    int retVal = 0;
    Adapter_Info adaptInfo;
    
    retVal = _PdGetAdapterInfo(pDoData->board, &adaptInfo);
    if (retVal < 0)
    {
        printf("BufferedDO: _PdGetAdapterInfo error %d. Could not obtain adapter type.\n", retVal);
        exit(EXIT_FAILURE);
    }
    
    pDoData->adapterType = adaptInfo.atType;
    if(adaptInfo.atType & atPD2DIO)
        printf("This is a PD2-DIO board.\n");
    else
    {
        printf("This board is not a PD2-DIO.\n");
        exit(EXIT_FAILURE);
    }
    
    pDoData->handle = PdAcquireSubsystem(pDoData->board, DigitalOut, 1);
    if(pDoData->handle < 0)
    {
        printf("BufferedDO: PdAcquireSubsystem failed. Error opening adapter.\n");
        exit(EXIT_FAILURE);
    }
    
    pDoData->state = unconfigured;
    
    retVal = _PdDIOReset(pDoData->handle);
    if (retVal < 0)
    {
        printf("BufferedDO: PdAOutReset error %d.\n", retVal);
        exit(EXIT_FAILURE);
    }
    
    return 0;
}


int BufferedDOTS(tBufferedDoData *pDoData)
{
    int retVal = 0;
    DWORD doCfg = pDoData->doCfg;
    DWORD count = 0;
    DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError | eStopped;
    DWORD events;
    DWORD cvDivider;
    DWORD scanIndex, numScans;
    DWORD bufferSize, minBufferSize;
    DWORD val_M_TPLR;
    

    retVal = _PdDIOEnableOutput(pDoData->handle, 0);
    if(retVal < 0)
    {
        printf("BufferedDO: _PdDIOEnableOutput failed.\n");
        exit(EXIT_FAILURE);
    }
    
    // Allocate the buffer used by the driver to transmit data to the device
    retVal = _PdRegisterBuffer(pDoData->handle, (WORD**)&pDoData->rawBuffer,
                              DigitalOut, pDoData->nbOfFrames, pDoData->nbOfPointsPerChannel,
                              pDoData->nbOfChannels,  BUF_BUFFERWRAPPED | BUF_DWORDVALUES |
							  BUF_BUFFERRECYCLED);
							  
    if (retVal < 0) { printf("BufferedDO: PdRegisterBuffer error %d.\n", retVal); exit(EXIT_FAILURE); }

    // Put data into the buffer.
    bufferSize = pDoData->nbOfFrames * pDoData->nbOfPointsPerChannel * pDoData->nbOfChannels;

    PutDataIntoBufferTSEQ(pDoData, bufferSize); 
        
    if(doCfg & AOB_EXTM)
        minBufferSize = 64;
    else
        minBufferSize = 2;
    
   // Check for some common errors
   if ((doCfg&(AOB_DMARD|AOB_EXTM)) == AOB_DMARD)
	   printf("Driver buffer: %dB. Internal DSP memory with DMA read.\n", bufferSize);
   else if ((doCfg&(AOB_DMARD|AOB_EXTM)) == AOB_EXTM)
	   printf("Driver buffer: %dB. External memory chip (%dkB).\n",bufferSize, minBufferSize);
   else
   {
	   printf("Error. Unknown memory transfer mode.\n");
	   exit(3);
   }

    if (bufferSize/1024 < minBufferSize)
    {
        printf("\nERROR: PowerDAQ Buffer size(%dk) < %dk.\n", bufferSize/1024, minBufferSize);
        printf("Please change the 'number of scans to generate' and 'number of frames'.\nDone.\n\n");
        exit(3);
    }   
    
    if ((bLastEntryFlagDetected) && (bufferSize/1024 > minBufferSize)){
        printf("\nERROR: PowerDAQ Buffer size(%dk) > %dk.\n", bufferSize/1024, minBufferSize);      
        printf("The size of the PowerDAQ buffer should be %dk if you use 'Last Entry' (TSLE) flag.\n",  minBufferSize);
        exit(3);      
    }

    // Set Clock Prescaler for DSP Timer
    switch(pDoData->preScalerFreq) 
    {                                                                   // PSFreq |  Counter Frequency  | Max. time interval 
    case 0:  val_M_TPLR = 0                                 ; break;    //  0        Don't use Prescaler     0.335s
    case 1:  val_M_TPLR = (DWORD)((dBASECL / 1000)-1)       ; break;    //  1             1kHz               16777s 
    case 2:  val_M_TPLR = (DWORD)((dBASECL / 10000)-1)      ; break;    //  2             10kHz              1677s
    case 3:  val_M_TPLR = (DWORD)((dBASECL / 100000)-1)     ; break;    //  3             100kHz             167s
    case 4:  val_M_TPLR = (DWORD)((dBASECL / 1000000)-1)    ; break;    //  4             1MHz               16s
    default: val_M_TPLR = 0;
    }

    if (doCfg&AOB_CVSTART1)           // If we selected to use external clock (TMR2), then
        val_M_TPLR|= M_PS1 | M_PS0;   // set corresponding timer prescaler register bits
    
    // Load Clock Prescaler
    retVal =_PdDspRegWrite(pDoData->handle, M_TPLR, val_M_TPLR);
    if (retVal < 0)
        printf("_PdDspRegWrite error %d\n", retVal);

    cvDivider = (DWORD)floor(dBASECL/pDoData->updateRate)-1;

    if ((doCfg&(AOB_CVSTART1|AOB_CVSTART0)) == AOB_CVSTART0)
        printf("Using internal clock with BASE=%fHz  cvDiv=%d \n", dBASECL, cvDivider);
    else if ((doCfg&(AOB_CVSTART1|AOB_CVSTART0)) == AOB_CVSTART1)
        printf("Using external clock (from pin TMR2) at %fHz\n", dBASECL);
    else
    {
       printf("\nError. Invalid clock source.\n");
       exit(3);
    }

    // Initialize DO Async operation
    retVal = _PdDOAsyncInit(pDoData->handle, 
		                    doCfg, cvDivider, eventsToNotify, 0, NULL);
    if (retVal < 0)
    {
        printf("BufferedDO: PdDOAsyncInit error %d\n", retVal);
        exit(EXIT_FAILURE);
    }
    
    pDoData->state = configured;
    
    retVal = _PdSetUserEvents(pDoData->handle, DigitalOut, eventsToNotify);
    if (retVal < 0)
    {
        printf("BufferedDO: _PdSetUserEvents error %d\n", retVal);
        exit(EXIT_FAILURE);
    }
    
    retVal = _PdDOAsyncStart(pDoData->handle);
    if (retVal < 0)
    {
        printf("BufferedDO: PdDOAsyncStart error %d. Couldn't enable DOut conversion.\n", retVal);
        exit(EXIT_FAILURE);
    }
    
    pDoData->state = running;

    printf("Generating... Press Ctrl+C to stop.\n");
    
    while (!pDoData->abort)
    {
        // Wait for interrupt.
        events = _PdWaitForEvent(pDoData->handle, eventsToNotify, 500);
        
         if ((events & eTimeout) && (bLastEntryFlagDetected == FALSE))
         {
             printf("BufferedDO: wait for event timed out. \n");
             break;
         }
     
         if ((events & eBufferError) || (events & eStopped))
         {
             printf("BufferedDO: buffer error or stopped.\n");
             break;
         }
       
         if ((events & eBufferDone) || (events & eFrameDone))
         {
			 printf("BufferedDO: buffer done or frame done\n");
             retVal = _PdDOGetBufState(pDoData->handle, pDoData->nbOfPointsPerChannel, 
                                       SCANRETMODE_MMAP, &scanIndex, &numScans);
             if(retVal < 0) { printf("BufferedDO: buffer error\n"); break; }
			 else { printf("SI:0x%x VS:0x%x\n", scanIndex, numScans); }
             
            count = count + numScans;
               
            printf("BufferedDO: sent %d scans at %d\n", numScans, scanIndex);
        }

        // re-set user events
        _PdSetUserEvents(pDoData->handle, DigitalOut, eventsToNotify);
    }
    
    return retVal;
}


void CleanUpBufferedDOTS(tBufferedDoData *pDoData)
{
    int retVal = 0;
    
    if(pDoData->state == running)
    {
        retVal = _PdDOAsyncStop(pDoData->handle);
        if (retVal < 0)
            printf("BufferedDO: PdDOAsyncStop error %d\n", retVal);
        
        pDoData->state = configured;
    }
    
    if(pDoData->state == configured)
    {
	  retVal = _PdDIOReset(pDoData->handle);
	  if (retVal < 0) printf("_PdDIOReset error %d\n", retVal);

      retVal = _PdDIOEnableOutput(pDoData->handle, 0x0);
	  if (retVal < 0) printf("_PdDIOEnableOutput error %d\n", retVal);

        retVal = _PdClearUserEvents(pDoData->handle, DigitalOut, eAllEvents);
        if (retVal < 0)
            printf("BufferedDO: PdClearUserEvents error %d\n", retVal);
        
        retVal = _PdDOAsyncTerm(pDoData->handle);
        if (retVal < 0)
            printf("BufferedDO: PdAOutAsyncTerm error %d\n", retVal);
        
        retVal = _PdUnregisterBuffer(pDoData->handle, (WORD *)pDoData->rawBuffer, DigitalOut);
        if (retVal < 0)
            printf("BufferedDO: PdUnregisterBuffer error %d\n", retVal);
        
        pDoData->state = unconfigured;
    }
    
    if(pDoData->state == unconfigured)
    {
        retVal = PdAcquireSubsystem(pDoData->handle, DigitalOut, 0);
        if (retVal < 0)
            printf("BufferedDO: PdReleaseSubsystem error %d\n", retVal);    
    }
    
    pDoData->state = closed;
}


// exit handler
void BufferedDOTSExitHandler(int status, void *arg)
{
   CleanUpBufferedDOTS((tBufferedDoData *)arg);
}


void SigInt(int signum)
{
   if (signum == SIGINT)
   {
      printf("\nCtrl+C detected. Generation stopped.\n");
      G_DoData.abort = TRUE;
   }
}


int main(int argc, char* argv[])
{

    PD_PARAMS params = {0, 1, {0}, 200000.0, 0, 1*1024, 1};
   
    ParseParameters(argc, argv, &params);
  
    // initializes acquisition session parameters
    G_DoData.board = params.board;
    G_DoData.nbOfChannels = params.numChannels;
    G_DoData.nbOfFrames = 2;
    G_DoData.nbOfPointsPerChannel = params.numSamplesPerChannel;
    G_DoData.updateRate = params.frequency;
    G_DoData.preScalerFreq = 1;
    G_DoData.doCfg = 0;
   
    // Configure Digital Output
    G_DoData.doCfg |= AOB_TSEQ;      // Enable 'time sequence' mode (uses the Last Entry flag)

    G_DoData.doCfg |= AOB_DMARD;     // Use faster internal on-DSP memory: 2kB  256-entry, or
                   // AOB_EXTM;      // use slower external memory chip:  64kB 8192-entry <400kHz

    // G_DoData.doCfg |= AOB_STARTTRIG0; // to use an external trigger pin to start generation

    G_DoData.doCfg |= AOB_CVSTART0;  // Use the internal oscillator (mode 01), or
                   // AOB_CVSTART1;  // use an external clock source wired in on TMR2 (mode 10)

    // In case that you use an external clock for a Time Sequence, then set dBASECL to
    // the frequency of the external clock (used by clDivider in PutDataIntoBufferTSEQ)
    if ((G_DoData.doCfg&(AOB_CVSTART0|AOB_CVSTART1)) == AOB_CVSTART1)
        dBASECL=params.frequency;      // assign frequency (in Hz) of external clock here

     // setup exit handler that will clean-up the session if an error occurs
     on_exit(BufferedDOTSExitHandler, &G_DoData);

     // set up SIGINT handler
     signal(SIGINT, SigInt);
    
    
     InitBufferedDOTS(&G_DoData);
    
     BufferedDOTS(&G_DoData);
    
     CleanUpBufferedDOTS(&G_DoData);
        
    return 0;
}
