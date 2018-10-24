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
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


#define PERIODIC_FREQ_HZ        100.0   // periodic rate
#define PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
                         
extern pd_board_t pd_board[PD_MAX_BOARDS];
pthread_t dio_thread;  // our periodic thread
int board_handle    =   1;  // first board in the system. See /proc/pwrdaq for board ID
int bAbort = 0;

void *dio_thread_proc(void *param)
{
   int retVal, i, j;
   char board_name[256];
   char serial[32];
   int board = 0;   // id of the board that we will use
   int nb_ports = 4;
   int port_mask = 0x09; // use ports 1 and 2 for output
   u32 value[4];
   struct timespec t1,t2;
   struct timespec abstime;
   
   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", board);
      return NULL;
   }

   printf("Found board %d: %s, s/n %s\n", board, board_name, serial);
         
   retVal = pd_dio_enable_output(board, port_mask); 
   if(!retVal)
   {
       printf("SingleDIO: pd_dio_enable_output error: %d\n", retVal);
       return NULL;
   }

   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);

   while(!bAbort)
   {
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, PERIOD_NS);

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

      printf("t2-t1 = %ld s %ld ns\n", t2.tv_sec, t2.tv_nsec); 
   }

 
   return NULL;
}


int main(void)                   // called when module is loaded
{
    pthread_attr_t attrib;              // thread attribute object. defined in pthreads
        
    printf("init_module rtl_DIO\n");

    pthread_attr_init(&attrib);         // Initialize thread attributes
    pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread
        
    // create the thread
    pthread_create(&dio_thread, &attrib, dio_thread_proc, NULL);
    
    // wait for module to be unloaded 
    rtl_main_wait();

    // Stop thread
    bAbort = 1;
    pthread_join(dio_thread, NULL);
    
    return 0;
}



