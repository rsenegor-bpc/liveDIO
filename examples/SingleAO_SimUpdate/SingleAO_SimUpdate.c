/*****************************************************************************/
/*                    Buffered      analog output example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform an update      */  
/*  on all channels at once. i                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2011 United Electronic Industries, Inc.                */
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


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _bufferedAoData
{
   int abort;                    // set to TRUE to abort generation
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;   // Size of the waveform to generate on each channel
   DWORD channelList[96];
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
} tBufferedAoData;


int InitBufferedAO(tBufferedAoData *pAoData);
int BufferedAO(tBufferedAoData *pAoData, DWORD *buffer);
void CleanUpBufferedAO(tBufferedAoData *pAoData);


static tBufferedAoData G_AoData;

// exit handler
void BufferedAOExitHandler(int status, void *arg)
{
   CleanUpBufferedAO((tBufferedAoData *)arg);
}


int InitBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;
   Adapter_Info adaptInfo;

   // get adapter type
   retVal = _PdGetAdapterInfo(pAoData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
      printf("This is an MFx board\n");
   else
      printf("This is an AO32 (or AO96) board\n");

   pAoData->handle = PdAcquireSubsystem(pAoData->board, AnalogOut, 1);
   if(pAoData->handle < 0)
   {
      printf("BufferedAO: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAoData->state = unconfigured;

   retVal = _PdAOutReset(pAoData->handle);
   if (retVal < 0)
   {
      printf("BufferedAO: PdAOutReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // need also to call this function if the board is a PD2-AO-xx
   if(pAoData->adapterType & atPD2AO)
   {
      retVal = _PdAO32Reset(pAoData->handle);
      if (retVal < 0)
      {
         printf("BufferedAO: PdAAO32Reset error %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   }

   return 0;
}


int BufferedAO(tBufferedAoData *pAoData, DWORD *rawBuffer)
{
   int retVal;
   DWORD aoCfg;
   DWORD cvDivider;
   DWORD dwOffset = 0;
   DWORD dwCount;
   DWORD  DMAoffset = 0;
   DWORD  DMAcount  = 0;
   DWORD  DMAdest   = 0;
   

   // set configuration - _PdAOutReset is called inside _PdAOutSetCfg
   aoCfg = AOB_CVSTART0 | AOB_DMAEN | AOB_INTCVSBASE| AOB_REGENERATE| AOB_REGENERATE_SINGLESHOT ;  // 33 MHz internal timebase
   retVal = _PdAOutSetCfg(pAoData->handle, aoCfg, 0);
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAOutSetCfg failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }

   // set 11 Mhz clock divisor
   cvDivider = 33000000.0 / (pAoData->updateRate*pAoData->nbOfChannels) -1;
   retVal = _PdAOutSetCvClk(pAoData->handle, cvDivider);
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAOutSetCvClk failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }

   // Program DSP DMA1 
   DMAoffset = 0xffffff - (pAoData->nbOfChannels-2);                               // DMA offset register
   DMAcount  = (pAoData->nbOfChannels-1);                                          // DMA count  register
   DMAdest   = (AO_REG0 + G_AoData.channelList[0] )|AO_WR | AO32_WRH;        // DMA destination register (store in the first channel)    
   retVal = _PdAODMASet(pAoData->handle, DMAoffset, DMAcount, DMAdest); 
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAODMASet failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }
   
   retVal = _PdAO32SetUpdateChannel(pAoData->handle, pAoData->nbOfChannels-1, TRUE);
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAO32SetUpdateChannel failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }
   
   retVal = _PdAOutEnableConv(pAoData->handle, TRUE); 
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAOutEnableConv failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAoData->state = configured;

   // Fill the FIFO with the first scan 
   retVal = _PdAOutPutBlock(pAoData->handle, pAoData->nbOfChannels, 
                             (DWORD*)&rawBuffer, &dwCount);
   if(retVal < 0)
   {
      printf("BufferedAO: _PdAOutPutBlock failed with code %x\n", retVal);
      exit(EXIT_FAILURE);
   }
   dwOffset = dwOffset + dwCount;


   retVal = _PdAOutEnableConv(pAoData->handle, TRUE);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdAOutEnableConv error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   //Start SW trigger
   retVal = _PdAOutSwStartTrig(pAoData->handle);
   if (retVal < 0)
   {
      printf("BufferedAO: PdAOutSwStartTrig failed with %d.\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAoData->state = running;

   while (!pAoData->abort)
   {
             usleep(1000000/pAoData->updateRate);

         // Write next scan to FIFO 
         retVal = _PdAOutPutBlock(pAoData->handle, pAoData->nbOfChannels, 
                                  (DWORD*)&rawBuffer[dwOffset], &dwCount);
         if(retVal < 0)
         {
            printf("BufferedAO: _PdAOutPutBlock failed with code %x\n", retVal);
            exit(EXIT_FAILURE);
         }
         
         
         // Set offset to point to the next scan of data to be generated at the next
         // iteration
         dwOffset = dwOffset + dwCount;
         if(dwOffset >= pAoData->nbOfChannels*pAoData->nbOfPointsPerChannel)
         {
             dwOffset = 0;
         }

         // trigger output
         retVal = _PdAOutSwStartTrig(pAoData->handle);
         if(retVal < 0)
         {
            printf("BufferedAO: _PdAOutSwStartTrig failed with code %x\n", retVal);
            exit(EXIT_FAILURE);
         }

         //printf("count = %d offset = %d\n", dwCount, dwOffset);     

   }

   return retVal;
}


void CleanUpBufferedAO(tBufferedAoData *pAoData)
{
   int retVal;
      
   if(pAoData->state == running)
   {
      // Stop and disable AOut conversions
      retVal = _PdAOutSwStopTrig(pAoData->handle);
      if (retVal < 0)
         printf("BufferedAO: _PdAOutSwStopTrig error %d\n", retVal);

      retVal = _PdAOutEnableConv(pAoData->handle, FALSE);
      if (retVal < 0)
         printf("BufferedAO: _PdAOutEnableConv error %d\n", retVal);

      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
      retVal = _PdAOutReset(pAoData->handle);
      if (retVal < 0)
         printf("BufferedAO: PdAOutReset error %d\n", retVal);
               
      // need also to call this function if the board is a PD2-AO-xx
      if(pAoData->adapterType & atPD2AO)
      {
         retVal = _PdAO32Reset(pAoData->handle);
         if (retVal < 0)
            printf("BufferedAO: PdAAO32Reset error %d\n", retVal);
      }

      pAoData->state = unconfigured;
   }

   if(pAoData->handle > 0 && pAoData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pAoData->handle, AnalogOut, 0);
      if (retVal < 0)
         printf("BufferedAO: PdReleaseSubsystem error %d\n", retVal);
   }

   pAoData->state = closed;
}


//////////////////////////////////////////////////////////////////////////
// Function to react to SIGINT (Ctrl+C)
void SigInt(int signum)
{
   if (signum == SIGINT)
   {
      printf("Ctrl+C detected. generation stopped\n");
      G_AoData.abort = TRUE;
   }
}


int main(int argc, char *argv[])
{
   double *voltBuffer = NULL;
   DWORD *rawBuffer;
   int i, j;


   // initializes acquisition session parameters
   G_AoData.board = 0;
   G_AoData.handle = 0;
   G_AoData.nbOfChannels = 1;
   G_AoData.nbOfPointsPerChannel = 1000;
   G_AoData.updateRate = 5000.0;
   G_AoData.state = closed;
   G_AoData.abort = FALSE;
   for(i=0; i<G_AoData.nbOfChannels; i++)
   {
	   G_AoData.channelList[i] = i;
   }

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedAOExitHandler, &G_AoData);

   // set up SIGINT handler
   signal(SIGINT, SigInt);

   // allocate memory for the generation buffer
   voltBuffer = (double *) malloc(G_AoData.nbOfChannels * G_AoData.nbOfPointsPerChannel *
                              sizeof(double));
   if(voltBuffer == NULL)
   {
      printf("BufferedAO: could not allocate enough memory for the generation buffer\n");
      exit(EXIT_FAILURE);
   }

   // allocate memory for the raw generation buffer
   rawBuffer = (DWORD *) malloc(G_AoData.nbOfChannels * G_AoData.nbOfPointsPerChannel *
                              sizeof(DWORD));
   if(rawBuffer == NULL)
   {
      printf("BufferedAO: could not allocate enough memory for the generation buffer\n");
      exit(EXIT_FAILURE);
   }


   // writes data to buffer. Each channel will generate a sinus of different frequency
   for(i=0; i<G_AoData.nbOfPointsPerChannel; i++)
   {
      for(j=0; j<G_AoData.nbOfChannels; j++)
      {
         voltBuffer[i*G_AoData.nbOfChannels + j] = 
               10.0 * sin(2.0 * 3.1415 * i * (j + 1) / G_AoData.nbOfPointsPerChannel);
               
         rawBuffer[i*G_AoData.nbOfChannels+j] =
               (unsigned int)floor((voltBuffer[i*G_AoData.nbOfChannels+j] + 10.0) / 20.0 * 0xFFFF);
      }
   }

   // initializes acquisition session
   InitBufferedAO(&G_AoData);

   // Special case for MFx boards where data for channel 0 and 1
   // must be stored on the same DWORD
   if((G_AoData.nbOfChannels > 1) && (G_AoData.adapterType & atMF))
   {
      for(i=0; i<G_AoData.nbOfPointsPerChannel;i++)
         rawBuffer[i] = rawBuffer[i*2] | (rawBuffer[i*2+1] << 12);

      // now for MFx boards everything will look like we have only one channel
      G_AoData.nbOfChannels = 1;
   }

   // For PD2-AO32 boards, we need to add the channel number to the upper 
   // 16 bits of each value
   if((G_AoData.nbOfChannels > 1) && (G_AoData.adapterType & atPD2AO))
   {
      for(i=0; i<G_AoData.nbOfPointsPerChannel;i++)
      {
         for(j=0; j<G_AoData.nbOfChannels; j++)
            rawBuffer[i*G_AoData.nbOfChannels+j] = 
               (rawBuffer[i*G_AoData.nbOfChannels+j] & 0xFFFF) | (j << 16);
      }
   }

   // run the acquisition
   BufferedAO(&G_AoData, rawBuffer);

   // Cleanup acquisition
   CleanUpBufferedAO(&G_AoData);

   // free acquisition buffer
   free(voltBuffer);
   free(rawBuffer);

   return 0;
}

