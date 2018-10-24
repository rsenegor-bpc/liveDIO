//===========================================================================
//
// NAME:    UCT_Async.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver timer event program.
//          This example shows how to program one of the PD2-MF board's
//          8254 timer to generate an interrupt periodically.
//          The interrupt is transmitted to the program as an event by
//          the kernel driver.
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    20-JAN-2006
//
// REV:     3.6.11
//
// R DATE:
//
// HISTORY:
//
//
//---------------------------------------------------------------------------
//      Copyright (C) 2006 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------
//

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

typedef enum _clockSource
{
   clkInternal1MHz,
   clkExternal,
   clkSoftware,
   clkUct0Out
} tClockSource;

typedef enum _gateSource
{
   gateSoftware,
   gateHardware   
} tGateSource;

typedef enum _counterMode
{
   EventCounting,
   SinglePulse,
   PulseTrain,
   SquareWave,
   swTriggeredStrobe,
   hwTriggeredStrobe   
} tCounterMode;


typedef struct _uctEventData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int counter;                  // counter to be used
   tClockSource clockSource;
   tGateSource gateSource;
   tCounterMode mode;
   tState state;                 // state of the acquisition session
   int abort;
   pthread_t  thread;
} tUctEventData;


int InitUctEvent(tUctEventData *pUctData);
int UctEvent(tUctEventData *pUctData, double frequency);
void CleanUpUctEvent(tUctEventData *pUctData);
void sigio_handler(int sig);

static unsigned int intClocks[3] = {UTB_CLK0, UTB_CLK1, UTB_CLK2};
static unsigned int extClocks[3] = {UTB_CLK0|UTB_CLK0_1, UTB_CLK1|UTB_CLK1_1, UTB_CLK2|UTB_CLK2_1};
static unsigned int uct0Clocks[3] = {UTB_CLK0_1, UTB_CLK1_1, UTB_CLK2_1};
static unsigned int hwGates[3] = {UTB_GATE0, UTB_GATE1, UTB_GATE2};
static unsigned int swGates[3] = {UTB_SWGATE0, UTB_SWGATE1, UTB_SWGATE2};
static unsigned int slctCounter[3] = {UCT_SelCtr0, UCT_SelCtr1, UCT_SelCtr2};
static unsigned int readCtrl[3] = {UCTREAD_UCT0 | UCTREAD_2BYTES, UCTREAD_UCT1 | UCTREAD_2BYTES, UCTREAD_UCT2 | UCTREAD_2BYTES};
static unsigned int uctEvents[3] = {eUct0Event, eUct1Event, eUct2Event};

static tUctEventData G_UctData;


int InitUctEvent(tUctEventData *pUctData)
{
   pUctData->handle = PdAcquireSubsystem(pUctData->board, CounterTimer, 1);
   if(pUctData->handle < 0)
   {
      printf("UctEvent: PdAcquireSubsystem failed\n");
      pthread_exit(NULL);
   }

   pUctData->state = unconfigured;

   errorChk(_PdUctReset(pUctData->handle));
   
   return 0;
}


