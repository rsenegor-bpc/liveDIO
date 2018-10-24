/*****************************************************************************/
/*                Asynchronous Digital input event example                   */
/*                                                                           */
/*  This example shows how to use the powerdaq API to set-up a PD2-DIO board */
/*  to generate an interrupt when the state of a digital input changes.      */
/*  The event coming from the board wakes-up the program, there is no        */
/*  polling involved.                                                        */
/*  For the PD2-MFx or PD2-AO boards look at the DIEvent example.            */
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
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "ParseParams.h"

/* convenience functions */
#define NSECS_PER_SEC 1000000000
#define timespec_normalize(t) {\
	if ((t)->tv_nsec >= NSECS_PER_SEC) { \
		(t)->tv_nsec -= NSECS_PER_SEC; \
		(t)->tv_sec++; \
	} else if ((t)->tv_nsec < 0) { \
		(t)->tv_nsec += NSECS_PER_SEC; \
		(t)->tv_sec--; \
	} \
}

#define timespec_add(t1, t2) do { \
	(t1)->tv_nsec += (t2)->tv_nsec;  \
	(t1)->tv_sec += (t2)->tv_sec; \
	timespec_normalize(t1);\
} while (0)

#define timespec_sub(t1, t2) do { \
	(t1)->tv_nsec -= (t2)->tv_nsec;  \
	(t1)->tv_sec -= (t2)->tv_sec; \
	timespec_normalize(t1);\
} while (0)


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _diEventData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int port;                     // port to use
   int line;                     // digital line to use on the selected port
   int abort;
   tState state;                 // state of the acquisition session
   struct sigaction io_act;
} tDioEventData;


int InitDioEvent(tDioEventData *pDioData);
int DioEvent(tDioEventData *pDioData);
void CleanUpDioEvent(tDioEventData *pDioData);
void sigio_handler(int sig);


static tDioEventData G_DioData;
static int G_EventCount = 0;
static struct timespec G_t;


// exit handler
void DioEventExitHandler(int status, void *arg)
{
   CleanUpDioEvent((tDioEventData *)arg);
}


int InitDioEvent(tDioEventData *pDioData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDioData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("SingleDIO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if(adaptInfo.atType & atPD2DIO)
   {
      printf("This is a PD2-DIO board\n");
   }
   else if(adaptInfo.atType & atPDLMF)
   {
      printf("This is a PDL-MF board\n");
   }
   else
   {
      printf("This board is not a PD2-DIO or PDL-MF\n");
      exit(EXIT_FAILURE);
   }

   pDioData->handle = PdAcquireSubsystem(pDioData->board, DigitalIn, 1);
   if(pDioData->handle < 0)
   {
      printf("DioEvent: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   retVal = _PdSetAsyncNotify(pDioData->handle, &pDioData->io_act, sigio_handler);
   if (retVal < 0)
   {
      printf("DioEvent: Failed to set up notification\n");
      exit(EXIT_FAILURE);
   }


   pDioData->state = unconfigured;

   retVal = _PdDIOReset(pDioData->handle);
   if (retVal < 0)
   {
      printf("DioEvent: PdDIOReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int DioEvent(tDioEventData *pDioData)
{
   int i, retVal;
   DWORD eventsToNotify = eDInEvent;
   DWORD masks[8]={0,0,0,0,0,0,0,0};
            
   // setup the board to generate an interrupt when the selected line
   // change state
   for(i=0; i<8; i++)
      if(i==pDioData->port)
         masks[i] = (1 << pDioData->line);
   retVal = _PdDIOSetIntrMask(pDioData->handle, masks);
   if (retVal < 0)
   {
      printf("DioEvent: _PdDIOSetIntrMask error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   //masks[0] = 0xFFFF;

   pDioData->state = configured;

   // event suff
   retVal = _PdDIOIntrEnable(pDioData->handle, TRUE);
   if (retVal < 0)
   {
      printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   retVal = _PdAdapterEnableInterrupt(pDioData->handle, TRUE);
   if (retVal < 0)
   {
      printf("DioEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   // set events we want to receive
   retVal = _PdSetUserEvents(pDioData->handle, DigitalIn|EdgeDetect, eventsToNotify);
   if (retVal < 0)
   {
      printf("DioEvent: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 
   
   pDioData->state = running;

   return retVal;
}


void CleanUpDioEvent(tDioEventData *pDioData)
{
   int retVal;
      
   if(pDioData->state == running)
   {
      retVal = _PdDIOIntrEnable(pDioData->handle, FALSE);
      if (retVal < 0)
         printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
         
      pDioData->state = configured;
   }

   if(pDioData->state == configured)
   {
      retVal = _PdDIOReset(pDioData->handle);
      if (retVal < 0)
         printf("DioEvent: _PdDIOReset error %d\n", retVal);

      pDioData->state = unconfigured;
   }

   if(pDioData->handle > 0 && pDioData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDioData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("DioEvent: PdReleaseSubsystem error %d\n", retVal);
   }

   pDioData->state = closed;
}


void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      G_DioData.abort = TRUE;
   }
}


//////////////////////////////////////////////////////////////////////////
// Event handler
void sigio_handler(int sig)
{
    unsigned long events;
    int retVal;
    DWORD intData[8];
    DWORD edgeData[8];
    tDioEventData *pDioData = &G_DioData;
    static int oldCount = 0;


    G_EventCount++;

    // figure out what events happened
    retVal = _PdGetUserEvents(pDioData->handle, DigitalIn|EdgeDetect, &events);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }

    //printf("Number of events = %d\n", G_EventCount);

    if (events & eDInEvent)     // something has happend
    {
       _PdDIOGetIntrData(pDioData->handle, intData, edgeData);
    
       //printf("DioEvent: event received! level %d, latch %d\n", 
       //       intData[pDioData->port], edgeData[pDioData->port]);
    }

    // Re-enable user events that asserted.
    retVal = _PdSetUserEvents(pDioData->handle, DigitalIn|EdgeDetect, events);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }

    retVal = _PdDIOIntrEnable(pDioData->handle, TRUE);
    if (retVal < 0)
    {
       printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
    } 

    if((G_EventCount % 100) == 0)
    {
       double delay;
       struct timespec deltaT, t;

       clock_gettime(CLOCK_REALTIME, &t);
       deltaT = t;
       timespec_sub(&deltaT, &G_t);
       delay = deltaT.tv_sec * 1000 + deltaT.tv_nsec / 1000000.0;
       printf("Got %d events in %f ms (%f events/s)\n", G_EventCount - oldCount, 
              delay, (G_EventCount - oldCount) * 1000.0 / delay);
       
       G_t = t;
       oldCount = G_EventCount;
    }
}


int main(int argc, char *argv[])
{
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DioData.board = params.board;
   G_DioData.handle = 0;
   G_DioData.port = 1;
   G_DioData.line = 0;
   G_DioData.abort = 0;
   G_DioData.state = closed;

   signal(SIGINT, SigInt);

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(DioEventExitHandler, &G_DioData);

   // initializes acquisition session
   InitDioEvent(&G_DioData);

   // Start the acquisition
   DioEvent(&G_DioData);

   G_EventCount = 0;
   clock_gettime(CLOCK_REALTIME, &G_t);
   
   while (!G_DioData.abort)
   {
      usleep(500000);
   }

   // Cleanup acquisition
   CleanUpDioEvent(&G_DioData);

   return 0;
}

