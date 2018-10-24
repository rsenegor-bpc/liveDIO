/*****************************************************************************/
/*                    Single update analog output example                    */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single update*/  
/*  The update is performed in a software timed fashion                      */
/*  that is appropriate for slow speed generation (up to 500Hz).             */
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
#include <math.h>
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "ParseParams.h"

#define errorChk(functionCall) {int error; if((error=functionCall)<0) { \
	                           fprintf(stderr, "Error %d at line %d in function call %s\n", error, __LINE__, #functionCall); \
	                           exit(EXIT_FAILURE);}}



typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _singleAoData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int abort;
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   DWORD channelList[64];
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
} tSingleAoData;


int InitSingleAO(tSingleAoData *pAoData);
int SingleAO(tSingleAoData *pAoData, double *buffer);
void CleanUpSingleAO(tSingleAoData *pAoData);
void SigInt(int signum);

static tSingleAoData G_AoData;

// exit handler
void SingleAOExitHandler(int status, void *arg)
{
   CleanUpSingleAO((tSingleAoData *)arg);
}


int InitSingleAO(tSingleAoData *pAoData)
{
   Adapter_Info adaptInfo;

   // get adapter type
   errorChk(_PdGetAdapterInfo(pAoData->board, &adaptInfo));
   
   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
      printf("This is an MFx board\n");
   else
      printf("This is an AO32 board\n");

   pAoData->handle = PdAcquireSubsystem(pAoData->board, AnalogOut, 1);
   if(pAoData->handle < 0)
   {
      printf("SingleAO: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAoData->state = unconfigured;

   errorChk(_PdAOutReset(pAoData->handle));
   
   // need also to call this function if the board is a PD2-AO-xx
   if(pAoData->adapterType & atPD2AO)
   {
      errorChk(_PdAO32Reset(pAoData->handle));
   }

   return 0;
}


int SingleAO(tSingleAoData *pAoData, double *buffer)
{
   int retVal = 0;
   int i, j;
   DWORD aoCfg;
   DWORD value;
      

   // set configuration - _PdAOutReset is called inside _PdAOutSetCfg
   aoCfg = 0;
   errorChk(_PdAOutSetCfg(pAoData->handle, aoCfg, 0));
      
   pAoData->state = configured;

   //Start SW trigger
   errorChk(_PdAOutSwStartTrig(pAoData->handle));
   
   pAoData->state = running;

   for(i=0; i<pAoData->nbOfPointsPerChannel; i++)
   {
      // write update value to the board
      if(pAoData->adapterType & atMF)
      {
         // special case for PD2-MFx boards
         value = (buffer[i * pAoData->nbOfChannels] + 10.0) / 20.0 * 0xFFF;
          
         // if the user requests two channels we also need
         // to add the second channel to value
         if(pAoData->nbOfChannels > 1)
         {
            value = value | ((int)((buffer[i * pAoData->nbOfChannels + 1] + 10.0) / 20.0 * 0xFFF) << 12);
         }

         errorChk(_PdAOutPutValue(pAoData->handle, value));
      }
      else if (pAoData->adapterType & atPD2AO)
      {
         // special case for PD2-AO boards
         for(j=0; j<pAoData->nbOfChannels; j++)
         {
            value = (buffer[i * pAoData->nbOfChannels + j] + 10.0) / 20.0 * 0xFFFF;
            errorChk(_PdAO32Write(pAoData->handle, j, value));
         }
      }
      else
      {
         // The board is not AO or MFx 
         break;
      }

      printf(".");
      fflush(stdout);
      if(pAoData->abort)
         break;

      usleep(1.0E6/pAoData->updateRate);
   }

   printf("\n");

   return retVal;
}


void CleanUpSingleAO(tSingleAoData *pAoData)
{
   if(pAoData->state == running)
   {
      pAoData->state = configured;
   }

   if(pAoData->state == configured)
   {
      errorChk(_PdAOutReset(pAoData->handle));
            
      // need also to call this function if the board is a PD2-AO-xx
      if(pAoData->adapterType & atPD2AO)
      {
         errorChk(_PdAO32Reset(pAoData->handle));
      }

      pAoData->state = unconfigured;
   }

   if(pAoData->handle > 0 && pAoData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pAoData->handle, AnalogOut, 0));
   }

   pAoData->state = closed;
}

void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected. Generation stopped.\n");
      G_AoData.abort = TRUE;
   }
}

int main(int argc, char *argv[])
{
   double *buffer = NULL;
   int i, j;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_AoData.board = params.board;
   G_AoData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AoData.channelList[i] = params.channels[i];
   G_AoData.handle = 0;
   G_AoData.abort = FALSE;
   G_AoData.nbOfPointsPerChannel = params.numSamplesPerChannel;
   G_AoData.updateRate = params.frequency;
   G_AoData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(SingleAOExitHandler, &G_AoData);

   signal(SIGINT, SigInt);

   // allocate memory for the generation buffer
   buffer = (double *) malloc(G_AoData.nbOfChannels * G_AoData.nbOfPointsPerChannel *
                              sizeof(double));
   if(buffer == NULL)
   {
      printf("SingleAO: could not allocate enough memory for the generation buffer\n");
      exit(EXIT_FAILURE);
   }

   // writes data to buffer. Each channel will generate a sinus of different frequency
   for(i=0; i<G_AoData.nbOfPointsPerChannel; i++)
   {
      for(j=0; j<G_AoData.nbOfChannels; j++)
      {
         buffer[i*G_AoData.nbOfChannels + j] = 
               10.0 * sin(2.0 * 3.1415 * i * (j + 1) / G_AoData.nbOfPointsPerChannel);
      }
   }

   // initializes acquisition session
   InitSingleAO(&G_AoData);

   // run the acquisition
   SingleAO(&G_AoData, buffer);

   // Cleanup acquisition
   CleanUpSingleAO(&G_AoData);

   // free acquisition buffer
   free(buffer);

   return 0;
}

