/*****************************************************************************/
/*                Buffered analog input and output example                   */
/*                                                                           */
/*  This example shows how to use the powerdaq API to acquire data on Analog */
/*  inputs and simulteneously replay it on analog outputs.                   */
/*  The acquisition and gneration are performed in a hardware timed fashion  */
/*  that is appropriate for fast speed (> 10kHz).                            */
/*                                                                           */
/*  This example acquires signals at the specified sample rate and generate  */
/*  them on the outputs at the same rate.                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2005 United Electronic Industries, Inc.                */
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
#include <math.h>
#include <pthread.h>
#include <errno.h>
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
   int trigger;
   tState state;                 // state of the acquisition session
   int resolution;
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
   unsigned short ANDMask;
   unsigned short XORMask;
} tBufferedAiData;


typedef struct _bufferedAoData
{
   int abort;                    // set to TRUE to abort generation
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   int nbOfChannels;             // number of channels
   DWORD channelList[64];
   int nbOfPointsPerChannel;     // number of samples per channel
   int nbOfFrames;
   int halfFIFOSize;             // Half size of the DAC FIFO
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
   unsigned int *rawBuffer;      // address of the buffer allocated by the driver to store
                                 // the raw binary data
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
} tBufferedAoData;


int InitBufferedAI(tBufferedAiData *pAiData);
int BufferedAI(tBufferedAiData *pAiData);
void CleanUpBufferedAI(tBufferedAiData *pAiData);
int InitBufferedAO(tBufferedAoData *pAoData);
int BufferedAO(tBufferedAoData *pAoData);
void CleanUpBufferedAO(tBufferedAoData *pAoData);

void* AIThreadProc(void* arg);
void* AOThreadProc(void* arg);

void SigInt(int signum);


static tBufferedAiData G_AiData;
static tBufferedAoData G_AoData;

// Those are used to synchonize AI and AO thread activities
static pthread_mutex_t G_FrameMtx;
static pthread_cond_t G_FrameEvent;

// Common buffer used to store acquired data
static unsigned short* G_FrameBuffer;
static int G_FrameReady = 0;


// exit handler
void BufferedAIAOExitHandler(int status, void *arg)
{
   CleanUpBufferedAI(&G_AiData);
   CleanUpBufferedAO(&G_AoData);
}


int InitBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;
   Adapter_Info adaptInfo;


   // get adapter type
   retVal = _PdGetAdapterInfo(pAiData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAiData->resolution = adaptInfo.SSI[AnalogIn].dwChBits;
   pAiData->maxCode = (1 << adaptInfo.SSI[AnalogIn].dwChBits)-1;
   pAiData->ANDMask = adaptInfo.SSI[AnalogIn].wAndMask;
   pAiData->XORMask = adaptInfo.SSI[AnalogIn].wXorMask;

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


int BufferedAI(tBufferedAiData *pAiData)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD scanIndex, numScans;
   DWORD aiCfg;
   
   // setup the board to use hardware internal clock
   aiCfg = AIB_CVSTART0 | AIB_CLSTART1 | AIB_CLSTART0 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   
   retVal = _PdRegisterBuffer(pAiData->handle, &pAiData->rawBuffer, AnalogIn,
                              pAiData->nbOfFrames, pAiData->nbOfSamplesPerChannel,
                              pAiData->nbOfChannels, /*BUF_BUFFERRECYCLED |*/ BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      printf("BufferedAI: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // set clock divider, assuming that we use the 11MHz timebase
   divider = (11000000.0 / pAiData->scanRate)-1;

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

   while(!pAiData->abort)
   {
      event = _PdWaitForEvent(pAiData->handle, eventsToNotify, 5000);

      retVal = _PdSetUserEvents(pAiData->handle, AnalogIn, eventsToNotify);
      if (retVal < 0)
      {
         printf("BufferedAI: _PdSetUserEvents error %d\n", retVal);
         exit(EXIT_FAILURE);
      }

      if (event & eTimeout)
      {
         printf("BufferedAI: wait timed out\n"); 
         exit(EXIT_FAILURE);
      }

      if ((event & eBufferError) || (event & eStopped))
      {
         printf("BufferedAI: buffer error\n");
         exit(EXIT_FAILURE);
      }

      if ((event & eBufferDone) || (event & eFrameDone))
      {
         retVal = _PdAInGetScans(pAiData->handle, pAiData->nbOfSamplesPerChannel, AIN_SCANRETMODE_MMAP, &scanIndex, &numScans);
         if(retVal < 0)
         {
            printf("BufferedAI: buffer error\n");
            exit(EXIT_FAILURE);
         }

         printf("BufferedAI: got %d scans at %d\n", numScans, scanIndex);

         // Copy frame to common buffer
         pthread_mutex_lock(&G_FrameMtx);
         memcpy(G_FrameBuffer, pAiData->rawBuffer + scanIndex * pAiData->nbOfChannels, 
                numScans * pAiData->nbOfChannels * sizeof(unsigned short));

         // Notify AO thread
         G_FrameReady = 1;
         pthread_cond_signal(&G_FrameEvent);
         pthread_mutex_unlock(&G_FrameMtx);
      }
   }

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

   pAoData->maxCode = (1 << adaptInfo.SSI[AnalogOut].dwChBits) - 1;

   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
   {
      printf("This is an MFx board\n");
   }
   else if(pAoData->adapterType & atPD2AO)
   {
      printf("This is an AO32 board\n");
   }
   else
   {
      printf("This device doesn't have AO capability\n");
      exit(EXIT_FAILURE);
   }

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


int BufferedAO(tBufferedAoData *pAoData)
{
   int retVal;
   DWORD aoCfg;
   DWORD cvDivider;
   DWORD count = 0;
   DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError | eStopped;
   DWORD events;
   DWORD scanIndex, numScans;
   int i, j, k;
   

   // setup the board to use hardware internal clock
   aoCfg = AOB_INTCVSBASE | AOB_CVSTART0;  // use 33 MHz internal timebase
      
   if(pAoData->adapterType & atPD2AO)
   { 
      retVal = _PdRegisterBuffer(pAoData->handle, (WORD**)&pAoData->rawBuffer, AnalogOut,
                              pAoData->nbOfFrames, pAoData->nbOfPointsPerChannel,
                              pAoData->nbOfChannels, 
                              BUF_BUFFERWRAPPED | BUF_DWORDVALUES);
   }
   else
   {  
      aoCfg |= AOB_DMAEN;
      retVal = _PdRegisterBuffer(pAoData->handle, (WORD**)&pAoData->rawBuffer, AnalogOut,
                              pAoData->nbOfFrames, pAoData->nbOfPointsPerChannel,
                              1, 
                              BUF_BUFFERWRAPPED | BUF_DWORDVALUES);
   }
   
   if (retVal < 0)
   {
      printf("BufferedAO: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }
   
    
   // Initializes all frames to generate 0V
   for(k=0; k<pAoData->nbOfFrames; k++)
   {
      for(i=0; i<pAoData->nbOfPointsPerChannel; i++)
      {
         for(j=0; j<pAoData->nbOfChannels; j++)
         {
            // We use the DWORD data format on AO boards, with this format, we need to specify
            // the ouput channel in the upper 16 bits of each sample
            if(pAoData->adapterType & atPD2AO)
            {
               pAoData->rawBuffer[k*pAoData->nbOfPointsPerChannel*pAoData->nbOfChannels+
                                  i*pAoData->nbOfChannels+j] =  (pAoData->channelList[j] << 16) | 0x7FFF;
            }
            else // MF boards have only 2 12 bits AO channels, data for both channels are combined in a single
                 // value: ch0 | ch1 << 12
            {
               if(0==j)
               {
                  pAoData->rawBuffer[k*pAoData->nbOfPointsPerChannel+i] = 0x7FF;
               }
               else if(1==j)
               {
                  pAoData->rawBuffer[k*pAoData->nbOfPointsPerChannel+i] |= 0x7FF << 12;
               }
            }
         }
      }
   }

   // set clock divider, assuming that we use the 11MHz timebase
   cvDivider = 33000000.0 / (pAoData->updateRate/**pAoData->nbOfChannels*/) -1;
   
   if(pAoData->adapterType & atMF)
      retVal = _PdAOutAsyncInit(pAoData->handle, aoCfg, 
                               cvDivider, eventsToNotify);
   else
      retVal = _PdAOAsyncInit(pAoData->handle, aoCfg, 
                               cvDivider, eventsToNotify, 
                               pAoData->nbOfChannels, (DWORD*)pAoData->channelList);
   if (retVal < 0)
   {
      printf("BufferedAO: PdAOutAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAoData->state = configured;

   retVal = _PdSetUserEvents(pAoData->handle, AnalogOut, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if(pAoData->adapterType & atMF)
      retVal = _PdAOutAsyncStart(pAoData->handle);
   else
      retVal = _PdAOAsyncStart(pAoData->handle);
   if (retVal < 0)
   {
      printf("BufferedAO: PdAOutAsyncStart error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pAoData->state = running;

   while (!pAoData->abort)
   {
      events = _PdWaitForEvent(pAoData->handle, eventsToNotify, 5000);

      retVal = _PdSetUserEvents(pAoData->handle, AnalogOut, eventsToNotify);
      if (retVal < 0)
      {
         printf("BufferedAO: _PdSetUserEvents error %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   
      if (events & eTimeout)
      {
         printf("BufferedAO: wait timed out\n"); 
         exit(EXIT_FAILURE);
      }
   
      if ((events & eBufferError) || (events & eStopped))
      {
         printf("BufferedAO: buffer error\n");
         exit(EXIT_FAILURE);
      }
   
      if ((events & eBufferDone) || (events & eFrameDone))
      {
         if(pAoData->adapterType & atMF)
            retVal = _PdAOutGetBufState(pAoData->handle, pAoData->nbOfPointsPerChannel, 
                                     SCANRETMODE_MMAP, &scanIndex, &numScans);
         else
            retVal = _PdAOGetBufState(pAoData->handle, pAoData->nbOfPointsPerChannel, 
                                     SCANRETMODE_MMAP, &scanIndex, &numScans);
         if(retVal < 0)
         {
            printf("BufferedAO: buffer error\n");
            exit(EXIT_FAILURE);
         }

         count = count + numScans;
   
         // Wait for AI frame done event before refreshing the AO buffer
         pthread_mutex_lock(&G_FrameMtx);

         // If the AI frame is not ready, wait for notification from AI thread
         if(!G_FrameReady)
         {
            int timeout_ms = 5000; // will wait for this amount of time for the event before timing out
            struct timespec ts_timo;
            clock_gettime(CLOCK_REALTIME, &ts_timo);
            ts_timo.tv_nsec += (timeout_ms % 1000) * 1000000;
            ts_timo.tv_sec += timeout_ms / 1000;
            if(ts_timo.tv_nsec >= 1000000000)
            {
               ++ts_timo.tv_sec;
               ts_timo.tv_nsec -= 1000000000;
            }

            retVal = pthread_cond_timedwait(&G_FrameEvent, &G_FrameMtx, &ts_timo);
            if(retVal == ETIMEDOUT)
            {
               fprintf(stderr, "BufferedAO thread, timed out while waiting for AI Frame event.\n");
               break;
            }
         }

         // Convert AI Frame buffer to AO format
         for(i=0; i<pAoData->nbOfPointsPerChannel; i++)
         {
            for(j=0; j<pAoData->nbOfChannels; j++)
            {
               // Convert data between analog input raw binary format and analog output raw binary format 
               unsigned long value = (((G_FrameBuffer[i*pAoData->nbOfChannels+j]) & G_AiData.ANDMask) ^ G_AiData.XORMask) >> (16 - G_AiData.resolution);
               
               // Scale data to match input and output channel resolutions (AI 12,14 or 16 bits -> AO 12 or 16 bits)
               value = (unsigned long)floor(((double)value / (double)G_AiData.maxCode) * (double)G_AoData.maxCode);
               
               // We use the DWORD data format on AO boards, with this format, we need to specify
               // the ouput channel in the upper 16 bits of each sample
               if(pAoData->adapterType & atPD2AO)
               {
                  pAoData->rawBuffer[(scanIndex+i)*pAoData->nbOfChannels + j] = (pAoData->channelList[j] << 16) | (value&0xFFFF);
               }
               else // MF boards have only 2 12 bits AO channels, data for both channels are combined in a single
                    // value: ch0 | ch1 << 12
               {
                   if(0==j)
                   {
                      pAoData->rawBuffer[scanIndex+i] = (value&0xFFF);
                   }
                   else if(1==j)
                   {
                      pAoData->rawBuffer[scanIndex+i] |= (value&0xFFF) << 12;
                   }
               }
            }
         }

         // Release the lock
         G_FrameReady = 0;
         pthread_mutex_unlock(&G_FrameMtx);

         printf("BufferedAO: sent %d scans at %d\n", numScans, scanIndex);
      }
   }

   return retVal;
}


void CleanUpBufferedAO(tBufferedAoData *pAoData)
{
   int retVal;
      
   if(pAoData->state == running)
   {
      if(pAoData->adapterType & atMF)
         retVal = _PdAOutAsyncStop(pAoData->handle);
      else
         retVal = _PdAOAsyncStop(pAoData->handle);
      if (retVal < 0)
         printf("BufferedAO: PdAOutAsyncStop error %d\n", retVal);

      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
      retVal = _PdClearUserEvents(pAoData->handle, AnalogOut, eAllEvents);
      if (retVal < 0)
         printf("BufferedAI: PdClearUserEvents error %d\n", retVal);
   
      if(pAoData->adapterType & atMF)
         retVal = _PdAOutAsyncTerm(pAoData->handle); 
      else
         retVal = _PdAOAsyncTerm(pAoData->handle);
      if (retVal < 0)
         printf("BufferedAO: PdAOutAsyncTerm error %d\n", retVal);
   
      retVal = _PdUnregisterBuffer(pAoData->handle, (WORD *)pAoData->rawBuffer, AnalogOut);
      if (retVal < 0)
         printf("BufferedAO: PdUnregisterBuffer error %d\n", retVal);

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



void* AOThreadProc(void *arg)
{
   sigset_t set;
   tBufferedAoData* pAoData = (tBufferedAoData*)(arg);

   // block the SIGINT signal, the main thread handles it 
   sigemptyset(&set);
   sigaddset(&set,SIGINT);
   pthread_sigmask(SIG_BLOCK,&set,NULL);

      
   // run the generation
   BufferedAO(pAoData);

   pthread_exit(NULL);
   return NULL;
}


void* AIThreadProc(void *arg)
{
   sigset_t set;
   tBufferedAiData* pAiData = (tBufferedAiData*)(arg);

   // block the SIGINT signal, the main thread handles it 
   sigemptyset(&set);
   sigaddset(&set,SIGINT);
   pthread_sigmask(SIG_BLOCK,&set,NULL);
 
   // run the acquisition
   BufferedAI(pAiData);
     
   pthread_exit(NULL);
   return NULL;
}


void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      G_AiData.abort = TRUE;
      G_AoData.abort = TRUE;
   }
}


int main(int argc, char *argv[])
{
   int i;
   PD_PARAMS params = {0, 1, {0,1}, 20000.0, 0, 5000};
   pthread_t aiThread, aoThread;
   pthread_attr_t attr;
      
      
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_AiData.board = params.board;
   G_AiData.handle = 0;
   G_AiData.abort = FALSE;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i];
   G_AiData.nbOfFrames = 8;
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

   // initializes generation session parameters
   G_AoData.board = params.board;
   G_AoData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AoData.channelList[i] = params.channels[i];
   G_AoData.handle = 0;
   G_AoData.nbOfPointsPerChannel = params.numSamplesPerChannel;
   G_AoData.updateRate = params.frequency;
   G_AoData.nbOfFrames = 4;
   G_AoData.halfFIFOSize = 1024;
   G_AoData.state = closed;
   G_AoData.abort = FALSE;
   G_AoData.rawBuffer = NULL;

   
   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedAIAOExitHandler, &G_AiData);

   signal(SIGINT, SigInt);

   pthread_cond_init(&G_FrameEvent, NULL);
   pthread_mutex_init(&G_FrameMtx, NULL);

   G_FrameBuffer = (unsigned short*)malloc(params.numChannels * params.numSamplesPerChannel *
                                           sizeof(unsigned short));

   InitBufferedAI(&G_AiData);
   InitBufferedAO(&G_AoData);

   // Initialize and set thread detached attribute 
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
 
   if (pthread_create(&aiThread, &attr, AIThreadProc, &G_AiData)) 
   {
      fprintf(stderr, "Cannot create AI thread\n");
   }  

   if (pthread_create(&aoThread, &attr, AOThreadProc, &G_AoData)) 
   {
      fprintf(stderr, "Cannot create AI thread\n");
   }

   pthread_attr_destroy(&attr);

   // wait for both threads to terminate
   pthread_join(aiThread, NULL);
   pthread_join(aoThread, NULL);

   CleanUpBufferedAI(&G_AiData);
   CleanUpBufferedAO(&G_AoData);

   pthread_mutex_destroy(&G_FrameMtx);
   pthread_cond_destroy(&G_FrameEvent);

   free(G_FrameBuffer);
   
   return 0;
}

