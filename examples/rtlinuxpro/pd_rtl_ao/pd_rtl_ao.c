//===========================================================================
//
// NAME:    pd_rtl_ao.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver AOut test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    06-MAY-2003
//
//
// HISTORY:
//
//
//---------------------------------------------------------------------------
//  (C) 2003 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux periodic task using the "pthreads" API.
// This process writes analog output with 1000 Hz
//
// This was compiled using the RTLinuxPro 2.1 
//

//#define PD2_AO_96 // undefine this line for PD2-AO-8/16/32
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

#define PERIODIC_FREQ_HZ        1000.0   // periodic rate
#define PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define CL_SIZE                 8      // current CL size

#define AO_CFG 0
//==============================================================================
// Prototypes
void *Periodic_entry_point(void *arg);   // the entry-point function of the rt task

//==============================================================================
// global variables
rtl_pthread_t periodic_thread;  // our periodic thread
int board_handle    =   0;  // first board in the system. See /proc/pwrdaq for board ID
int bStop = 0;

//==============================================================================
void *Periodic_entry_point(void *arg) // the entry-point function of the rt task
{  
   int ret;
   int i;
   static u16 val = 0x4000;
   struct timespec abstime;

   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);

   ret = pd_aout_set_config(board_handle, AO_CFG, 0);

   while (!bStop)    // this loop wakes up once per period
   {
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, PERIOD_NS);

      // send data to the ao card
#ifdef PD2_AO_96
      for (i = 0; i < 95; i++)
      {
         ret = pd_ao96_writex(board_handle, i, val, 0, 0);
      }
#else
      for (i = 0; i < 31; i++)
      {
         ret = pd_ao32_writex(board_handle, i, val, 0, 0);
      }
#endif
      val += 0x400; // choose whatever increment you want
      if (val > 0xC000)
      {
         val = 0x4000; // +/-5V ramp
      }
   }
   return NULL;
}

//==============================================================================
int main(void)                   // called when module is loaded
{
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads

   printf("init_module pd_rtl_ao\n");

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&periodic_thread, &attrib, Periodic_entry_point, NULL);

   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   bStop = 1;
   pthread_join(periodic_thread, NULL);

   return 0;
}
