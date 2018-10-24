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
//  (C) 2005 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux periodic task using the "pthreads" API.
// This process writes a character each frame to an rtl_fifo, frequency = 10 Hz
//
//
// View the output with "cat /pd_rtl_ai"
//
//
// This was compiled using RTLinuxPro 2.2 
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

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1

//==============================================================================
// Prototypes
void *Periodic_entry_point(void *framerate_fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
pthread_t periodic_thread;  // our periodic thread
int board_handle    =   1;  // first board in the system. See /proc/pwrdaq for board ID
int bStop = 0;
int fifod;

//==============================================================================
void *Periodic_entry_point(void *arg) // the entry-point function of the rt task
{  
   int ret;
   static u32 adc_cl[CL_SIZE];
   static u16 adc_data[CL_SIZE];
   int i, counter = 0;
   u32 samples;
   struct timespec abstime;


   // fill CL with "slow bit" flag
   for (i = 0; i < CL_SIZE; i++)
   {
      adc_cl[i] = i | 0x100;
   }

   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);

   printf("Setting task period to %lld\n", PERIOD_NS);

   ret = pd_ain_set_config(board_handle, AIN_CFG, 0, 0);
   ret = pd_ain_set_channel_list(board_handle, CL_SIZE, adc_cl);
   ret = pd_ain_set_enable_conversion(board_handle, 1);
   ret = pd_ain_sw_start_trigger(board_handle);
   ret = pd_ain_sw_cl_start(board_handle);

   while (!bStop && counter < 100)    // this loop wakes up once per period
   {
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, PERIOD_NS);

      // send previous powerdaq samples to user space via rtl_fifo
      samples = 0;
      pd_ain_get_samples(board_handle, CL_SIZE, adc_data, &samples);

      printf("%d, Got %d samples\n", counter, samples);

      // Send samples through the fifo
      ret = write(fifod, adc_data, sizeof(adc_data));         

      // program powerdaq for a new burst
      ret = pd_ain_sw_cl_start(board_handle);      // starts clock

      counter++;
   }

   return NULL;
}


//==============================================================================
int main(void)                   // called when module is loaded
{
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads

   printf("init_module pd_rtl_ai\n");

   // make a the FIFO that is available to RTLinux and Linux applications 
   mkfifo( "/dev/pd_rtl_ai", 0666);
   fifod = open("/dev/pd_rtl_ai", O_WRONLY | O_NONBLOCK);

   // Resize the FIFO so that it can hold 8K of data
   ftruncate(fifod, 8 << 10);

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&periodic_thread, &attrib, Periodic_entry_point, NULL);

   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   bStop = 1;
   pthread_join(periodic_thread, NULL);

   // Close and remove the FIFO
   close(fifod);
   unlink("/dev/pd_rtl_ai");

   return 0;
}


