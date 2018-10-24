/******************************************************************************/
/*              EXternal scan clock example using counter/timer               */
/*                                                                            */
/*  This example shows how to use the powerdaq API to perform an Analog Input */
/*  data acquisition using one of the counter/timer to generate the scan      */
/*  clock and interrupt after each scan.                                      */
/*  It will only work for PD-MFx and PD2-MFx boards.                          */
/*  The signal CTRx_OUT must be connected to CL_CLK_IN.                       */
/*                                                                            */
/*  This example can work with its own interrupt handler or with the one      */
/*  installed by default by the powerdaq driver.                              */
/*  -To use custom irq handler:                                               */
/*      load the driver with the following option: insmod pwrdaq.o rqstirq=0  */
/*      Uncomment the line "#define CUSTOM_IRQ_HANDLER 1" in this source file */
/*  -To use the driver's irq handler:                                         */
/*      Comment out the line "#define CUSTOM_IRQ_HANDLER 1"                   */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*      Copyright (C) 2003 United Electronic Industries, Inc.                 */
/*      All rights reserved.                                                  */
/*----------------------------------------------------------------------------*/
/*                                                                            */ 
/******************************************************************************/


#include <pthread.h>
#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl.h>
#include <time.h>
#include <linux/slab.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#include "scan_uct.h"

//#define CUSTOM_IRQ_HANDLER

#define DATA_FIFO_ID       0      // rt-fifo on which to put newframe flag
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags


// Externs from the library
extern pd_board_t pd_board[PD_MAX_BOARDS];

typedef struct PD_ANALOG_IN_STRUCT
{
   DWORD Board;
   DWORD TimeoutMs;
   DWORD Mode;
   DWORD Type;
   DWORD Range;
   DWORD Gain;
   DWORD NumChannels;
   DWORD SampleRate;
} tAiData;

int pd_error, ai_running;
int intid;
pthread_t ai_thread;

#define BUFFER_SIZE	640000
#define CL_SIZE 64
unsigned short Buffer[BUFFER_SIZE];
unsigned short* pBuf;      // pointer to current write position in the buffer
unsigned int sampleCount;
DWORD eventsToNotify = eUct0Event;
    
tAiData AiData;
tUctGenSquareWaveData uctData;

int AinInit(tAiData *pAiData)
{
    DWORD Cfg;
    DWORD ChList[CL_SIZE];
    unsigned int i;
    int retVal;

   // set config word
   Cfg = pAiData->Mode | (pAiData->Type<<1) | (pAiData->Range<<2)
   | AIB_CVSTART0 | AIB_CLSTART1;  // Set CV internal and CL external
   pd_ain_set_config(pAiData->Board, Cfg, 0, 0);
   
   pd_ain_set_cv_clock(pAiData->Board, 109);
   
   // set channel list 
   for (i = 0; i < pAiData->NumChannels; i++) 
       ChList[i] = i | (pAiData->Gain<<6);
   pd_ain_set_channel_list(pAiData->Board, pAiData->NumChannels, &ChList[0]);

   // set event to interrupt when a scan is finished
#ifdef CUSTOM_IRQ_HANDLER
   // enable events 
   retVal = pd_adapter_set_board_event1(pAiData->Board, UTB_Uct0Im);
   if(!retVal)
   {
      rtl_printf("scan_int: pd_adapter_set_board_event1 failed with code %x\n", retVal);
      return retVal;
   }
#else
   // set events we want to receive
   retVal = pd_set_user_events(pAiData->Board, CounterTimer, eventsToNotify);
   if(!retVal)
   {
      rtl_printf("scan_int: _PdSetUserEvents failed with code %x\n", retVal);
      return retVal;
   }
#endif
   // enable interrupts
   pd_adapter_enable_interrupt(pAiData->Board,1);
  
   // reset the sample counter
   sampleCount = 0;
     
   // start conversion
   pd_ain_set_enable_conversion(pAiData->Board,1);
   pd_ain_sw_start_trigger(pAiData->Board);
      
   return 0;
}

int AoutInit(int board_num)
{
    DWORD aoCfg = 0;
    
    pd_aout_reset(board_num);
    pd_aout_set_config(board_num, aoCfg, 0);
    pd_aout_sw_start_trigger(board_num);
       
    return 0;
}

// ISR, only used if CUSTOM_IRQ_HANDLER is defined.
unsigned int uct_isr(unsigned int trigger_irq,struct pt_regs *regs) 
{
    int ret;
    int board = AiData.Board;
    TEvents Events;
    
    if (pd_dsp_int_status(board)) 
    { 
        // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        // check is FHF?
        if (Events.ADUIntr & UTB_Uct0IntrSC) 
        {
            // Notify waiting thread that the event occured
            pthread_wakeup_np(ai_thread);
        }
    }

    return 0;
}


