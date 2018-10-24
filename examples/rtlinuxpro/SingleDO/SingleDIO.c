/*****************************************************************************/
/*                    Single digital input/output example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single       */
/*  digital acquisition or update with a PD2-DIO boards.                     */
/*  For the PDx-MFx and PD2-AO boards look at the SingleDI and SingleDO      */
/*  examples.                                                                */
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


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _singleDioData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int nbOfPorts;                // nb of ports of 16 lines to use
   unsigned char OutPorts;       // mask for ports of 16 lines to use for output
   int nbOfSamplesPerPort;       // number of samples per port
   unsigned long portList[64];
   double scanRate;              // sampling frequency on each port
   tState state;                 // state of the acquisition session
   int bAbort;
   pthread_t thread;
} tSingleDioData;

#define errorChk(functionCall) {int error; if((error=functionCall)<0) { \
	                           rtl_printf("Error %d at line %d in function call %s\n", error, __LINE__, #functionCall); \
	                           pthread_exit((void*)error);}}

int InitSingleDIO(tSingleDioData *pDioData);
int SingleDIO(tSingleDioData *pDioData);
void CleanUpSingleDIO(tSingleDioData *pDioData);


static tSingleDioData G_DioData;


int InitSingleDIO(tSingleDioData *pDioData)
{
   pDioData->handle = PdAcquireSubsystem(pDioData->board, DigitalIn, 1);
   if(pDioData->handle < 0)
   {
      printf("SingleDIO: PdAcquireSubsystem failed\n");
      pthread_exit(NULL);;
   }

   pDioData->state = unconfigured;

   errorChk(_PdDIOReset(pDioData->handle));
   
   return 0;
}

void CleanUpSingleDIO(tSingleDioData *pDioData)
{
   if(pDioData->state == running)
   {
      errorChk(_PdDIOReset(pDioData->handle));
      
      pDioData->state = unconfigured;
   }

   if(pDioData->handle > 0 && pDioData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pDioData->handle, DigitalIn, 0));
   }

   pDioData->state = closed;
}

void* SingleDIOProc(void *arg)
{ 
   tSingleDioData *pDioData = (tSingleDioData*)arg;
   hrtime_t taskPeriod;
   int count, j;
   DWORD values[8];
   struct timespec  t1, t2, abstime;
   long duration;

   // initializes acquisition session
   InitSingleDIO(pDioData);

   errorChk(_PdDIOEnableOutput(pDioData->handle, pDioData->OutPorts));

   taskPeriod = ((hrtime_t)((1.0/pDioData->scanRate) * NSECS_PER_SEC));
      
   pDioData->state = running;

   clock_gettime(CLOCK_REALTIME, &t1);
   abstime = t1;

   count = 0;
   while(!pDioData->bAbort)
   {
      count++;

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, taskPeriod);

      for(j=0; j<8; j++)
      {
         values[j] = count%2 ? 0xFFFFFFFF : 0;
      }
      
      errorChk(_PdDIOWriteAll(pDioData->handle, values));

      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &abstime, NULL);
   }

   clock_gettime(CLOCK_REALTIME, &t2);

   timespec_sub(&t2, &t1);

   duration = t2.tv_sec*1000000.0+t2.tv_nsec/1000.0;

   rtl_printf("%d us\n", duration/count);

   // initializes acquisition session
   CleanUpSingleDIO(pDioData);

   return NULL;
}

int main(int argc, char *argv[])
{
   int i;
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads
   
   // initializes acquisition session parameters
   G_DioData.board = 0;
   G_DioData.nbOfPorts = 4;
   for(i=0; i<G_DioData.nbOfPorts; i++)
       G_DioData.portList[i] = i;
   G_DioData.handle = 0;
   G_DioData.OutPorts = 0x0F; // use ports 1 and 2 for output
   G_DioData.nbOfSamplesPerPort = 1000000;
   G_DioData.scanRate = 100000;
   G_DioData.state = closed;
   G_DioData.bAbort = 0;

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&G_DioData.thread, &attrib, SingleDIOProc, &G_DioData);

   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   G_DioData.bAbort = 1;
   rtl_usleep(500000);
   pthread_cancel(G_DioData.thread);
   pthread_join(G_DioData.thread, NULL);

   return 0;
}

