/*****************************************************************************/
/*                    Buffered analog output example                         */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a buffered     */
/*  waveform generation under RT-Linux. The generation is performed in a     */
/*  hardware timed fashion that is appropriate for fast speed acquisition    */
/*  (> 1MHz).                                                                */
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

typedef struct _bufferedAoData
{
   int board;                    // board number to be used for the AI operation
   unsigned int *rawBuffer;      // address of the buffer allocated by the driver to store
                                 // the raw binary data
   int isAoBoard;                // adapter type, shoul be atMF or atPD2AO
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   int nbOfFrames;
   int halfFIFOSize;             // Half size of the DAC FIFO
   int updatePeriod_us;          // update period in nanoseconds
   tState state;                 // state of the acquisition session
} tBufferedAoData;


pthread_t   ao_thread;
tBufferedAoData aoData;

int InitBufferedAO(tBufferedAoData *pAoData);
int BufferedAO(tBufferedAoData *pAoData);
void CleanUpBufferedAO(tBufferedAoData *pAoData);

int InitBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;

   pAoData->state = unconfigured;

   retVal = pd_aout_reset(pAoData->board);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_aout_reset error %d\n", retVal);
      return retVal;
   }

   // need also to call this function if the board is a PD2-AO-xx
   if(pAoData->isAoBoard)
   {
      retVal = pd_ao32_reset(pAoData->board);
      if (retVal < 0)
      {
         rtl_printf("BufferedAO: PdAAO32Reset error %d\n", retVal);
      }
   }

   return retVal;
}


int BufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;
   DWORD aoCfg;
   DWORD cvdivider;
   DWORD chanList[8]={0, 1, 2, 3, 4, 5, 6, 7};
   DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError | eStopped;
   DWORD events;
   int updatesGenerated = 0;
   int i;
   TAinAsyncCfg asyncCfg;
   TScan_Info scanInfo;
   
   // setup the board to use hardware internal clock
   if(pAoData->isAoBoard)
      aoCfg = AOB_CVSTART0 | AOB_AOUT32;  // 11 MHz internal timebase
   else
      aoCfg = AOB_CVSTART0;
   
   retVal = pd_register_daq_buffer(pAoData->board, AnalogOut, pAoData->nbOfChannels, 
                                   pAoData->nbOfPointsPerChannel, pAoData->nbOfFrames, 
                                   (unsigned short*)pAoData->rawBuffer, BUF_DWORDVALUES |
                                   BUF_BUFFERRECYCLED | BUF_BUFFERWRAPPED);
   if (retVal < 0)
   {
      rtl_printf("BufferedAO: pd_register_daq_buffer error %d\n", retVal);
      return retVal;
   }

   // set clock divider, assuming that we use the 11MHz timebase
   cvdivider = 11000 * (pAoData->updatePeriod_us / 1000)-1;
   rtl_printf("rtl_buf_ao: CV divider = %d\n", cvdivider);

   asyncCfg.dwAInCfg = aoCfg;
   for(i=0; i<pAoData->nbOfChannels; i++)
      asyncCfg.dwChList[i] = chanList[i];
   asyncCfg.dwChListSize = pAoData->nbOfChannels;
   asyncCfg.dwClRate = cvdivider;
   asyncCfg.dwCvRate = cvdivider;
   asyncCfg.dwEventsNotify = eventsToNotify;
   asyncCfg.Subsystem = AnalogOut;

   retVal = pd_aout_async_init(pAoData->board, &asyncCfg);
   if (retVal < 0)
   {
      rtl_printf("BufferedAO: pd_aout_async_init error %d\n", retVal);
      return retVal;
   }

   pAoData->state = configured;

   retVal = pd_set_user_events(pAoData->board, AnalogOut, eventsToNotify);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_set_user_events error %d\n", retVal);
      return retVal;
   }

   retVal = pd_aout_async_start(pAoData->board);
   if (retVal < 0)
   {
      rtl_printf("BufferedAI: pd_aout_async_start error %d\n", retVal);
      return retVal;
   }

   pAoData->state = running;

   // Generates 10 frames and stop
   while(updatesGenerated < 10*pAoData->nbOfPointsPerChannel)
   {
      events = pd_sleep_on_event(pAoData->board, AnalogOut, eventsToNotify, 5000);

      rtl_printf("BufferedAO: received event 0x%x\n", events);

      retVal = pd_set_user_events(pAoData->board, AnalogOut, eventsToNotify);
      if (retVal < 0)
      {
         rtl_printf("BufferedAO: _PdSetUserEvents error %d\n", retVal);
         return retVal;
      }

      if (events & eTimeout)
      {
         rtl_printf("BufferedAO: wait timed out\n"); 
         return retVal;
      }

      if ((events & eBufferError) || (events & eStopped))
      {
         rtl_printf("BufferedAO: buffer error\n");
         return retVal;
      }

      if ((events & eBufferDone) || (events & eFrameDone))
      {
         scanInfo.ScanIndex = 0;
         scanInfo.NumValidScans = 0;
         scanInfo.Subsystem = AnalogOut;
         scanInfo.ScanRetMode = AIN_SCANRETMODE_RAW;
         scanInfo.NumScans = pAoData->nbOfPointsPerChannel; 
         retVal = pd_aout_get_scans(pAoData->board, &scanInfo);
         if(retVal < 0)
         {
            rtl_printf("BufferedAO: buffer error\n");
            return retVal;
         }

         rtl_printf("BufferedAO: generated %d updates at %d\n", scanInfo.NumValidScans, scanInfo.ScanIndex);
         updatesGenerated = updatesGenerated + scanInfo.NumValidScans;
      }
   } 
   return retVal;
}


void CleanUpBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;
      
   if(pAoData->state == running)
   {
      retVal = pd_aout_async_stop(pAoData->board);
      if (retVal < 0)
         rtl_printf("BufferedAO: pd_ain_async_stop error %d\n", retVal);

      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
      retVal = pd_clear_user_events(pAoData->board, AnalogOut, eAllEvents);
      if (retVal < 0)
         rtl_printf("BufferedAO: pd_clear_user_events error %d\n", retVal);
   
      retVal = pd_aout_async_term(pAoData->board);  
      if (retVal < 0)
         rtl_printf("BufferedAO: pd_ain_async_term error %d\n", retVal);
   
      retVal = pd_unregister_daq_buffer(pAoData->board, AnalogOut);
      if (retVal < 0)
         rtl_printf("BufferedAO: pd_unregister_daq_buffer error %d\n", retVal);

      pAoData->state = unconfigured;
   } 

   pAoData->state = closed;
}


void *ao_thread_proc(void *param)
{
   int ret, i, j;
   char board_name[256];
   char serial[32];
   int bHigh = FALSE;
         
   //pthread_make_periodic_np(pthread_self(),gethrtime(), AI_PERIOD_NS);

   aoData.board = 0;
   aoData.nbOfChannels = 2;
   aoData.nbOfFrames = 16;
   aoData.rawBuffer = NULL;
   aoData.nbOfPointsPerChannel = 500;
   aoData.updatePeriod_us = 1000; // 1000Hz = 1/1000us
   aoData.isAoBoard = 0;
   aoData.state = closed;

   // enumerate boards
   if(pd_enum_devices(aoData.board, board_name, sizeof(board_name), 
                      serial, sizeof(serial))<0)
   {
      rtl_printf("Error: Could not find board %d\n", aoData.board);
      return NULL;
   }

   rtl_printf("Found board %d: %s, s/n %s\n", aoData.board, board_name, serial);

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

   ret = InitBufferedAO(&aoData);
   if(ret < 0)
   {
      rtl_printf("Error: InitBufferedAO returned %d\n", ret);
      return NULL;
   }

   ret = BufferedAO(&aoData);
   if(ret < 0)
   {
      rtl_printf("Error: BufferedAO returned %d\n", ret);
      return NULL;
   }

   CleanUpBufferedAO(&aoData);
   
   kfree(aoData.rawBuffer);

   return NULL;
}


int init_module(void)
{    
   rtl_printf("Initializing RTL buffered AO module\n");
   
   
   /*rtf_destroy(DATA_FIFO);
   rtf_destroy(COMMAND_FIFO);

   // Create FIFO that will be used to send acquired data to user space
   rtf_create(DATA_FIFO, 4000);

   // Create FIFO that will be used to receive data from the user space
   rtf_create(COMMAND_FIFO, 500);*/
   
   // Create thread that will perform the acquisition and writing of
   // data to the data FIFO
   pthread_create (&ao_thread,  NULL, ao_thread_proc, NULL);

   // Create thread that will receive and execute commands from the 
   // command FIFO
   //pthread_create (&cmd_thread,  NULL, CMD_thread, NULL);

   // Creates a handler for the command FIFO
   //rtf_create_handler(COMMAND_FIFO, &command_fifo_handler);
         
   return 0;
} 


void cleanup_module(void)
{
   rtl_printf("Cleaning-up RTL buffered AO module\n");

   // Clean-up data acquisition if needed
   if(aoData.state != closed)
      CleanUpBufferedAO(&aoData);

   //rtf_destroy(COMMAND_FIFO);
   //rtf_destroy(DATA_FIFO);

   pthread_cancel (ao_thread);
   pthread_join (ao_thread, NULL);
}




