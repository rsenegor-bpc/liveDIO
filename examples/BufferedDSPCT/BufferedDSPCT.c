/*****************************************************************************/
/*                    Buffered counter/timer example                         */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  event counting. The acquisition is performed in a hardware timed fashion */
/*  that is appropriate for fast speed acquisition (> 1MHz).                 */
/*                                                                           */
/*  The buffered event counting example only works with PD2-DIO-CT boards.   */
/*  You can use up to two counters (counters 0 and 2).                       */
/*  Use TMR0 pin to count events on coutner 0.                               */
/*  Use TMR0 and TMR2 to count events on counters 0 and 2.                   */
/*  Connect TMR1 and IRQD for 2 channel counting.                            */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2003 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "gnuplot.h"
#include "ParseParams.h"

typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _bufferedDspCtData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   unsigned short *rawBuffer;    // address of the buffer allocated by the driver to store
                                 // the raw binary data
   DWORD channelList[2];
   int nbOfChannels;             // number of channels
   int nbOfFrames;               // number of frames used in the asynchronous circular buffer
   int nbOfSamplesPerChannel;    // number of samples per channel
   double scanRate;              // sampling frequency on each channel
   tState state;                 // state of the acquisition session
} tBufferedDspCtData;


int InitBufferedDPCT(tBufferedDspCtData *pDspCtData);
int BufferedDSPCT(tBufferedDspCtData *pDspCtData, WORD *buffer);
void CleanUpBufferedDSPCT(tBufferedDspCtData *pDspCtData);


static tBufferedDspCtData G_DspCtData;

// exit handler
void BufferedDSPCTExitHandler(int status, void *arg)
{
   CleanUpBufferedDSPCT((tBufferedDspCtData *)arg);
}


