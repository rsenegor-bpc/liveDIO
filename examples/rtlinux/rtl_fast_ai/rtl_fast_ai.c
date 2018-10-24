//===========================================================================
//
// NAME:    pd_rtl_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver asynchronous AIn test program
//
// AUTHOR:  Alex Ivchenko and Steve Rosenbluth
//
// DATE:    12-SEP-2000
//
//
// HISTORY:
//
//      Rev 0.1,     12-SEP-2000,     Initial version.
//
//---------------------------------------------------------------------------
//  (C) 2000 United Electronic Industries, Inc.
//  (C) 2000 Jim Henson Creature Shop
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux periodic task using the "pthreads" API.
// This process writes a character each frame to an rtl_fifo, frequency = 10 Hz
//
// modules pd.o, rtl_fifo, rtl_posixio, rtl_sched, rtl_time must be loaded before
// this one
// Insert the module with "insmod pd_rtl_ai.o"
//
// View the output with "cat /dev/rtf0"
//
// PowerDAQ Linux driver pd.o shall be compiled with
// #define _NO_USERSPACE
// and
// #deifne _PD_RT
//
// Uncomment those #defines in powerdaq-internal.h (lines 54 and 58)
// Then, "make clean; make; make install" in the PowerDAQ driver directory
// New driver will be located in ./drv/pd.o
//
// This was compiled using the RTLinux 2.2 distribution on kernel 2.2.14,
// it should work on RTLinux 2.3.and kernel 2.2.17
//

#include <pthread.h>
#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


#define PERIODIC_FREQ_HZ        10.0   // periodic rate
#define FRAME_PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define FRAMERATE_FIFO_ID       0      // rt-fifo on which to put newframe flag
#define FRAMERATE_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_RESULT_QTY          0x100  // maximum number of samples is limited by maximum CL size = 0x100
#define MAX_CL_SIZE             0x100  // maximum number of CL entries on MFx board
#define CL_SIZE                 8      // current CL size
#define PDAQ_ENABLE 1

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1

//==============================================================================
// Prototypes
void *Periodic_entry_point(void *framerate_fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
pthread_t periodic_thread;  // our periodic thread
int board_handle = 1;  // first board in the system. See /proc/pwrdaq for board ID

//==============================================================================
void *Periodic_entry_point(void *framerate_fifo_id) // the entry-point function of the rt task
{  // id of fifo to which to send flag is sent in as arg from scheduler
   int ret;
   static u32 adc_cl[MAX_RESULT_QTY];
   static u16 adc_data[MAX_RESULT_QTY];
   int framerate_rtfifo = (int)framerate_fifo_id;
   int i, offs;
   char buf[1024];
   u32 samples;
   u32 tscLow1, tscHigh1, tscLow2, tscHigh2;


   // fill CL with "slow bit" flag
   for (i = 0; i < CL_SIZE; i++) adc_cl[i] = i | 0x100;

   // make this thread RT periodic. This happens only the first time we're called
   // schedules task to run once per period
   if ( pthread_make_periodic_np(pthread_self(),gethrtime(),FRAME_PERIOD_NS) )
   {
      printk("ERROR: Unable to register periodic thread !\n");
   }

   ret = pd_ain_set_config(board_handle, AIN_CFG, 0, 0);
   ret = pd_ain_set_channel_list(board_handle, CL_SIZE, adc_cl);
   ret = pd_ain_set_enable_conversion(board_handle, PDAQ_ENABLE);
   ret = pd_ain_sw_start_trigger(board_handle);
   ret = pd_ain_sw_cl_start(board_handle);

   while (1)    // this loop wakes up once per period
   {
      asm volatile
      (
          "rdtsc" : "=a" (tscLow1), "=d" (tscHigh1)
      );

      // send previous powerdaq samples to user space via rtl_fifo
      samples = 0;
      pd_ain_get_samples(board_handle, MAX_RESULT_QTY, adc_data, &samples);

      asm volatile
      (
          "rdtsc" : "=a" (tscLow2), "=d" (tscHigh2)
      );

      printk("Read one scan in %d cycles\n", tscLow2-tscLow1);

      // convert first 8 samples to output thru the fifo
      offs = 0;
      for (i = 0; i < CL_SIZE; i++)
         offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
      offs += sprintf(buf+offs, "\n");

      // throw data into rtl fifo
      ret = rtf_put(framerate_rtfifo, buf, offs+1);

      // program powerdaq for a new burst
      ret = pd_ain_sw_cl_start(board_handle);      // starts clock

      // end this thread until next periodic call
      pthread_wait_np();
   }
}


//==============================================================================
int init_module(void)                   // called when module is loaded
{
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads
   struct sched_param sched_param;     // struct defined in pthreads

   printk("init_module pd_rtl_ai\n");

   // create fifo we can talk via /dev/rtf0
   rtf_destroy(FRAMERATE_FIFO_ID);     // free up the resource, just in case
   rtf_create(FRAMERATE_FIFO_ID, FRAMERATE_FIFO_LENGTH);  // fifo from which rt task talks to device

   // prepare periodic thread for creation
   pthread_attr_init(&attrib);         // initialize thread attributes
   sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);  // obtain highest priority
   pthread_attr_setschedparam(&attrib, &sched_param);    // set our priority

   // create the thread
   if ( pthread_create(&periodic_thread, &attrib, Periodic_entry_point,
                       (void *)FRAMERATE_FIFO_ID) );

   // wake up our thread
   pthread_wakeup_np(periodic_thread);

   return 0;
}


//==============================================================================
void cleanup_module(void)
{
   printk("cleanup_module pd_rtl_ai\n");

   // get rid of fifo and thread
   rtf_destroy(FRAMERATE_FIFO_ID);      // free up the resource
   pthread_delete_np(periodic_thread);  // delete the thread
}