int AnInEvents(tAiData *pAiData)
{
    TEvents Events;
    unsigned int samples;
    int ret;


    // get board status
    pd_adapter_get_board_status(pAiData->Board, &Events);
    
    printk("Got event 0x%x\n", Events.ADUIntr);

    // save board status now... or whatever you want to do
    if (Events.ADUIntr & UTB_Uct0IntrSC)
    {     
        // Get data in the buffer, if the buffer overflows, write data to the last position
        if(sampleCount <= (BUFFER_SIZE - AiData.NumChannels)) 
        		pBuf = Buffer + sampleCount;
        
        ret = pd_ain_get_samples(pAiData->Board, pAiData->NumChannels<<1, pBuf, &samples);
        if(samples > 0)
        {
            // Do processing here  
            sampleCount = sampleCount + samples;
            
            // output some data
            pd_aout_put_value(pAiData->Board, 0x7f);
        }
    }
    
    return samples;
}


void AInTerminate()
{
   printk("Acquisition stopped\n");
   ai_running = 0;
   pd_ain_reset(AiData.Board);
   pd_adapter_set_board_event1(AiData.Board, UTB_Uct0IntrSC);
   pd_adapter_enable_interrupt(AiData.Board, 0);
   
   // Kill the isr thread
   pthread_cancel(ai_thread);
   pthread_join(ai_thread, NULL);
}

void* ai_thread_proc(void*  arg )
{
   int ret;
   tAiData *pAiData = (tAiData*)arg;
   DWORD events;

#ifdef CUSTOM_IRQ_HANDLER
   // get hold on interrupt vector
   if(rtl_request_irq(pd_board[pAiData->Board].irq, uct_isr))
   {
      printk("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return NULL;
   }

   // enable it
   rtl_hard_enable_irq(pd_board[pAiData->Board].irq);
#endif
 
   printk("Acquisition is started\n");    
	
   while(ai_running)
   {
       // Wait for a timer interrupt that signals us that a scan
       // has been acquired
#ifdef CUSTOM_IRQ_HANDLER
       pthread_suspend_np(pthread_self());
       if(!ai_running)
          break;

       events = eUct0Event;
#else
       events = pd_sleep_on_event(pAiData->Board, CounterTimer, eventsToNotify, pAiData->TimeoutMs);
#endif

       printk("Got IRQ\n");

       if (events & eTimeout)
       {
          rtl_printf("wait timed out\n"); 
          return NULL;
       }

       if (events & eUct0Event) 
       {
	   printk("Got UCT event\n");
           ret = AnInEvents(pAiData);
       }
      
       // reenable events/interrupts
#ifdef CUSTOM_IRQ_HANDLER 
       pd_adapter_set_board_event1(pAiData->Board, UTB_Uct0Im);

       // reenable irq handling
       pd_adapter_enable_interrupt(pAiData->Board, 1);
       rtl_hard_enable_irq(pd_board[pAiData->Board].irq);
#else
       pd_set_user_events(pAiData->Board, CounterTimer, events);
#endif
    }

    pthread_exit(NULL); 
  
    return NULL;
}



int init_module()
{
   int ret;
   char board_name[256];
   char serial[32];
   
   // use board 0
   AiData.Board = 0;
   AiData.TimeoutMs = 5000;
   AiData.Mode = 0;
   AiData.Type = 1;
   AiData.Range = 1;	 
   AiData.Gain = 0;	 
   AiData.NumChannels = CL_SIZE;

	
   // enumerate boards
   if(pd_enum_devices(AiData.Board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", AiData.Board);
      return -ENXIO;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", AiData.Board, board_name, serial);
    
     
   // Initializes counter/timer  
   uctData.board = AiData.Board;
   uctData.counter = 0;
   uctData.clockSource = clkInternal1MHz;
   uctData.gateSource = gateSoftware;
   uctData.mode = SquareWave;
   uctData.state = closed;
    
   InitUctGenSquareWave(&uctData);
   StartUctGenSquareWave(&uctData, 1000.0);  
       
   // Initializes and starts AI
   AinInit(&AiData);  
   AoutInit(AiData.Board);
   ai_running = 1;                                  
   
   // create fifo we can talk via /dev/rtf0
   rtf_destroy(DATA_FIFO_ID);     // free up the resource, just in case
   rtf_create(DATA_FIFO_ID, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device


   // Create thread that will perform the acquisition and reading of
   // data from the data FIFO
   ret = pthread_create (&ai_thread,  NULL, ai_thread_proc, &AiData);
   
#ifdef CUSTOM_IRQ_HANDLER
   // enable it
   rtl_hard_enable_irq(pd_board[AiData.Board].irq);
#endif

   return 0;
}


void cleanup_module()
{
   AInTerminate();
	
   printk("Acquired %d samples.\n", sampleCount);
    
   StopUctGenSquareWave(&uctData);
   CleanUpUctGenSquareWave(&uctData);

   rtf_destroy(DATA_FIFO_ID);

#ifdef CUSTOM_IRQ_HANDLER
   rtl_free_irq(pd_board[AiData.Board].irq);  // release IRQ
#endif
}