int InitBufferedDSPCT(tBufferedDspCtData *pDspCtData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDspCtData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedDSPCT: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if(adaptInfo.atType & atPD2DIO)
      printf("This is a PD2-DIO board\n");
   else
   {
      printf("This board is not a PD2-DIO\n");
      exit(EXIT_FAILURE);
   }

   pDspCtData->handle = PdAcquireSubsystem(pDspCtData->board, DSPCounter, 1);
   if(pDspCtData->handle < 0)
   {
      printf("BufferedDSPCT: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pDspCtData->state = unconfigured;

   return 0;
}


int BufferedDSPCT(tBufferedDspCtData *pDspCtData, WORD *buffer)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD scanIndex, numScans;
   int i, j;
   DWORD diCfg;

   
   // Allocate and register memory for the data 
   retVal = _PdRegisterBuffer(pDspCtData->handle, &pDspCtData->rawBuffer, DSPCounter,
                              pDspCtData->nbOfFrames, pDspCtData->nbOfSamplesPerChannel,
                              pDspCtData->nbOfChannels, 0);
   if (retVal < 0)
   {
      printf("BufferedAI: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // setup the board to use hardware internal clock
   diCfg = AIB_CVSTART0 | AIB_CLSTART0 | AIB_CLSTART1;

   // set clock divider, assuming that we use the 11MHz timebase
   divider = (11000000.0 / pDspCtData->scanRate)-1;

   retVal = _PdCTAsyncInit(pDspCtData->handle, diCfg, 
                           divider, eventsToNotify, 
                            pDspCtData->nbOfChannels);
   if (retVal < 0)
   {
      printf("BufferedDSPCT: PdDIAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDspCtData->state = configured;

   retVal = _PdSetUserEvents(pDspCtData->handle, DSPCounter, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedDSPCT: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdCTAsyncStart(pDspCtData->handle);
   if (retVal < 0)
   {
      printf("BufferedDSPCT: _PdDIAsyncStart error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDspCtData->state = running;

   event = _PdWaitForEvent(pDspCtData->handle, eventsToNotify, 5000);

   retVal = _PdSetUserEvents(pDspCtData->handle, DSPCounter, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedDSPCT: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if (event & eTimeout)
   {
      printf("BufferedDSPCT: wait timed out\n"); 
      exit(EXIT_FAILURE);
   }

   if ((event & eBufferError) || (event & eStopped))
   {
      printf("BufferedDSPCT: buffer error\n");
      exit(EXIT_FAILURE);
   }

   if ((event & eBufferDone) || (event & eFrameDone))
   {
      retVal = _PdCTGetBufState(pDspCtData->handle, pDspCtData->nbOfSamplesPerChannel,
                              AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
      if(retVal < 0)
      {
         printf("BufferedDSPCT: buffer error\n");
         exit(EXIT_FAILURE);
      }

      printf("BufferedDSPCT: got %d scans at %d\n", numScans, scanIndex);

      memcpy(buffer, &pDspCtData->rawBuffer[scanIndex*pDspCtData->nbOfChannels], numScans * pDspCtData->nbOfChannels * sizeof(WORD));

      printf("BufferedDSPCT:");
      for(j=0; j<pDspCtData->nbOfSamplesPerChannel; j++)
      {
         printf("%d ", j);
         for(i=0; i<pDspCtData->nbOfChannels; i++)
            printf(" port%d = 0x%x", i, *(buffer + j*pDspCtData->nbOfChannels + i));
         printf("\n");
      }
   }

   return retVal;
}


void CleanUpBufferedDSPCT(tBufferedDspCtData *pDspCtData)
{
   int retVal;
      
   if(pDspCtData->state == running)
   {
      retVal = _PdCTAsyncStop(pDspCtData->handle);
      if (retVal < 0)
         printf("BufferedDSPCT: _PdDIAsyncStop error %d\n", retVal);

      pDspCtData->state = configured;
   }

   if(pDspCtData->state == configured)
   {
      retVal = _PdClearUserEvents(pDspCtData->handle, DSPCounter, eAllEvents);
      if (retVal < 0)
         printf("BufferedDSPCT: PdClearUserEvents error %d\n", retVal);
   
      retVal = _PdCTAsyncTerm(pDspCtData->handle);  
      if (retVal < 0)
         printf("BufferedDSPCT: _PdDIAsyncTerm error %d\n", retVal);
   
      retVal = _PdUnregisterBuffer(pDspCtData->handle, pDspCtData->rawBuffer, DSPCounter);
      if (retVal < 0)
         printf("BufferedDSPCT: PdUnregisterBuffer error %d\n", retVal);

      pDspCtData->state = unconfigured;
   }

   if(pDspCtData->handle > 0 && pDspCtData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDspCtData->handle, DSPCounter, 0);
      if (retVal < 0)
         printf("BufferedDSPCT: PdReleaseSubsystem error %d\n", retVal);
   }

   pDspCtData->state = closed;
}


int main(int argc, char *argv[])
{
   WORD *buffer = NULL;
   int i;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DspCtData.board = params.board;
   G_DspCtData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_DspCtData.channelList[i] = params.channels[i];
   G_DspCtData.handle = 0;
   G_DspCtData.nbOfFrames = 4;
   G_DspCtData.rawBuffer = NULL;
   G_DspCtData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_DspCtData.scanRate = params.frequency;
   G_DspCtData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedDSPCTExitHandler, &G_DspCtData);

   // initializes acquisition session
   InitBufferedDSPCT(&G_DspCtData);

   // allocate memory for the buffer
   buffer = (WORD *) malloc(G_DspCtData.nbOfChannels * G_DspCtData.nbOfSamplesPerChannel *
                              sizeof(WORD));
   if(buffer == NULL)
   {
      printf("BufferedDSPCT: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   BufferedDSPCT(&G_DspCtData, buffer);

   // Cleanup acquisition
   CleanUpBufferedDSPCT(&G_DspCtData);

   // free acquisition buffer
   free(buffer);

   return 0;
}

