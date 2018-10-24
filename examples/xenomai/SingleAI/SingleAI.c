/*****************************************************************************/
/*                    Single scan analog input example                       */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single scan  */
/*  acquisition. The acquisition is performed in a software timed fashion    */
/*  that is appropriate for slow speed acquisition (up to 500Hz).            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2001 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

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
   unsigned int configBits;
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
} tSingleAiData;


int InitSingleAI(tSingleAiData *pAiData);
int SingleAI(tSingleAiData *pAiData, double *buffer);
void CleanUpSingleAI(tSingleAiData *pAiData);


static tSingleAiData G_AiData;

// exit handler
void SingleAIExitHandler(int status, void *arg)
{
   CleanUpSingleAI((tSingleAiData *)arg);
}


int InitSingleAI(tSingleAiData *pAiData)
{
   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("SingleAI: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAiData->state = unconfigured;

   errorChk(_PdAInReset(pAiData->handle));

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

void SingleAiProc(void *arg)
{ 
   int ret;
   tSingleAiData *pAiData = (tSingleAiData*)arg;
   long long taskPeriod;
   DWORD numScans, totalScans=0;
   int count = 0;
   RTIME start, stop;
   RT_TIMER_INFO timer_info;
   double duration;
    
   // initializes acquisition session
   InitSingleAI(pAiData);
      
   // setup the board to use software clock
   pAiData->configBits = AIB_CVSTART0 | AIB_CVSTART1 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   errorChk(_PdAInSetCfg(pAiData->handle, pAiData->configBits, 0, 0));
   errorChk(_PdAInSetChList(pAiData->handle, pAiData->nbOfChannels, (DWORD*)pAiData->channelList));
   errorChk(_PdAInEnableConv(pAiData->handle, TRUE));
   errorChk(_PdAInSwStartTrig(pAiData->handle));
   
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
   
   start = rt_timer_read();
   
   while (!pAiData->bAbort && count < pAiData->nbOfSamplesPerChannel) 
   {
      errorChk(_PdAInSwClStart(pAiData->handle));
      
      ret = rt_task_wait_period(NULL);
      if (ret) 
      {
         printf("error while rt_task_wait_period, code %d (%s)\n",ret, strerror(-ret));
         break;
      }
      
      errorChk(_PdAInGetSamples(pAiData->handle, pAiData->nbOfChannels, 
                                pAiData->rawData + count*pAiData->nbOfChannels, &numScans));
      
      totalScans = totalScans + numScans;
      count++;
   }
   
   stop = rt_timer_read();
   duration = rt_timer_ticks2ns(stop-start)/1000000.0;
   printf("Acquired %d scans (%ld) in %f ms (%f Hz)\n", count, totalScans, duration, count * 1000.0/duration); 
   
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
   }
}

int main(int argc, char *argv[])
{
   double *buffer = NULL;
   int nbOfBoards;
   int i, ret;
   int gain = 0;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 1000};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
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

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(SingleAIExitHandler, &G_AiData);
   
   signal(SIGTERM, SignalHandler);
   signal(SIGINT, SignalHandler);

   // no memory-swapping for this programm 
   mlockall(MCL_CURRENT | MCL_FUTURE);


   nbOfBoards = PdGetNumberAdapters();

   // allocate memory for the buffer
   buffer = (double *) malloc(G_AiData.nbOfChannels * 
                              G_AiData.nbOfSamplesPerChannel *
                              sizeof(double));
   if(buffer == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }
   
   G_AiData.rawData = (unsigned short *) malloc(G_AiData.nbOfChannels * 
                                                G_AiData.nbOfSamplesPerChannel * 
                                                sizeof(unsigned short));
   if(G_AiData.rawData == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
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
   
   ret = rt_task_start(&G_AiData.task,&SingleAiProc,&G_AiData);
   if (ret) 
   {
      printf("failed to start write_task, code %d\n",ret);
      exit(EXIT_FAILURE);
   }
   
   //pause();
      
   // Stop task
   //G_AiData.bAbort = 1;

   ret = sem_wait(&G_AiData.stopEvent);
   if(ret)
   {
      printf("sem_wait failed with error %d\n", ret);
   }

   errorChk(PdAInRawToVolts(G_AiData.board, G_AiData.configBits, G_AiData.rawData, buffer, 
                            G_AiData.nbOfSamplesPerChannel * G_AiData.nbOfChannels));
   
   // Call Gnuplot to display the acquired data
   sendDataToGnuPlot(-1, 0.0, 1.0/G_AiData.scanRate, buffer, 
                     G_AiData.nbOfChannels, G_AiData.nbOfSamplesPerChannel);
   
   // free acquisition buffer
   free(buffer);
   free(G_AiData.rawData);
   
   return 0;
}

