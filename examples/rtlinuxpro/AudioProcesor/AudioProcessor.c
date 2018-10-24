/*****************************************************************************/
/*                    Single scan analog input/output example                */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single scan  */
/*  acquisition followed by the generation of the scan on the analog outputs.*/
/*  The acquisition/genertaion is timed by the RTLinux timer.                */
/*  that is appropriate for moderate acquisition speed (up to 50kHz).        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2005 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */ 
/*****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "powerdaq.h"
#include "powerdaq32.h"


#define errorChk(functionCall) {int error; if((error=functionCall)<0) { \
	                           rtl_printf("Error %d at line %d in function call %s\n", error, __LINE__, #functionCall); \
	                           pthread_exit((void*)error);}}

typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _singleAiData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int nbOfChannels;             // number of channels
   int nbOfSamplesPerChannel;    // number of samples per channel
   unsigned int channelList[64];
   unsigned int configBits;
   double scanRate;              // sampling frequency on each channel
   int polarity;                 // polarity of the signal to acquire, possible value
                                 // is AIN_UNIPOLAR or AIN_BIPOLAR
   int range;                    // range of the signal to acquire, possible value
                                 // is AIN_RANGE_5V or AIN_RANGE_10V
   int inputMode;                // input mode possible value is AIN_SINGLE_ENDED or AIN_DIFFERENTIAL
   int trigger;
   tState state;                 // state of the acquisition session
   int bAbort;
   unsigned short* rawData;
   pthread_t  thread;
   int resolution;
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
   unsigned short ANDMask;
   unsigned short XORMask;
} tSingleAiData;


typedef struct _singleAoData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int abort;
   int adapterType;              // adapter type, shoul be atMF or atPD2AO
   unsigned int channelList[64];
   unsigned int configBits;
   int nbOfChannels;             // number of channels
   int nbOfPointsPerChannel;     // number of samples per channel
   double updateRate;            // sampling frequency on each channel
   tState state;                 // state of the acquisition session
   int maxCode;                  // maximum possible value (0xFFF for 12 bits, 0xFFFF for 16 bits)
   int resolution;
   unsigned short* rawData;
} tSingleAoData;

typedef enum _effectType
{
   Echo,
   Reverb,
   Chorus
} tEffectType;

typedef struct _audioEffectData
{
   unsigned short* buffer;
   int bufferSize;
   tEffectType type;
   double decay;
   int delaySize;
   int currentSample;
} tAudioEffectData;


int InitSingleAI(tSingleAiData *pAiData);
void CleanUpSingleAI(tSingleAiData *pAiData);

int InitSingleAO(tSingleAoData *pAiData);
void CleanUpSingleAO(tSingleAoData *pAiData);


static tSingleAiData G_AiData;
static tSingleAoData G_AoData;
static tAudioEffectData G_AudioEffect;


int InitSingleAI(tSingleAiData *pAiData)
{
   Adapter_Info adaptInfo;

   // get adapter type
   errorChk(_PdGetAdapterInfo(pAiData->board, &adaptInfo));

   pAiData->resolution = adaptInfo.SSI[AnalogIn].dwChBits;
   pAiData->maxCode = (1 << adaptInfo.SSI[AnalogIn].dwChBits)-1;
   pAiData->ANDMask = adaptInfo.SSI[AnalogIn].wAndMask;
   pAiData->XORMask = adaptInfo.SSI[AnalogIn].wXorMask;

   pAiData->handle = PdAcquireSubsystem(pAiData->board, AnalogIn, 1);
   if(pAiData->handle < 0)
   {
      printf("SingleAIAO: PdAcquireSubsystem failed\n");
      pthread_exit(NULL);
   }

   pAiData->state = unconfigured;

   errorChk(_PdAInReset(pAiData->handle));

   return 0;
}

int InitSingleAO(tSingleAoData *pAoData)
{
   Adapter_Info adaptInfo;

   // get adapter type
   errorChk(_PdGetAdapterInfo(pAoData->board, &adaptInfo));
   
   pAoData->resolution = adaptInfo.SSI[AnalogOut].dwChBits;
   pAoData->maxCode = (1 << adaptInfo.SSI[AnalogOut].dwChBits) - 1;
   pAoData->adapterType = adaptInfo.atType;
   if(pAoData->adapterType & atMF)
   {
      printf("This is an MFx board\n");
   }
   else
   {
      printf("This is an AO32 board\n");
   }

   pAoData->handle = PdAcquireSubsystem(pAoData->board, AnalogOut, 1);
   if(pAoData->handle < 0)
   {
      printf("SingleAIAO: PdAcquireSubsystem failed\n");
      pthread_exit(NULL);
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


unsigned short ProcessEffect(tAudioEffectData* pEffect, unsigned short input)
{
   int i;
   unsigned short output;

   if(pEffect->currentSample >= pEffect->delaySize)
   {
      i = pEffect->currentSample - pEffect->delaySize; 
   }
   else
   {
      i = pEffect->currentSample - pEffect->delaySize + pEffect->bufferSize;
   }

   output = input + pEffect->buffer[i] * pEffect->decay;

   // put new sample in circular buffer
   if(pEffect->currentSample < (pEffect->bufferSize-1))
   {
      pEffect->currentSample++;
   }
   else
   {
      pEffect->currentSample = 0;
   } 

   pEffect->buffer[pEffect->currentSample] = input;
   pEffect->buffer[i] = output;


   return output;
}


void* SingleAiAoProc(void *arg)
{ 
   tSingleAiData *pAiData = (tSingleAiData*)arg;
   tSingleAoData *pAoData = &G_AoData;
   hrtime_t taskPeriod;
   DWORD numScans, totalScans=0;
   int i, count = 0;
   struct timespec abstime, starttime, stoptime;
   long duration;
      
   rtl_printf("starting thread\n");
       
   // initializes acquisition session
   InitSingleAI(pAiData);
   InitSingleAO(pAoData);
      
   // setup analog input subsystem to use software clock
   pAiData->configBits = AIB_CVSTART0 | AIB_CVSTART1 | pAiData->range | pAiData->inputMode | 
           pAiData->polarity | pAiData->trigger; 
   errorChk(_PdAInSetCfg(pAiData->handle, pAiData->configBits, 0, 0));
   errorChk(_PdAInSetChList(pAiData->handle, pAiData->nbOfChannels, (DWORD*)pAiData->channelList));
   errorChk(_PdAInEnableConv(pAiData->handle, TRUE));
   errorChk(_PdAInSwStartTrig(pAiData->handle));

   // setup the analog output subsystem
   pAoData->configBits = 0;
   errorChk(_PdAOutSetCfg(pAoData->handle, pAoData->configBits, 0));
   errorChk(_PdAOutSwStartTrig(pAoData->handle));

   taskPeriod = ((hrtime_t)((1.0/pAiData->scanRate) * NSECS_PER_SEC));
   
   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);
   starttime = abstime;
   
   while (!pAiData->bAbort) 
   {
      // Setup absolute time for next loop
      timespec_add_ns(&abstime, taskPeriod);
      
      errorChk(_PdAInSwClStart(pAiData->handle));
      
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);
      
      errorChk(_PdAInGetSamples(pAiData->handle, pAiData->nbOfChannels, 
                                pAiData->rawData, &numScans));
      
      if(numScans > 0)
      {
         for (i=0; i<pAiData->nbOfChannels; i++)
         {
            // Convert data between analog input raw binary format and analog output raw binary format         
            pAiData->rawData[i]= ((pAiData->rawData[i] & pAiData->ANDMask) ^ pAiData->XORMask) 
                                  >> (16 - pAiData->resolution);
   
            // Scale data to match input and output channel resolutions (AI 12,14 or 16 bits -> AO 12 or 16 bits)
            pAiData->rawData[i] = pAiData->rawData[i] >> (16 - pAoData->resolution);

            // Process sample
            pAoData->rawData[i] = ProcessEffect(&G_AudioEffect, pAiData->rawData[i]);
         }
   
         if (pAoData->adapterType & atMF)
         {
            if (pAoData->nbOfChannels > 1)
            {
               pAoData->rawData[0] = (pAoData->rawData[0]&0xFFF) | ((pAoData->rawData[1]&0xFFF) << 12);
            }
            errorChk(_PdAOutPutValue(pAoData->handle, pAoData->rawData[0]));
         } 
         else
         {
            for (i=0; i<pAoData->nbOfChannels; i++)
            {
               errorChk(_PdAO32Write(pAoData->handle, i, pAoData->rawData[i]));
            }
         }
      }
      else
      {
         printf("No scans available!\n");
         break;
      }

      totalScans = totalScans + numScans;
      count++;
   }
   
   clock_gettime(CLOCK_REALTIME, &stoptime);
   timespec_sub(&stoptime, &starttime);

   duration = stoptime.tv_sec*1000000.0+stoptime.tv_nsec/1000.0;
   
   printf("Acquired %d scans (%ld) in %ld us\n", count, totalScans, 
          duration); 
   
   CleanUpSingleAI(pAiData);
   CleanUpSingleAO(pAoData);
   
   return NULL;
}

int main(int argc, char *argv[])
{
   int i, fifod;
   int gain = 0;
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads
   

   // initializes acquisition session parameters
   G_AiData.board = 1;
   G_AiData.nbOfChannels = 1;
   for(i=0; i<G_AiData.nbOfChannels; i++)
       G_AiData.channelList[i] = i | (gain << 6);
   G_AiData.handle = -1;
   G_AiData.nbOfSamplesPerChannel = 1000;
   G_AiData.scanRate = 30000.0;
   G_AiData.polarity = AIN_BIPOLAR;
   G_AiData.range = AIN_RANGE_10V;
   G_AiData.inputMode = AIN_SINGLE_ENDED;
   G_AiData.state = closed;
   G_AiData.bAbort = 0;
   G_AiData.trigger = 0;

   // Configure analog output parameters
   G_AoData.board = 1;
   G_AoData.nbOfChannels = 1;
   for(i=0; i<G_AoData.nbOfChannels; i++)
       G_AoData.channelList[i] = i;
   G_AoData.handle = -1;
   G_AoData.nbOfPointsPerChannel = G_AiData.nbOfSamplesPerChannel;
   G_AoData.updateRate = G_AiData.scanRate;
   G_AoData.state = closed;

   G_AudioEffect.type = Echo;
   G_AudioEffect.currentSample = -1;
   G_AudioEffect.decay = 0.8;
   G_AudioEffect.delaySize = 500;
   G_AudioEffect.bufferSize = 4096;
   G_AudioEffect.buffer = (unsigned short*)rtl_gpos_malloc(G_AudioEffect.bufferSize*
                                                           sizeof(unsigned short));
   memset(G_AudioEffect.buffer, G_AudioEffect.bufferSize*
          sizeof(unsigned short), 0);

   // make a the FIFO that is available to RTLinux and Linux applications 
   mkfifo( "/dev/AudioProcessor", 0666);
   fifod = open("/dev/AudioProcessor", O_RDWR | O_NONBLOCK);

   // Resize the FIFO so that it can hold 8K of data
   ftruncate(fifod, 8 << 10);

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   G_AiData.rawData = (unsigned short *) rtl_gpos_malloc(G_AiData.nbOfChannels * 
                                                         sizeof(unsigned short));
   if(G_AiData.rawData == NULL)
   {
      printf("SingleAIAO: could not allocate enough memory for the acquisition buffer\n");
      return -ENOMEM;
   }
   
   G_AoData.rawData = (unsigned short *) rtl_gpos_malloc(G_AoData.nbOfChannels * 
                                                         sizeof(unsigned short));
   if(G_AoData.rawData == NULL)
   {
      printf("SingleAIAO: could not allocate enough memory for the generation buffer\n");
      return -ENOMEM;
   }

   // create the thread
   pthread_create(&G_AiData.thread, &attrib, SingleAiAoProc, &G_AiData);

   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   G_AiData.bAbort = 1;
   //rtl_usleep(500000);
   //pthread_cancel(G_AiData.thread);
   pthread_join(G_AiData.thread, NULL);

   // Close and remove the FIFO
   close(fifod);
   unlink("/dev/AudioProcessor");
   
   // free acquisition buffer
   rtl_gpos_free(G_AiData.rawData);
   rtl_gpos_free(G_AoData.rawData);
   rtl_gpos_free(G_AudioEffect.buffer);

   return 0;
}

