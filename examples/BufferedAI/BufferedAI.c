/*****************************************************************************/
/*                    Buffered analog input example                          */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  acquisition. The acquisition is performed in a hardware timed fashion    */
/*  that is appropriate for fast speed acquisition (> 1MHz).                 */
/*                                                                           */
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
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
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
   unsigned long fifoSize;
} tBufferedAiData;


int InitBufferedAI(tBufferedAiData *pAiData);
int BufferedAI(tBufferedAiData *pAiData, double *buffer);
void CleanUpBufferedAI(tBufferedAiData *pAiData);
void SigInt(int signum);


static tBufferedAiData G_AiData;

// exit handler
void BufferedAIExitHandler(int status, void *arg)
{
   CleanUpBufferedAI((tBufferedAiData *)arg);
}


int InitBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;
   Adapter_Info adaptInfo;

   // get fifo size
   retVal = _PdGetAdapterInfo(pAiData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->fifoSize = adaptInfo.SSI[AnalogIn].dwFifoSize;

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


int BufferedAI(tBufferedAiData *pAiData, double *buffer)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD scanIndex, numScans;
   int i;
   DWORD aiCfg;
   struct timeval tv1, tv2, tvb; 
   FILE* gnuplot = NULL;

   gnuplot = GnuPlotOpen();
   
   // setup the board to use hardware internal clock
   aiCfg = AIB_CLSTART0 | AIB_CVSTART1 | AIB_CVSTART0 | pAiData->range | pAiData->inputMode | 
           AIB_INTCVSBASE | AIB_INTCLSBASE | pAiData->polarity | pAiData->trigger; 
   
   retVal = _PdAcquireBuffer(pAiData->handle, (void**)&pAiData->rawBuffer,
                              pAiData->nbOfFrames, pAiData->nbOfSamplesPerChannel,
                              pAiData->nbOfChannels, AnalogIn,
                             /*BUF_BUFFERRECYCLED |*/ BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      printf("BufferedAI: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // set clock divider, assuming that we use the 33MHz timebase
   divider = (33000000.0 / pAiData->scanRate)-1;

   retVal = _PdAInAsyncInit(pAiData->handle, aiCfg, 0, 0,
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

   // Initializes timestamps
   gettimeofday(&tv1, NULL);
   gettimeofday(&tvb, NULL);

   while(!pAiData->abort)
   {
      if(pAiData->nbOfSamplesPerChannel*pAiData->nbOfChannels < pAiData->fifoSize/2)
      {
         usleep(50000);

         _PdImmediateUpdate(pAiData->handle);
      }
      else
      {
        event = _PdWaitForEvent(pAiData->handle, eventsToNotify, 3000);
      }

      retVal = _PdGetUserEvents(pAiData->handle, AnalogIn, &event);

      printf("Received event 0x%x\n", event);

      retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
      if (retVal < 0)
      {
         printf("BufferedAI: _PdSetUserEvents error %d\n", retVal);
         exit(EXIT_FAILURE);
      }

      if (event & eTimeout)
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

         return retVal;
      }

      if ((event & eBufferError) || (event & eStopped))
      {
         printf("BufferedAI: buffer error\n");
         exit(EXIT_FAILURE);
      }

      if ((event & eBufferDone) || (event & eFrameDone))
      {
         gettimeofday(&tv2, NULL);

         retVal = _PdAInGetScans(pAiData->handle, pAiData->nbOfSamplesPerChannel, AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
         if(retVal < 0)
         {
            printf("BufferedAI: _PdAInGetScans error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         if (event & eBufferDone)
         {
            printf("BufferedAI: got full buffer in %f ms\n", (tv2.tv_sec-tvb.tv_sec)*1000.0 + (tv2.tv_usec-tvb.tv_usec)/1000.0);
            tvb = tv2;
         }

         printf("BufferedAI: got %d scans at %d in %f ms\n", numScans, scanIndex, (tv2.tv_sec - tv1.tv_sec) * 1000.0 + (tv2.tv_usec - tv1.tv_usec)/1000.0);
         tv1 = tv2;
         if(numScans > 0)
         {
            retVal = PdAInRawToVolts(pAiData->board, aiCfg, (pAiData->rawBuffer + scanIndex*pAiData->nbOfChannels),
                               buffer, numScans * pAiData->nbOfChannels);
            if(retVal < 0)
            {
               printf("BufferedAI: Error %d in PdAInRawToVolts\n", retVal);
               exit(EXIT_FAILURE);
            }  

            printf("BufferedAI:");
            for(i=0; i<pAiData->nbOfChannels; i++)
               printf(" ch%d = %f", i, *(buffer + i));
            printf("\n");
         }
      }
   }

   if(gnuplot != NULL)
      GnuPlot(gnuplot, 0.0, 1/pAiData->scanRate, buffer, pAiData->nbOfChannels, numScans);

   return retVal;
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
   
      retVal = _PdReleaseBuffer(pAiData->handle, AnalogIn, pAiData->rawBuffer);
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
   double *buffer = NULL;
   int i;
   PD_PARAMS params = {0, 1, {0}, 20000.0, 0, 8192};
   char s[256];
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_AiData.board = params.board;
   G_AiData.handle = 0;
   G_AiData.abort = FALSE;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i];
   G_AiData.nbOfFrames = 10;
   G_AiData.rawBuffer = NULL;
   G_AiData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_AiData.scanRate = params.frequency;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_5V;
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
   buffer = (double *) malloc(G_AiData.nbOfChannels * G_AiData.nbOfSamplesPerChannel *
                              sizeof(double));
   if(buffer == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   BufferedAI(&G_AiData, buffer);

   // Cleanup acquisition
   CleanUpBufferedAI(&G_AiData);

   fprintf(stderr,"Press enter to end program.\n");
   fgets(s,256,stdin);
   while(strcmp(s,"\n")!=0) 
   {
      fgets(s,256,stdin); 
   }

   // free acquisition buffer
   free(buffer);

   return 0;
}

