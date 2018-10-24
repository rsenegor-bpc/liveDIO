//===========================================================================
//
// NAME:    rtl_int_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ RTAI driver interrupt-based AIn test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    18-JAN-2003
//
// HISTORY:
//
//      Rev 0.1,     18-JAN-2003,     Ported RTLinux example to RTAI.
//
//---------------------------------------------------------------------------
//  (C) 2003 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTAI hard interrupt handler to serve PowerDAQ board.
// This process writes a character each frame to an rt_fifo (0)
//
// modules pwdaq.o, rtai_fifo.o, rtai_sched.o, rtai.o must be loaded before
// this one
// Insert the module with "insmod rtai_int_ai.o"
//
// View the output with "cat /dev/rtf0"
//
// PowerDAQ Linux driver pwdaq.o shall be compiled with
// make RTAI=1
//
//
// This was compiled using the RTAI 24.1.10 distribution on kernel 2.4.18,
//

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define data_fifo_id       0      // rt-fifo on which to put newframe flag
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define XFERSZ     0x200  // maximum number of samples is limited by maximum CL size = 0x100
#define MAX_CL_SIZE        0x100  // maximum number of CL entries on MFx board
#define CL_SIZE            2      // current CL size
#define PDAQ_ENABLE 1

#define CV_CLK 11099
#define CL_CLK 11099

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CLSTART0 | AIB_CLSTART1

//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];
int hbrd = 1;  // first board in the system. See /proc/pwrdaq for board ID

//==============================================================================
// ISR
int pd_isr(int trigger_irq) 
{
   int ret;
   int board = hbrd;

   static u16 adc_data[XFERSZ];
   int data_rtfifo = (int)data_fifo_id;
   int i, offs;
   char buf[1024];   // temp char buffer
   u32 samples = 0;  // sample counter

   tEvents Events;
   u32 mask2;

   if ( pd_dsp_int_status(board) )
   { // interrupt pending
      pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

      rt_printk("Got IRQ\n");

      // check what happens and process events
      ret = pd_adapter_get_board_status(board, &Events);
      // check is FHF?
      if ( (Events.ADUIntr & AIB_FHF) || (Events.AIOIntr & AIB_FHFSC) )
      //        if ( (Events.ADUIntr & AIB_CLDone) || (Events.AIOIntr & AIB_CLDoneSC) ) 
      {
         // transfer data to temp buffer
         ret &= pd_ain_get_xfer_samples(board, adc_data); 
         samples = XFERSZ;

         // check FIFO for overrun (FF flag)
         if (Events.ADUIntr & AIB_FHF)
         {
            rt_printk("Got FHF status\n");
         }

         if (Events.ADUIntr & AIB_CLDone)
            rt_printk("Got CLDone status\n");

         if (Events.AIOIntr & AIB_CLDoneSC)
            rt_printk("GOT CLDoneSC\n");

         if (Events.AIOIntr & AIB_FHFSC)
         {
            rt_printk("Got FHFSC\n");
         }

         // reset event register
         mask2 = AIB_FHFIm | AIB_FFIm;
//	   mask2 = AIB_CLDoneIm | AIB_FFIm;
         pd_adapter_set_board_event2(board,mask2);

         // convert first 8 samples to output thru the fifo
         offs = 0;
         for (i = 0; i < CL_SIZE; i++)
            offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
         offs += sprintf(buf+offs, "\n");

         // throw data into rtl fifo
         ret = rtf_put(data_rtfifo, buf, offs+1);
      }
   }

   // reenable irq handling
   pd_adapter_enable_interrupt(board, 1);
   rt_enable_irq(pd_board[board].irq);

   return 0;
}

//==============================================================================
// AIn start
int AInStart (int brd) 
{
   int i, ret;
   u32 mask2;
   u32 adc_cl[64] = {0};

   ret = pd_ain_reset(brd);

   // fill CL with "slow bit" flag
   for (i = 0; i < CL_SIZE; i++) adc_cl[i] = i;

   ret = pd_ain_set_config(brd, AIN_CFG, 0, 0);
   ret &= pd_ain_set_cv_clock(brd, CV_CLK);
   ret &= pd_ain_set_cl_clock(brd, CL_CLK);

   ret &= pd_ain_set_channel_list(brd, CL_SIZE, adc_cl);

   // enable events
   mask2 = AIB_FFIm | AIB_FHFIm;  // FIFO full and half-full masks
   //mask2 = AIB_CLDoneIm | AIB_FFIm;
   ret &= pd_dio256_write_output(brd, 0xFFFF86, XFERSZ);
   ret &= pd_dio256_write_output(brd, 0xFFFF90, AIB_CLDoneIm);
   ret &= pd_adapter_set_board_event2(brd, mask2);
   ret &= pd_adapter_enable_interrupt(brd, 1);

   ret &= pd_ain_set_enable_conversion(brd, 1);
   ret &= pd_ain_sw_start_trigger(brd);

   if (ret != 1)
      rt_printk("Bad return value\n");

   return ret;
}

//==============================================================================
// AIn stop
int AInStop (int brd) 
{
   int ret;

   ret = pd_adapter_enable_interrupt(brd, 0);
   ret &= pd_ain_reset(brd); // this cleans all AIn configuration and triggers
   return ret;
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
   rt_printk("init_module rtai_int_ai\n");

   // create fifo we can talk via /dev/rtf0
   rtf_destroy(data_fifo_id);     // free up the resource, just in case
   rtf_create(data_fifo_id, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device

   // get hold on interrupt vector
   if (rt_request_global_irq(pd_board[hbrd].irq, (void(*)(void))pd_isr))
   {
      printk("rt_request_global_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return -EIO;
   }

   // kick analog input
   AInStart(hbrd);

   // enable it
   rt_enable_irq(pd_board[hbrd].irq);

   //pd_adapter_test_interrupt(hbrd); // for test purpose only

   return 0;
}


//==============================================================================
void cleanup_module(void)
{
   rt_printk("cleanup_module rtai_int_ai\n");

   // stop analog input
   AInStop(hbrd);

   // get rid of fifo and thread
   rtf_destroy(data_fifo_id);      // free up the resource
   rt_free_global_irq(pd_board[hbrd].irq);  // release IRQ 
}
