/*****************************************************************************/
/*                    Buffered analog input example                          */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  acquisition on two boards. The clocks can be synchronized if you connect */
/*  the boards with a PD-CONN-SYNC cable.                                    */
/*  The acquisition is performed in a hardware timed fashion                 */
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
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
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
   int nbOfFrames;               // number of frames used in the asynchronous circular buffer
   int nbOfSamplesPerChannel;    // number of samples per channel
   double scanRate;              // sampling frequency on each channel
   int polarity;                 // polarity of the signal to acquire, possible value
                                 // is AIN_UNIPOLAR or AIN_BIPOLAR
   int range;                    // range of the signal to acquire, possible value
                                 // is AIN_RANGE_5V or AIN_RANGE_10V
   int inputMode;                // input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL
   int trigger;                  // if set, the acquisition will wait for a trigger signal before starting
   int CLClock;                  // configuration of the CL clock
   int CVClock;                  // configuration of the CV clock
   tState state;                 // state of the acquisition session
   pthread_mutex_t *lock;
} tBufferedAiData;


int InitBufferedAI(tBufferedAiData *pAiData);
int BufferedAI(tBufferedAiData *pAiData, double *buffer);
void CleanUpBufferedAI(tBufferedAiData *pAiData);
void SigInt(int signum);
void* AIThreadProc(void *arg);

// Number of boards to synchronize
#define NB_BOARDS 3 
static tBufferedAiData G_AiData[NB_BOARDS];

// exit handler
void BufferedAIExitHandler(int status, void *arg)
{
   int j;

   for (j=0; j<NB_BOARDS; j++)
   {
      CleanUpBufferedAI(&G_AiData[j]);
   }
}


int InitBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;

   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if (pAiData->handle < 0)
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

   // initializes timestamps
   gettimeofday(&tv1, NULL);
   gettimeofday(&tv2, NULL);
   gettimeofday(&tvb, NULL);

   // setup the board to use hardware internal clock
   aiCfg = pAiData->CLClock | pAiData->CVClock | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 

   retVal = _PdRegisterBuffer(pAiData->handle, &pAiData->rawBuffer, AnalogIn,
                              pAiData->nbOfFrames, pAiData->nbOfSamplesPerChannel,
                              pAiData->nbOfChannels, BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      printf("BufferedAI(board%d): PdRegisterBuffer error %d\n", pAiData->board, retVal);
      exit(EXIT_FAILURE);
   }

   // set clock divider, assuming that we use the 11MHz timebase
   divider = (11000000.0 / pAiData->scanRate)-1;

   retVal = _PdAInAsyncInit(pAiData->handle, aiCfg, 0, 0,
                            divider, divider, eventsToNotify, 
                            pAiData->nbOfChannels, pAiData->channelList);
   if (retVal < 0)
   {
      printf("TestAsyncAI(board%d): PdAInAsyncInit error %d\n", pAiData->board, retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->state = configured;

   retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedAI(board%d): PdAInAsyncInit error %d\n", pAiData->board, retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdAInAsyncStart(pAiData->handle);
   if (retVal < 0)
   {
      printf("BufferedAI(board%d): PdAInAsyncStart error %d\n", pAiData->board, retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->state = running;

   while (!pAiData->abort)
   {
      event = _PdWaitForEvent(pAiData->handle, eventsToNotify, 5000);

      retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
      if (retVal < 0)
      {
         printf("BufferedAI(board%d): _PdSetUserEvents error %d\n", pAiData->board, retVal);
         exit(EXIT_FAILURE);
      }

      if (event & eTimeout)
      {
         printf("BufferedAI(board%d): wait timed out\n", pAiData->board); 
         exit(EXIT_FAILURE);
      }

      if ((event & eBufferError) || (event & eStopped))
      {
         printf("BufferedAI(board%d): buffer error\n", pAiData->board);
         exit(EXIT_FAILURE);
      }

      if ((event & eBufferDone) || (event & eFrameDone))
      {
         gettimeofday(&tv2, NULL);

         retVal = _PdAInGetScans(pAiData->handle, pAiData->nbOfSamplesPerChannel, AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
         if (retVal < 0)
         {
            printf("BufferedAI(board%d): buffer error\n", pAiData->board);
            exit(EXIT_FAILURE);
         }

         retVal = PdAInRawToVolts(pAiData->board, aiCfg, (pAiData->rawBuffer + scanIndex*pAiData->nbOfChannels),
                                  buffer, numScans * pAiData->nbOfChannels);
         if (retVal < 0)
         {
            printf("BufferedAI(board%d): Error %d in PdAInRawToVolts\n", pAiData->board, retVal);
            exit(EXIT_FAILURE);
         }

         pthread_mutex_lock(pAiData->lock);

         if (event & eBufferDone)
         {
            printf("BufferedAI(board%d): got full buffer in %f ms\n\n", pAiData->board, (tv2.tv_sec-tvb.tv_sec)*1000.0 + (tv2.tv_usec-tvb.tv_usec)/1000.0);
            tvb = tv2;
         }

         printf("BufferedAI(board%d): got %d scans at %d in %f ms\n", pAiData->board, numScans, scanIndex, (tv2.tv_sec - tv1.tv_sec) * 1000.0 + (tv2.tv_usec - tv1.tv_usec)/1000.0);
         tv1 = tv2;
         printf("BufferedAI(board%d):",pAiData->board);
         for (i=0; i<pAiData->nbOfChannels; i++)
            printf(" ch%d = %f", i, *(buffer + i));
         printf("\n");

         pthread_mutex_unlock(pAiData->lock);
      }
   }

   return retVal;
}


void CleanUpBufferedAI(tBufferedAiData *pAiData)
{
   int retVal;

   if (pAiData->state == running)
   {
      retVal = _PdAInAsyncStop(pAiData->handle);
      if (retVal < 0)
         printf("BufferedAI: PdAInAsyncStop error %d\n", retVal);

      pAiData->state = configured;
   }

   if (pAiData->state == configured)
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

   if (pAiData->handle > 0 && pAiData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pAiData->handle, AnalogIn, 0);
      if (retVal < 0)
         printf("BufferedAI: PdReleaseSubsystem error %d\n", retVal);
   }

   pAiData->state = closed;
}


void* AIThreadProc(void *arg)
{
   sigset_t set;
   double *buffer;
   tBufferedAiData* pAiData = (tBufferedAiData*)(arg);

   // block the SIGINT signal, the main thread handles it 
   sigemptyset(&set);
   sigaddset(&set,SIGINT);
   pthread_sigmask(SIG_BLOCK,&set,NULL);

   // initializes acquisition session
   InitBufferedAI(pAiData);

   // allocate memory for the buffer
   buffer = (double *) malloc(pAiData->nbOfChannels * pAiData->nbOfSamplesPerChannel *
                              sizeof(double));
   if (buffer == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   BufferedAI(pAiData, buffer);

   // Call Gnuplot to display the acquired data
   sendDataToGnuPlot(pAiData->handle, 0.0, 1.0/pAiData->scanRate, buffer, 
                     pAiData->nbOfChannels, pAiData->nbOfSamplesPerChannel);


   // Cleanup acquisition
   CleanUpBufferedAI(pAiData);

   // free acquisition buffer
   free(buffer);

   return NULL;
}


void SigInt(int signum)
{
   int i;

   if (signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      for (i=0; i<NB_BOARDS; i++)
      {
         G_AiData[i].abort = TRUE;
      }
   }
}


int main(int argc, char *argv[])
{
   int i, j, retVal;
   PD_PARAMS params = {0, 1, {0}, 10000.0, 0, 8192};
   pthread_t aiThreads[NB_BOARDS];
   Adapter_Info adaptInfo[NB_BOARDS];
   pthread_mutex_t lock;

   ParseParameters(argc, argv, &params);

   pthread_mutex_init(&lock, NULL);

   // initializes acquisition session parameters
   for (j=0; j<NB_BOARDS; j++)
   {
      // get adapter info
      retVal = _PdGetAdapterInfo(j, &adaptInfo[j]);
      if (retVal < 0)
      {
         printf("BufferedAI: _PdGetAdapterInfo error %d\n", retVal);
         exit(EXIT_FAILURE);
      }

      G_AiData[j].board = params.board+j;
      G_AiData[j].handle = 0;
      G_AiData[j].abort = FALSE;
      G_AiData[j].nbOfChannels = params.numChannels;
      for (i=0; i<params.numChannels; i++)
         G_AiData[j].channelList[i] = params.channels[i];
      G_AiData[j].nbOfFrames = 16;
      G_AiData[j].rawBuffer = NULL;
      G_AiData[j].nbOfSamplesPerChannel = params.numSamplesPerChannel;
      G_AiData[j].scanRate = params.frequency;
      G_AiData[j].polarity = AIN_BIPOLAR;
      G_AiData[j].range = AIN_RANGE_10V;
      G_AiData[j].inputMode = AIN_SINGLE_ENDED;
      G_AiData[j].state = closed;
      if (params.trigger == 1)
         G_AiData[j].trigger = AIB_STARTTRIG0;
      else if (params.trigger == 2)
         G_AiData[j].trigger = AIB_STARTTRIG0 + AIB_STARTTRIG1;
      else
         G_AiData[j].trigger = 0;

      G_AiData[j].lock = &lock;

      // Set CV clock to be continuous, on MF boards delay between
      // channels within the same scan will be minimal
      G_AiData[j].CVClock = AIB_CVSTART1 | AIB_CVSTART0;

      // Configure first boards as slaves and last board as master
      if (j<(NB_BOARDS-1))
      {
         // Slaves use an external clock coming into the sync connector
         G_AiData[j].CLClock = AIB_CLSTART1;
         printf("Board%d (sn#0x%s) is a slave\n", j, adaptInfo[j].lpSerialNum);
      } else
      {
         // Master uses its internal clock
         G_AiData[j].CLClock = AIB_CLSTART0;
         printf("Board%d (sn#0x%s) is the master\n", j, adaptInfo[j].lpSerialNum);
      }

   }

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedAIExitHandler, NULL);

   signal(SIGINT, SigInt);

   for (j=NB_BOARDS-1; j>=0; j--)
   {
      if (pthread_create(&aiThreads[j], NULL, AIThreadProc, &G_AiData[j]))
      {
         fprintf(stderr, "Cannot create AI thread\n");
      }

      // delay to visually verify that the boards are
      // well synchronized.
      usleep(100000);
   }

   for (j=0; j<NB_BOARDS; j++)
   {
      pthread_join(aiThreads[j], NULL);
   }

   pthread_mutex_destroy(&lock);

   return 0;
}

