/*****************************************************************************/
/*                Buffered analog input and output example                   */
/*                                                                           */
/*  This example shows how to use the powerdaq API to simultaneously perform */
/*  a buffered acquisition and a buffered generation.                        */
/*  The acquisition and gneration are performed in a hardware timed fashion  */
/*  that is appropriate for fast speed (> 10kHz).                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2003 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
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
   int trigger;
   tState state;                 // state of the acquisition session
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
} tBufferedAoData;


int InitBufferedAI(tBufferedAiData *pAiData);
int BufferedAI(tBufferedAiData *pAiData, double *buffer);
void CleanUpBufferedAI(tBufferedAiData *pAiData);
int InitBufferedAO(tBufferedAoData *pAoData);
int BufferedAO(tBufferedAoData *pAoData, double *buffer);
void CleanUpBufferedAO(tBufferedAoData *pAoData);

void* AIThreadProc(void* arg);
void* AOThreadProc(void* arg);

double getWave(int index, int channel);
void SigInt(int signum);


static tBufferedAiData G_AiData;
static tBufferedAoData G_AoData;

// exit handler
void BufferedAIAOExitHandler(int status, void *arg)
{
   CleanUpBufferedAI(&G_AiData);
   CleanUpBufferedAO(&G_AoData);
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


int BufferedAI(tBufferedAiData *pAiData, double *buffer)
{
   int retVal;
   DWORD divider;
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD scanIndex, numScans;
   int i;
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

         printf("BufferedAI: got %ld scans at %ld\n", numScans, scanIndex);

         retVal = PdAInRawToVolts(pAiData->board, aiCfg, pAiData->rawBuffer + scanIndex * pAiData->nbOfChannels,
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

   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
      printf("This is an MFx board\n");
   else if(pAoData->adapterType & atPD2AO)
      printf("This is an AO32 board\n");
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


int BufferedAO(tBufferedAoData *pAoData, double *voltBuffer)
{
   int retVal;
   DWORD aoCfg;
   DWORD cvDivider;
   DWORD count = 0;
   DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError | eStopped;
   DWORD events;
   DWORD multiplier;
   DWORD scanIndex, numScans;
   int i, j, k;
   

   // setup the board to use hardware internal clock
   aoCfg = AOB_INTCVSBASE | AOB_CVSTART0;  // 33 MHz internal timebase
      
   if(pAoData->adapterType & atPD2AO)
   {
      multiplier = 0xFFFF;  
      retVal = _PdRegisterBuffer(pAoData->handle, (WORD**)&pAoData->rawBuffer, AnalogOut,
                              pAoData->nbOfFrames, pAoData->nbOfPointsPerChannel,
                              pAoData->nbOfChannels, 
                              BUF_BUFFERWRAPPED | BUF_DWORDVALUES);
   }
   else
   {
      multiplier = 0xFFF;  
      retVal = _PdRegisterBuffer(pAoData->handle, (WORD**)&pAoData->rawBuffer, AnalogOut,
                              pAoData->nbOfFrames, pAoData->nbOfPointsPerChannel,
                              1, 
                              BUF_BUFFERWRAPPED | BUF_DWORDVALUES);
      aoCfg |= AOB_DMAEN;
   }
   
   if (retVal < 0)
   {
      printf("BufferedAO: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }
   
   
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
                                  i*pAoData->nbOfChannels+j] = (pAoData->channelList[j] << 16) |
                                  (unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier);
            }
            else // MF boards have only 2 12 bits AO channels, data for both channels are combined in a single
                 // value: ch0 | ch1 << 12
            {
               if(0==j)
               {
                  pAoData->rawBuffer[k*pAoData->nbOfPointsPerChannel+i] = 
                                     (unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier);
               }
               else if(1==j)
               {
                  pAoData->rawBuffer[k*pAoData->nbOfPointsPerChannel+i] |= 
                                     ((unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier)) << 12;
               }
            }
         }
      }
   }

   // set clock divider, assuming that we use the 11MHz timebase
   cvDivider = 33000000.0 / pAoData->updateRate -1;
   
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

         // update the frame that was just generated with new data
         for(i=0; i<pAoData->nbOfPointsPerChannel; i++)
         {
            for(j=0; j<pAoData->nbOfChannels; j++)
            {
               // We use the DWORD data format on AO boards, with this format, we need to specify
               // the ouput channel in the upper 16 bits of each sample
               if(pAoData->adapterType & atPD2AO)
               {
                  pAoData->rawBuffer[scanIndex*pAoData->nbOfChannels+
                                     i*pAoData->nbOfChannels+j] = (pAoData->channelList[j] << 16) |
                                     (unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier);
               }
               else // MF boards have only 2 12 bits AO channels, data for both channels are combined in a single
                    // value: ch0 | ch1 << 12
               {
                   if(0==j)
                   {
                      pAoData->rawBuffer[scanIndex+i] = 
                                          (unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier);
                   }
                   else if(1==j)
                   {
                      pAoData->rawBuffer[scanIndex+i] |= 
                                          ((unsigned int)floor((voltBuffer[i*pAoData->nbOfChannels+j] + 10.0) / 20.0 * multiplier)) << 12;
                   }
               }
            }
         }

         count = count + numScans;
   
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


double getWave(int index, int channel)
{
   int nbOfPointsPerCycle;
   double value;

   nbOfPointsPerCycle = ((index / 500) + 1) + 500;

   value = 0.5 * sin(2*2.0 * 3.1415 * index * (channel + 1) / nbOfPointsPerCycle);

   //value = (index % 500) * 20.0/500 - 10.0;
   
   return(value);
}


void* AOThreadProc(void *arg)
{
   sigset_t set;
   int nbOfSubBuffers;
   int i,j;
   double *voltBuffer;
   tBufferedAoData* pAoData = (tBufferedAoData*)(arg);

   // block the SIGINT signal, the main thread handles it 
   sigemptyset(&set);
   sigaddset(&set,SIGINT);
   pthread_sigmask(SIG_BLOCK,&set,NULL);

   // allocate memory for the generation buffer
   voltBuffer = (double *) malloc(G_AoData.nbOfChannels * G_AoData.nbOfPointsPerChannel *
                              sizeof(double));
   if(voltBuffer == NULL)
   {
      printf("BufferedAO: could not allocate enough memory for the generation buffer\n");
      exit(EXIT_FAILURE);
   }

   // writes data to buffer. Each channel will generate a sinus of different frequency
   nbOfSubBuffers = pAoData->nbOfPointsPerChannel * pAoData->nbOfChannels / pAoData->halfFIFOSize;
   for(i=0; i<pAoData->nbOfPointsPerChannel; i++)
   {
      for(j=0; j<pAoData->nbOfChannels; j++)
      {
         voltBuffer[i*pAoData->nbOfChannels + j] = getWave(i, j);
      }
   }

   // run the generation
   BufferedAO(pAoData, voltBuffer);

   // free generation buffer
   free(voltBuffer);

   return NULL;
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

   // allocate memory for the buffer
   buffer = (double *) malloc(pAiData->nbOfChannels * pAiData->nbOfSamplesPerChannel *
                               sizeof(double));
   if(buffer == NULL)
   {
      printf("BufferedAIAO: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   // run the acquisition
   BufferedAI(pAiData, buffer);

   
   // Call Gnuplot to display the acquired data
   sendDataToGnuPlot(pAiData->handle, 0.0, 1.0/pAiData->scanRate, buffer, 
                     pAiData->nbOfChannels, pAiData->nbOfSamplesPerChannel);

   // free acquisition buffer
   free(buffer);

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
   PD_PARAMS params = {0, 1, {0,1}, 50000.0, 0, 4096};
   pthread_t aiThread, aoThread;
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_AiData.board = params.board;
   G_AiData.handle = 0;
   G_AiData.abort = FALSE;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i];
   G_AiData.nbOfFrames = 4;
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
   G_AoData.nbOfFrames = 4 ;
   G_AoData.halfFIFOSize = 1024;
   G_AoData.state = closed;
   G_AoData.abort = FALSE;
   G_AoData.rawBuffer = NULL;


   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedAIAOExitHandler, &G_AiData);

   signal(SIGINT, SigInt);

   InitBufferedAI(&G_AiData);
   InitBufferedAO(&G_AoData);
   
   if (pthread_create(&aiThread, NULL, AIThreadProc, &G_AiData)) 
   {
      fprintf(stderr, "Cannot create AI thread\n");
   }  

   if (pthread_create(&aoThread, NULL, AOThreadProc, &G_AoData)) 
   {
      fprintf(stderr, "Cannot create AI thread\n");
   }

   pthread_join(aiThread, NULL);
   pthread_join(aoThread, NULL);

   CleanUpBufferedAI(&G_AiData);
   CleanUpBufferedAO(&G_AoData);
   
   return 0;
}

