//=================================================================================
//
// NAME:    pdct_evt.c
//
// DESCRIPTION: PowerDAQ DSP Counter/Timer Events Test application
//
// NOTES:
//
// AUTHOR:  Dennis L. Kraplin
//
// DATE:    02-MAY-2001
//
// HISTORY:
//
//      Rev 0.1,     02-FEB-2001,  D.K.,   Initial version.
//      Rev 0.2,     02-MAY-2001,  A.I.,   CT control is moved to the driver
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 2001 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================
//  DSP Counter/Timer module has three independent 24-bit count-up counters 
// and one 21-bit count-down prescaler counter.
//  
// Each counter is a set of four 24-bit registers:
//
//	Load register, which contains initial value of the counter when it start/
//                 restart counting. Load register has a write only access.
//	Control/Status register (read/write) which defines a counter counting mode, 
//                 reload mode (when the counter value reloaded each time, 
//                 after it reach compare value), input/output inversion flag 
//                 and interrupt flag/status bits.
//	Count register (read only), which can be used to read actual value of the 
//                 counter (useful in the measurement modes).
//	Compare register (read/write) is used to determine a output waveform shape.
//
// Prescaler can be used as a clock source for any counter which allows to 
//  generate very low frequencies, 
//   Prescaler load register contains initial value of the prescaler counter, 
//                 which reloads each time when prescaler counter reach zero. 
//                 Also different sources can be specified as a prescaler 
//                 clock. (Internal CLK/2, Timer0, Timer1, Timer2)
//	Count register (read only), which can be used to read actual value of the 
//                 prescaler.
//
//  To get more details on counter/timer functionality please refer to the 
//  Motorola DSP 56301 user manual (Motorola P/N DSP 56301UM/AD).
//
//  Please note that TIOx pins are bi-directional and it is strongly 
//  recommended to use at 100-200 OHm series resistor to connect external 
//  signals to those pins.
//===========================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"
#include "pd_dsp_ct.h"
#include "powerdaq_Uct.h"

#include "ParseParams.h"


int TestAsyncEvent(int handle, int timer);
void SigInt(int signum);
void SigIoHandler(int signum);

#define DCT_BASE 33000000 // 33MHz timebase clock


int    bStop = FALSE;                 // stop variable
struct sigaction io_act;
int boardHandle;


void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping counter\n");
      bStop = TRUE;
   }
}
 

/****************************************************************************
 Our event handler
 Processes incoming event
****************************************************************************/
void SigIoHandler(int signum)
{
    unsigned int events;
    unsigned long count;
    int retVal;
    
    // figure out what events happened
    retVal = _PdGetUserEvents(boardHandle, CounterTimer, &events);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }

    printf("sigio>ev=0x%x\n", events);

    
    // deal with the events
    if (events & eUct0Event)
    {
       // read the number of counted events
       retVal = PdDspUctRead(boardHandle, 0, &count);
       if (retVal < 0)
       {
          printf("DspEvent: PdDspUctRead failed with error code %d\n", retVal);
          exit(EXIT_FAILURE);
       }
   
       printf("DspEvent: counter 0 counted %ld events\n", count);       
    }

    if (events & eUct1Event)
    {
       // read the number of counted events
       retVal = PdDspUctRead(boardHandle, 1, &count);
       if (retVal < 0)
       {
          printf("DspEvent: PdDspUctRead failed with error code %d\n", retVal);
          exit(EXIT_FAILURE);
       }
   
       printf("DspEvent: counter 1 counted %ld events\n", count);       
    }

    if (events & eUct2Event)
    {
       // read the number of counted events
       retVal = PdDspUctRead(boardHandle, 2, &count);
       if (retVal < 0)
       {
          printf("DspEvent: PdDspUctRead failed with error code %d\n", retVal);
          exit(EXIT_FAILURE);
       }
   
       printf("DspEvent: counter 2 counted %ld events\n", count);       
    }

    // Re-enable user events that asserted.
    retVal = _PdSetUserEvents(boardHandle, CounterTimer, events);
    if (retVal<0) 
    {
       printf("_PdGetUserEvents error %d\n", retVal);
    }
}


