/*****************************************************************************/
/*                Asynchronous Digital input event example                   */
/*                                                                           */
/*  This example shows how to use the powerdaq API to set-up a PD2-MFx or    */
/*  PD2-AO board to generate an interrupt when the state of a digital input  */
/*  changes. The event coming from the board wakes-up the program, there is  */
/*  no polling invloved.                                                     */
/*  For the PD2-DIO boards look at the DIOEvent example.                     */
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


typedef enum _edgeSense
{
   rising,
   falling
} tEdgeSense;

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
   int line;                     // digital lines to use
   tEdgeSense edge;              // type of edge that will trigger the interrupt
   tState state;                 // state of the acquisition session
} tDiEventData;


int InitDiEvent(tDiEventData *pDiData);
int DiEvent(tDiEventData *pDiData);
void CleanUpDiEvent(tDiEventData *pDiData);


static tDiEventData G_DiData;

// exit handler
void DiEventExitHandler(int status, void *arg)
{
   CleanUpDiEvent((tDiEventData *)arg);
}


int InitDiEvent(tDiEventData *pDiData)
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
      printf("DiEvent: PdDInReset error %d\n", retVal);
      exit(EXIT_FAILURE);
   }

   return 0;
}


int DiEvent(tDiEventData *pDiData)
{
   int retVal;
   DWORD diCfg;
   DWORD status, level, latch;
   DWORD eventsToNotify = eDInEvent;
   DWORD eventsReceived;
   int risingEdges[8] = {DIB_0CFG0, DIB_1CFG0, DIB_2CFG0, DIB_3CFG0, DIB_4CFG0, DIB_5CFG0, DIB_6CFG0, DIB_7CFG0};
   int fallingEdges[8] = {DIB_0CFG1, DIB_1CFG1, DIB_2CFG1, DIB_3CFG1, DIB_4CFG1, DIB_5CFG1, DIB_6CFG1, DIB_7CFG1};

      
   // setup the board to generate an interrupt when the selected line
   // receives the specified edge
   if(pDiData->edge == rising)
      diCfg = risingEdges[pDiData->line];
   else
      diCfg = fallingEdges[pDiData->line];
   
   retVal = _PdDInSetCfg(pDiData->handle, diCfg);
   if (retVal < 0)
   {
      printf("DiEvent: _PdDInSetCfg error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   pDiData->state = configured;

   // event suff
   retVal = _PdAdapterEnableInterrupt(pDiData->handle, TRUE);
   if (retVal < 0)
   {
      printf("DiEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 

   // set events we want to receive
   retVal = _PdSetUserEvents(pDiData->handle, DigitalIn, eventsToNotify);
   if (retVal < 0)
   {
      printf("DiEvent: _PdSetUserEvents error %d\n", retVal);
      exit(EXIT_FAILURE);
   } 
   
   pDiData->state = running;

   eventsReceived = _PdWaitForEvent(pDiData->handle, eventsToNotify, 5000);
   
   if (eventsReceived & eTimeout)
   {
      printf("DiEvent: wait timed out\n"); 
      exit(EXIT_FAILURE);
   }

   if ( eventsReceived & eDInEvent )     // something has happend
   {
      _PdDInGetStatus(pDiData->handle, &status);

      _PdDInClearData(pDiData->handle);

      level = (status & 0xFF) >> pDiData->line;
      latch = (status & 0x00FF) >> (pDiData->line + 8);


      printf("DIEvent: event received! level %d, latch %d\n", level, latch);
   }


   return retVal;
}


void CleanUpDiEvent(tDiEventData *pDiData)
{
   int retVal;
      
   if(pDiData->state == running)
   {
      retVal = _PdAdapterEnableInterrupt(pDiData->handle, TRUE);
      if (retVal < 0)
         printf("DiEvent: _PdAdapterEnableInterrupt error %d\n", retVal);
         
      pDiData->state = configured;
   }

   if(pDiData->state == configured)
   {
      retVal = _PdDInReset(pDiData->handle);
      if (retVal < 0)
         printf("DiEvent: _PdDinReset error %d\n", retVal);

      pDiData->state = unconfigured;
   }

   if(pDiData->handle > 0 && pDiData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDiData->handle, DigitalIn, 0);
      if (retVal < 0)
         printf("DiEvent: PdReleaseSubsystem error %d\n", retVal);
   }

   pDiData->state = closed;
}


int main(int argc, char *argv[])
{
   PD_PARAMS params = {0, 1, {0}, 1000.0, 0, 4096};
   
   ParseParameters(argc, argv, &params);

   // initializes acquisition session parameters
   G_DiData.board = params.board;
   G_DiData.handle = 0;
   G_DiData.line = 0;
   G_DiData.edge = rising;
   G_DiData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(DiEventExitHandler, &G_DiData);

   // initializes acquisition session
   InitDiEvent(&G_DiData);

   // run the acquisition
   DiEvent(&G_DiData);

   // Cleanup acquisition
   CleanUpDiEvent(&G_DiData);

   return 0;
}

