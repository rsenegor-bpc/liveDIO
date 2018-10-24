/*****************************************************************************/
/*                    Buffered analog input example                          */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  acquisition under RT-Linux. The acquisition is performed in a hardware   */
/*  timed fashion that is appropriate for fast speed acquisition (> 1MHz).   */
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


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _bufferedAiData
{
   int board;                    // board number to be used for the AI operation
   unsigned short *rawBuffer;    // address of the buffer allocated by the driver to store
                                 // the raw binary data
   int nbOfChannels;             // number of channels
   int nbOfFrames;               // number of frames used in the asynchronous circular buffer
   int nbOfSamplesPerChannel;    // number of samples per channel
   int scanPeriod_us;            // sampling period on each channel in microseconds
   int polarity;                 // polarity of the signal to acquire, possible value
                                 // is AIN_UNIPOLAR or AIN_BIPOLAR
   int range;                    // range of the signal to acquire, possible value
                                 // is AIN_RANGE_5V or AIN_RANGE_10V
   int inputMode;                // input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL
   tState state;                 // state of the acquisition session
} tBufferedAiData;


pthread_t   ai_thread;
tBufferedAiData aiData;

int InitBufferedAI(tBufferedAiData *pAiData);
int BufferedAI(tBufferedAiData *pAiData);
void CleanUpBufferedAI(tBufferedAiData *pAiData);

int InitBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;

   pAiData->state = unconfigured;

   retVal = pd_ain_reset(pAiData->board);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_ain_reset error %d\n", retVal);
   }

   return retVal;
}


int BufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;
   DWORD divider;
   DWORD chanList[8]={0, 1, 2, 3, 4, 5, 6, 7};
   DWORD eventsToNotify = eFrameDone | eBufferDone | eTimeout | eBufferError | eStopped;
   DWORD event;
   DWORD aiCfg;
   DWORD scansAcquired = 0;
   TAinAsyncCfg asyncCfg;
   TScan_Info scanInfo;
   
   // setup the board to use hardware internal clock
   aiCfg = AIB_CVSTART0 | AIB_CVSTART1 | AIB_CLSTART0 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity; 
   
   retVal = pd_register_daq_buffer(pAiData->board, AnalogIn, pAiData->nbOfChannels, 
                                   pAiData->nbOfSamplesPerChannel, pAiData->nbOfFrames, 
                                   pAiData->rawBuffer, BUF_BUFFERRECYCLED | BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_register_daq_buffer error %d\n", retVal);
      return retVal;
   }

   // set clock divider, assuming that we use the 11MHz timebase
   divider = 11000 * (pAiData->scanPeriod_us / 1000)-1;
      
   asyncCfg.dwAInCfg = aiCfg;
   asyncCfg.dwAInPostTrigCount = 0;
   asyncCfg.dwAInPreTrigCount = 0;
   memcpy(asyncCfg.dwChList, chanList, pAiData->nbOfChannels * sizeof(uint32_t));
   asyncCfg.dwChListSize = pAiData->nbOfChannels;
   asyncCfg.dwClRate = divider;
   asyncCfg.dwCvRate = divider;
   asyncCfg.dwEventsNotify = eventsToNotify;
   asyncCfg.Subsystem = AnalogIn;
   retVal = pd_ain_async_init(pAiData->board, &asyncCfg);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_ain_async_init error %d\n", retVal);
      return retVal;
   }

   pAiData->state = configured;

   retVal = pd_set_user_events(pAiData->board, AnalogIn, eventsToNotify);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_set_user_events error %d\n", retVal);
      return retVal;
   }

   retVal = pd_ain_async_start(pAiData->board);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: PdAInAsyncStart error %d\n", retVal);
      return retVal;
   }

   pAiData->state = running;

   // Acquire 10 frames then exit
   while(scansAcquired < 10*pAiData->nbOfSamplesPerChannel)
   {
      event = pd_sleep_on_event(pAiData->board, AnalogIn, eventsToNotify, 5000);

      rtl_printf("BufferedAI: received event %d\n", event);

      retVal = pd_set_user_events(pAiData->board, AnalogIn, eventsToNotify);
      if (retVal < 0)
      {
         rtl_printf("BufferedAI: _PdSetUserEvents error %d\n", retVal);
         return retVal;
      }

      if (event & eTimeout)
      {
         rtl_printf("BufferedAI: wait timed out\n"); 
         return retVal;
      }

      if ((event & eBufferError) || (event & eStopped))
      {
         rtl_printf("BufferedAI: buffer error\n");
         return retVal;
      }

      if ((event & eBufferDone) || (event & eFrameDone))
      {
         scanInfo.ScanIndex = 0;
         scanInfo.NumValidScans = 0;
         scanInfo.Subsystem = AnalogIn;
         scanInfo.ScanRetMode = AIN_SCANRETMODE_RAW;
         scanInfo.NumScans = pAiData->nbOfSamplesPerChannel; 
         retVal = pd_ain_get_scans(pAiData->board, &scanInfo);
         if(retVal < 0)
         {
            rtl_printf("BufferedAI: buffer error\n");
            return retVal;
         }

         rtl_printf("BufferedAI: got %d scans at %d\n", scanInfo.NumValidScans, scanInfo.ScanIndex);
         scansAcquired = scansAcquired + scanInfo.NumValidScans;
      }
   } 
   return retVal;
}


