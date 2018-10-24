/*****************************************************************************/
/*                 Simple analog output example                              */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a waveform     */
/*  generation under RT-Linux. The generation is performed in a software     */
/*  timed fashion that is appropriate for slow speed generation (> 1MHz).    */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2003 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*  author: Frederic Villeneuve                                              */
/*                                                                           */ 
/*****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define PERIODIC_FREQ_HZ        10000.0   // periodic rate
#define PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
                          

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
} tBufferedAoData;


//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];
pthread_t   ao_thread;
tBufferedAoData aoData;


int StartSimpleAO(tBufferedAoData *pAoData);
int WaitSimpleAO(tBufferedAoData *pAoData, int timeoutms);
void CleanUpSimpleAO(tBufferedAoData *pAoData);

int StartBufferedAO(tBufferedAoData *pAoData)
{
   int retVal = 0;
   DWORD aoCfg;
   
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

   // setup the board to use software clock
   /*if(pAoData->isAoBoard)
      aoCfg = AOB_CVSTART0 | AOB_AOUT32;  // 11 MHz internal timebase
   else
      aoCfg = AOB_CVSTART0;*/
   aoCfg = 0;

   retVal = pd_aout_set_config(pAoData->board, aoCfg, 0);
   if(!retVal)
   {
      rtl_printf("BufferedAO: _PdAOutSetCfg failed with code %x\n", retVal);
      return retVal;
   }

   pAoData->state = configured;
  
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


void *ao_thread_proc(void *param)
{
   int ret, i, j;
   int bHigh = FALSE;
   struct timespec abstime;

   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);


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
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, PERIOD_NS);

      if(aoData.isAoBoard)
      {
      }
      else
      {
         ret = pd_aout_put_value(aoData.board, aoData.rawBuffer[i%aoData.nbOfPointsPerChannel]);
         if(!ret)
         {
            rtl_printf("Error: pd_aout_put_value returned %d\n", ret);
            return NULL;
         }

         ret = pd_aout_sw_cv_start(aoData.board);
         if(!ret)
         {
            rtl_printf("Error: pd_aout_sw_cv_start returned %d\n", ret);
            return NULL;
         }

         i++;
      }
   }

   CleanUpBufferedAO(&aoData);
   
   return NULL;
}


int data_fifo_handler(unsigned int fifo_id)
{
   return 0;
}


int main(void)
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
   
   // Allocate memory to acquire in
   aoData.rawBuffer = (unsigned int*) rtl_gpos_malloc(aoData.nbOfChannels * aoData.nbOfPointsPerChannel *
                                                sizeof(unsigned int));
   if(aoData.rawBuffer == NULL)
   {
      rtl_printf("Error: could not allocate enough memory for the generation buffer\n");
      return 0;
   }

   // Create thread that will perform the generation 
   ret = rtl_pthread_create (&ao_thread,  NULL, ao_thread_proc, NULL);
   
   rtl_main_wait();

   aoData.abort = TRUE;

   // wait for thread to finish
   rtl_pthread_join (ao_thread, NULL);

   rtl_gpos_free(aoData.rawBuffer);

   // Clean-up data acquisition if needed
   if(aoData.state != closed)
      CleanUpBufferedAO(&aoData);
}




