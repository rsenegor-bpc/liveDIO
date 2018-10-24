//===========================================================================
//
//   NAME: dsp_irqs.c
//
// BOARDS SUPPORTED: PDx-DIO,PDx-AO 
//
// OS SUPPORTED: Linux
//
// DESCRIPTION:   PowerDAQ DIO/AO board DSP high-speed IRQs application
//                IRQ response time is 100nS + OS delay time
//
// This example shows how to use available on the PDx-DIO and 
// PDx-AO boards high-speed digital interrupt lines.  Those lines are 
// capable of detection of at least 16.5nS NEGATIVE pulse. 
// IRQx signal must be wired to IRQA/IRQB/IRQC/IRQD terminal on
// PDx-DIO-STP-64 and to IRQB on PD-AO-STP. Please note, that 
// PD-DIO-CBL-16 or compatible cable must be used to connect IRQ 
// lines on the board with the  terminal.
// IRQx signals are 7kV ESD and +/-30V protected but 
// recommended signal level is a standard TTL (0..0.8V is logic
// zero and 2.0..5.0V is logic one).
//
//  NOTES:  VERY IMPORTANT : if IRQx signals are wired on the screw
//          terminal they must be either in high impedance state during 
//          the reset or in the following configuration
//          PDx-DIO : (IRQA=1,IRQB=0,IRQC=0,IRQD=1) 
//          PDx-AO :  (IRQB=0) 
//
//          FOR  PDx-DIO AVAILABLE IRQA/IRQB/IRQC/IRQD LINES 
//          FOR  PDx-AO  AVAILABLE only IRQB line, this line is used 
//          as an external update strobe - negative pulse on this line forse 
//          all DACs to be updated - and interrupt may be used to inform 
//          user application about this event
//
//
//
// VERSION: 1.0/28-JUN-2004
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 2005 United Electronic Industries, Inc.
//      All rights reserved.
//
//===========================================================================

//
//===========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"
#include "ParseParams.h"


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _DspIrqData
{
   int board;                    // board number to be used for the AI operation
   int nbOfBoards;             // number of boards installed
   int handle;                // board handle
   int port;                     // port to use
   int line;                     // digital line to use on the selected port
   int abort;
   int bAOboard;
   tState state;                 // state of the acquisition session
} tDspIrqData;


int InitDspIrq(tDspIrqData *pDspIrqData);
int DspIrq(tDspIrqData *pDspIrqData);
void CleanUpDspIrq(tDspIrqData *pDspIrqData);

static tDspIrqData G_DspIrqData;

void DspIrqExitHandler(int status, void* arg)
{
   CleanUpDspIrq(&G_DspIrqData);
}

