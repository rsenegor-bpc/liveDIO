/*****************************************************************************/
/*                Asynchronous Digital input event example                   */
/*                                                                           */
/*  This example shows how to use the powerdaq API to set-up a PD2-DIO       */
/*  or a PDL-MF board to generate an interrupt when the state of a digital   */
/*  input changes.                                                           */
/*  The event coming from the board wakes-up the program, there is no        */
/*  polling involved.                                                        */
/*  For the PD2-MFx or PD2-AO boards look at the DIEvent example.            */
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

#include "ParseParams.h"


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;

typedef struct _diEventData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int port;                     // port to use
   int line;                     // digital line to use on the selected port
   int abort;
   tState state;                 // state of the acquisition session
} tDioEventData;


int InitDioEvent(tDioEventData *pDioData);
int DioEvent(tDioEventData *pDioData);
void CleanUpDioEvent(tDioEventData *pDioData);


static tDioEventData G_DioData;

// exit handler
void DioEventExitHandler(int status, void *arg)
{
   CleanUpDioEvent((tDioEventData *)arg);
}


int InitDioEvent(tDioEventData *pDioData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDioData->board, &adaptInfo);
   if (retVal < 0)
   {
      printf("SingleDIO: _PdGetAdapterInfo error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   if(adaptInfo.atType & atPD2DIO)
   {
      printf("This is a PD2-DIO board\n");
   }
   else if(adaptInfo.atType & atPDLMF)
   {
      printf("This is a PDL-MF board\n");
   }
   else
   {
      printf("This board is not a PD2-DIO or PDL-MF\n");
      exit(EXIT_FAILURE);
   }

   pDioData->handle = PdAcquireSubsystem(pDioData->board, DigitalIn, 1);
   if(pDioData->handle < 0)
   {
      printf("DioEvent: PdAcquireSubsystem failed\n");
      exit(EXIT_FAILURE);
   }

   pDioData->state = unconfigured;

   retVal = _PdDIOReset(pDioData->handle);
   if (retVal < 0)
   {
      printf("DioEvent: PdDIOReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int DioEvent(tDioEventData *pDioData)
{
   int i, retVal;
   DWORD eventsToNotify = eDInEvent;
   DWORD eventsReceived;
   DWORD masks[8]={0,0,0,0,0,0,0,0};
   DWORD intData[8];
   DWORD edgeData[8];
         
   // setup the board to generate an interrupt when the selected line
   // change state
   for(i=0; i<8; i++)
      if(i==pDioData->port)
         masks[i] = (1 << pDioData->line);
   retVal = _PdDIOSetIntrMask(pDioData->handle, masks);
   printf("ret mask : %d\n", retVal);
   if (retVal < 0)
   {
      printf("DioEvent: _PdDIOSetIntrMask error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   //masks[0] = 0xFFFF;

   pDioData->state = configured;

   // event suff
   retVal = _PdDIOIntrEnable(pDioData->handle, TRUE);
   if (retVal < 0)
   {
      printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   retVal = _PdAdapterEnableInterrupt(pDioData->handle, TRUE);
   if (retVal < 0)
   {
      printf("DioEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   // set events we want to receive
   retVal = _PdSetUserEvents(pDioData->handle, DigitalIn|EdgeDetect, eventsToNotify);
   if (retVal < 0)
   {
      printf("DioEvent: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 
   
   pDioData->state = running;

   while(!pDioData->abort)
   {
       eventsReceived = _PdWaitForEvent(pDioData->handle, eventsToNotify, 5000);

       if (eventsReceived & eTimeout)
       {
          printf("DioEvent: wait timed out\n"); 
          exit(EXIT_FAILURE);
       }
    
       if (eventsReceived & eDInEvent)     // something has happend
       {
          retVal = _PdDIOGetIntrData(pDioData->handle, intData, edgeData);
          if (retVal < 0)
          {
             printf("DioEvent: _PdDIOGetIntrData error %d\n", retVal);
             exit(EXIT_FAILURE);
          } 
    
          printf("DioEvent: event received! level 0x%x, latch 0x%x\n", 
                 intData[pDioData->port], edgeData[pDioData->port]);
       
          // re-enable events we want to receive
          retVal = _PdSetUserEvents(pDioData->handle, DigitalIn|EdgeDetect, eventsToNotify);
          if (retVal < 0)
          {
             printf("DioEvent: _PdSetUserEvents error %d\n", retVal);
             exit(EXIT_FAILURE);
          } 
    
          retVal = _PdDIOIntrEnable(pDioData->handle, TRUE);
          if (retVal < 0)
          {
             printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
             exit(EXIT_FAILURE);
          } 
       }
   }


   return retVal;
}


void CleanUpDioEvent(tDioEventData *pDioData)
{
   int retVal;
      
   if(pDioData->state == running)
   {
      retVal = _PdDIOIntrEnable(pDioData->handle, FALSE);
      if (retVal < 0)
         printf("DioEvent: _PdDIOIntrEnable error %d\n", retVal);
         
      pDioData->state = configured;
   }

   if(pDioData->state == configured)
   {
      retVal = _PdDIOReset(pDioData->handle);
      if (retVal < 0)
         printf("DioEvent: _PdDIOReset error %d\n", retVal);

      pDioData->state = unconfigured;
   }

   if(pDioData->handle > 0 && pDioData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDioData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("DioEvent: PdReleaseSubsystem error %d\n", retVal);
   }

   pDioData->state = closed;
}


void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      G_DioData.abort = TRUE;
   }
}


int main(int argc, char *argv[])
{
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};

   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DioData.board = params.board;
   G_DioData.handle = 0;
   G_DioData.port = 1;
   G_DioData.line = 0;
   G_DioData.abort = 0;
   G_DioData.state = closed;

   signal(SIGINT, SigInt);

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(DioEventExitHandler, &G_DioData);

   // initializes acquisition session
   InitDioEvent(&G_DioData);

   // run the acquisition
   DioEvent(&G_DioData);

   // Cleanup acquisition
   CleanUpDioEvent(&G_DioData);

   return 0;
}

