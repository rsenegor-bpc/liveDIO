/*****************************************************************************/
/*              Universal Counter/Timer - Frequency measurement              */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a Frequency    */
/*  Measurement on a signal connected to the source of one of the counters.  */
/*  It can only measure frequencies up to 65.563 kHz.                        */
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
   hwTriggeredStrobe,
   FrequencyMeas
} tCounterMode;


typedef struct _uctMeasFrequencyData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int counter;                  // counter to be used
   tClockSource clockSource;
   tGateSource gateSource;
   tCounterMode mode;
   tState state;                 // state of the acquisition session
} tUctMeasFrequencyData;


int InitUctMeasFrequency(tUctMeasFrequencyData *pUctData);
int SingleUctMeasFrequency(tUctMeasFrequencyData *pUctData);
void CleanUpUctMeasFrequency(tUctMeasFrequencyData *pUctData);


static tUctMeasFrequencyData G_UctData;

// exit handler
void UctMeasFrequencyExitHandler(int status, void *arg)
{
   CleanUpUctMeasFrequency((tUctMeasFrequencyData *)arg);
}


int InitUctMeasFrequency(tUctMeasFrequencyData *pUctData)
{
   int retVal = 0;

   pUctData->handle = PdAcquireSubsystem(pUctData->board, CounterTimer, 1);
   if(pUctData->handle < 0)
   {
      printf("UctMeasFrequency: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pUctData->state = unconfigured;

   retVal = _PdUctReset(pUctData->handle);
   if (retVal < 0)
   {
      printf("UctMeasFrequency: PdUctReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int UctMeasFrequency(tUctMeasFrequencyData *pUctData)
{
   int retVal;
   unsigned int uctCfg = 0;
   unsigned int count;
   unsigned int status;
   unsigned int intClocks[3] = {UTB_CLK0, UTB_CLK1, UTB_CLK2};
   unsigned int readCtrl[3] = {UCTREAD_UCT0 | UCTREAD_2BYTES, UCTREAD_UCT1 | UCTREAD_2BYTES, UCTREAD_UCT2 | UCTREAD_2BYTES};
   unsigned int freqCfg[3] = {UTB_FREQMODE0, UTB_FREQMODE1, UTB_FREQMODE2};
  
   uctCfg = uctCfg | intClocks[pUctData->counter];
   
   //Set up the UCT subsystem                        
   retVal = _PdUctSetCfg(pUctData->handle, uctCfg | freqCfg[pUctData->counter]);
   if (retVal < 0)
   {
      printf("UctMeasFrequency: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }


   pUctData->state = configured;

   
   pUctData->state = running;

   // sleep for 1.2s
   usleep(1200000);

   retVal = _PdUctGetStatus(pUctData->handle, &status);
   if (retVal < 0)
   {
      printf("UctMeasFrequency: _PdUctGetStatus error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   printf("UctMeasFrequency: status is 0x%x\n", status);


   retVal = _PdUctSetCfg(pUctData->handle, uctCfg);
   if (retVal < 0)
   {
      printf("UctMeasFrequency: _PdUctSetCfg failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }


   // read the number of counted events
   retVal = _PdUctRead(pUctData->handle, readCtrl[pUctData->counter] | UCT_ReadBack, &count);
   if (retVal < 0)
   {
      printf("UctMeasFrequency: _PdUctRead failed with error code %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   count = (65536-count)-3;

   if(count > 0xFFF0)
      count = 0;

   if(count == 0)
      count = 0xFFFF;

   printf("UCT_MeasFrequency: frequency is %d Hz\n", count);

   pUctData->state = configured;

   return retVal;
}


void CleanUpUctMeasFrequency(tUctMeasFrequencyData *pUctData)
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
         printf("UctMeasFrequency: _PdUctReset error %d\n", retVal);

      pUctData->state = unconfigured;
   }

   if(pUctData->handle > 0 && pUctData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pUctData->handle, CounterTimer, 0);
      if (retVal < 0)
         printf("UctMeasFrequency: PdReleaseSubsystem error %d\n", retVal);
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
   G_UctData.mode = FrequencyMeas;
   G_UctData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(UctMeasFrequencyExitHandler, &G_UctData);

   // initializes acquisition session
   InitUctMeasFrequency(&G_UctData);

   // run the acquisition
   UctMeasFrequency(&G_UctData);

   // Cleanup acquisition
   CleanUpUctMeasFrequency(&G_UctData);

   return 0;
}