int InitDspIrq(tDspIrqData *pDspIrqData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;
   
   // get adapter type
   retVal = _PdGetAdapterInfo(pDspIrqData->board, &adaptInfo);
   if (retVal<0)
   {
      printf("SingleDIO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }
   
   if(adaptInfo.atType & atPD2AO)
   {
      printf("This is an AO board\n");
      pDspIrqData->bAOboard = TRUE;
   }
   else if(adaptInfo.atType & atPD2DIO)
   {
      printf("This is a DIO board\n");
      pDspIrqData->bAOboard = FALSE;
   }
   else
   {
      printf("This board is not a PD2-AO or PD2-DIO board\n");
      exit(EXIT_FAILURE);
   }
   
   pDspIrqData->handle = PdAcquireSubsystem(pDspIrqData->board, DigitalIn, 1);
   if(pDspIrqData->handle < 0)
   {
      printf("DspIrqData: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }
   
   pDspIrqData->state = unconfigured;
   
 /*  retVal = _PdDInReset(pDspIrqData->handle);
   if (retVal < 0)
   {
      printf("DspIrqData: PdDIOReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }
   */
   return 0;
}


int DspIrq(tDspIrqData *pDspIrqData)
{
   int retVal;
   DWORD eventsToNotify = eDInEvent;
   DWORD eventsReceived;
   DWORD dwEvents;
   int count = 0;

   
   pDspIrqData->state = configured;
   
   retVal = _PdAdapterEnableInterrupt(pDspIrqData->handle, TRUE);
   if (retVal < 0)
   {
      printf("pDspIrqData: _PdAdapterEnableInterrupt error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   // configure IRQx 
   if (pDspIrqData->bAOboard)
      retVal = _PdDInSetCfg(pDspIrqData->handle, DIB_IRQBIm);
   else
      retVal = _PdDInSetCfg(pDspIrqData->handle, (DIB_IRQAIm|DIB_IRQBIm|DIB_IRQCIm|DIB_IRQDIm));
   
   if (retVal < 0) 
   {
      printf("_PdDInSetCfg error\n");
      exit(EXIT_FAILURE);
   }

   // set events we want to receive
   retVal = _PdSetUserEvents(pDspIrqData->handle, DigitalIn|EdgeDetect, eventsToNotify);
   if (retVal < 0)
   {
      printf("pDspIrqData: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 
   
   pDspIrqData->state = running;
   
   while(!pDspIrqData->abort)
   {
      eventsReceived = _PdWaitForEvent(pDspIrqData->handle, eventsToNotify, 1000);
      if(eventsReceived & eTimeout)
      {
         printf(".");
         fflush(stdout);
      }
      else
      {
         retVal = _PdDInGetStatus(pDspIrqData->handle, &dwEvents);
         if(retVal < 0)
         { 
            printf("Error %d in _PdDInGetStatus()\n", retVal);
            exit(EXIT_FAILURE);
         }
         printf("%d: Status = 0x%x\n", count++, dwEvents);
            
         // Check individual interrupt status bits
         if (pDspIrqData->bAOboard)
         {           
            if (dwEvents & (DIB_IRQBSC<<0xC)) printf("IRQB Event\n"); // check IRQB (bit#18) for AO board                     
         }
         else
         {
            if (dwEvents & DIB_IRQASC) printf("IRQA Event\n");     // check IRQA,B,C,D for DIO board
            if (dwEvents & DIB_IRQBSC) printf("IRQB Event\n");
            if (dwEvents & DIB_IRQDSC) printf("IRQD Event\n"); 
            if (dwEvents & DIB_IRQCSC) printf("IRQC Event\n");
         }
      }      

      if (pDspIrqData->bAOboard)
         retVal = _PdDInSetCfg(pDspIrqData->handle, DIB_IRQBIm);
      else
         retVal = _PdDInSetCfg(pDspIrqData->handle, (DIB_IRQAIm|DIB_IRQBIm|DIB_IRQCIm|DIB_IRQDIm));
   
      if (retVal < 0) 
      {
         printf("_PdDInSetCfg error\n");
         exit(EXIT_FAILURE);
      }
       
      // re-enable events we want to receive
      retVal = _PdSetUserEvents(pDspIrqData->handle, DigitalIn|EdgeDetect, eventsToNotify);
      if (retVal < 0)
      {
         printf("pDspIrqData: _PdSetUserEvents error %d\n", retVal);
         exit(EXIT_FAILURE);
      } 
   }
 
   return retVal;
}


void CleanUpDspIrq(tDspIrqData *pDspIrqData)
{
   int retVal;
   
   if(pDspIrqData->state == running)
   {
      retVal = _PdDIOIntrEnable(pDspIrqData->handle, FALSE);
      if (retVal < 0)
         printf("DspIrqData: _PdDIOIntrEnable error %d\n", retVal);
      
      pDspIrqData->state = configured;
   }
   
   if(pDspIrqData->state == configured)
   {
      retVal = _PdDIOReset(pDspIrqData->handle);
      if (retVal < 0)
         printf("DspIrqData: _PdDIOReset error %d\n", retVal);

      pDspIrqData->state = unconfigured;
   }
   
   if(pDspIrqData->handle > 0 && pDspIrqData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDspIrqData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("DspIrqData: PdReleaseSubsystem error %d\n", retVal);
   }
   
   pDspIrqData->state = closed;
}

void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping program.\n");
      G_DspIrqData.abort = TRUE;
   }
}

/****************************************************************************

****************************************************************************/
int main(int argc, char *argv[])
{
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0};
   
   ParseParameters(argc, argv, &params);
   
   // initializes acquisition session parameters
   G_DspIrqData.board = params.board;
   G_DspIrqData.handle = 0;
   G_DspIrqData.port = 0;
   G_DspIrqData.line = 0;
   G_DspIrqData.abort = 0;
   G_DspIrqData.state = closed;

   signal(SIGINT, SigInt);

   on_exit(DspIrqExitHandler, &G_DspIrqData);
   
   // initializes acquisition session
   InitDspIrq(&G_DspIrqData);
   
   // run the acquisition
   DspIrq(&G_DspIrqData);
   
   // Cleanup acquisition
   CleanUpDspIrq(&G_DspIrqData);
   
   return 0;
}
