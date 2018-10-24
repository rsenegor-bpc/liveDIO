/*****************************************************************************/
/*                 Digital input change detection example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to generate an interrupt  */
/*  when one of the digital input line state changes.                        */
/*  This example is designed to work with a "floating message" clock such as */
/*  the one from Fantazein.                                                  */
/*  Each time the board detects a state change on line 2 it starts outputing */
/*  characters on the digital output port 0. Each character is displayed     */
/*  column per column.                                                       */
/*                                                                           */
/*  This example can work with its own interrupt handler or with the one     */
/*  installed by default by the powerdaq driver.                             */
/*  -To use custom irq handler:                                              */
/*      load the driver with the following option: insmod pwrdaq.o rqstirq=0 */
/*      Uncomment the line "#define CUSTOM_IRQ_HANDLER 1" in this source file*/
/*  -To use the driver's irq handler:                                        */
/*      Comment out the line "#define CUSTOM_IRQ_HANDLER 1"                  */
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

#define CUSTOM_IRQ_HANDLER 

#define FONTDATAMAX 2048
#define TEXTMAX 256

extern unsigned char fontdata_8x8[FONTDATAMAX];


extern pd_board_t pd_board[PD_MAX_BOARDS];
RT_TASK   dio_task;
RT_TASK   gettext_task;
int bAbort = FALSE;
int board = 1;   // id of the board that we will use
unsigned char rotatedFont[FONTDATAMAX];
char textBuffer[TEXTMAX];
int textSize = TEXTMAX;

void di_event_isr(int irq);

void di_event_proc(int param)
{
   int retVal, i, j;
   char board_name[256];
   char serial[32];
   unsigned long eventsToNotify = eDInEvent;
   unsigned long event;
   int risingEdges[8] = {DIB_0CFG0, DIB_1CFG0, DIB_2CFG0, DIB_3CFG0, DIB_4CFG0, DIB_5CFG0, DIB_6CFG0, DIB_7CFG0};
   int fallingEdges[8] = {DIB_0CFG1, DIB_1CFG1, DIB_2CFG1, DIB_3CFG1, DIB_4CFG1, DIB_5CFG1, DIB_6CFG1, DIB_7CFG1};
   unsigned long lineMask = 4; // trigger signal from the clock is connected on digital input line 2
   unsigned long diCfg;
   unsigned int status, count = 0;
   int bufStart = 0;
   

   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rt_printk("Error: Could not find board %d\n", board);
      return;
   }

   rt_printk("Found board %d: %s, s/n %s\n", board, board_name, serial);

   // setup the board to generate an interrupt when the selected line
   // receives the specified edge
   diCfg = 0;
   for(i=0; i<8; i++)
   {
      if((1 << i) & lineMask)
         diCfg |= risingEdges[i];
   }

   if (!pd_din_set_config(board, diCfg))
   {
      rt_printk("rtl_DI_event: pd_din_set_config fails");
      return;
   }


   if (!pd_adapter_enable_interrupt(board, 1))
   {
      rt_printk("rtl_DI_event: pd_adapter_enable_interrupt fails");
      return;
   }

#ifdef CUSTOM_IRQ_HANDLER
   // get hold on interrupt vector
   if(rt_request_global_irq(pd_board[board].irq, (void*)di_event_isr))
   {
      printk("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return;
   }

   // enable it
   rt_enable_irq(pd_board[board].irq);

   // enable events 
   if(!pd_adapter_set_board_event1(board, DIB_IntrIm))
   {
      rt_printk("rtl_DI_event: pd_adapter_set_board_event1 failed\n");
      return;
   }
#else
   retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rt_printk("rtl_DI_event: pd_set_user_events error %d\n", retVal);
      return;
   }
