/*****************************************************************************/
/*                    Buffered digital input example                         */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  acquisition. The acquisition is performed in a hardware timed fashion    */
/*  that is appropriate for fast speed acquisition (> 1MHz).                 */
/*  Each channel is a 16-bit port on the board.                              */
/*                                                                           */
/*  The buffered digital input only works with PD2-DIO boards.               */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2001 United Electronic Industries, Inc.                */
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

typedef struct _bufferedDiData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   unsigned short *rawBuffer;    // address of the buffer allocated by the driver to store
                                 // the raw binary data
   DWORD channelList[64];
   int nbOfChannels;             // number of channels
   int nbOfFrames;               // number of frames used in the asynchronous circular buffer
   int nbOfSamplesPerChannel;    // number of samples per channel
   double scanRate;              // sampling frequency on each channel
   tState state;                 // state of the acquisition session
} tBufferedDiData;


int InitBufferedDI(tBufferedDiData *pDiData);
int BufferedDI(tBufferedDiData *pDiData, WORD *buffer);
void CleanUpBufferedDI(tBufferedDiData *pDiData);
int G_stop = 0;
void sighandler(int sig)
{
    G_stop =1;
}

static tBufferedDiData G_DiData;

// exit handler
void BufferedDIExitHandler(int status, void *arg)
{
   CleanUpBufferedDI((tBufferedDiData *)arg);
}


