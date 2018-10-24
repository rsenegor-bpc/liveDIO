/******************************************************************************/
/*              EXternal scan clock example using counter/timer               */
/*                                                                            */
/*  This example shows how to use the powerdaq API to perform an Analog Input */
/*  data acquisition using one of the counter/timer to generate the scan      */
/*  clock and interrupt after each scan.                                      */
/*  It will only work for PD-MFx and PD2-MFx boards.                          */
/*  The signal CTRx_OUT must be connected to CL_CLK_IN.                       */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*      Copyright (C) 2006 United Electronic Industries, Inc.                 */
/*      All rights reserved.                                                  */
/*----------------------------------------------------------------------------*/
/*                                                                            */ 
/******************************************************************************/


#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#include "uct8254.h"

// Externs from the library
extern pd_board_t pd_board[PD_MAX_BOARDS];

int pd_error, uct_running;
int event_count;
pthread_t uct_thread;
sem_t uct_sem;
struct timespec starttime, stoptime;
    
tUctGenSquareWaveData uctData;
void* uct_thread_proc(void*  arg);

int UctEventInit(int board)
{
   int retVal;

   // enable events 
   retVal = pd_adapter_set_board_event1(board, UTB_Uct0Im);
   if(!retVal)
   {
      rtl_printf("scan_int: pd_adapter_set_board_event1 failed with code %x\n", retVal);
      return retVal;
   }

   sem_init(&uct_sem, 1, 0);
   
   uct_running = 1;
   event_count = 0;
   
   // Create thread that will wait for events coming from the timer 
   rtl_pthread_create (&uct_thread,  NULL, uct_thread_proc, &uctData);

   clock_gettime(CLOCK_REALTIME, &starttime);
   
   return 0;
}

// ISR, only used if CUSTOM_IRQ_HANDLER is defined.
unsigned int uct_isr(unsigned int irq,struct rtl_frame *frame) 
{
    int ret;
    tEvents Events;
    
    if (pd_dsp_int_status(uctData.board)) 
    { 
        // interrupt pending
        pd_dsp_acknowledge_interrupt(uctData.board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(uctData.board, &Events);
        // check is FHF?
        if (Events.ADUIntr & UTB_Uct0IntrSC) 
        {
            // Notify waiting thread that the event occured
            sem_post(&uct_sem);
        }
    }
    else
    {
        rtl_global_pend_irq(irq);
	rtl_hard_enable_irq(irq);
    }

    return 0;
}


void UctEventTerminate(void)
{
   uct_running = 0;
   pd_adapter_set_board_event1(uctData.board, UTB_Uct0IntrSC);
   pd_adapter_enable_interrupt(uctData.board, 0);
   
   rtl_free_irq(pd_board[uctData.board].irq);  // release IRQ
  
   clock_gettime(CLOCK_REALTIME, &stoptime);
   timespec_sub(&stoptime, &starttime);
   
   // Kill the isr thread
   rtl_pthread_cancel(uct_thread);
   rtl_pthread_join(uct_thread, NULL);

   rtl_sem_destroy(&uct_sem);

   rtl_printf("received %d events in %ld ms\n", event_count, 
	      stoptime.tv_sec*1000.0 + stoptime.tv_nsec/1000000.0);
}

void* uct_thread_proc(void*  arg )
{
   // get hold on interrupt vector
   if(rtl_request_irq(pd_board[uctData.board].irq, uct_isr))
   {
      printk("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return NULL;
   }

   // enable it
   rtl_hard_enable_irq(pd_board[uctData.board].irq);

   // enable interrupts
   pd_adapter_enable_interrupt(uctData.board,1);

   rtl_printf("Timer interrupt is started\n");    
	
   while(uct_running)
   {
       // Wait for a timer interrupt that signals us that a scan
       // has been acquired
       sem_wait(&uct_sem);
       if(!uct_running)
          break;

       //printk("Got IRQ\n");
       event_count++;

       // reenable events/interrupts
       pd_adapter_set_board_event1(uctData.board, UTB_Uct0Im);

       // reenable irq handling
       pd_adapter_enable_interrupt(uctData.board, 1);
       rtl_hard_enable_irq(pd_board[uctData.board].irq);
    }

    rtl_printf("uct_thread is terminating\n");

    pthread_exit(NULL); 
  
    return NULL;
}



int main(void)
{
   char board_name[256];
   char serial[32];
   
   // Initializes counter/timer  
   uctData.board = 1;
   uctData.counter = 0;
   uctData.clockSource = clkInternal1MHz;
   uctData.gateSource = gateSoftware;
   uctData.mode = SquareWave;
   uctData.state = closed;
    
   // enumerate boards
   if(pd_enum_devices(uctData.board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", uctData.board);
      return -ENXIO;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", uctData.board, board_name, serial);
    
   // Configure timer to generate interrupts
   InitUctGenSquareWave(&uctData);
   StartUctGenSquareWave(&uctData, 10000.0);  
       
   // Initializes and starts 
   UctEventInit(uctData.board);  

   rtl_main_wait();
   
   UctEventTerminate();
    
   StopUctGenSquareWave(&uctData);
   CleanUpUctGenSquareWave(&uctData);
}

