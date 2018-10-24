/*****************************************************************************/
/*              Universal Counter/Timer - Pulse width measurement            */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a pulse width  */
/*  Measurement by counting the number of clock ticks while a signal         */
/*  connected on the counter gate is high.                                   */
/*  It will only work for PD-MFx and PD2-MFx boards.                         */
/*  The signal to count should be connected to the CTRx_GATE input.          */
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


typedef struct _uctMeasPulseWidthData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int counter;                  // counter to be used
   tClockSource clockSource;
   tGateSource gateSource;
   tCounterMode mode;
   tState state;                 // state of the acquisition session
} tUctMeasPulseWidthData;


int InitUctMeasPulseWidth(tUctMeasPulseWidthData *pUctData);
int SingleUctMeasPulseWidth(tUctMeasPulseWidthData *pUctData);
void CleanUpUctMeasPulseWidth(tUctMeasPulseWidthData *pUctData);


static tUctMeasPulseWidthData G_UctData;

// exit handler
void UctMeasPulseWidthExitHandler(int status, void *arg)
{
   CleanUpUctMeasPulseWidth((tUctMeasPulseWidthData *)arg);
}


int InitUctMeasPulseWidth(tUctMeasPulseWidthData *pUctData)
{
   int retVal = 0;

   pUctData->handle = PdAcquireSubsystem(pUctData->board, CounterTimer, 1);
   if(pUctData->handle < 0)
   {
      printf("UctMeasPulseWidth: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pUctData->state = unconfigured;

   retVal = _PdUctReset(pUctData->handle);
   if (retVal < 0)
   {
      printf("UctMeasPulseWidth: PdUctReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int UctMeasPulseWidth(tUctMeasPulseWidthData *pUctData)
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
      printf("UctMeasPulseWidth: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   // Configure counter mode
   retVal = _PdUctWrite(pUctData->handle, uctCtrl);
   if (retVal < 0)
   {
      printf("UctMeasPulseWidth: _PdUctWrite failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   //Set up the UCT subsystem                        
   retVal = _PdUctSetCfg(pUctData->handle, uctCfg);
   if (retVal < 0)
   {
      printf("UctMeasPulseWidth: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }


   pUctData->state = configured;

   
   pUctData->state = running;

   usleep(50000);

   // read the number of counted events
   retVal = _PdUctRead(pUctData->handle, readCtrl[pUctData->counter], &count);
   if (retVal < 0)
   {
      printf("UctMeasPulseWidth: _PdUctRead failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   printf("UCT_MeasPulseWidth: counted %d events, pulse width is %f ms\n", 65536-count, (65536-count)/1000.0);

   pUctData->state = configured;

   return retVal;
}


void CleanUpUctMeasPulseWidth(tUctMeasPulseWidthData *pUctData)
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
         printf("UctMeasPulseWidth: _PdUctReset error %d\n", retVal);

      pUctData->state = unconfigured;
   }

   if(pUctData->handle > 0 && pUctData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pUctData->handle, CounterTimer, 0);
      if (retVal < 0)
         printf("UctMeasPulseWidth: PdReleaseSubsystem error %d\n", retVal);
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
   G_UctData.gateSource = gateHardware;
   G_UctData.mode = swTriggeredStrobe;
   G_UctData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(UctMeasPulseWidthExitHandler, &G_UctData);

   // initializes acquisition session
   InitUctMeasPulseWidth(&G_UctData);

   // run the acquisition
   UctMeasPulseWidth(&G_UctData);

   // Cleanup acquisition
   CleanUpUctMeasPulseWidth(&G_UctData);

   return 0;
}

