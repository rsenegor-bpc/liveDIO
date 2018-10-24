/*****************************************************************************/
/*              Universal Counter/Timer - Event counting example             */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform event counting */
/*  It will only work for PD-MFx and PD2-MFx boards.                         */
/*  The signal to count should be connected to the CTRx_IN input.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2003 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <pthread.h>
#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl.h>
#include <time.h>
#include <linux/slab.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#include "uct.h"

static unsigned int intClocks[3] = {UTB_CLK0, UTB_CLK1, UTB_CLK2};
static unsigned int extClocks[3] = {UTB_CLK0|UTB_CLK0_1, UTB_CLK1|UTB_CLK1_1, UTB_CLK2|UTB_CLK2_1};
static unsigned int uct0Clocks[3] = {UTB_CLK0_1, UTB_CLK1_1, UTB_CLK2_1};
static unsigned int hwGates[3] = {UTB_GATE0, UTB_GATE1, UTB_GATE2};
static unsigned int swGates[3] = {UTB_SWGATE0, UTB_SWGATE1, UTB_SWGATE2};
static unsigned int slctCounter[3] = {UCT_SelCtr0, UCT_SelCtr1, UCT_SelCtr2};
static unsigned int readCtrl[3] = {UCTREAD_UCT0 | UCTREAD_2BYTES, UCTREAD_UCT1 | UCTREAD_2BYTES, UCTREAD_UCT2 | UCTREAD_2BYTES};
   


int InitUct(tUctData *pUctData)
{
   int retVal = 0;

   pUctData->state = unconfigured;

   retVal = pd_uct_reset(pUctData->board);
   if (retVal < 0)
   {
      printk("Uct: PdUctReset error %d\n", retVal);
      return retVal;;
   }

   return 0;
}


int StartUct(tUctData *pUctData, int param)
{
   int retVal;
   unsigned int uctCfg = 0;
   unsigned int uctCtrl = 0;
  
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
   retVal = pd_uct_set_config(pUctData->board, intClocks[pUctData->counter]);
   if (retVal < 0)
   {
      printk("Uct: _PdUctSetCfg failed with error code %d\n", retVal);
      return retVal;
   }

   // Configure counter mode and period of the square wave
   // here 100 ticks = 0.1ms
   retVal = pd_uct_write(pUctData->board, uctCtrl | (param << 8));
   if (retVal < 0)
   {
      printk("Uct: _PdUctWrite failed with error code %d\n", retVal);
      return retVal;
   }

   //Set up the UCT subsystem                        
   retVal = pd_uct_set_config(pUctData->board, uctCfg);
   if (retVal < 0)
   {
      printk("Uct: _PdUctSetCfg failed with error code %d\n", retVal);
      return retVal;
   }


   pUctData->state = configured;

   // set the software gate to start counting
   if(pUctData->gateSource == gateSoftware)
   {
      retVal = pd_uct_set_sw_gate(pUctData->board, 1 << pUctData->counter);
      if (retVal < 0)
     {
         printk("Uct: _PdUctSwSetGate failed with error code %d\n", retVal);
         return retVal;
      }
   }


   pUctData->state = running;

   return retVal;
}


int StopUct(tUctData *pUctData)
{
   int retVal;
   unsigned int count;
   
    // disable the software gate to stop counting
   if(pUctData->gateSource == gateSoftware)
   {
      retVal = pd_uct_set_sw_gate(pUctData->board, 0 << pUctData->counter);
      if (retVal < 0)
      {
         printk("Uct: _PdUctSwSetGate failed with error code %d\n", retVal);
         return retVal;
      }
   }

   // read the number of counted events
   retVal = pd_uct_read(pUctData->board, readCtrl[pUctData->counter], &count);
   if (retVal < 0)
   {
      printk("Uct: _PdUctRead failed with error code %d\n", retVal);
      return retVal;
   }

   //rtl_printf("UCT_GenSquareWave: counted %ld events\n", 65536-count);

   pUctData->state = configured;

   return retVal;
}


int CleanUpUct(tUctData *pUctData)
{
   int retVal = 0;
      
   if(pUctData->state == running)
   {
      pUctData->state = configured;
   }

   if(pUctData->state == configured)
   {
      retVal = pd_uct_reset(pUctData->board);
      if (retVal < 0)
         printk("Uct: _PdUctReset error %d\n", retVal);

      pUctData->state = unconfigured;
   }

   pUctData->state = closed;
   
   return retVal;
}



