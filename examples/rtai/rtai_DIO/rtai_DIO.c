/*****************************************************************************/
/*                            DIO Board example                              */
/*                                                                           */
/*  This example shows how to use the powerdaq API to read/write the digital */
/*  ports on a PD2-DIOx board.                                               */
/*  This example only works with PD2-DIOxx boards. See rtl_DI for an         */
/*  example that works on PD2-MFx boards.                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2002 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define PERIODIC_FREQ_HZ        2.0   // periodic rate
#define PERIODIC_PERIOD_NS ((RTIME)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0)) 
#define DATA_FIFO       0                               

extern pd_board_t pd_board[PD_MAX_BOARDS];
RT_TASK   dio_task;
int bAbort = FALSE;

void dio_task_proc(int param)
{
   int retVal, i, j;
   char board_name[256];
   char buf[1024];   // temp char buffer
   int offs;
   char serial[32];
   int board = 0;   // id of the board that we will use
   int nb_ports = 4;
   int port_mask = 0x09; // use ports 1 and 2 for output
   u32 value[4];
   RTIME t1,t2;


   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      printk("Error: Could not find board %d\n", board);
      return;
   }

   printk("Found board %d: %s, s/n %s\n", board, board_name, serial);
         
   retVal = pd_dio_enable_output(board, port_mask); 
   if(!retVal)
   {
       printk("SingleDIO: pd_dio_enable_output error: %d\n", retVal);
       return;
   }

   while(!bAbort)
   {
      t1 = rt_get_time();	   
      for(i=0; i<1000; i++)
      {
          for(j=0; j<nb_ports; j++)
          {
             if((1 << j) & port_mask)
             {
                value[j] = i*j;
                retVal = pd_dio_write(board, j, value[j]);
                if(!retVal)
                {
                   printk("SingleDIO: _PdDIOWrite error: %d\n", retVal);
                   return;
                }
             }
             else
             {
                retVal = pd_dio_read(board, j, &value[j]);
                if(!retVal)
                {
                   printk("SingleDIO: _PdDIORead error: %d\n", retVal);
                   return;
                }
             }
          }
      }

      t2 = rt_get_time();

      printk("t2-t1 = %d\n", (int)(count2nano(t2-t1))); 


      offs = 0;
      for(j=0; j<nb_ports; j++)
         offs += sprintf(buf+offs, " port %d: 0x%x", j, value[j]);
      offs += sprintf(buf+offs, "\n");

      // throw data into rt fifo
      retVal = rtf_put(DATA_FIFO, buf, offs+1);

      // wait until next periodic call
      rt_task_wait_period();
   }

   
   return;
}


int init_module(void)
{    
   printk("Initializing DIO RT module\n");
   
   
   rtf_destroy(DATA_FIFO);
      
   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // create the task
   rt_task_init(&dio_task, dio_task_proc, 0, 10000, 0, 0, 0);
   
   // Make task periodic
   rt_task_make_periodic(&dio_task, rt_get_time(), nano2count(PERIODIC_PERIOD_NS));
   
   start_rt_timer(nano2count(100000));
         
   return 0;
} 


void cleanup_module(void)
{
   printk("Cleaning-up DIO RT module\n");

   bAbort = TRUE;
   // get rid of fifo and thread
   stop_rt_timer();
   rtf_destroy(DATA_FIFO);      // free up the resource
   rt_task_delete(&dio_task);  // delete the thread}
}



