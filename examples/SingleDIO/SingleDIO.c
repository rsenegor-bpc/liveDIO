/******************************************************************/
/*  PD2-DIO-128I/MT test                                          */
/*  R. Senegor, October 2018                                      */
/*  BluePoint Controls                                            */
/*                                                                */
/*  EXTERNAL LOOPBACK FIXTURE REQUIRED FOR SUCCESS                */
/*                                                                */
/*  This file will allow the user to individually test            */
/*  UEI PD2-DIO-128I/MT cards to verify DIO operation.            */
/*                                                                */
/*  It does so by addressing and exciting each DIO port           */
/*  via nested loops. During operation all LEDs should            */
/*  incrementally illuminate then turn off.                       */
/*                                                                */
/*  Software adapted from United Electronic Industries, Inc. test */
/******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
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

typedef struct _singleDioData
{
   int board;                    // board number to be used for the AI operation
   int handle;                   // board handle
   int nbOfPorts;                // nb of ports of 16 lines to use
   unsigned char OutPorts;       // mask for ports of 16 lines to use for output
   int nbOfSamplesPerPort;       // number of samples per port
   unsigned long portList[64];
   double scanRate;              // sampling frequency on each port
   tState state;                 // state of the acquisition session
} tSingleDioData;

int InitSingleDIO(tSingleDioData *pDioData);
int SingleDIO(tSingleDioData *pDioData);
void CleanUpSingleDIO(tSingleDioData *pDioData);


static tSingleDioData G_DioData;

// exit handler
void SingleDIOExitHandler(int status, void *arg)
{
   CleanUpSingleDIO((tSingleDioData *)arg);
}


int InitSingleDIO(tSingleDioData *pDioData)
{
   Adapter_Info adaptInfo;
   int retVal = 0;

   // get adapter type
   retVal = _PdGetAdapterInfo(pDioData->board, &adaptInfo);
   if (retVal < 0)
   {
      return 1;
   }

   if(adaptInfo.atType & atPD2DIO) {
   }
   else
   {
      return 1;
   }

   pDioData->handle = PdAcquireSubsystem(pDioData->board, DigitalIn, 1);
   if(pDioData->handle < 0)
   {
      return 1;
   }

   pDioData->state = unconfigured;

   retVal = _PdDIOReset(pDioData->handle);
   if (retVal < 0)
   {
      return 1;
   }

   return 0;
}


int SingleDIO(tSingleDioData *pDioData)
{
   int retVal;
   int i, j, failFlag=0;
   DWORD readVal, writeVal;
   int count = 0;

   system ("dialog --title 'PD2-DIO-128I/MT Test' --infobox 'Running DIO test\n\nLEDs will flash in sequence...\n\nTest will take approx. 10 minutes\n\nDo not power off or unplug.' 15 40");

   /*Header on log file (including current time and date)*/
   
   
   retVal = _PdDIOEnableOutput(pDioData->handle, pDioData->OutPorts);
   if(retVal < 0)
   {
      failFlag = 1;
   }
   
   pDioData->state = configured;

   
   pDioData->state = running;

   //write 0 to all 4 DIO lines to make sure all DO ports are off (start at 0)

   for(j=0;j<pDioData->OutPorts;j++)
   {
      retVal = _PdDIOWrite(pDioData->handle, j, 0);
   }

   for(i=0; i<pDioData->nbOfSamplesPerPort; i++)
   {
      for(j=0; j<pDioData->OutPorts; j++)
      {
         if(count < pDioData->nbOfSamplesPerPort)
         {
            writeVal = count;
            retVal = _PdDIOWrite(pDioData->handle, j, writeVal);
            if(retVal < 0)
            {
               failFlag = 1;
            }
         }
         else
         {
            count=0;
         }
         usleep(1000);     //Microseconds between each IO stream set
         retVal = _PdDIORead(pDioData->handle, j, &readVal);
         if(retVal < 0)
         {
            failFlag = 1;
         }

     /*Normalize read/write values to remove extraneous bits*/
            readVal = 0xff0000 | readVal;
            writeVal = 0xff0000 | writeVal;

           if((readVal != writeVal))         
           {
            failFlag = 1;
           }

      }

      count++;

      usleep(1.0E3/pDioData->scanRate);
   }

   /*Turn all LEDs off after test*/

   system ("dialog --title 'PD2-DIO-128I/MT Test' --infobox 'Running DIO test\n\nPlease wait for LEDs to turn off...' 10 25");

   for(j=0;j<pDioData->OutPorts;j++)
   {
      retVal = _PdDIOWrite(pDioData->handle, j, 0);
   }

   return failFlag;
}


