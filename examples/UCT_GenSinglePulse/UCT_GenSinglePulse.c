/*****************************************************************************/
/*              Universal Counter/Timer - Event counting example             */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform event counting */
/*  It will only work for PD-MFx and PD2-MFx boards.                         */
/*  The signal to count should be connected to the CTRx_IN input.            */
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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
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


typedef struct _uctGenSinglePulseData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int counter;                  // counter to be used
   tClockSource clockSource;
   tGateSource gateSource;
   tCounterMode mode;
   tState state;                 // state of the acquisition session
} tUctGenSinglePulseData;


int InitUctGenSinglePulse(tUctGenSinglePulseData *pUctData);
int SingleUctGenSinglePulse(tUctGenSinglePulseData *pUctData);
void CleanUpUctGenSinglePulse(tUctGenSinglePulseData *pUctData);


static tUctGenSinglePulseData G_UctData;

// exit handler
void UctGenSinglePulseExitHandler(int status, void *arg)
{
   CleanUpUctGenSinglePulse((tUctGenSinglePulseData *)arg);
}


int InitUctGenSinglePulse(tUctGenSinglePulseData *pUctData)
{
   int retVal = 0;

   pUctData->handle = PdAcquireSubsystem(pUctData->board, CounterTimer, 1);
   if(pUctData->handle < 0)
   {
      printf("UctGenSinglePulse: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pUctData->state = unconfigured;

   retVal = _PdUctReset(pUctData->handle);
   if (retVal < 0)
   {
      printf("UctGenSinglePulse: PdUctReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int UctGenSinglePulse(tUctGenSinglePulseData *pUctData)
{
   int retVal;
   unsigned int uctCfg = 0;
   unsigned int uctCtrl = 0;
   unsigned int count;
   unsigned int intClocks[3] = {UTB_CLK0, UTB_CLK1, UTB_CLK2};
   unsigned int extClocks[3] = {UTB_CLK0|UTB_CLK0_1, UTB_CLK1|UTB_CLK1_1, UTB_CLK2|UTB_CLK2_1};
   unsigned int uct0Clocks[3] = {UTB_CLK0_1, UTB_CLK1_1, UTB_CLK2_1};
   unsigned int hwGates[3] = {UTB_GATE0, UTB_GATE1, UTB_GATE2};
   unsigned int swGates[3] = {UTB_SWGATE0, UTB_SWGATE1, UTB_SWGATE2};
   unsigned int slctCounter[3] = {UCT_SelCtr0, UCT_SelCtr1, UCT_SelCtr2};
   unsigned int readCtrl[3] = {UCTREAD_UCT0 | UCTREAD_2BYTES, UCTREAD_UCT1 | UCTREAD_2BYTES, UCTREAD_UCT2 | UCTREAD_2BYTES};
   
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
      printf("UctGenSinglePulse: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // Configure counter mode and delay before generating the
   // pulse. Here we chose 10ms which is 10000 ticks at 1MHz
   retVal = _PdUctWrite(pUctData->handle, uctCtrl | (5000 << 8));
   if (retVal < 0)
   {
      printf("UctGenSinglePulse: _PdUctWrite failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }



   //Set up the UCT subsystem                        
   retVal = _PdUctSetCfg(pUctData->handle, uctCfg);
   if (retVal < 0)
   {
      printf("UctGenSinglePulse: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }


   pUctData->state = configured;

   // set the software gate to start counting
   if(pUctData->gateSource == gateSoftware)
   {
      retVal = _PdUctSwSetGate(pUctData->handle, 1 << pUctData->counter);
      if (retVal < 0)
      {
         printf("UctGenSinglePulse: _PdUctSwSetGate failed with error code %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   }


   pUctData->state = running;


   // wait 50 ms
   usleep(5000);

   
   // disable the software gate to stop counting
   if(pUctData->gateSource == gateSoftware)
   {
      retVal = _PdUctSwSetGate(pUctData->handle, 0 << pUctData->counter);
      if (retVal < 0)
      {
         printf("UctGenSinglePulse: _PdUctSwSetGate failed with error code %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   }

   // read the number of counted events
   retVal = _PdUctRead(pUctData->handle, readCtrl[pUctData->counter], &count);
   if (retVal < 0)
   {
      printf("UctGenSinglePulse: _PdUctRead failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   printf("UCT_GenSinglePulse: counted %d events\n", 65536-count);

   pUctData->state = configured;

   return retVal;
}


void CleanUpUctGenSinglePulse(tUctGenSinglePulseData *pUctData)
{
   int retVal;
      
   if(pUctData->state == running)
   {
      pUctData->state = configured;
   }

   if(pUctData->state == configured)
   {
      retVal = _PdUctReset(pUctData->handle);
      if (retVal < 0)
         printf("UctGenSinglePulse: _PdUctReset error %d\n", retVal);

      pUctData->state = unconfigured;
   }

   if(pUctData->handle > 0 && pUctData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pUctData->handle, CounterTimer, 0);
      if (retVal < 0)
         printf("UctGenSinglePulse: PdReleaseSubsystem error %d\n", retVal);
   }

   pUctData->state = closed;
}


int main(int argc, char *argv[])
{
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};

   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_UctData.board = params.board;
   G_UctData.handle = 0;
   G_UctData.counter = 0;
   G_UctData.clockSource = clkInternal1MHz;
   G_UctData.gateSource = gateSoftware;
   G_UctData.mode = SinglePulse;
   G_UctData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(UctGenSinglePulseExitHandler, &G_UctData);

   // initializes acquisition session
   InitUctGenSinglePulse(&G_UctData);

   // run the acquisition
   UctGenSinglePulse(&G_UctData);

   // Cleanup acquisition
   CleanUpUctGenSinglePulse(&G_UctData);

   return 0;
}