void CleanUpBufferedAI(tBufferedAiData *pAiData)
{
   int retVal = 0;
      
   if(pAiData->state == running)
   {
      retVal = pd_ain_async_stop(pAiData->board);
      if (retVal < 0)
         rtl_printf("BufferedAI: pd_ain_async_stop error %d\n", retVal);

      pAiData->state = configured;
   }

   if(pAiData->state == configured)
   {
      retVal = pd_clear_user_events(pAiData->board, AnalogIn, eAllEvents);
      if (retVal < 0)
         rtl_printf("BufferedAI: pd_clear_user_events error %d\n", retVal);
   
      retVal = pd_ain_async_term(pAiData->board);  
      if (retVal < 0)
         rtl_printf("BufferedAI: pd_ain_async_term error %d\n", retVal);
   
      retVal = pd_unregister_daq_buffer(pAiData->board, AnalogIn);
      if (retVal < 0)
         rtl_printf("BufferedAI: pd_unregister_daq_buffer error %d\n", retVal);

      pAiData->state = unconfigured;
   }

   pAiData->state = closed;
}


void *ai_thread_proc(void *param)
{
   int ret;
   char board_name[256];
   char serial[32];
         
   //pthread_make_periodic_np(pthread_self(),gethrtime(), AI_PERIOD_NS);

   aiData.board = 0;
   aiData.nbOfChannels = 1;
   aiData.nbOfFrames = 16;
   aiData.rawBuffer = NULL;
   aiData.nbOfSamplesPerChannel = 500;
   aiData.scanPeriod_us = 1000; // 1000Hz = 1/1000us
   aiData.polarity = AIN_BIPOLAR;
   aiData.range = AIN_RANGE_10V;
   aiData.inputMode = AIN_SINGLE_ENDED;
   aiData.state = closed;

   // enumerate boards
   if(pd_enum_devices(aiData.board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", aiData.board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", aiData.board, board_name, serial);

   // Allocate memory to acquire in
   aiData.rawBuffer = (unsigned short*) kmalloc(aiData.nbOfChannels * aiData.nbOfSamplesPerChannel *
                                                sizeof(unsigned short), GFP_KERNEL);
   if(aiData.rawBuffer == NULL)
   {
      rtl_printf("Error: could not allocate enough memory for the acquisition buffer\n");
      return NULL;
   }

   ret = InitBufferedAI(&aiData);
   if(ret < 0)
   {
      rtl_printf("Error: InitBufferedAI returned %d\n", ret);
      return NULL;
   }

   ret = BufferedAI(&aiData);
   if(ret < 0)
   {
      rtl_printf("Error: InitBufferedAI returned %d\n", ret);
      return NULL;
   }

   // send data to FIFO

   CleanUpBufferedAI(&aiData);
   
   kfree(aiData.rawBuffer);

   return NULL;
}


int init_module(void)
{    
   rtl_printf("Initializing RTL buffered AI module\n");
   
   
   /*rtf_destroy(DATA_FIFO);
   rtf_destroy(COMMAND_FIFO);

   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // Create FIFO that will be used to receive data from the user space
   rtf_create(COMMAND_FIFO, 500);*/
   
   // Create thread that will perform the acquisition and writing of
   // data to the data FIFO
   pthread_create (&ai_thread,  NULL, ai_thread_proc, NULL);

   // Create thread that will receive and execute commands from the 
   // command FIFO
   //pthread_create (&cmd_thread,  NULL, CMD_thread, NULL);

   // Creates a handler for the command FIFO
   //rtf_create_handler(COMMAND_FIFO, &command_fifo_handler);
         
   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up AIRead_RT module\n");

   // Clean-up data acquisition if needed
   if(aiData.state != closed)
      CleanUpBufferedAI(&aiData);

   //rtf_destroy(COMMAND_FIFO);
   //rtf_destroy(DATA_FIFO);

   pthread_cancel (ai_thread);
   pthread_join (ai_thread, NULL);
}




