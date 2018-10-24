/*****************************************************************************/
/*                    Single digital input example                           */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single       */
/*  digital acquisition. The acquisition is performed in a software timed    */
/*  fashion that is appropriate for slow speed acquisition (up to 500Hz).    */
/*  This example only works on PDx-MFx and PD2-AO boards.                    */
/*  For the PD2-DIO boards look at the SingleDIO example.                    */
/*                                                                           */
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
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "gnuplot.h"
#include "ParseParams.h"

typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _singleDiData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   unsigned long portList[64];
   int nbOfPorts;                // number of digital lines to use
   int nbOfSamplesPerPort;       // number of samples per port
   double scanRate;              // sampling frequency on each channel
   tState state;                 // state of the acquisition session
} tSingleDiData;


int InitSingleDI(tSingleDiData *pDiData);
int SingleDI(tSingleDiData *pDiData);
void CleanUpSingleDI(tSingleDiData *pDiData);


static tSingleDiData G_DiData;

// exit handler
void SingleDIExitHandler(int status, void *arg)
{
   CleanUpSingleDI((tSingleDiData *)arg);
}


int InitSingleDI(tSingleDiData *pDiData)
{
   int retVal = 0;

   pDiData->handle = PdAcquireSubsystem(pDiData->board, DigitalIn, 1);
   if(pDiData->handle < 0)
   {
      printf("SingleAI: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pDiData->state = unconfigured;

   retVal = _PdDInReset(pDiData->handle);
   if (retVal < 0)
   {
      printf("SingleDI: PdDInReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int SingleDI(tSingleDiData *pDiData)
{
   int retVal;
   int i;
   DWORD value;
      
   pDiData->state = configured;
   
   pDiData->state = running;

   for(i=0; i<pDiData->nbOfSamplesPerPort; i++)
   {
      retVal = _PdDInRead(pDiData->handle, &value);
      if(retVal < 0)
      {
         printf("SingleDI: _PdDInRead error: %d\n", retVal);
         exit(EXIT_FAILURE);
      }

      printf("SingleDI: read %x\n", value);

      usleep(1.0E6/pDiData->scanRate);
   }

   return retVal;
}


void CleanUpSingleDI(tSingleDiData *pDiData)
{
   int retVal;
      
   if(pDiData->state == running)
   {
      pDiData->state = configured;
   }

   if(pDiData->state == configured)
   {
      retVal = _PdDInReset(pDiData->handle);
      if (retVal < 0)
         printf("SingleDI: _PdDinReset error %d\n", retVal);

      pDiData->state = unconfigured;
   }

   if(pDiData->handle > 0 && pDiData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDiData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("SingleDI: PdReleaseSubsystem error %d\n", retVal);
   }

   pDiData->state = closed;
}


int main(int argc, char *argv[])
{
   int i;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DiData.board = params.board;
   G_DiData.nbOfPorts = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_DiData.portList[i] = params.channels[i];
   G_DiData.handle = 0;
   G_DiData.nbOfSamplesPerPort = params.numSamplesPerChannel;
   G_DiData.scanRate = params.frequency;
   G_DiData.state = closed;


   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(SingleDIExitHandler, &G_DiData);

   // initializes acquisition session
   InitSingleDI(&G_DiData);

   // run the acquisition
   SingleDI(&G_DiData);

   // Cleanup acquisition
   CleanUpSingleDI(&G_DiData);

   return 0;
}

