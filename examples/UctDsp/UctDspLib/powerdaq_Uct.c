#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include "pd_dsp_ct.h"
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"
#include "powerdaq_Uct.h"

// Configure the selected counter to perform one of the counting
// timing operation
// 
// GenSinglePulse 
// GenPulseTrain
// CountEvent
// MeasPulseWidth
// MeasPeriod
// Capture
//
// set reload to 1 to automatically rearm the counter/timer
//
// set inverted to 1 to invert the polarity of the input/output
//
// Set the timebase divider
//
// For Capture, GenSinglePulse, GenPulseTrain, MeasPulseWidth and Measperiod
// it is used to divide the 33MHz internal clock

// For CountEvent
// it is used to divide the signal that is being counted
//
// it can be any value from 1 to 2^21-1
// the divider is shared by all counters
//
// ticks1 and ticks2 load parameters in the counter/timer
//
// GenSinglePulse: ticks1 is the delay before the pulse, ticks2 is the width of the pulse
// GenPulseTrain: ticks1 and ticks2 define the duty cycle of the pulse train
// CountEvent: ticks1 defines the number of event after which the counter will generate an event
// MeasPulseWidth: N/A
// MeasPeriod: N/A
// Capture: N/A

int PdDspUctConfig(int handle, 
                   unsigned long timerNb, 
                   tDspUctSrc timerSrc,
                   tDspUctMode mode,
                   unsigned long divider,
                   unsigned long ticks1,
                   unsigned long ticks2,
                   unsigned long inverted)
{
   unsigned long dspUctMode;
   DWORD dwTCSR;
   unsigned long preScalerSrc = 0;
   unsigned long load, compare;
   unsigned long rearm = 0;
   int retVal = 0;

   // Check counter number and load appropriate prescaler source
   // just in case tre prescaler is needed later
   switch(timerSrc)
   {
   case Clock:
      preScalerSrc = M_PS_CLK;
      break;
   case Clock2:
      preScalerSrc = M_PS_CLK2;
      break;
   case CT0:
      preScalerSrc = M_PS_TIO0;
      break;
   case CT1:
      preScalerSrc = M_PS_TIO1;
      break;
   case CT2:
      preScalerSrc = M_PS_TIO2;
      break;
   default:
      return(-1);
   }
           
   // set the counter timer to the appropriate mode
   switch(mode)
   {
   case GenSinglePulse:
      dspUctMode = DCT_PWM;
      rearm = 0;
      load = ((1 << 24) - 1) - (ticks2 + ticks1);
      compare = ((1 << 24) - 1) - ticks2;
      break; 
   case GenPulseTrain:
      dspUctMode = DCT_PWM;
      rearm = 1;
      load = ((1 << 24) - 1) - (ticks2 + ticks1);
      compare = ((1 << 24) - 1) - ticks2;
      break;
   case CountEvent:
      dspUctMode = DCT_EventCounter;
      load = 0;
      compare = ticks1;
      break;
   case  MeasPulseWidth:
      dspUctMode = DCT_InputWidth;
      load = 0;
      compare = 0;
      break;
   case MeasPeriod:
      dspUctMode = DCT_InputPeriod;
      load = 0;
      compare = 0;
      break;
   default:
      return -1;
   }

   // Read current status value register 
   retVal = _PdDspRegRead(handle, 
                           _PdDspCtGetStatusAddr(timerNb),
                           &dwTCSR);
   if (retVal < 0) 
      return retVal;

   // Leave Timer/Interrupt enable bits
   dwTCSR = dwTCSR & (M_TE | M_TCIE | M_TOIE);

   if (rearm) 
      dwTCSR = dwTCSR | M_TRM | dspUctMode ;
   else
      dwTCSR = dwTCSR | dspUctMode;
    
   if (inverted) 
      dwTCSR = dwTCSR | M_INV;

   // clock is at 33MHz
   // max toggle time is (2^24 - 1) / 33E6 = 0.5 sec
   // for more use the prescaler
   // max toggle time is then (2^24 - 1) / (33E6 / (2^21 - 1)) = 1066192 sec
   // ex: to count for 5 sec, set ps to 20 (timebase of 33MHz/20 = 1.65 MHz)
   //     set compare to 8250000 (5 / (1/1650000))  
   
   // if divider is greater than 1 we need the prescaler
   if(divider > 1)
      dwTCSR = dwTCSR | M_PCE;

   // Write Control register
   retVal = _PdDspRegWrite(handle, 
                           _PdDspCtGetStatusAddr(timerNb),
                           dwTCSR);
   if (retVal < 0) 
      return retVal;

   if(divider > 1)
   {
      // load the divider into the prescaler
      retVal = _PdDspPSLoad(handle, divider, preScalerSrc);
      if (retVal < 0) 
         return retVal;
   }

   // Program counter
   // Load register
   retVal = _PdDspRegWrite(handle, 
                           _PdDspCtGetLoadAddr(timerNb),
                           load);
   if (retVal < 0) 
      return retVal;

   // Program counter
   // compare register
   retVal = _PdDspRegWrite(handle, 
                           _PdDspCtGetCompareAddr(timerNb),
                           compare);
   if (retVal < 0) 
      return retVal;


   return retVal;
}


// Start the operation configured with PdDspUctConfig on the specified
// counter/timer.
int PdDspUctStart(int handle, unsigned long timerNb)
{
   int retVal;

   retVal = _PdDspCtEnableCounter(handle, timerNb, 1);
   return retVal;
}

// Configure the counter/timer to generate an interrupt 
// when the counter overflow or the configured operation
// is complete.
/*int PdDspUctSetEvent(int handle, unsigned long events)
{
}*/


// Reads counter value for the following operating modes:
// CountEvent, MeasPulseWidth, MeasPeriod, Capture
int PdDspUctRead(int handle, unsigned long timerNb, unsigned long *count)
{
   int retVal = 0;

   retVal = _PdDspCtGetCount(handle,timerNb, (DWORD *)count);
   return retVal;
}

// Stop the current operation and reset the counter/timer
int PdDspUctStop(int handle, unsigned long timerNb)
{
   int retVal;

   retVal = _PdDspCtEnableCounter(handle, timerNb, 0);
   return retVal;
}

int msTimeValDiff(struct timeval t1, struct timeval t2)
{
   int msDiff;
   
   msDiff = (t1.tv_sec -  t2.tv_sec) * 1000;
   msDiff = msDiff - t1.tv_usec / 1000 + t2.tv_usec / 1000;
   return msDiff;
}

int PdDspUctWaitForEvent(int handle, int timerNb, tDspUctMode mode, int timeout)
{
   int retVal;
   DWORD dwStatus = 0;
   DWORD flag;
   struct timeval  t1, t2;

   if(mode == GenSinglePulse)
      flag = M_TOF;
   else
      flag = M_TCF;


   gettimeofday(&t1, NULL);

   do
   {
      retVal = _PdDspCtGetStatus(handle, timerNb, &dwStatus);
      gettimeofday(&t2, NULL);
   } 
   while ((retVal == 0) && (!(dwStatus & flag)) && 
          (msTimeValDiff(t2,t1)<timeout)); 

   printf("DspCtStatus = 0x%x\n", dwStatus);

   if(!(dwStatus & flag))
      retVal = -1;

   return (retVal);
}




