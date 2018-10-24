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

#include <pthread.h>
#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <posix/time.h>
#include <rtl.h>
#include <linux/slab.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define PERIODIC_FREQ_HZ        2.0   // periodic rate
#define PERIODIC_PERIOD_NS ((RTIME)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0)) 
#define DATA_FIFO       0                               

extern pd_board_t pd_board[PD_MAX_BOARDS];
pthread_t   dio_thread;
pthread_attr_t attr;
struct sched_param sched_param;
int bAbort = FALSE;

void *dio_thread_proc(void *param)
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
   struct timespec t1,t2;

   // Make thread periodic at 10 Hz (10 000 000 ns)
   pthread_make_periodic_np(pthread_self(),clock_gethrtime(CLOCK_REALTIME), 100000000LL);

   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", board, board_name, serial);
         
   retVal = pd_dio_enable_output(board, port_mask); 
   if(!retVal)
   {
       rtl_printf("SingleDIO: pd_dio_enable_output error: %d\n", retVal);
       return NULL;
   }

   while(!bAbort)
   {
      clock_gettime(CLOCK_REALTIME, &t1);    
      for(i=0; i<6; i++)
      {
          for(j=0; j<nb_ports; j++)
          {
             if((1 << j) & port_mask)
             {
                value[j] = i*j;
                retVal = pd_dio_write(board, j, value[j]);
                if(!retVal)
                {
                   rtl_printf("SingleDIO: _PdDIOWrite error: %d\n", retVal);
                   return NULL;
                }
             }
             else
             {
                retVal = pd_dio_read(board, j, &value[j]);
                if(!retVal)
                {
                   rtl_printf("SingleDIO: _PdDIORead error: %d\n", retVal);
                   return NULL;
                }
             }
          }
      }

      clock_gettime(CLOCK_REALTIME, &t2); 

      timespec_sub(&t2, &t1);

      rtl_printf("t2-t1 = %ld s %ld ns\n", t2.tv_sec, t2.tv_nsec); 


      offs = 0;
      for(j=0; j<nb_ports; j++)
         offs += sprintf(buf+offs, " port %d: 0x%x", j, value[j]);
      offs += sprintf(buf+offs, "\n");

      // throw data into rt fifo
      retVal = rtf_put(DATA_FIFO, buf, offs+1);

      // wait until next periodic call
      pthread_wait_np();
   }

 
   return NULL;
}


int init_module(void)
{    
   rtl_printf("Initializing DIO RT module\n");
   
   
   rtf_destroy(DATA_FIFO);
      
   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // create the task
   pthread_attr_init(&attr);
   sched_param.sched_priority = 10;
   pthread_attr_setschedparam(&attr, &sched_param);
   pthread_create (&dio_thread,  &attr, dio_thread_proc, NULL);

   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up DIO RT module\n");

   bAbort = TRUE;
   // get rid of fifo and thread
   rtf_destroy(DATA_FIFO);      // free up the resource
   pthread_cancel (dio_thread);
   pthread_join (dio_thread, NULL);
}



