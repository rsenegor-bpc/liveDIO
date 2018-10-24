/*****************************************************************************/
/*                 Interrupt driven analog output example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a waveform     */
/*  generation under RT-Linux. The generation is performed in a hardware     */
/*  timed fashion that is appropriate for fast speed generation (> 1MHz).    */
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
/*      Copyright (C) 2003 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*  author: Frederic Villeneuve                                              */
/*                                                                           */ 
/*****************************************************************************/

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

// Comment-out the following line to use the powerdaq driver built-in interrupt handler
// Uncomment it to install your own interrupt handler                            
//#define CUSTOM_IRQ_HANDLER 1

typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _bufferedAoData
{
   int board;                    // board number to be used for the AI operation
   unsigned int *rawBuffer;      // address of the buffer allocated by the driver to store
                                 // the raw binary data
   int isAoBoard;                // adapter type
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   int nbOfFrames;
   int halfFIFOSize;             // Half size of the DAC FIFO
   int updatePeriod_us;          // update period in nanoseconds
   tState state;                 // state of the acquisition session

   int abort;                    // Signal the thread to exit

   pthread_cond_t   event;       // condition used to signal the thread that an 
   pthread_mutex_t  event_lock;  // interrupt occured.
} tBufferedAoData;


#define DATA_FIFO_ID       0      // rt-fifo on which to put newframe flag
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags

pthread_t   ao_thread;
tBufferedAoData aoData;
DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError;
DWORD dwOffset;
//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];


int StartBufferedAO(tBufferedAoData *pAoData);
int WaitBufferedAO(tBufferedAoData *pAoData, int timeoutms);
void CleanUpBufferedAO(tBufferedAoData *pAoData);

int StartBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;
   DWORD aoCfg;
   DWORD cvdivider;
   DWORD dwCount;

   rtl_printf("entering StartBufferedAO\n");
   
   pAoData->state = unconfigured;

   retVal = pd_aout_reset(pAoData->board);
   if (!retVal)
   {
      rtl_printf("BufferedAI: pd_aout_reset error %d\n", retVal);
      return retVal;
   }

   // need also to call this function if the board is a PD2-AO-xx
   if(pAoData->isAoBoard)
   {
      retVal = pd_ao32_reset(pAoData->board);
      if (!retVal)
      {
         rtl_printf("BufferedAO: PdAO32Reset error %d\n", retVal);
         return retVal;
      }
   }

   // setup the board to use hardware internal clock
   if(pAoData->isAoBoard)
      aoCfg = AOB_CVSTART0 | AOB_AOUT32;  // 11 MHz internal timebase
   else
      aoCfg = AOB_CVSTART0;

   retVal = pd_aout_set_config(pAoData->board, aoCfg, 0);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdAOutSetCfg failed with code %x\n", retVal);
      return retVal;
   }

   // set 11 Mhz clock divisor
   cvdivider = 11 * pAoData->updatePeriod_us - 1;
   rtl_printf("rtl_int_ao: CV divider = %d\n", cvdivider);

   retVal = pd_aout_set_cv_clk(pAoData->board, cvdivider);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdAOutSetCvClk failed with code %x\n", retVal);
      return retVal;
   }

#ifdef CUSTOM_IRQ_HANDLER
   // enable events 
   retVal = pd_aout_set_events(pAoData->board, AOB_HalfDoneIm | AOB_BufDoneIm);
   if(!retVal)
   {
      rtl_printf("BufferedAO: pd_aout_set_events failed with code %x\n", retVal);
      return retVal;
   }
#else
   // set events we want to receive
   retVal = pd_set_user_events(pAoData->board, AnalogOut, eventsToNotify);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdSetUserEvents failed with code %x\n", retVal);
      return retVal;
   }