////////////////////////////////////////////////////////////////////////////////
// DSP Counter/Timer Event test code
//
int TestAsyncEvent(int handle, int timer)
{
   int     retVal;
   DWORD   events[3] = {eUct0Event, eUct1Event, eUct2Event};
   DWORD   timers[3] = {DCT_UCT0, DCT_UCT1, DCT_UCT2};
    
   bStop = FALSE;

   // stop counter
   retVal = PdDspUctStop(handle, timer);
    
   // Configure the timer to generate a pulse-train
   // timebase is 33MHz
   retVal = PdDspUctConfig(handle, 
                           timer, 
                           Clock,
                           GenPulseTrain,
                           24,                   //divider
                           20000,                //ticks1
                           20000,                //ticks2
                           0);                  //inverted
   if(retVal < 0)
   {
      printf("DspEvent : PdDspUctConfig error %d\n", retVal);
      return retVal;
   }

   /*retVal = _PdDspCtEnableCounter(handle, timers[timer], FALSE);
   if(retVal < 0)
   {
      printf("DspEvent: _PdDspCtEnableCounter failed\n");
      return retVal;
   }*/

   // Configures the driver to send us a signal when the event occurs
   retVal = _PdSetAsyncNotify(handle, &io_act, SigIoHandler);
   if (retVal < 0)
   {
      printf("DspEvent: Failed to set up notification\n");
      return retVal;
   }

   retVal = _PdAdapterEnableInterrupt(handle, TRUE);
   if (retVal < 0)
   {
      printf("DspEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
      return retVal;
   }

   /*retVal = _PdDspCtLoad(handle, timers[timer], 0x0, 1650000, DCT_TimerPulse, TRUE, FALSE, FALSE);
   if (retVal < 0)
   {
      printf("DspEvent: _PdDspCtLoad failed\n");
      return retVal;
   }  */
   
   // Clear & reset counter/timer events
   retVal = _PdSetUserEvents(handle, CounterTimer, events[timer]);
   if (retVal < 0)
   {
      printf("DspEvent: _PdSetUserEvents failed with error code %d\n", retVal);
      return retVal;
   }

   // Configure events
   retVal = _PdDspCtEnableInterrupts(handle, timers[timer], TRUE, FALSE);
   if (retVal < 0)
   {
      printf("DspEvent: _PdDspCtEnableInterrupts error %d\n", retVal);
      return retVal;
   }

   retVal = PdDspUctStart(handle, timer);
   if(retVal < 0)
   {
      printf("DspEvent : PdDspUctStart error %d\n", retVal);
      return retVal;
   }

   /*retVal = _PdDspCtEnableCounter(handle, timers[timer], TRUE);
   if (retVal < 0)
   {
      printf("DspEvent: _PddspCtEnableCounter failed\n");
      return retVal;
   }  */

   while (!bStop)
   {
      usleep(100000);
   }


   // Stop interrupts and timer
   retVal = _PdClearUserEvents(handle, CounterTimer, events[timer]);
   if (retVal < 0)
   {
      printf("DspEvent: _PdClearUserEvents failed with error code %d\n", retVal);
      return retVal;
   }

   // Configure events
   /*retVal = _PdDspCtEnableInterrupts(handle, timers[timer], FALSE, FALSE);
   if (retVal < 0)
   {
      printf("DspEvent: _PdDspCtEnableInterrupts error %d\n", retVal);
      return retVal;
   }*/

   retVal = _PdAdapterEnableInterrupt(handle, FALSE);
   if (retVal < 0)
   {
      printf("UctAsync: _PdAdapterEnableInterrupt error %d\n", retVal);
      return retVal;
   } 

   /*retVal = _PdDspCtEnableCounter(handle, timers[timer], FALSE);
   if (retVal < 0)
   {
      printf("DspEvent: _PdDspEnableCounter\n");
      return retVal;
   }

   retVal = _PdDspCtLoad(handle, timers[timer], 0, 0, 0, 0, 0, 0);
   if (retVal < 0)
   {
      printf("DspEvent: _PdDspCtLoad failed\n");
      return retVal;
   }  */

   retVal = PdDspUctStop(handle, timer);
   if(retVal < 0)
   {
      printf("DspEvent : PdDspUctStop error %d\n", retVal);
      return retVal;
   }

   return retVal;
}


/****************************************************************************

****************************************************************************/
int main(int argc, char *argv[])
{
    int retVal = 0;
    PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
    ParseParameters(argc, argv, &params);

    
    // Print hello world
    printf("UEI PowerDAQ Test Application - DSP Counter/Timer Events Test. (C)UEI, 2001\n");

    signal(SIGINT, SigInt);

    // Open the PowerDAQ PCI adapter.
    boardHandle = PdAcquireSubsystem(params.board, CounterTimer, 1);
    if (boardHandle < 0) 
       printf("PdAdapterAcquireSubsystem error %d\n", retVal);


    retVal = TestAsyncEvent(boardHandle, 0);
    if (retVal < 0) 
       printf("TestAsyncEvent error %d\n", retVal);

    
    // return subsystem
    retVal = PdAcquireSubsystem(boardHandle, CounterTimer, 0);
    if (retVal < 0) 
       printf("PdAdapterAcquireSubsystem error %d\n", retVal);


    return 0;
}

