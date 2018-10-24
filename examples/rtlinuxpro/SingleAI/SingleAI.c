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

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "powerdaq.h"
#include "powerdaq32.h"


#define errorChk(functionCall) {int error; if((error=functionCall)<0) { \
	                           rtl_printf("Error %d at line %d in function call %s\n", error, __LINE__, #functionCall); \
	                           pthread_exit((void*)error);}}

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
   unsigned long channelList[64];
   unsigned long configBits;
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
   pthread_t  thread;
} tSingleAiData;


int InitSingleAI(tSingleAiData *pAiData);
int SingleAI(tSingleAiData *pAiData, double *buffer);
void CleanUpSingleAI(tSingleAiData *pAiData);


static tSingleAiData G_AiData;


int InitSingleAI(tSingleAiData *pAiData)
{
   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("SingleAI: PdAcquireSubsystem failed\n");
      pthread_exit(NULL);
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

void* SingleAiProc(void *arg)
{ 
   tSingleAiData *pAiData = (tSingleAiData*)arg;
   hrtime_t taskPeriod;
   DWORD numScans, totalScans=0;
   int count = 0;
   struct timespec abstime, starttime, stoptime;
   long duration;
      
   rtl_printf("starting thread\n");
       
   // initializes acquisition session
   InitSingleAI(pAiData);
      
   // setup the board to use software clock
   pAiData->configBits = AIB_CVSTART0 | AIB_CVSTART1 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   errorChk(_PdAInSetCfg(pAiData->handle, pAiData->configBits, 0, 0));
   errorChk(_PdAInSetChList(pAiData->handle, pAiData->nbOfChannels, (DWORD*)pAiData->channelList));
   errorChk(_PdAInEnableConv(pAiData->handle, TRUE));
   errorChk(_PdAInSwStartTrig(pAiData->handle));
   
   taskPeriod = ((hrtime_t)((1.0/pAiData->scanRate) * NSECS_PER_SEC));
   
   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);
   starttime = abstime;
   
   while (!pAiData->bAbort && count < pAiData->nbOfSamplesPerChannel) 
   {
      // Setup absolute time for next loop
      timespec_add_ns(&abstime, taskPeriod);
      
      errorChk(_PdAInSwClStart(pAiData->handle));
      
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);
      
      errorChk(_PdAInGetSamples(pAiData->handle, pAiData->nbOfChannels, 
                                pAiData->rawData + count*pAiData->nbOfChannels, &numScans));
      
      totalScans = totalScans + numScans;
      count++;
   }
   
   clock_gettime(CLOCK_REALTIME, &stoptime);
   timespec_sub(&stoptime, &starttime);

   duration = stoptime.tv_sec*1000000.0+stoptime.tv_nsec/1000.0;
   
   rtl_printf("Acquired %d scans (%ld) in %ld us\n", count, totalScans, 
              duration); 
   
   CleanUpSingleAI(pAiData);
   
   return NULL;
}

int main(int argc, char *argv[])
{
   int i, fifod;
   int gain = 0;
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads
   

   // initializes acquisition session parameters
   G_AiData.board = 0;
   G_AiData.nbOfChannels = 1;
   for(i=0; i<G_AiData.nbOfChannels; i++)
       G_AiData.channelList[i] = i | (gain << 6);
   G_AiData.handle = -1;
   G_AiData.nbOfSamplesPerChannel = 1000;
   G_AiData.scanRate = 50000.0;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_10V;
   G_AiData.inputMode = AIN_SINGLE_ENDED;
   G_AiData.state = closed;
   G_AiData.bAbort = 0;
   /*if(params.trigger == 1)
      G_AiData.trigger = AIB_STARTTRIG0;
   else if(params.trigger == 2)
      G_AiData.trigger = AIB_STARTTRIG0 + AIB_STARTTRIG1;
   else*/
      G_AiData.trigger = 0;

   // make a the FIFO that is available to RTLinux and Linux applications 
   mkfifo( "/dev/SingleAI", 0666);
   fifod = open("/dev/SingleAI", O_WRONLY | O_NONBLOCK);

   // Resize the FIFO so that it can hold 8K of data
   ftruncate(fifod, 8 << 10);

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&G_AiData.thread, &attrib, SingleAiProc, &G_AiData);

   G_AiData.rawData = (unsigned short *) rtl_gpos_malloc(G_AiData.nbOfChannels * 
                                                G_AiData.nbOfSamplesPerChannel * 
                                                sizeof(unsigned short));
   if(G_AiData.rawData == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      return -ENOMEM;
   }
   
   
   
   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   G_AiData.bAbort = 1;
   rtl_usleep(500000);
   pthread_cancel(G_AiData.thread);
   pthread_join(G_AiData.thread, NULL);

   // Close and remove the FIFO
   close(fifod);
   unlink("/dev/SingleAI");
   
   // free acquisition buffer
   rtl_gpos_free(G_AiData.rawData);

   return 0;
}

