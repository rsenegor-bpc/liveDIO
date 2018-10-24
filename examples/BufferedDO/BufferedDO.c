/*****************************************************************************/
/*                    Buffered digital output example                        */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a digital      */  
/*  buffered update. The update is performed in a hardware timed fashion     */
/*  that is appropriate for fast speed pattern generation (> 1MHz).          */
/*  Each channel is a 16-bit port on the board.                              */
/*                                                                           */
/*  The buffered digital input only works with PD2-DIO boards.               */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2001 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "ParseParams.h"


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _bufferedDoData
{
   int abort;                    // set to TRUE to abort generation
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   DWORD channelList[64];
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   int nbOfFrames;
   unsigned char outPorts;       // mask that selects the ports of 16 lines that
                                 // will be used for digital output
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
   unsigned int *rawBuffer;      // address of the buffer allocated by the driver to store
} tBufferedDoData;


int InitBufferedDO(tBufferedDoData *pDoData);
int BufferedDO(tBufferedDoData *pDoData);
void CleanUpBufferedDO(tBufferedDoData *pDoData);


static tBufferedDoData G_DoData;

// exit handler
void BufferedDOExitHandler(int status, void *arg)
{
   CleanUpBufferedDO((tBufferedDoData *)arg);
}


int InitBufferedDO(tBufferedDoData *pDoData)
{
   int retVal = 0;
   Adapter_Info adaptInfo;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDoData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("BufferedAO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDoData->adapterType = adaptInfo.atType;
   if(adaptInfo.atType & atPD2DIO)
      printf("This is a PD2-DIO board\n");
   else
   {
      printf("This board is not a PD2-DIO\n");
      exit(EXIT_FAILURE);
   }

   pDoData->handle = PdAcquireSubsystem(pDoData->board, DigitalOut, 1);
   if(pDoData->handle < 0)
   {
      printf("BufferedDO: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pDoData->state = unconfigured;

   retVal = _PdDIOReset(pDoData->handle);
   if (retVal < 0)
   {
      printf("BufferedDO: PdAOutReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int BufferedDO(tBufferedDoData *pDoData)
{
   int retVal;
   DWORD aoCfg;
   DWORD cvDivider;
   DWORD count = 0;
   DWORD eventsToNotify = eTimeout | eFrameDone | eBufferDone | eBufferError | eStopped;
   DWORD events;
   DWORD scanIndex, numScans;
   int i, j, k, outIndex;
   int bit_shift;

   
   retVal = _PdDIOEnableOutput(pDoData->handle, pDoData->outPorts);
   if(pDoData->handle < 0)
   {
      printf("BufferedDO: _PdDioEnableOutput failed\n");
      exit(EXIT_FAILURE);
   }

   // setup the board to use hardware internal clock
   aoCfg = AOB_CVSTART0;  // 11 MHz internal timebase
      
   retVal = _PdRegisterBuffer(pDoData->handle, (WORD**)&pDoData->rawBuffer, AnalogOut,
                              pDoData->nbOfFrames, pDoData->nbOfPointsPerChannel,
                              pDoData->nbOfChannels, 
                              BUF_BUFFERRECYCLED | BUF_BUFFERWRAPPED | BUF_DWORDVALUES);
   if (retVal < 0)
   {
      printf("BufferedDO: PdRegisterBuffer error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   for(i=0; i<pDoData->nbOfFrames; i++)
   {
      for(j=0; j<pDoData->nbOfPointsPerChannel; j++)
      {
          for(k=0; k<pDoData->nbOfChannels; k++)
          {
              outIndex = i * pDoData->nbOfPointsPerChannel * pDoData->nbOfChannels +
                            j * pDoData->nbOfChannels;
              bit_shift = (int)floor((sin(outIndex * 6.28 / pDoData->nbOfPointsPerChannel)+1) / 2 * 7 + 0.5);
              pDoData->rawBuffer[outIndex + k] = 1 << bit_shift;
          }
      }
   }

   // set clock divider, assuming that we use the 11MHz timebase
   cvDivider = 11000000.0 / pDoData->updateRate -1;
   
   retVal = _PdDOAsyncInit(pDoData->handle, aoCfg, 
                           cvDivider, eventsToNotify, 
                           pDoData->nbOfChannels, pDoData->channelList);
   if (retVal < 0)
   {
      printf("BufferedDO: PdDOAsyncInit error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDoData->state = configured;

   retVal = _PdSetUserEvents(pDoData->handle, DigitalOut, eventsToNotify);
   if (retVal < 0)
   {
      printf("BufferedDO: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   retVal = _PdDOAsyncStart(pDoData->handle);
   if (retVal < 0)
   {
      printf("BufferedDO: PdDOAsyncStart error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   pDoData->state = running;

   while (!pDoData->abort)
   {
      events = _PdWaitForEvent(pDoData->handle, eventsToNotify, 5000);

      retVal = _PdSetUserEvents(pDoData->handle, DigitalOut, eventsToNotify);
      if (retVal < 0)
      {
         printf("BufferedDO: _PdSetUserEvents error %d\n", retVal);
         exit(EXIT_FAILURE);
      }
   
      if (events & eTimeout)
      {
         printf("BufferedDO: wait timed out\n"); 
         exit(EXIT_FAILURE);
      }
   
      if ((events & eBufferError) || (events & eStopped))
      {
         printf("BufferedDO: buffer error\n");
         exit(EXIT_FAILURE);
      }
   
      if ((events & eBufferDone) || (events & eFrameDone))
      {
         retVal = _PdDOGetBufState(pDoData->handle, pDoData->nbOfPointsPerChannel, 
                                   SCANRETMODE_MMAP, &scanIndex, &numScans);
         if(retVal < 0)
         {
            printf("BufferedDO: buffer error\n");
            exit(EXIT_FAILURE);
         }

         // update buffer on the fly
         /*for(j=0; j<pDoData->nbOfPointsPerChannel; j++)
         {
             for(k=0; k<pDoData->nbOfChannels; k++)
             {
                 outIndex = scanIndex + j * pDoData->nbOfChannels;
                 if((outIndex+k) < (pDoData->nbOfFrames * pDoData->nbOfPointsPerChannel * pDoData->nbOfChannels))
                 {
                    bit_shift = (int)floor((sin(outIndex * 6.28 / pDoData->nbOfPointsPerChannel)+1) / 2 * 7 + 0.5);
                    pDoData->rawBuffer[outIndex + k] = 3 << bit_shift;
                 }
             }
         }*/

         count = count + numScans;
   
         printf("BufferedDO: sent %d scans at %d\n", numScans, scanIndex);
      }
   }

   return retVal;
}


void CleanUpBufferedDO(tBufferedDoData *pDoData)
{
   int retVal;
      
   if(pDoData->state == running)
   {
      retVal = _PdDOAsyncStop(pDoData->handle);
      if (retVal < 0)
         printf("BufferedDO: PdDOAsyncStop error %d\n", retVal);

      pDoData->state = configured;
   }

   if(pDoData->state == configured)
   {
      retVal = _PdClearUserEvents(pDoData->handle, DigitalOut, eAllEvents);
      if (retVal < 0)
         printf("BufferedDO: PdClearUserEvents error %d\n", retVal);
   
      retVal = _PdDOAsyncTerm(pDoData->handle);
      if (retVal < 0)
         printf("BufferedDO: PdAOutAsyncTerm error %d\n", retVal);
   
      retVal = _PdUnregisterBuffer(pDoData->handle, (WORD *)pDoData->rawBuffer, DigitalOut);
      if (retVal < 0)
         printf("BufferedDO: PdUnregisterBuffer error %d\n", retVal);

      pDoData->state = unconfigured;
   }

   if(pDoData->handle > 0 && pDoData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDoData->handle, DigitalOut, 0);
      if (retVal < 0)
         printf("BufferedDO: PdReleaseSubsystem error %d\n", retVal);
   }

   pDoData->state = closed;
}


//////////////////////////////////////////////////////////////////////////
// Function to react to SIGINT (Ctrl+C)
void SigInt(int signum)
{
   if (signum == SIGINT)
   {
      printf("Ctrl+C detected. generation stopped\n");
      G_DoData.abort = TRUE;
   }
}


int main(int argc, char *argv[])
{
   int i;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DoData.board = params.board;
   G_DoData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
      G_DoData.channelList[i] = params.channels[i];
   G_DoData.handle = 0;
   G_DoData.nbOfPointsPerChannel = params.numSamplesPerChannel;
   G_DoData.updateRate = params.frequency;
   G_DoData.nbOfFrames = 4;
   G_DoData.state = closed;
   G_DoData.abort = FALSE;
   G_DoData.rawBuffer = NULL;
   G_DoData.outPorts = 0x03;  // select ports 1 and 3 for output

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(BufferedDOExitHandler, &G_DoData);

   // set up SIGINT handler
   signal(SIGINT, SigInt);

   // initializes acquisition session
   InitBufferedDO(&G_DoData);

   // run the acquisition
   BufferedDO(&G_DoData);

   // Cleanup acquisition
   CleanUpBufferedDO(&G_DoData);

   return 0;
}
