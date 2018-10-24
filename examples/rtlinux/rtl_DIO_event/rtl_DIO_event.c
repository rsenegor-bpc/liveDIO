/*****************************************************************************/
/*                 Digital input change detection example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to generate an interrupt  */
/*  when one of the digital input line state changes.                        */
/*  This example only works with PD2-DIOxx boards. See rtl_DI_event for an   */
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


extern pd_board_t pd_board[PD_MAX_BOARDS];
pthread_t   dio_thread;
int bAbort = FALSE;

void *dio_event_thread_proc(void *param)
{
   int retVal, i;
   char board_name[256];
   char serial[32];
   int board = 0;   // id of the board that we will use
   DWORD eventsToNotify = eDInEvent;
   DWORD event;
   int port = 0;
   int line = 1;

   // enumerate boards
   if(pd_enum_devices(board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", board, board_name, serial);

   // setup the board to generate an interrupt when the selected line
   // change state
   for(i=0; i<8; i++)
      if(i==port)
         pd_board[board].DinSS.intrMask[i] = (1 << line);
   retVal = pd_dio256_setIntrMask(board);
   if (!retVal)
   {
      rtl_printf("rtl_DIO_event: _PdDIOSetIntrMask error %d\n", retVal);
      return NULL;
   } 

   // event suff
   retVal = pd_dio256_intrEnable(board, 1);
   if (!retVal)
   {
      rtl_printf("rtl_DIO_event: _PdDIOIntrEnable error %d\n", retVal);
      return NULL;
   } 

   if (!pd_adapter_enable_interrupt(board, 1))
   {
      rtl_printf("rtl_DIO_event: pd_adapter_enable_interrupt fails");
      return NULL;
   }

   retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rtl_printf("rtl_DIO_event: pd_set_user_events error %d\n", retVal);
      return NULL;
   }

   // Acquire 10 frames then exit
   while(!bAbort)
   {
      event = pd_sleep_on_event(board, DigitalIn, eventsToNotify, 5000);

      rtl_printf("rtl_DIO_event: received event %d\n", event);

      if (event & eTimeout)
      {
         rtl_printf("rtl_DIO_event: wait timed out\n"); 
         bAbort = TRUE;
         continue;
      }
    
      // deal with the events
      if (event & eDInEvent)
      {
          pd_dio256_getIntrData(board);

          rtl_printf("rtl_DIO_event: event received! level %d, latch %d\n", 
                  pd_board[board].DinSS.intrData[port], 
                  pd_board[board].DinSS.intrData[port + 8]);       
      }

      
      // re-enable event
      retVal = pd_set_user_events(board, DigitalIn, eventsToNotify);
      if (retVal < 0)
      {
         rtl_printf("rtl_DIO_event: _PdSetUserEvents error %d\n", retVal);
         bAbort = TRUE;
      }
   }

   // Stop interrupts and timer
   retVal = pd_clear_user_events(board, DigitalIn, eventsToNotify);
   if (!retVal)
   {
      rtl_printf("rtl_DIO_event: _PdClearUserEvents failed with error code %d\n", retVal);
      return NULL;
   }

   retVal = pd_adapter_enable_interrupt(board, 0);
   if (!retVal)
   {
      rtl_printf("rtl_DIO_event: _PdAdapterEnableInterrupt error %d\n", retVal);
      return NULL;
   } 

                          
   return NULL;
}


int init_module(void)
{    
   rtl_printf("Initializing RTL DIO event module\n");
   
   
   /*rtf_destroy(DATA_FIFO);
   rtf_destroy(COMMAND_FIFO);

   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // Create FIFO that will be used to receive data from the user space
   rtf_create(COMMAND_FIFO, 500);*/
   
   // Create thread that will perform the acquisition and writing of
   // data to the data FIFO
   bAbort = FALSE;
   pthread_create (&dio_thread,  NULL, dio_event_thread_proc, NULL);

   // Create thread that will receive and execute commands from the 
   // command FIFO
   //pthread_create (&cmd_thread,  NULL, CMD_thread, NULL);

   // Creates a handler for the command FIFO
   //rtf_create_handler(COMMAND_FIFO, &command_fifo_handler);
         
   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up DIO event RT module\n");

   //rtf_destroy(COMMAND_FIFO);
   //rtf_destroy(DATA_FIFO);

   bAbort = TRUE;
   pthread_cancel (dio_thread);
   pthread_join (dio_thread, NULL);
}




