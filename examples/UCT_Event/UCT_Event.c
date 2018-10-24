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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "ParseParams.h"


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
   struct sigaction io_act;
   int abort;
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

// exit handler
void UctEventExitHandler(int status, void *arg)
{
   CleanUpUctEvent((tUctEventData *)arg);
}

void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping counter\n");
      G_UctData.abort = TRUE;
   }
}


int InitUctEvent(tUctEventData *pUctData)
{
   int retVal = 0;

   pUctData->handle = PdAcquireSubsystem(pUctData->board, CounterTimer, 1);
   if(pUctData->handle < 0)
   {
      printf("UctEvent: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pUctData->state = unconfigured;

   retVal = _PdUctReset(pUctData->handle);
   if (retVal < 0)
   {
      printf("UctEvent: PdUctReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int UctEvent(tUctEventData *pUctData, double frequency)
{
   int retVal;
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
   retVal = _PdUctSetCfg(pUctData->handle, intClocks[pUctData->counter]);
   if (retVal < 0)
   {
      printf("UctEvent: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // Configure counter mode and period of the square wave
   nbTicks = (unsigned short)(1000000.0 / frequency);
   printf("Will generate an event every %d ticks\n", nbTicks);
   retVal = _PdUctWrite(pUctData->handle, uctCtrl | (nbTicks << 8));
   if (retVal < 0)
   {
      printf("UctEvent: _PdUctWrite failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   //Set up the UCT subsystem                        
   retVal = _PdUctSetCfg(pUctData->handle, uctCfg);
   if (retVal < 0)
   {
      printf("UctEvent: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdAdapterEnableInterrupt(pUctData->handle, TRUE);
   if (retVal < 0)
   {
      printf("UctEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   // Clear & reset counter/timer events
   retVal = _PdSetUserEvents(pUctData->handle, CounterTimer, uctEvents[pUctData->counter]);
   if (retVal < 0)
   {
      printf("UctEvent: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pUctData->state = configured;

   // set the software gate to start counting
   if(pUctData->gateSource == gateSoftware)
   {
      retVal = _PdUctSwSetGate(pUctData->handle, 1 << pUctData->counter);
      if (retVal < 0)
      {
         printf("UctEvent: _PdUctSwSetGate failed with error code %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   }


   pUctData->state = running;

   gettimeofday(&t1, NULL);

   while(!pUctData->abort)
   {
      events = _PdWaitForEvent(pUctData->handle, eventsToNotify, 5000);
	   
	  if (events & eTimeout)
	  {
	     printf("DiEvent: wait timed out\n"); 
	     exit(EXIT_FAILURE);
	  }
    
      // deal with the events
      if (events & eUct0Event)
      {
          // read the number of counted events
          retVal = _PdUctRead(pUctData->handle, readCtrl[0], &count);
          if (retVal < 0)
          {
             printf("UctEvent: _PdUctRead failed with error code %d\n", retVal);
             exit(EXIT_FAILURE);
          }
   
          //printf("UCT_Event: counter 0 counted %d events\n", count);       
      }

       if (events & eUct1Event)
       {
          // read the number of counted events
          retVal = _PdUctRead(pUctData->handle, readCtrl[1], &count);
          if (retVal < 0)
          {
             printf("UctEvent: _PdUctRead failed with error code %d\n", retVal);
             exit(EXIT_FAILURE);
          }
   
          printf("UCT_Event: counter 1 counted %d events\n", count);       
       }

       if (events & eUct2Event)
       {
          // read the number of counted events
          retVal = _PdUctRead(pUctData->handle, readCtrl[2], &count);
          if (retVal < 0)
          {
             printf("UctEvent: _PdUctRead failed with error code %d\n", retVal);
             exit(EXIT_FAILURE);
          }
   
          printf("UCT_Event: counter 2 counted %d events\n", count);       
       }
       
       numEvents++;

       // Re-enable user events that asserted.
       retVal = _PdSetUserEvents(pUctData->handle, CounterTimer, events);
       if (retVal<0) 
       {
          printf("_PdGetUserEvents error %d\n", retVal);
       }
   }
   
   gettimeofday(&t2, NULL);
   printf("UCT_Event: got %d events in %f ms, %f events/s\n", numEvents, 
          (t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0,
          numEvents / ((t2.tv_sec-t1.tv_sec) + (t2.tv_usec-t1.tv_usec)/1000000.0));

   return retVal;
}


void CleanUpUctEvent(tUctEventData *pUctData)
{
   int retVal;
      
   if(pUctData->state == running)
   {
      // disable the software gate to stop counting
      if(pUctData->gateSource == gateSoftware)
      {
         retVal = _PdUctSwSetGate(pUctData->handle, 0 << pUctData->counter);
         if (retVal < 0)
         {
            printf("UctEvent: _PdUctSwSetGate failed with error code %d\n", retVal);
            exit(EXIT_FAILURE);
         }
      }

      pUctData->state = configured;
   }

   if(pUctData->state == configured)
   {
      retVal = _PdAdapterEnableInterrupt(pUctData->handle, FALSE);
      if (retVal < 0)
      {
         printf("UctEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
         exit(EXIT_FAILURE);
      } 

      retVal = _PdUctReset(pUctData->handle);
      if (retVal < 0)
         printf("UctEvent: _PdUctReset error %d\n", retVal);

      pUctData->state = unconfigured;
   }

   if(pUctData->handle > 0 && pUctData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pUctData->handle, CounterTimer, 0);
      if (retVal < 0)
         printf("UctEvent: PdReleaseSubsystem error %d\n", retVal);
   }

   pUctData->state = closed;
}


//////////////////////////////////////////////////////////////////////////
// Main routine
int main(int argc, char* argv[])
{   
   PWRDAQ_VERSION Version;
   int ret;
   int iNumAdp;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};
   
   ParseParameters(argc, argv, &params);

   ret = PdGetVersion(&Version);
   printf("Driver version: %d\n", ret);

   iNumAdp = ret = PdGetNumberAdapters();
   printf("Number of adapters installed: %d\n\n", ret);

   // initializes acquisition session parameters
   G_UctData.board = params.board;
   G_UctData.handle = 0;
   G_UctData.counter = 0;
   G_UctData.abort = FALSE;
   G_UctData.clockSource = clkInternal1MHz;
   G_UctData.gateSource = gateSoftware;
   G_UctData.mode = PulseTrain;
   G_UctData.state = closed;

   // setup exit handler that will clean-up the session
   // if an error occurs
   on_exit(UctEventExitHandler, &G_UctData);

   signal(SIGINT, SigInt);

   InitUctEvent(&G_UctData);

   // Start generating events at specified frequency in Hz
   UctEvent(&G_UctData, params.frequency);

   CleanUpUctEvent(&G_UctData);

   return 0;
}
