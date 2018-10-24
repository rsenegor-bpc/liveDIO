/*****************************************************************************/
/*                    Single scan analog input example                       */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single scan  */
/*  acquisition. The acquisition is performed in a software timed fashion    */
/*  that is appropriate for slow speed acquisition (up to 500Hz).            */
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
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"

#include "gnuplot.h"
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

typedef struct _singleAiData
{
   int board;                    /* board number to be used for the AI operation*/
   int handle;                   /* board handle*/
   int nbOfChannels;             /* number of channels*/
   int nbOfSamplesPerChannel;    /* number of samples per channel*/
   DWORD channelList[64];
   double scanRate;              /* sampling frequency on each channel*/
   int polarity;                 /* polarity of the signal to acquire, possible value*/
                                 /* is AIN_UNIPOLAR or AIN_BIPOLAR*/
   int range;                    /* range of the signal to acquire, possible value*/
                                 /* is AIN_RANGE_5V or AIN_RANGE_10V*/
   int inputMode;                /* input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL*/
   int trigger;
   tState state;                 /* state of the acquisition session*/
} tSingleAiData;


int InitSingleAI(tSingleAiData *pAiData);
int SingleAI(tSingleAiData *pAiData, double *buffer);
void CleanUpSingleAI(tSingleAiData *pAiData);


static tSingleAiData G_AiData;

/* exit handler*/
void SingleAIExitHandler(int status, void *arg)
{
   CleanUpSingleAI((tSingleAiData *)arg);
}


int InitSingleAI(tSingleAiData *pAiData)
{
   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("SingleAI: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pAiData->state = unconfigured;

   errorChk(_PdAInReset(pAiData->handle));

   return 0;
}


int SingleAI(tSingleAiData *pAiData, double *buffer)
{
   DWORD numScans = 0;
   int i;
   DWORD aiCfg;
   unsigned short *rawBuffer = NULL;
      
   /* setup the board to use software clock*/
   aiCfg = AIB_CVSTART0 | AIB_CVSTART1 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   errorChk(_PdAInSetCfg(pAiData->handle, aiCfg, 0, 0));
   errorChk(_PdAInSetChList(pAiData->handle, pAiData->nbOfChannels, (DWORD*)pAiData->channelList));
   
   /* allocate memory for the buffer that will receive binary data from the board*/
   rawBuffer = (unsigned short*)malloc(pAiData->nbOfSamplesPerChannel*
                               pAiData->nbOfChannels*sizeof(unsigned short));
   if(rawBuffer == NULL)
   {
      printf("SingleAI: Can't allocate memory for raw buffer\n");
      exit(EXIT_FAILURE);
   }

   pAiData->state = configured;

   errorChk(_PdAInEnableConv(pAiData->handle, TRUE));

   /*Start SW trigger*/
   if(pAiData->trigger == 0)
   {
      errorChk(_PdAInSwStartTrig(pAiData->handle));
   }

   pAiData->state = running;

   for(i=0; i<pAiData->nbOfSamplesPerChannel; i++)
   {
      errorChk(_PdAInSwClStart(pAiData->handle));
      printf(".");
      fflush(stdout);
      usleep(1.0E6/pAiData->scanRate);
   }

   printf("\n");

   errorChk(_PdAInGetSamples(pAiData->handle, pAiData->nbOfChannels * 
                             pAiData->nbOfSamplesPerChannel, rawBuffer, &numScans));
   
   printf("SingleAI: detected %d samples.\n", numScans);
   
   errorChk(PdAInRawToVolts(pAiData->board, aiCfg, rawBuffer, buffer, 
                               pAiData->nbOfSamplesPerChannel * pAiData->nbOfChannels));
   
   printf("SingleAI:");
   for(i=0; i<pAiData->nbOfChannels; i++)
      printf(" ch%d = %f (0x%x)", i, *(buffer + i), *(rawBuffer + i));
   printf("\n");       
   
   free(rawBuffer);

   return 0;
}


void CleanUpSingleAI(tSingleAiData *pAiData)
{
   if(pAiData->state == running)
   {
      pAiData->state = configured;
   }

   if(pAiData->state == configured)
   {
      errorChk(_PdAInReset(pAiData->handle));
      pAiData->state = unconfigured;
   }

   if(pAiData->handle >= 0 && pAiData->state == unconfigured)
   {
      errorChk(PdAcquireSubsystem(pAiData->handle, AnalogIn, 0));
   }

   pAiData->state = closed;
}


int main(int argc, char *argv[])
{
   double *buffer = NULL;
   int nbOfBoards;
   int i;
   int gain = 0;
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 100};
   
   ParseParameters(argc, argv, &params);

   /* initializes acquisition session parameters */
   G_AiData.board = params.board;
   G_AiData.nbOfChannels = params.numChannels;
   for(i=0; i<params.numChannels; i++)
       G_AiData.channelList[i] = params.channels[i] | (gain << 6);
   G_AiData.handle = -1;
   G_AiData.nbOfSamplesPerChannel = params.numSamplesPerChannel;
   G_AiData.scanRate = params.frequency;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_10V;
   G_AiData.inputMode = AIN_SINGLE_ENDED;
   G_AiData.state = closed;
   if(params.trigger == 1)
      G_AiData.trigger = AIB_STARTTRIG0;
   else if(params.trigger == 2)
      G_AiData.trigger = AIB_STARTTRIG0 + AIB_STARTTRIG1;
   else
      G_AiData.trigger = 0;

   /* setup exit handler that will clean-up the acquisition session */
   /* if an error occurs */
   on_exit(SingleAIExitHandler, &G_AiData);

   nbOfBoards = PdGetNumberAdapters();
   
   /* initializes acquisition session */
   InitSingleAI(&G_AiData);

   /* allocate memory for the buffer */
   buffer = (double *) malloc(G_AiData.nbOfChannels * G_AiData.nbOfSamplesPerChannel *
                              sizeof(double));
   if(buffer == NULL)
   {
      printf("SingleAI: could not allocate enough memory for the acquisition buffer\n");
      exit(EXIT_FAILURE);
   }

   /* run the acquisition */
   SingleAI(&G_AiData, buffer);

   /* Call Gnuplot to display the acquired data */
   sendDataToGnuPlot(G_AiData.handle, 0.0, 1.0/G_AiData.scanRate, buffer, 
                     G_AiData.nbOfChannels, G_AiData.nbOfSamplesPerChannel);

   /* Cleanup acquisition */
   CleanUpSingleAI(&G_AiData);

   /* free acquisition buffer */
   free(buffer);

   return 0;
}