int UctEvent(tUctEventData *pUctData, double frequency)
{
   unsigned int uctCfg = 0;
   unsigned int uctCtrl = 0;
   unsigned short nbTicks;
   unsigned int eventsToNotify = eTimeout | uctEvents[pUctData->counter];
   unsigned int events;
   unsigned int count;
   int numEvents = 0;
   struct timeval t1, t2;
    
      
   switch(pUctData->clockSource)
   {
   case clkInternal1MHz:
      uctCfg = uctCfg | intClocks[pUctData->counter];
      break;
   case clkExternal:
      uctCfg = uctCfg | extClocks[pUctData->counter];
      break;
   case clkSoftware:
      break;
   case clkUct0Out:
      uctCfg = uctCfg | uct0Clocks[pUctData->counter];
      break;
   }

   switch(pUctData->gateSource)
   {
   case gateSoftware:
      uctCfg = uctCfg | swGates[pUctData->counter];
      break;
   case gateHardware:
      uctCfg = uctCfg | hwGates[pUctData->counter];
      break;
   }

   switch(pUctData->mode)
   {
   case EventCounting:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode0 | UCT_RW16bit;
      break;
   case SinglePulse:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode1 | UCT_RW16bit;
      break;
   case PulseTrain:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode2 | UCT_RW16bit;
      break;
   case SquareWave:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode3 | UCT_RW16bit;
      break;
   case swTriggeredStrobe:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode4 | UCT_RW16bit;
      break;
   case hwTriggeredStrobe:
      uctCtrl = uctCtrl | slctCounter[pUctData->counter] | UCT_Mode5 | UCT_RW16bit;
      break;
   }

   // Make sure that we have some clock to write                    
   errorChk(_PdUctSetCfg(pUctData->handle, intClocks[pUctData->counter]));
   
   // Configure counter mode and period of the square wave
   nbTicks = (unsigned short)(1000000.0 / frequency);
   printf("Will generate an event every %d ticks\n", nbTicks);
   errorChk(_PdUctWrite(pUctData->handle, uctCtrl | (nbTicks << 8)));
   
   //Set up the UCT subsystem                        
   errorChk(_PdUctSetCfg(pUctData->handle, uctCfg));
   errorChk(_PdAdapterEnableInterrupt(pUctData->handle, TRUE));
   
   // Clear & reset counter/timer events
   errorChk(_PdSetUserEvents(pUctData->handle, CounterTimer, uctEvents[pUctData->counter]));
   
   pUctData->state = configured;

   // set the software gate to start counting
   if(pUctData->gateSource == gateSoftware)
   {
      errorChk(_PdUctSwSetGate(pUctData->handle, 1 << pUctData->counter));
   }

   pUctData->state = running;

   //gettimeofday(&t1, NULL);

   while(!pUctData->abort)
   {
      events = _PdWaitForEvent(pUctData->handle, eventsToNotify, 5000);

      if (events & eTimeout)
      {
	  printf("DiEvent: wait timed out\n"); 
	  //return -1;
      }
    
      // deal with the events
      if (events & eUct0Event)
      {
          // read the number of counted events
          errorChk(_PdUctRead(pUctData->handle, readCtrl[0], &count));
             
          //printf("UCT_Event: counter 0 counted %d events\n", count);       
      }

       if (events & eUct1Event)
       {
          // read the number of counted events
          errorChk(_PdUctRead(pUctData->handle, readCtrl[1], &count));
             
          printf("UCT_Event: counter 1 counted %d events\n", count);       
       }

       if (events & eUct2Event)
       {
          // read the number of counted events
          errorChk(_PdUctRead(pUctData->handle, readCtrl[2], &count));
          
          printf("UCT_Event: counter 2 counted %d events\n", count);       
       }
       
       numEvents++;

       // Re-enable user events that asserted.
       errorChk(_PdSetUserEvents(pUctData->handle, CounterTimer, eventsToNotify));
   }
   
   //gettimeofday(&t2, NULL);
   //printf("UCT_Event: got %d events in %f ms, %f events/s\n", numEvents, 
   //       (t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0,
   //       numEvents / ((t2.tv_sec-t1.tv_sec) + (t2.tv_usec-t1.tv_usec)/1000000.0));
   printf("Got %d events\n", numEvents);

   return 0;
}


void CleanUpUctEvent(tUctEventData *pUctData)
{
   if(pUctData->state == running)
   {
      // disable the software gate to stop counting
      if(pUctData->gateSource == gateSoftware)
      {
         errorChk(_PdUctSwSetGate(pUctData->handle, 0 << pUctData->counter));
      }

      pUctData->state = configured;
   }

   if(pUctData->state == configured)
   {
      errorChk(_PdAdapterEnableInterrupt(pUctData->handle, FALSE));    
      errorChk(_PdUctReset(pUctData->handle));
      
      pUctData->state = unconfigured;
   }

   if(pUctData->handle > 0 && pUctData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pUctData->handle, CounterTimer, 0));
   }

   pUctData->state = closed;
}

void* UctEventProc(void *arg)
{ 
   tUctEventData *pUctData = (tUctEventData*)arg;

   InitUctEvent(pUctData);

   // Start generating events at specified frequency in Hz
   UctEvent(pUctData, 25000.0);

   CleanUpUctEvent(pUctData);

   return NULL;
}


//////////////////////////////////////////////////////////////////////////
// Main routine
int main(int argc, char* argv[])
{   
   int ret;
   int iNumAdp;
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads


   iNumAdp = ret = PdGetNumberAdapters();
   printf("Number of adapters installed: %d\n\n", ret);

   // initializes acquisition session parameters
   G_UctData.board = 1;
   G_UctData.handle = 0;
   G_UctData.counter = 0;
   G_UctData.abort = FALSE;
   G_UctData.clockSource = clkInternal1MHz;
   G_UctData.gateSource = gateSoftware;
   G_UctData.mode = PulseTrain;
   G_UctData.state = closed;

   // create the thread
   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread
   pthread_create(&G_UctData.thread, &attrib, UctEventProc, &G_UctData);

   rtl_main_wait();

   printf("UCT_Event is ending\n");

   G_UctData.abort = 1;
   rtl_usleep(1000000);
   //pthread_cancel(G_UctData.thread);
   pthread_join(G_UctData.thread, NULL);

   return 0;
}
