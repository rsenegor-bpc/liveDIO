/*****************************************************************************/
/*                 Digital input change detection example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to generate an interrupt  */
/*  when one of the digital input line state changes.                        */
/*  This example only works with PD2-MFx boards. See rtl_DIO_event for an    */
/*  example that works on PD2-DIOx boards.                                   */
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
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define CUSTOM_IRQ_HANDLER 

extern pd_board_t pd_board[PD_MAX_BOARDS];
pthread_t   di_thread;
sem_t       di_sem;
int bAbort = FALSE;
int board = 1;   // id of the board that we will use

unsigned int di_event_isr(unsigned int trigger_irq,struct rtl_frame *frame);

void *di_event_thread_proc(void *param)
{
   int retVal, i;
   char board_name[256];
   char serial[32];
   unsigned long eventsToNotify = eDInEvent;
   unsigned long event;
   int risingEdges[8] = {DIB_0CFG0, DIB_1CFG0, DIB_2CFG0, DIB_3CFG0, DIB_4CFG0, DIB_5CFG0, DIB_6CFG0, DIB_7CFG0};
   int fallingEdges[8] = {DIB_0CFG1, DIB_1CFG1, DIB_2CFG1, DIB_3CFG1, DIB_4CFG1, DIB_5CFG1, DIB_6CFG1, DIB_7CFG1};
   unsigned long lineMask = 0xFF;
   unsigned long diCfg;
   unsigned long status, count = 0;


   // enumerate boards
   if (pd_enum_devices(board, board_name, sizeof(board_name), 
                       serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", board, board_name, serial);

   // setup the board to generate an interrupt when the selected line
   // receives the specified edge
   diCfg = 0;
   for (i=0; i<8; i++)
   {
      if ((1 << i) & lineMask)
         diCfg |= risingEdges[i];
   }

   if (!pd_din_set_config(board, diCfg))
   {
      rtl_printf("rtl_DI_event: pd_din_set_config fails");
      return NULL;
   }


   if (!pd_adapter_enable_interrupt(board, 1))
   {
      rtl_printf("rtl_DI_event: pd_adapter_enable_interrupt fails");
      return NULL;
   }

#ifdef CUSTOM_IRQ_HANDLER
   // enable events 
   if (!pd_adapter_set_board_event1(board, DIB_IntrIm))
   {
      rtl_printf("rtl_DI_event: pd_adapter_set_board_event1 failed\n");
      return NULL;
   }
#else
   retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rtl_printf("rtl_DI_event: pd_set_user_events error %d\n", retVal);
      return NULL;
   }
#endif

   // Acquire 10 frames then exit
   while (!bAbort)
   {
#ifdef CUSTOM_IRQ_HANDLER
      sem_wait(&di_sem);
      if (bAbort)
         break;

      event = eDInEvent;
#else
      event = pd_sleep_on_event(board, DigitalIn, eventsToNotify, 5000);
#endif

      rtl_printf("rtl_DI_event: received event %d, #%d\n", event, count++);

      if (event & eTimeout)
      {
         rtl_printf("rtl_DI_event: wait timed out\n"); 
         bAbort = TRUE;
         continue;
      }

      // deal with the events
      if (event & eDInEvent)
      {
         pd_din_status(board, &status);
         pd_din_clear_data(board);

         rtl_printf("rtl_DI_event: event received! status 0x%x\n", status);       
      }


      // re-enable event
#ifdef CUSTOM_IRQ_HANDLER 
      pd_adapter_set_board_event1(board, DIB_IntrIm);

      // reenable irq handling
      pd_adapter_enable_interrupt(board, 1);
      rtl_hard_enable_irq(pd_board[board].irq);
#else
      retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
      if (retVal < 0)
      {
         rtl_printf("rtl_DI_event: _PdSetUserEvents error %d\n", retVal);
         bAbort = TRUE;
      }
#endif
   }

   // Stop interrupts and timer
   retVal = pd_clear_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rtl_printf("rtl_DI_event: _PdClearUserEvents failed with error code %d\n", retVal);
      return NULL;
   }

   retVal = pd_adapter_enable_interrupt(board, 0);
   if (!retVal)
   {
      rtl_printf("rtl_DI_event: _PdAdapterEnableInterrupt error %d\n", retVal);
      return NULL;
   }


   return NULL;
}


// ISR, only used if CUSTOM_IRQ_HANDLER is defined.
unsigned int di_event_isr(unsigned int irq,struct rtl_frame *frame) 
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
         sem_post(&di_sem);
      }
   } 
   else
   {
      rtl_global_pend_irq(irq);
      rtl_hard_enable_irq(irq);
   }

   return 0;
}


int main(void)                   // called when module is loaded
{
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads

   printf("init_module rtl_DI_event\n");

#ifdef CUSTOM_IRQ_HANDLER
   sem_init (&di_sem, 1, 0);

   // get hold on interrupt vector
   if (rtl_request_irq(pd_board[board].irq, di_event_isr))
   {
      rtl_printf("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return -1;
   }

   // enable it
   rtl_hard_enable_irq(pd_board[board].irq);
#endif

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&di_thread, &attrib, di_event_thread_proc, NULL);

   // wait for module to be unloaded 
   rtl_main_wait();

   bAbort = TRUE;

#ifdef CUSTOM_IRQ_HANDLER
   rtl_free_irq(pd_board[board].irq);  // release IRQ

   // signal the thread to unlock itself
   sem_post(&di_sem);
#endif

   pthread_join(di_thread, NULL);

   return 0;
}