int InitBufferedDI(tBufferedDiData *pDiData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDiData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedDI: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if(adaptInfo.atType & atPD2DIO)
      printf("This is a PD2-DIO board\n");
   else
   {
      printf("This board is not a PD2-DIO\n");
      exit(EXIT_FAILURE);
   }

   pDiData->handle = PdAcquireSubsystem(pDiData->board, DigitalIn, 1);
   if(pDiData->handle < 0)
   {
      printf("BufferedDI: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pDiData->state = unconfigured;

   retVal = _PdDIOReset(pDiData->handle);
   if (retVal < 0)
   {
      printf("BufferedDI: PdDInReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int BufferedDI(tBufferedDiData *pDiData, WORD *buffer)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eDataError | eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD scanIndex, numScans;
   int i, j;
   DWORD diCfg;

   // Configure all ports for input
   retVal = _PdDIOEnableOutput(pDiData->handle, 0);
   if(retVal < 0)
   {
      printf("BufferedDI: _PdDioEnableOutput failed\n");
      exit(EXIT_FAILURE);
   }

   
   // Allocate and register memory for the data 
   retVal = _PdRegisterBuffer(pDiData->handle, &pDiData->rawBuffer, DigitalIn,
                              pDiData->nbOfFrames, pDiData->nbOfSamplesPerChannel,
                              pDiData->nbOfChannels, BUF_BUFFERRECYCLED | BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      printf("BufferedAI: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // setup the board to use hardware internal clock
   diCfg = AIB_INPRANGE | AIB_CVSTART0 | AIB_CLSTART0 | AIB_CLSTART1;

   // set clock divider, assuming that we use the 11MHz timebase
   divider = (11000000.0 / pDiData->scanRate)-1;

   retVal = _PdDIAsyncInit(pDiData->handle, diCfg, 
                           divider, eventsToNotify, 
                            pDiData->nbOfChannels, 0);
   if (retVal < 0)
   {
      printf("BufferedDI: PdDIAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDiData->state = configured;

   retVal = _PdSetUserEvents(pDiData->handle, DigitalIn, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedDI: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdDIAsyncStart(pDiData->handle);
   if (retVal < 0)
   {
      printf("BufferedDI: _PdDIAsyncStart error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDiData->state = running;
   while(!G_stop)
   {
	   event = _PdWaitForEvent(pDiData->handle, eventsToNotify, 5000);

	   retVal = _PdSetUserEvents(pDiData->handle, DigitalIn, eventsToNotify);
	   if (retVal < 0)
	   {
		  printf("BufferedDI: _PdSetUserEvents error %d\n", retVal);
		  exit(EXIT_FAILURE);
	   }

	   if (event & eTimeout)
	   {
		  printf("BufferedDI: wait timed out\n");
		  exit(EXIT_FAILURE);
	   }

	   if ((event & eBufferError) || (event & eStopped))
	   {
		  printf("BufferedDI: buffer error\n");
		  exit(EXIT_FAILURE);
	   }

	   if ((event & eBufferDone) || (event & eFrameDone))
	   {
		  retVal = _PdDIGetBufState(pDiData->handle, pDiData->nbOfSamplesPerChannel,
								  AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
		  if(retVal < 0)
		  {
			 printf("BufferedDI: buffer error\n");
			 exit(EXIT_FAILURE);
		  }

		  printf("BufferedDI: got %d scans at %d\n", numScans, scanIndex);

		  memcpy(buffer, &pDiData->rawBuffer[scanIndex*pDiData->nbOfChannels], numScans * pDiData->nbOfChannels * sizeof(WORD));

		  if (event & eBufferDone)
		  {
			  printf("Buffer done!\n");
		  }
	/*
		  printf("BufferedDI:");
		  for(j=0; j<pDiData->nbOfSamplesPerChannel; j++)
		  {
			 printf("%d ", j);
			 for(i=0; i<pDiData->nbOfChannels; i++)
				printf(" port%d = 0x%x", i, *(buffer + j*pDiData->nbOfChannels + i));
			 printf("\n");
		  }*/
	   }
    }
    return retVal;
}


void CleanUpBufferedDI(tBufferedDiData *pDiData)
{
   int retVal;
      
   if(pDiData->state == running)
   {
      retVal = _PdDIAsyncStop(pDiData->handle);
      if (retVal < 0)
         printf("BufferedDI: _PdDIAsyncStop error %d\n", retVal);

      pDiData->state = configured;
   }

   if(pDiData->state == configured)
   {
      retVal = _PdClearUserEvents(pDiData->handle, DigitalIn, eAllEvents);
      if (retVal < 0)
         printf("BufferedDI: PdClearUserEvents error %d\n", retVal);
   
      retVal = _PdDIAsyncTerm(pDiData->handle);  
      if (retVal < 0)
         printf("BufferedDI: _PdDIAsyncTerm error %d\n", retVal);
   
      retVal = _PdUnregisterBuffer(pDiData->handle, pDiData->rawBuffer, DigitalIn);
      if (retVal < 0)
         printf("BufferedDI: PdUnregisterBuffer error %d\n", retVal);

      pDiData->state = unconfigured;
   }

   if(pDiData->handle > 0 && pDiData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDiData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("BufferedDI: PdReleaseSubsystem error %d\n", retVal);
   }

   pDiData->state = closed;
}


int main(int argc, char *argv[])
{
   WORD *buffer = NULL;
   int i;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);
   signal(SIGINT, sighandler);

   // initializes acquisition session parameters
   G_DiData.board = params.board;
   G_DiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_DiData.channelList[i] = params.channels[i];
   G_DiData.handle = 0;
   G_DiData.nbOfFrames = 16;
   G_DiData.rawBuffer = NULL;
   G_DiData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_DiData.scanRate = params.frequency;
   G_DiData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedDIExitHandler, &G_DiData);

   // initializes acquisition session
   InitBufferedDI(&G_DiData);

   // allocate memory for the buffer
   buffer = (WORD *) malloc(G_DiData.nbOfChannels * G_DiData.nbOfSamplesPerChannel *
                              sizeof(WORD));
   if(buffer == NULL)
   {
      printf("BufferedDI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   BufferedDI(&G_DiData, buffer);

   // Cleanup acquisition
   CleanUpBufferedDI(&G_DiData);

   // free acquisition buffer
   free(buffer);

   return 0;
}

