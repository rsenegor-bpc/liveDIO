/*****************************************************************************/
/*                    DIO event example                                      */
/*                                                                           */
/*  This example shows how to use the powerdaq API to generate an interrupt  */
/*  when one of the digital input line state changes.                        */
/*  This example only works with PD2-DIOxx boards. See rtl_DI_event for an   */
/*  example that works on PD2-MFx boards.                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2004 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


extern pd_board_t pd_board[PD_MAX_BOARDS];
RT_TASK   dio_task;
int bAbort = FALSE;
int board = 0;   // id of the board that we will use

//==============================================================================
// ISR
int pd_isr(int trigger_irq) 
{
    int ret;
    tEvents Events;
    
    if ( pd_dsp_int_status(board) ) 
    { // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        
        // check for non-recoverable errors
        if (Events.ADUIntr & DIB_IntrSC) 
        {
           // Wake-up task
           rt_task_resume(&dio_task);
        }
    }

    return 0;
}


void dio_task_proc(int param)
{
   int retVal, i;
   char board_name[256];
   char serial[32];
   int inport = 0;
   int outport = 1;
   int line = 0;


   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rt_printk("Error: Could not find board %d\n", board);
      return;
   }

   rt_printk("Found board %d: %s, s/n %s\n", board, board_name, serial);

   retVal = pd_dio_enable_output(board, (1 << (outport)));
   
         
   // setup the board to generate an interrupt when the selected line
   // change state
   for(i=0; i<8; i++)
      if(i==inport)
         pd_board[board].DinSS.intrMask[i] = (1 << line);
   retVal = pd_dio256_setIntrMask(board);
   if (!retVal)
   {
      rt_printk("rtl_DIO_event: _PdDIOSetIntrMask error %d\n", retVal);
      return;
   } 

   while(!bAbort)
   {
      retVal = pd_adapter_set_board_event1(board, DIB_IntrIm /*| DIB_IntrSC*/);
      if (!retVal)
      {
         rt_printk("rtl_DIO_event: pd_adapter_set_board_event1 error %d\n", retVal);
         return;
      } 
	
      retVal = pd_dio256_intrEnable(board, 1);
      if (!retVal)
      {
         rt_printk("rtl_DIO_event: _PdDIOIntrEnable error %d\n", retVal);
         return;
      } 

      if (!pd_adapter_enable_interrupt(board, 1))
      {
         rt_printk("rtl_DIO_event: pd_adapter_enable_interrupt fails");
         return;
      }

      rt_enable_irq(pd_board[board].irq);

      pd_dio_write(board, outport, 0);
      pd_dio_write(board, outport, 0xFFFF);

      // Wait for event
      rt_task_suspend(&dio_task);
   
      pd_dio_write(board, inport, 0);
 
      // deal with the events
      pd_dio256_getIntrData(board);

      rt_printk("rtl_DIO_event: event received! level %d, latch %d\n", 
                  pd_board[board].DinSS.intrData[inport], 
                  pd_board[board].DinSS.intrData[inport + 8]);

      rt_sleep(nano2count(10000));

   }

   // Stop interrupts
   retVal = pd_adapter_enable_interrupt(board, 0);
   if (!retVal)
   {
      rt_printk("rtl_DIO_event: _PdAdapterEnableInterrupt error %d\n", retVal);
      return;
   }                      
}


int init_module(void)
{    
   rt_printk("Initializing RTL DIO event module\n");
   
   // get hold on interrupt vector
   if(rt_request_global_irq(pd_board[board].irq, (void(*)(void))pd_isr))
   {
       printk("rt_request_global_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
       return -EIO;
   }

   rt_start_timer(nano2count(10000));

   // Create thread that will perform the acquisition and writing of
   // data to the data FIFO
   bAbort = FALSE;
   rt_task_init(&dio_task, dio_task_proc, 0, 10000, 0, 0, 0);
   rt_task_resume(&dio_task);
       
   return 0;
} 


void cleanup_module(void)
{
   rt_printk("Cleaning-up DIO event RT module\n");

   bAbort = TRUE;
   rt_task_delete(&dio_task);

   rt_free_global_irq(pd_board[board].irq);  // release IRQ 

   rt_stop_timer();
}




