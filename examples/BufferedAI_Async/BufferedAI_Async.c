/*****************************************************************************/
/*                    Buffered analog input example                          */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  acquisition. The acquisition is performed in a hardware timed fashion    */
/*  that is appropriate for fast speed acquisition (> 1MHz).                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2004 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

typedef struct _bufferedAiData
{
   int abort;
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   unsigned short *rawBuffer;    // address of the buffer allocated by the driver to store
                                 // the raw binary data
   int nbOfChannels;             // number of channels
   DWORD channelList[64];
   DWORD aiCfg;
   int nbOfFrames;               // number of frames used in the asynchronous circular buffer
   int nbOfSamplesPerChannel;    // number of samples per channel
   double scanRate;              // sampling frequency on each channel
   int polarity;                 // polarity of the signal to acquire, possible value
                                 // is AIN_UNIPOLAR or AIN_BIPOLAR
   int range;                    // range of the signal to acquire, possible value
                                 // is AIN_RANGE_5V or AIN_RANGE_10V
   int inputMode;                // input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL
   int trigger;
   tState state;                 // state of the acquisition session
   struct sigaction io_act;
} tBufferedAiData;


int InitBufferedAI(tBufferedAiData *pAiData);
int StartBufferedAI(tBufferedAiData *pAiData);
void CleanUpBufferedAI(tBufferedAiData *pAiData);
void SigInt(int signum);
void SigBufferedAI(int sig);


static tBufferedAiData G_AiData;
double* G_VoltBuffer;
int G_EventCount = 0;

// exit handler
void BufferedAIExitHandler(int status, void *arg)
{
   CleanUpBufferedAI((tBufferedAiData *)arg);
}


int InitBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;

   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("BufferedAI: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAiData->state = unconfigured;

   retVal = _PdAInReset(pAiData->handle);
   if (retVal < 0)
   {
      printf("BufferedAI: PdAInReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int StartBufferedAI(tBufferedAiData *pAiData)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   
   
   // setup the board to use hardware internal clock
   pAiData->aiCfg = AIB_CLSTART0 | AIB_CVSTART1 | AIB_CVSTART0 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   
   retVal = _PdRegisterBuffer(pAiData->handle, &pAiData->rawBuffer, AnalogIn,
                              pAiData->nbOfFrames, pAiData->nbOfSamplesPerChannel,
                              pAiData->nbOfChannels, BUF_BUFFERRECYCLED | BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      printf("BufferedAI: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdSetAsyncNotify(pAiData->handle, &pAiData->io_act, SigBufferedAI);
   if (retVal < 0)
   {
      printf("BufferedAI: Failed to set up notification\n");
      exit(EXIT_FAILURE);
   }


   // set clock divider, assuming that we use the 11MHz timebase
   divider = (11000000.0 / pAiData->scanRate)-1;

   retVal = _PdAInAsyncInit(pAiData->handle, pAiData->aiCfg, 0, 0,
                            divider, divider, eventsToNotify, 
                            pAiData->nbOfChannels, pAiData->channelList);
   if (retVal < 0)
   {
      printf("TestAsyncAI: PdAInAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->state = configured;

   retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedAI: PdAInAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdAInAsyncStart(pAiData->handle);
   if (retVal < 0)
   {
      printf("BufferedAI: PdAInAsyncStart error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->state = running;

   while(!pAiData->abort)
   {
      sleep(1);
   }

   // Call Gnuplot to display the acquired data
   sendDataToGnuPlot(pAiData->handle, 0.0, 1.0/pAiData->scanRate, G_VoltBuffer, 
                     pAiData->nbOfChannels, pAiData->nbOfSamplesPerChannel);
   return retVal;
}


//////////////////////////////////////////////////////////////////////////
// Event handler
void SigBufferedAI(int sig)
{
    unsigned int events;
    int retVal;
    DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
    tBufferedAiData *pAiData = &G_AiData;
    DWORD scanIndex, numScans;
    int i;
   

    G_EventCount++;
    printf("Received event %d\n", G_EventCount);

    // figure out what events happened
    retVal = _PdGetUserEvents(pAiData->handle, AnalogIn, &events);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }

    if (events & eTimeout)
    {
       printf("BufferedAI: timeout error\n");

       // Get whatever has been already acquired in the board's FIFO
       _PdImmediateUpdate(pAiData->handle);

       retVal = _PdAInGetScans(pAiData->handle, pAiData->nbOfSamplesPerChannel, AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
       if(retVal < 0)
       {
          printf("BufferedAI: timo buffer error\n");
          exit(EXIT_FAILURE);
       }

       printf("BufferedAI: After ImUpd got %d scans at %d\n", numScans, scanIndex);

       return;
    }

    if ((events & eBufferError) || (events & eStopped))
    {
       printf("BufferedAI: buffer error\n");
       exit(EXIT_FAILURE);
    }

    if ((events & eBufferDone) || (events & eFrameDone))
    {
       retVal = _PdAInGetScans(pAiData->handle, pAiData->nbOfSamplesPerChannel, AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
       if(retVal < 0)
       {
          printf("BufferedAI: buffer error\n");
          exit(EXIT_FAILURE);
       }

       printf("BufferedAI: got %d scans at %d\n", numScans, scanIndex);

       retVal = PdAInRawToVolts(pAiData->board, pAiData->aiCfg, (pAiData->rawBuffer + scanIndex*pAiData->nbOfChannels),
                             G_VoltBuffer, numScans * pAiData->nbOfChannels);
       if(retVal < 0)
       {
          printf("BufferedAI: Error %d in PdAInRawToVolts\n", retVal);
          exit(EXIT_FAILURE);
       }  

       printf("BufferedAI:");
       for(i=0; i<pAiData->nbOfChannels; i++)
          printf(" ch%d = %f", i, *(G_VoltBuffer + i));
       printf("\n");

    }
  
    // Re-enable user events that asserted.
    retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }
}

void CleanUpBufferedAI(tBufferedAiData *pAiData)
{
   int retVal;
      
   if(pAiData->state == running)
   {
      retVal = _PdAInAsyncStop(pAiData->handle);
      if (retVal < 0)
         printf("BufferedAI: PdAInAsyncStop error %d\n", retVal);

      pAiData->state = configured;
   }

   if(pAiData->state == configured)
   {
      retVal = _PdClearUserEvents(pAiData->handle, AnalogIn, eAllEvents);
      if (retVal < 0)
         printf("BufferedAI: PdClearUserEvents error %d\n", retVal);
   
      retVal = _PdAInAsyncTerm(pAiData->handle);  
      if (retVal < 0)
         printf("BufferedAI: PdAInAsyncTerm error %d\n", retVal);
   
      retVal = _PdUnregisterBuffer(pAiData->handle, pAiData->rawBuffer, AnalogIn);
      if (retVal < 0)
         printf("BufferedAI: PdUnregisterBuffer error %d\n", retVal);

      pAiData->state = unconfigured;
   }

   if(pAiData->handle > 0 && pAiData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pAiData->handle, AnalogIn, 0);
      if (retVal < 0)
         printf("BufferedAI: PdReleaseSubsystem error %d\n", retVal);
   }

   pAiData->state = closed;
}


void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      G_AiData.abort = TRUE;
   }
}


int main(int argc, char *argv[])
{
   int i;
   PD_PARAMS params = {0, 1, {0}, 10000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_AiData.board = params.board;
   G_AiData.handle = 0;
   G_AiData.abort = FALSE;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i];
   G_AiData.nbOfFrames = 16;
   G_AiData.rawBuffer = NULL;
   G_AiData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_AiData.scanRate = params.frequency;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_10V;
   G_AiData.inputMode = AIN_SINGLE_ENDED;
   G_AiData.state = closed;
   if(params.trigger == 1)
       G_AiData.trigger = AIB_STARTTRIG0;
   else if(params.trigger == 2)
       G_AiData.trigger = AIB_STARTTRIG0 + AIB_STARTTRIG1;
   else
       G_AiData.trigger = 0;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedAIExitHandler, &G_AiData);

   signal(SIGINT, SigInt);
   
   // initializes acquisition session
   InitBufferedAI(&G_AiData);

   // allocate memory for the buffer
   G_VoltBuffer = (double *) malloc(G_AiData.nbOfChannels * G_AiData.nbOfSamplesPerChannel *
                              sizeof(double));
   if(G_VoltBuffer == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   StartBufferedAI(&G_AiData);

   // Cleanup acquisition
   CleanUpBufferedAI(&G_AiData);

   // free acquisition buffer
   free(G_VoltBuffer);

   return 0;
}