#endif

   while(!bAbort)
   {
#ifdef CUSTOM_IRQ_HANDLER
      // Wait for event
      rt_task_suspend(&dio_task);
      if(bAbort)
         break;

      event = eDInEvent;
#else
      event = pd_sleep_on_event(board, DigitalIn, eventsToNotify, 5000);
#endif

      if (event & eTimeout)
      {
         rt_printk("rtl_DI_event: wait timed out\n"); 
         bAbort = TRUE;
         continue;
      }
    
      // deal with the events
      if (event & eDInEvent)
      {
         pd_din_status(board, &status);
         pd_din_clear_data(board);
     
         rt_printk("rtl_DI_event: event received! status 0x%x\n", status);               
	 
         // Flash output line 0 when we get a line state change coming from 
         // digital input line 2
         if(status & lineMask)
         {
            int displaySize = 16;
            bufStart = count % textSize;
            RTIME delay = 100*1000;
   
            for(i=0; i<displaySize; i++)
            {	  
               int letter = textBuffer[(bufStart+i)%textSize];
                  
               for(j=0; j<8; j++)
               {
                  pd_dout_write_outputs(board, rotatedFont[letter*8 + j]);
                  rt_sleep(nano2count(delay));
                  pd_dout_write_outputs(board, 0x00);
                  rt_sleep(nano2count(delay));
               }
               // Add one extra blank between letters
               //rt_sleep(nano2count(delay));
            }

            count++;
         }
      }

      
      // re-enable event
#ifdef CUSTOM_IRQ_HANDLER 
      pd_adapter_set_board_event1(board, DIB_IntrIm);

      // reenable irq handling
      pd_adapter_enable_interrupt(board, 1);
      rt_enable_irq(pd_board[board].irq);
#else
      retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
      if (retVal < 0)
      {
         rt_printk("rtl_DI_event: _PdSetUserEvents error %d\n", retVal);
         bAbort = TRUE;
      }
#endif
   }

   // Stop interrupts and timer
   retVal = pd_clear_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rt_printk("rtl_DI_event: _PdClearUserEvents failed with error code %d\n", retVal);
      return;
   }

   retVal = pd_adapter_enable_interrupt(board, 0);
   if (!retVal)
   {
      rt_printk("rtl_DI_event: _PdAdapterEnableInterrupt error %d\n", retVal);
      return;
   } 

                          
   return;
}


// ISR, only used if CUSTOM_IRQ_HANDLER is defined.
void di_event_isr(int irq) 
{
    int ret;
    tEvents Events;
    
    if (pd_dsp_int_status(board)) 
    { 
        // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        // check is FHF?
        if (Events.ADUIntr & DIB_IntrSC) 
        {
            // Notify waiting thread that the event occured
            rt_task_resume(&dio_task);
        }
    }
    else
    {
       rt_pend_linux_irq(irq);
       rt_enable_irq(irq);
    }

    return; 
}


int text_fifo_handler(unsigned int fifo, int rw)
{
   int ret = 0;

   if(rw == 'w')
   {
      rt_printk("receiving data from FIFO\n");
      ret = rtf_get(fifo, textBuffer, TEXTMAX-1);
      if(ret > 0)
      {
         textSize = ret-1;
         textBuffer[textSize] = '\0';
      }
   }
   return 0;
}


int init_module(void)
{  
   int i, j;

   rt_printk("Initializing RTL DI event module\n");

   // We need to rotate the font because we are going to output
   // characters column by column and they are stored line by line
   // in the data buffer.
   for(i=0; i<FONTDATAMAX/8; i++)
   {
      for(j=0; j<8; j++)
      {
         rotatedFont[i*8+(7-j)] = (((fontdata_8x8[i*8] >> j) & 0x01) << 7) |
                              (((fontdata_8x8[i*8+1] >> j) & 0x01) << 6) |
                              (((fontdata_8x8[i*8+2] >> j) & 0x01) << 5) |
                              (((fontdata_8x8[i*8+3] >> j) & 0x01) << 4) |
                              (((fontdata_8x8[i*8+4] >> j) & 0x01) << 3) |
                              (((fontdata_8x8[i*8+5] >> j) & 0x01) << 2) |
                              (((fontdata_8x8[i*8+6] >> j) & 0x01) << 1) |
                              (((fontdata_8x8[i*8+7] >> j) & 0x01) << 0);
      }
   }

   strcpy(textBuffer, "Test Fantazein!");
   textSize = 15;

   // Create the FIFO that we use to transmit data between user and kernel space
   rtf_create(0, TEXTMAX);

   // register fifo handler to receive text from user space
   rtf_create_handler(0, X_FIFO_HANDLER(text_fifo_handler));

   start_rt_timer(nano2count(100000));

   // Create thread that will perform the digital line change detection
   bAbort = FALSE;
   rt_task_init(&dio_task, di_event_proc, 0, 10000, 0, 0, 0);

   rt_task_resume(&dio_task);

   return 0;
} 


void cleanup_module(void)
{
   rt_printk("Cleaning-up DI event RT module\n");

   bAbort = TRUE;
   rt_task_delete(&dio_task);

#ifdef CUSTOM_IRQ_HANDLER
   rt_free_global_irq(pd_board[board].irq);  // release IRQ
#endif

   rtf_destroy(0);

   stop_rt_timer();
}