void CleanUpSingleDIO(tSingleDioData *pDioData)
{
   int retVal;
      
   if(pDioData->state == running)
   {
      pDioData->state = configured;
   }

   if(pDioData->state == configured)
   {
      retVal = _PdDIOReset(pDioData->handle);
      if (retVal < 0)
      pDioData->state = unconfigured;
   }

   if(pDioData->handle > 0 && pDioData->state == unconfigured)
   {
      retVal = PdAcquireSubsystem(pDioData->handle, DigitalIn, 0);
   }

   pDioData->state = closed;
}


int main(int argc, char *argv[])
{

 int restartFlag=1;

 int i,k,j, failFlag[3] = {0,0}, initFlag=1;

//Default parameters
   PD_PARAMS params = {0, 4, {0,1,2,3}, 5.0, 0};

   int statusStart = system ("dialog --title 'PD2-DIO-128I/MT Test' --yesno 'Start DIO test?' 10 25");

   ParseParameters(argc, argv, &params);

 while(1) {

if((!statusStart && initFlag) | (!restartFlag && initFlag)) {            //if valid entry to start test and no initialization OR if we've recently restarted the test with bad initialization
   
   j=-1;   

   // initializes acquisition session parameters
   G_DioData.board = params.board;
   G_DioData.nbOfPorts = params.numChannels;
   for(i=0; i<params.numChannels; i++)
   {
       G_DioData.portList[i] = params.channels[i];
   }

   G_DioData.handle = 0;
   G_DioData.OutPorts = params.numChannels; // use ports 1 and 2 for output
   G_DioData.nbOfSamplesPerPort = 65536;
   G_DioData.scanRate = params.frequency;
   G_DioData.state = closed;

   // setup exit handler that will clean-up the acquisition session
   // if an error occurs
   on_exit(SingleDIOExitHandler, &G_DioData);

while(j<4 && initFlag) {      //Limit how many values of j (device number) we test. InitSingleDIO returns 1 on fail. While both conditions are true, try the next device number for success.

   j++;   //Next device number

   PD_PARAMS params = {j, 4, {0,1,2,3}, 5.0, 0};  //Format device parameters

   ParseParameters(argc, argv, &params);      //Parse parameters to prepare for next InitSingleDIO

   initFlag = InitSingleDIO(&G_DioData);

}
}

   if((statusStart != 0) ^ (!restartFlag)) {
           restartFlag = system ("dialog --title 'DIO Test' --yesno 'Test not run.\n\nUser quit.\n\nRestart test?' 10 25");
	   if(!restartFlag) {continue;}
           if(restartFlag) {break;}
   }

   if(!restartFlag) {
      failFlag[0] = 0, failFlag[1] = 0;
   }

   if(!initFlag) {

      for(k=0;k<2;k++) {

         // run the acquisition
         failFlag[k] = SingleDIO(&G_DioData);

         // Cleanup acquisition
         CleanUpSingleDIO(&G_DioData);

         usleep(1000000);

      }
   }



     if(initFlag && statusStart == 0) {
        restartFlag = system ("dialog --title 'DIO Test' --yesno 'Test not run.\n\nBoard not detected.\n\nRestart test?' 10 25");

        if(restartFlag) {break;}
}

   if(!initFlag && !failFlag[0] && !failFlag[1]) {

      restartFlag = system ("dialog --title 'DIO Test complete!' --yesno 'PASS\n\nRestart test?' 10 25");
         if(restartFlag) {break;}
   }

   else {
            restartFlag = system ("dialog --title 'DIO Test complete!' --yesno '\n\nFAIL\n\nPlease contact BluePoint Controls: support@bluepointcontrols.com\n\nRestart test?' 15 40");

            if(restartFlag) {break;}
   }


     
   system("clear");
 }
   system("clear");
   return 0;
}
