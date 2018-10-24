/*****************************************************************************/
/*                    DSP Counter timer example                              */
/*                                                                           */
/*  This example shows how to use the powerdaq API to time a RTL task using  */
/*  one of the onboard DSP counter/timers.                                   */
/*  This example only works with PD2-DIOxx boards. See rtl_UCT_event for an  */
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
#include <rtl.h>
#include <linux/slab.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"
#include "pd_dsp_ct.h"



pthread_t   uct_thread;
int bAbort = FALSE;

void *uct_thread_proc(void *param)
{
   int retVal;
   char board_name[256];
   char serial[32];
   int board = 0;   // id of the board that we will use
   int timer = 0;   // timer to use
   DWORD   events[3] = {eUct0Event, eUct1Event, eUct2Event};
   DWORD   timers[3] = {DCT_UCT0, DCT_UCT1, DCT_UCT2};
   DWORD   count;

         
   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("rtl_UctDsp_event: Could not find board %d\n", board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", board, board_name, serial);

   // disable counter
   retVal = pd_dspct_enable_counter(board, timers[timer], FALSE);
   if(!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdDspCtEnableCounter failed\n");
      return NULL;
   }


   retVal = pd_dspct_load(board, timers[timer], 0x0, 1650000, DCT_TimerPulse, TRUE, FALSE, FALSE);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdDspCtLoad failed\n");
      return NULL;
   }  

   if (!pd_adapter_enable_interrupt(board, 1))
   {
      rtl_printf("rtl_UctDsp_event: pd_adapter_enable_interrupt fails");
      return NULL;
   }

   retVal = pd_set_user_events(board, CounterTimer, events[timer]);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: pd_set_user_events error %d\n", retVal);
      return NULL;
   }

   // enable counter
   retVal = pd_dspct_enable_counter(board, timers[timer], TRUE);
   if(!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdDspCtEnableCounter failed\n");
      return NULL;
   }

   // Acquire 10 frames then exit
   while(!bAbort)
   {
      event = pd_sleep_on_event(board, CounterTimer, events[timer], 5000);

      rtl_printf("rtl_UctDsp_event: received event %d\n", event);

      if (event & eTimeout)
      {
         rtl_printf("rtl_UctDsp_event: wait timed out\n"); 
         bAbort = TRUE;
      }
    
      // deal with the events
      if (event & eUct0Event)
      {
         // read the number of counted events
         retVal = pd_dspct_get_count(board, 0, &count);
         if (!retVal)
         {
            rtl_printf("rtl_UctDsp_event: PdDspUctRead failed with error code %d\n", retVal);
            bAbort = TRUE;
         }
   
         rtl_printf("rtl_UctDsp_event: counter 0 counted %ld events\n", count);       
      }

      if (event & eUct1Event)
      {
         // read the number of counted events
         retVal = pd_dspct_get_count(board, 1, &count);
         if (retVal < 0)
         {
            rtl_printf("rtl_UctDsp_event: PdDspUctRead failed with error code %d\n", retVal);
            bAbort = TRUE;
         }
   
         rtl_printf("rtl_UctDsp_event: counter 1 counted %ld events\n", count);       
      }

      if (event & eUct2Event)
      {
         // read the number of counted events
         retVal = pd_dspct_get_count(board, 2, &count);
         if (retVal < 0)
         {
            rtl_printf("rtl_UctDsp_event: PdDspUctRead failed with error code %d\n", retVal);
            bAbort = TRUE;
         }
   
         rtl_printf("rtl_UctDsp_event: counter 2 counted %ld events\n", count);       
      }

      // re-enable event
      retVal = pd_set_user_events(board, CounterTimer, events[timer]);
      if (retVal < 0)
      {
         rtl_printf("rtl_UctDsp_event: _PdSetUserEvents error %d\n", retVal);
         bAbort = TRUE;
      }
   }

   // Stop interrupts and timer
   retVal = pd_clear_user_events(board, CounterTimer, events[timer]);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdClearUserEvents failed with error code %d\n", retVal);
      return NULL;
   }

   retVal = pd_adapter_enable_interrupt(board, 0);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdAdapterEnableInterrupt error %d\n", retVal);
      return NULL;
   } 

   retVal = pd_dspct_enable_counter(board, timers[timer], FALSE);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdDspEnableCounter\n");
      return NULL;
   }

   retVal = pd_dspct_load(board, timers[timer], 0, 0, 0, 0, 0, 0);
   if (!retVal)
   {
      rtl_printf("rtl_UctDsp_event: _PdDspCtLoad failed\n");
      return NULL;
   }  
                       
   return NULL;
}


int init_module(void)
{    
   rtl_printf("Initializing RTL UctDsp Event module\n");
   
   
   /*rtf_destroy(DATA_FIFO);
   rtf_destroy(COMMAND_FIFO);

   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // Create FIFO that will be used to receive data from the user space
   rtf_create(COMMAND_FIFO, 500);*/
   
   // Create thread that will perform the acquisition and writing of
   // data to the data FIFO
   bAbort = FALSE;
   pthread_create (&uct_thread,  NULL, uct_thread_proc, NULL);

   // Create thread that will receive and execute commands from the 
   // command FIFO
   //pthread_create (&cmd_thread,  NULL, CMD_thread, NULL);

   // Creates a handler for the command FIFO
   //rtf_create_handler(COMMAND_FIFO, &command_fifo_handler);
         
   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up RTL UctDsp Event module\n");

   //rtf_destroy(COMMAND_FIFO);
   //rtf_destroy(DATA_FIFO);

   bAbort = TRUE;
   pthread_cancel (uct_thread);
   pthread_join (uct_thread, NULL);
}




