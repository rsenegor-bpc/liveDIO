//===========================================================================
//
// NAME:    rtl_int_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ RTLinux driver interrupt-based AIn test program
//
// AUTHOR:  Alex Ivchenko
//
// DATE:    12-JUL-2001
//
// HISTORY:
//
//      Rev 0.1,     12-JUL-2001,     Initial version.
//
//---------------------------------------------------------------------------
//  (C) 2001 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux hard interrupt handler to serve PowerDAQ board.
// This process writes a character each frame to an rtl_fifo (0)
//
//
// View the output with "cat /rtl_int_ai"
//
// This was compiled using RTLinuxPro 2.2,
//
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


#define PERIODIC_FREQ_HZ        10000.0   // periodic rate
#define PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define CL_SIZE                 8      // current CL size


#define data_fifo_id       0      // rt-fifo on which to put newframe flag
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define XFERSZ     0x200  // maximum number of samples is limited by maximum CL size = 0x100

#define CV_CLK 11999
#define CL_CLK 11999

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CLSTART0 | AIB_CLSTART1

//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];
int board_handle = 1;  // first board in the system. See /proc/pwrdaq for board ID
int fifod;

//==============================================================================
// ISR
unsigned int pd_isr(unsigned int trigger_irq,struct rtl_frame *frame) 
{
   int ret;
   int board = board_handle;
   static u16 adc_data[XFERSZ];
   u32 samples = 0;  // sample counter
   tEvents Events;
   u32 mask2;


   // Do we have an interrupt pending
   if ( pd_dsp_int_status(board) )
   { 
      pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

      // check what happens and process events
      ret = pd_adapter_get_board_status(board, &Events);
      // check is FHF?
      if ( (Events.ADUIntr & AIB_FHF) || (Events.ADUIntr & AIB_FHFSC) )
      {
         // transfer data to temp buffer
         ret &= pd_ain_get_xfer_samples(board, adc_data); 
         samples = XFERSZ;

         // check FIFO for overrun (FF flag)
         if (Events.ADUIntr & AIB_FFSC)
         {
            printf("FF!\n");
         }
         // reset event register
         mask2 = AIB_FHFIm | AIB_FFIm;
         pd_adapter_set_board_event2(board,mask2);

         // convert first 8 samples to output thru the fifo
         /*offs = 0;
         for (i = 0; i < CL_SIZE; i++)
            offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
         offs += sprintf(buf+offs, "\n");

         // throw data into rtl fifo
         ret = rtf_put(data_rtfifo, buf, offs+1);  */
      }
   }

   // reenable irq handling
   pd_adapter_enable_interrupt(board, 1);
   rtl_hard_enable_irq(pd_board[board].irq);

   return 0;
}

//==============================================================================
// AIn start
int AInStart (int brd) 
{
   int i, ret;
   u32 mask2;
   u32 adc_cl[64] = {0};

   // get hold on interrupt vector
   if (rtl_request_irq(pd_board[brd].irq, pd_isr))
   {
      printf("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return -EIO;
   }

   ret = pd_ain_reset(brd);

   // fill Channel List
   for (i = 0; i < CL_SIZE; i++)
   {
      adc_cl[i] = i;
   }

   ret = pd_ain_set_config(brd, AIN_CFG, 0, 0);
   ret &= pd_ain_set_cv_clock(brd, CV_CLK);
   ret &= pd_ain_set_cl_clock(brd, CL_CLK);

   ret &= pd_ain_set_channel_list(brd, CL_SIZE, adc_cl);

   // enable events
   mask2 = AIB_FFIm | AIB_FHFIm;  // FIFO full and half-full masks 
   ret &= pd_adapter_set_board_event2(brd, mask2);
   ret &= pd_adapter_enable_interrupt(brd, 1);

   ret &= pd_ain_set_enable_conversion(brd, 1);
   ret &= pd_ain_sw_start_trigger(brd);

   // enable interruptions
   rtl_hard_enable_irq(pd_board[brd].irq);

   return ret;
}

//==============================================================================
// AIn stop
int AInStop (int brd) 
{
   int ret;

   ret = pd_adapter_enable_interrupt(brd, 0);
   ret &= pd_ain_reset(brd); // this cleans all AIn configuration and triggers

   rtl_free_irq(pd_board[brd].irq);  // release IRQ

   return ret;
}


int main(void)                   // called when module is loaded
{
   printf("init_module rtl_int_ai\n");

   // make a the FIFO that is available to RTLinux and Linux applications 
   mkfifo( "/dev/rtl_int_ai", 0666);
   fifod = open("/dev/rtl_int_ai", O_WRONLY | O_NONBLOCK);

   // Resize the FIFO so that it can hold 8K of data
   ftruncate(fifod, 8 << 10);

   AInStart(board_handle);

   // wait for module to be unloaded 
   rtl_main_wait();

   AInStop(board_handle);

   // Close and remove the FIFO
   close(fifod);
   unlink("/dev/rtl_int_ai");

   return 0;
}

