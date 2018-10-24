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
// This implements a RTAI periodic task.
// This process writes a character each frame to an rtl_fifo, frequency = 10 Hz
//
//
// Compile the PowerDAQ Linux driver with the command make RTAI=1
//

//#define PD2_AO_96 // undefine this line for PD2-AO-8/16/32

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define PERIODIC_FREQ_HZ        10.0   // periodic rate
#define FRAME_PERIOD_NS ((RTIME)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define FRAMERATE_FIFO_ID       0      // rt-fifo on which to put newframe flag
#define FRAMERATE_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_RESULT_QTY          0x100  // maximum number of samples is limited by maximum CL size = 0x100

#define AO_CFG 0
//==============================================================================
// Prototypes
void Periodic_entry_point(int fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
RT_TASK periodic_task;  // our periodic thread
int board_handle    =   0;  // first board in the system. See /proc/pwrdaq for board ID
int boardId;                // model id of the board
int bAbort = 0;

//==============================================================================
void Periodic_entry_point(int fifo_id) // the entry-point function of the rt task
{  
   int ret;
   int i, j, offs;
   int maxVal;
   static u16 val = 0;
   char buf[256];
   int numValues = 20;
   RTIME t1,t2;

   ret = pd_aout_set_config(board_handle, AO_CFG, 0);

   if(PD_IS_AO(boardId))
   {
      rt_printk("This is an AO board\n");
      maxVal = 0xFFFF;  // AO boards have 16 bits DACs
   }
   else
   {
      rt_printk("This is a MF board\n");
      maxVal = 0xFFF;   // MF boards have 12 bits DACs
   }

   while (!bAbort)    // this loop wakes up once per period
   {
      val=0;
      t1 = rt_get_time();
      // Write bursts of n values every cycles
      // Measure how long it takes to write those
      // n values to the DAC
      for(j=0; j<numValues; j++)
      {
         // send data to the board
         if(PD_IS_AO(boardId))
         {
         #ifdef PD2_AO_96
            for (i = 0; i < 95; i++)
               ret = pd_ao96_writex(board_handle, i, val, 0, 0);
         #else
            for (i = 0; i < 31; i++)
               ret = pd_ao32_writex(board_handle, i, val, 0, 0);
         #endif 
         }
         else
         {
            ret = pd_aout_put_value(board_handle, val);
         }
         val += 0x10*(j+1); // choose whatever increment you want
         if (val > maxVal)
         {
            val = 0; 
         }
      }
      t2 = rt_get_time();

      rt_printk("Generated %d values in %ld nsec\n", numValues, count2nano(t2-t1));
         
      // write informtation about ongoing activity
      offs = 0;
      offs += sprintf(buf+offs,".");
      ret = rtf_put(fifo_id, buf, offs+1);

      // suspend this thread until next periodic call
      rt_task_wait_period();
   }
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
   char board_name[256];
   char serial[32];

   rt_printk("init_module pd_rtl_ao\n");

   // enumerate boards
   if ((boardId = pd_enum_devices(board_handle, board_name, sizeof(board_name), 
                       serial, sizeof(serial)))<0)
   {
      rt_printk("Error: Could not find board %d\n", board_handle);
      return -ENXIO;
   }

   rt_printk("Found board %d: %s, s/n %s, id 0x%x\n", board_handle, board_name, serial, boardId);

   // create fifo we can talk via /dev/rtf0
   rtf_destroy(FRAMERATE_FIFO_ID);     // free up the resource, just in case
   rtf_create(FRAMERATE_FIFO_ID, FRAMERATE_FIFO_LENGTH);  // fifo from which rt task talks to device

   rt_set_periodic_mode();

   // create the task
   rt_task_init(&periodic_task, Periodic_entry_point, 0, 10000, FRAMERATE_FIFO_ID, 0, 0);
   // Make task periodic
   rt_task_make_periodic(&periodic_task, rt_get_time(), nano2count(FRAME_PERIOD_NS));
   //rt_task_resume(&periodic_task);

   // start rtai timer with period of 100 usec
   start_rt_timer(nano2count(10000));

   return 0;
}


//==============================================================================
void cleanup_module(void)
{
   rt_printk("cleanup_module pd_rtl_ao\n");

   // terminate the task
   bAbort = 1;
   rt_task_delete(&periodic_task);  // delete the thread

   stop_rt_timer();
   rtf_destroy(FRAMERATE_FIFO_ID);      // free up the resource
}