#endif

   // event suff
   retVal = pd_adapter_enable_interrupt(pAoData->board, TRUE);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdAdapterEnableInterrupt failed with code %x\n", retVal);
      return retVal;
   }

   pAoData->state = configured;

   // Fill the FIFO with the first block of data
   retVal = pd_aout_put_block(pAoData->board, pAoData->halfFIFOSize * 2, 
                             (DWORD*)pAoData->rawBuffer, &dwCount);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdAOutPutBlock failed with code %x\n", retVal);
      return retVal;
   }
   
   dwOffset = dwOffset + dwCount;


   retVal = pd_aout_set_enable_conversion(pAoData->board, TRUE);
   if (!retVal)
   {
      rtl_printf("BufferedAO: _PdAOutEnableConv error %d\n", retVal);
      return retVal;
   }

   // Start SW trigger
   retVal = pd_aout_sw_start_trigger(pAoData->board);
   if (!retVal)
   {
      rtl_printf("BufferedAO: PdAOutSwStartTrig failed with %d.\n", retVal);
      return retVal;
   }

   pAoData->state = running;

   rtl_printf("Leaving StartBufferedAO\n");

   return retVal;
}


int WaitBufferedAO(tBufferedAoData *pAoData, int timeoutms)
{
   int retVal = 0;
   DWORD events;
   DWORD dwCount;

   rtl_printf("Entering WaitBufferedAO\n");
      
#ifdef CUSTOM_IRQ_HANDLER
   struct timespec timeout;

   // Get the lock, pthrea_cond_wait will unlock it while waiting and
   // relock it when it gets signaled
   retVal = pthread_mutex_lock(&pAoData->event_lock);
   if (retVal != 0)
   {
       rtl_printf("WaitBufferedAO: Could not acquire event lock\n");
       return retVal;
   }

   // Set a timeout of 5 sec from now for waiting for the answer
   timeout.tv_sec =  time(0) + timeoutms/1000;
   timeout.tv_nsec = (timeoutms % 1000) + 1000000;

   retVal = pthread_cond_timedwait(&pAoData->event, &pAoData->event_lock, &timeout);
   if (retVal != 0)
   {
      if (retVal == ETIMEDOUT)
      {
         rtl_printf("Timeout\n");
         events = eTimeout;
      } 
      else
      {
         rtl_printf("Unexpected error\n");
         return 0;
      }
   }
   else
      events = eFrameDone;
   
   pthread_mutex_unlock(&pAoData->event_lock);
#else
   events = pd_sleep_on_event(pAoData->board, AnalogOut, eventsToNotify, timeoutms);
#endif

   rtl_printf("BufferedAO: received event 0x%x\n", events);

      
   if (events & eTimeout)
   {
      rtl_printf("BufferedAO: wait timed out\n"); 
      return 0;
   }

   if ((events & eBufferError) || (events & eStopped))
   {
      rtl_printf("BufferedAO: buffer error\n");
      return 0;
   }

   if ((events & eBufferDone) || (events & eFrameDone))
   {
      // Write block of data into DSP FIFO
      retVal = pd_aout_put_block(pAoData->board, pAoData->halfFIFOSize, 
                               (DWORD*)&pAoData->rawBuffer[dwOffset], &dwCount);
      if(!retVal)
      {
         rtl_printf("BufferedAO: _PdAOutPutBlock failed with code %x\n", retVal);
         return retVal;
      }

      // Add number of points have being successfully writter to DSP FIFO
      dwOffset = dwOffset + dwCount;

      // trigger output
      retVal = pd_aout_set_enable_conversion(pAoData->board, TRUE);
      if(!retVal)
      {
         rtl_printf("BufferedAO: _PdAOutEnableConv failed with code %x\n", retVal);
         return retVal;
      }
      
      // trigger output
      retVal = pd_aout_sw_start_trigger(pAoData->board);
      if(!retVal)
      {
         rtl_printf("BufferedAO: _PdAOutSwStartTrig failed with code %x\n", retVal);
         return retVal;
      }

      // Wrap waveform buffer around
      if ((dwOffset+pAoData->halfFIFOSize) >= (pAoData->nbOfChannels * pAoData->nbOfPointsPerChannel)) 
         dwOffset = 0;

      rtl_printf("count = %d offset = %d\n", dwCount, dwOffset); 
   }

   // reenable events/interrupts
#ifdef CUSTOM_IRQ_HANDLER 
   pd_aout_set_events(pAoData->board, AOB_HalfDoneIm | AOB_BufDoneIm);

   // reenable irq handling
   pd_adapter_enable_interrupt(pAoData->board, 1);
   rtl_hard_enable_irq(pd_board[pAoData->board].irq);
#else
   retVal = pd_set_user_events(pAoData->board, AnalogOut, events);
   if (!retVal)
   {
      rtl_printf("BufferedAO: _PdSetUserEvents error %d\n", retVal);
      return retVal;
   }
#endif

   rtl_printf("Leaving waitBufferedAO\n");

   return retVal;
}


void CleanUpBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;    

   rtl_printf("Entering CleanupBufferedAO\n");
      
   if(pAoData->state == running)
   {
      // Stop and disable AOut conversions
      retVal = pd_aout_sw_stop_trigger(pAoData->board);
      if (!retVal)
         rtl_printf("BufferedAO: _PdAOutSwStopTrig error %d\n", retVal);

      retVal = pd_aout_set_enable_conversion(pAoData->board, FALSE);
      if (!retVal)
         rtl_printf("BufferedAO: _PdAOutEnableConv error %d\n", retVal);

      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
#ifndef CUSTOM_IRQ_HANDLER
      retVal = pd_clear_user_events(pAoData->board, AnalogOut, eAllEvents);
      if (!retVal)
         rtl_printf("BufferedAO: PdClearUserEvents error %d\n", retVal);
#endif
      
      retVal = pd_adapter_enable_interrupt(pAoData->board, 0);
      if (!retVal)
         rtl_printf("BufferedAO: pd_adapter_enable_interrupt error %d\n", retVal);

      retVal = pd_aout_reset(pAoData->board);
      if (!retVal)
         rtl_printf("BufferedAO: PdAOutReset error %d\n", retVal);
               
      // need also to call this function if the board is a PD2-AO-xx
      if(pAoData->isAoBoard)
      {
         retVal = pd_ao32_reset(pAoData->board);
         if (!retVal)
            rtl_printf("BufferedAO: PdAAO32Reset error %d\n", retVal);
      }

      pAoData->state = unconfigured;
   }

   pAoData->state = closed;

   rtl_printf("Leaving CleanupBufferedAO\n");
}


// ISR, only used if CUSTOM_IRQ_HANDLER is defined.
unsigned int pd_isr(unsigned int trigger_irq,struct pt_regs *regs) 
{
    int ret;
    int board = aoData.board;
    TEvents Events;
    
    if (pd_dsp_int_status(board)) 
    { // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        // check is FHF?
        if ((Events.AOutIntr & AOB_HalfDoneSC) || (Events.AOutIntr & AOB_HalfDoneSC)) 
        {
            // Notify waiting thread that the event occured
            pthread_cond_signal(&aoData.event);
        }
    }

    return 0;
}


void *ao_thread_proc(void *param)
{
   int ret, i, j;
   int bHigh = FALSE;
         

   // Allocate memory to acquire in
   aoData.rawBuffer = (unsigned int*) kmalloc(aoData.nbOfChannels * aoData.nbOfPointsPerChannel *
                                                sizeof(unsigned int), GFP_KERNEL);
   if(aoData.rawBuffer == NULL)
   {
      rtl_printf("Error: could not allocate enough memory for the generation buffer\n");
      return NULL;
   }

   // Fill buffer with square wave
   for(i=0; i<aoData.nbOfPointsPerChannel; i++)
   {
      for(j=0; j<aoData.nbOfChannels; j++)
      {
         // switch state every 40 updates
         if(i % 40)
            bHigh = !bHigh;
            
         if(bHigh)  
            aoData.rawBuffer[i*aoData.nbOfChannels + j] = 0xFFF;
         else
            aoData.rawBuffer[i*aoData.nbOfChannels + j] = 0x0;
      }
   }

   // Special case for MFx boards where data for channel 0 and 1
   // must be stored on the same DWORD
   if((aoData.nbOfChannels > 1) && (!aoData.isAoBoard))
   {
      for(i=0; i<aoData.nbOfPointsPerChannel;i++)
         aoData.rawBuffer[i] = aoData.rawBuffer[i*2] | (aoData.rawBuffer[i*2+1] << 12);

      // now for MFx boards everything will look like we have only one channel
      aoData.nbOfChannels = 1;
   }

   // For PD2-AO32 boards, we need to add the channel number to the upper 
   // 16 bits of each value
   if((aoData.nbOfChannels > 1) && aoData.isAoBoard)
   {
      for(i=0; i<aoData.nbOfPointsPerChannel;i++)
      {
         for(j=0; j<aoData.nbOfChannels; j++)
            aoData.rawBuffer[i*aoData.nbOfChannels+j] = 
               (aoData.rawBuffer[i*aoData.nbOfChannels+j] & 0xFFFF) | (j << 16);
      }
   }


   ret = StartBufferedAO(&aoData);
   if(!ret)
   {
      rtl_printf("Error: StartBufferedAO returned %d\n", ret);
      return NULL;
   }

   while(!aoData.abort)
   {
      ret = WaitBufferedAO(&aoData, 5000);
      if(!ret)
      {
         rtl_printf("Error: BufferedAOEvent returned %d\n", ret);
         return NULL;
      }

      rtl_printf("WaitbufferedAO returned %d\n", ret);
   }

   CleanUpBufferedAO(&aoData);
   
   kfree(aoData.rawBuffer);

   return NULL;
}


