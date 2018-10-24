/*****************************************************************************/
/*                    Single scan analog input/output example                */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single scan  */
/*  acquisition followed by the generation of the scan on the analog outputs.*/
/*  The acquisition/genertaion is timed by the RTAI/fusion timer             */
/*  that is appropriate for moderate acquisition speed (up to 50kHz).        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2005 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <native/task.h>
#include <native/event.h>
#include <native/timer.h>

#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "gnuplot.h"
#include "ParseParams.h"

#define errorChk(functionCall) {int error; if((error=functionCall)<0) { \
	                           fprintf(stderr, "Error %d at line %d in function call %s\n", error, __LINE__, #functionCall); \
	                           exit(EXIT_FAILURE);}}

typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _singleAiData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int nbOfChannels;             // number of channels
   int nbOfSamplesPerChannel;    // number of samples per channel
   unsigned int channelList[64];
   double scanRate;              // sampling frequency on each channel
   int polarity;                 // polarity of the signal to acquire, possible value
                                 // is AIN_UNIPOLAR or AIN_BIPOLAR
   int range;                    // range of the signal to acquire, possible value
                                 // is AIN_RANGE_5V or AIN_RANGE_10V
   int inputMode;                // input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL
   int trigger;
   tState state;                 // state of the acquisition session
   int bAbort;
   unsigned short* rawData;
   RT_TASK  task;
   sem_t stopEvent;
   int resolution;
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
   unsigned short ANDMask;
   unsigned short XORMask;
} tSingleAiData;

typedef struct _singleAoData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int abort;
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   unsigned int channelList[64];
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
   int resolution;
   unsigned short* rawData;
} tSingleAoData;



int InitSingleAI(tSingleAiData *pAiData);
void CleanUpSingleAI(tSingleAiData *pAiData);

int InitSingleAO(tSingleAoData *pAiData);
void CleanUpSingleAO(tSingleAoData *pAiData);


static tSingleAiData G_AiData;
static tSingleAoData G_AoData;

// exit handler
void SingleAIAOExitHandler(int status, void *arg)
{
   CleanUpSingleAI((tSingleAiData *)arg);
   CleanUpSingleAO(&G_AoData);
}


int InitSingleAI(tSingleAiData *pAiData)
{
   Adapter_Info adaptInfo;

   // get adapter type
   errorChk(_PdGetAdapterInfo(pAiData->board, &adaptInfo));

   pAiData->resolution = adaptInfo.SSI[AnalogIn].dwChBits;
   pAiData->maxCode = (1 << adaptInfo.SSI[AnalogIn].dwChBits)-1;
   pAiData->ANDMask = adaptInfo.SSI[AnalogIn].wAndMask;
   pAiData->XORMask = adaptInfo.SSI[AnalogIn].wXorMask;

   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("SingleAIAO: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAiData->state = unconfigured;

   errorChk(_PdAInReset(pAiData->handle));

   return 0;
}

int InitSingleAO(tSingleAoData *pAoData)
{
   Adapter_Info adaptInfo;

   // get adapter type
   errorChk(_PdGetAdapterInfo(pAoData->board, &adaptInfo));
   
   pAoData->resolution = adaptInfo.SSI[AnalogOut].dwChBits;
   pAoData->maxCode = (1 << adaptInfo.SSI[AnalogOut].dwChBits) - 1;
   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
   {
      printf("This is an MFx board\n");
   }
   else
   {
      printf("This is an AO32 board\n");
   }

   pAoData->handle = PdAcquireSubsystem(pAoData->board, AnalogOut, 1);
   if(pAoData->handle < 0)
   {
      printf("SingleAO: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAoData->state = unconfigured;

   errorChk(_PdAOutReset(pAoData->handle));
   
   // need also to call this function if the board is a PD2-AO-xx
   if(pAoData->adapterType & atPD2AO)
   {
      errorChk(_PdAO32Reset(pAoData->handle));
   }

   return 0;
}



void CleanUpSingleAI(tSingleAiData *pAiData)
{
   if(pAiData->state == running)
   {
      pAiData->state = configured;
   }

   if(pAiData->state == configured)
   {
      errorChk(_PdAInReset(pAiData->handle));
      pAiData->state = unconfigured;
   }

   if(pAiData->handle >= 0 && pAiData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pAiData->handle, AnalogIn, 0));
   }

   pAiData->state = closed;
}

void CleanUpSingleAO(tSingleAoData *pAoData)
{
   if(pAoData->state == running)
   {
      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
      errorChk(_PdAOutReset(pAoData->handle));
            
      // need also to call this function if the board is a PD2-AO-xx
      if(pAoData->adapterType & atPD2AO)
      {
         errorChk(_PdAO32Reset(pAoData->handle));
      }

      pAoData->state = unconfigured;
   }

   if(pAoData->handle > 0 && pAoData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pAoData->handle, AnalogOut, 0));
   }

   pAoData->state = closed;
}

void SingleAIAOProc(void *arg)
{ 
   int i, ret;
   tSingleAiData *pAiData = (tSingleAiData*)arg;
   tSingleAoData *pAoData = &G_AoData;
   long long taskPeriod;
   DWORD aiCfg, aoCfg, numScans;
   int count = 0;
   RTIME start, stop;
   RT_TIMER_INFO timer_info;
   double duration;
    
   // initializes acquisition session
   InitSingleAI(pAiData);
   InitSingleAO(pAoData);
      
   // setup the board to use software clock
   aiCfg = AIB_CVSTART0 | AIB_CVSTART1 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   aoCfg = 0;

   errorChk(_PdAInSetCfg(pAiData->handle, aiCfg, 0, 0));
   errorChk(_PdAInSetChList(pAiData->handle, pAiData->nbOfChannels, (DWORD*)pAiData->channelList));
   errorChk(_PdAInEnableConv(pAiData->handle, TRUE));
   errorChk(_PdAInSwStartTrig(pAiData->handle));
   
   errorChk(_PdAOutSetCfg(pAoData->handle, aoCfg, 0));
   errorChk(_PdAOutSwStartTrig(pAoData->handle));

   rt_timer_inquire(&timer_info);

   if(timer_info.period == TM_ONESHOT)
   {
      taskPeriod = rt_timer_ns2ticks(1000000000ll / pAiData->scanRate);
      printf("One shot mode: Setting task period to %lld ns (%lld ms)\n", taskPeriod, taskPeriod/ 1000000); 
   }
   else
   {
      taskPeriod = (1000000000ll / pAiData->scanRate)/ timer_info.period;
      printf("Periodic mode: Setting task period to %lld timer ticks, %lld ns\n", taskPeriod, taskPeriod*timer_info.period);
   }

   ret = rt_task_set_periodic(NULL, TM_NOW, taskPeriod);
   if (ret) 
   {
      printf("error while set periodic, code %d\n",ret);
      return;
   }
   
   //switch to primary mode 
   ret = rt_task_set_mode(0, T_PRIMARY, NULL);
   if (ret) 
   {
      printf("error while rt_task_set_mode, code %d\n",ret);
      return;
   }

   errorChk(_PdAInSwClStart(pAiData->handle));
   
   start = rt_timer_read();
   
   while (!pAiData->bAbort) 
   {
      ret = rt_task_wait_period(NULL);
      if (ret) 
      {
         printf("error while rt_task_wait_period, code %d (%s)\n",ret, strerror(-ret));
         break;
      }
      
      errorChk(_PdAInGetSamples(pAiData->handle, pAiData->nbOfChannels, 
                                pAiData->rawData, &numScans));

      if(numScans > 0)
      {
         for (i=0; i<pAiData->nbOfChannels; i++)
         {
            // Convert data between analog input raw binary format and analog output raw binary format         
            pAoData->rawData[i]= ((pAiData->rawData[i] & pAiData->ANDMask) ^ pAiData->XORMask) 
                                  >> (16 - pAiData->resolution);
   
            // Scale data to match input and output channel resolutions (AI 12,14 or 16 bits -> AO 12 or 16 bits)
            pAoData->rawData[i] = pAoData->rawData[i] >> (16 - pAoData->resolution);
         }
   
         if (pAoData->adapterType & atMF)
         {
            if (pAoData->nbOfChannels > 1)
            {
               pAoData->rawData[0] = (pAoData->rawData[0]&0xFFF) | ((pAoData->rawData[1]&0xFFF) << 12);
            }
            errorChk(_PdAOutPutValue(pAoData->handle, pAoData->rawData[0]));
         } 
         else
         {
            for (i=0; i<pAoData->nbOfChannels; i++)
            {
               errorChk(_PdAO32Write(pAoData->handle, i, pAoData->rawData[i]));
            }
         }
      }
      else
      {
         printf("No scans available!\n");
         break;
      }

      errorChk(_PdAInSwClStart(pAiData->handle));

      count++;
   }
   
   stop = rt_timer_read();
   duration = rt_timer_ticks2ns(stop-start)/1000000.0;
   printf("Acquired %d scans in %f ms (%f Hz)\n", count, duration, count * 1000.0/duration); 
   
   CleanUpSingleAO(pAoData);
   CleanUpSingleAI(pAiData);
   
   sem_post(&pAiData->stopEvent);
   
   printf("\n");
}

void SignalHandler(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      G_AiData.bAbort = TRUE;
   }
   else if(signum == SIGTERM)
   {
      printf("Program is terminating\n");
      G_AiData.bAbort = TRUE;
      CleanUpSingleAI(&G_AiData);
      CleanUpSingleAO(&G_AoData);
   }
}

int main(int argc, char *argv[])
{
   int nbOfBoards;
   int i, ret;
   int gain = 0;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};
   
   ParseParameters(argc, argv, &params);

   // Configure acquisition session parameters
   G_AiData.board = params.board;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i] | (gain << 6);
   G_AiData.handle = -1;
   G_AiData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_AiData.scanRate = params.frequency;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_10V;
   G_AiData.inputMode = AIN_SINGLE_ENDED;
   G_AiData.state = closed;
   G_AiData.bAbort = 0;
   if(params.trigger == 1)
      G_AiData.trigger = AIB_STARTTRIG0;
   else if(params.trigger == 2)
      G_AiData.trigger = AIB_STARTTRIG0 + AIB_STARTTRIG1;
   else
      G_AiData.trigger = 0;

   // Configure analog output parameters
   G_AoData.board = params.board;
   G_AoData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AoData.channelList[i] = params.channels[i];
   G_AoData.handle = -1;
   G_AoData.abort = FALSE;
   G_AoData.nbOfPointsPerChannel = params.numSamplesPerChannel;
   G_AoData.updateRate = params.frequency;
   G_AoData.state = closed;


   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(SingleAIAOExitHandler, &G_AiData);
   
   signal(SIGTERM, SignalHandler);
   signal(SIGINT, SignalHandler);

   // no memory-swapping for this programm 
   mlockall(MCL_CURRENT | MCL_FUTURE);

   nbOfBoards = PdGetNumberAdapters();

   // allocate memory for the buffer   
   G_AiData.rawData = (unsigned short *) malloc(G_AiData.nbOfChannels * sizeof(unsigned short));
   if(G_AiData.rawData == NULL)
   {
      printf("SingleAIAO: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   G_AoData.rawData = (unsigned short *) malloc(G_AoData.nbOfChannels * sizeof(unsigned short));
   if(G_AoData.rawData == NULL)
   {
      printf("SingleAIAO: could not allocate enough memory for the generation buffer\n");
      exit(EXIT_FAILURE);
   }
   
   ret = rt_task_create(&G_AiData.task,"SingleAiTask",0,T_HIPRIO,0);
   if (ret) 
   {
      printf("failed to create task, code %d\n",ret);
      exit(EXIT_FAILURE);
   }
   
   ret = sem_init(&G_AiData.stopEvent,0,0);
   if (ret) 
   {
      printf("failed to create semaphore, code %d\n",ret);
      exit(EXIT_FAILURE);
   }
   
   ret = rt_task_start(&G_AiData.task,&SingleAIAOProc,&G_AiData);
   if (ret) 
   {
      printf("failed to start write_task, code %d\n",ret);
      exit(EXIT_FAILURE);
   }
   
   pause();
      
   //sleep(2);

   printf("received signal\n");

   G_AiData.bAbort = 1;

   ret = sem_wait(&G_AiData.stopEvent);
   if(ret)
   {
      printf("sem_wait failed with error %d\n", ret);
   }
   
   // free acquisition and generation buffers
   free(G_AoData.rawData);
   free(G_AiData.rawData);
   
   return 0;
}