int data_fifo_handler(unsigned int fifo_id)
{
   return 0;
}


int init_module(void)
{
   char board_name[256];
   char serial[32];
   int ret;

   rtl_printf("Initializing RTL interrupt-driven AO module\n");

   aoData.board = 0;
   aoData.abort = FALSE;
   aoData.nbOfChannels = 2;
   aoData.nbOfFrames = 16;
   aoData.rawBuffer = NULL;
   aoData.nbOfPointsPerChannel = 10000;
   aoData.updatePeriod_us = 1000; // 1000Hz = 1/1000 us
   aoData.isAoBoard = 0;
   aoData.state = closed;
   aoData.halfFIFOSize = 1024;

   // enumerate boards
   if(pd_enum_devices(aoData.board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", aoData.board);
      return -ENXIO;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", aoData.board, board_name, serial);

   
#ifdef CUSTOM_IRQ_HANDLER
   // Initializes the conditional variable used by the subsystem to notify
   // events from the IRQ handler
   ret = pthread_cond_init(&aoData.event, NULL);
   if (ret != 0)
   {
      rtl_printf("Error %d, can't create conditional variable\n", ret);
      return ret;
   }

   // Initializes the mutex that protects access to the condition
   ret = pthread_mutex_init(&aoData.event_lock, NULL);
   if (ret != 0)
   {
      rtl_printf("Error %d, can't create mutex\n", ret);
      return ret;;
   }

   // get hold on interrupt vector
   if(rtl_request_irq(pd_board[aoData.board].irq, pd_isr))
   {
      printk("rtl_request_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
      return -EIO;
   }
#endif
   
   // create fifo we can talk via /dev/rtf0
   rtf_destroy(DATA_FIFO_ID);     // free up the resource, just in case
   rtf_create(DATA_FIFO_ID, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device


   // Create thread that will perform the acquisition and reading of
   // data from the data FIFO
   ret = pthread_create (&ao_thread,  NULL, ao_thread_proc, NULL);

   
   // Creates a handler for the data FIFO
   rtf_create_handler(DATA_FIFO_ID, &data_fifo_handler);

#ifdef CUSTOM_IRQ_HANDLER
   // enable it
   rtl_hard_enable_irq(pd_board[aoData.board].irq);
#endif

   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up RTL interrupt-driven AO module\n");

   aoData.abort = TRUE;

   // wait for thread to finish
   pthread_join (ao_thread, NULL);

   // Clean-up data acquisition if needed
   if(aoData.state != closed)
      CleanUpBufferedAO(&aoData);

   rtf_destroy(DATA_FIFO_ID);

#ifdef CUSTOM_IRQ_HANDLER
   rtl_free_irq(pd_board[aoData.board].irq);  // release IRQ

   pthread_cond_destroy(&aoData.event);
   pthread_mutex_destroy(&aoData.event_lock);
#endif
}




