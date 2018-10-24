//=================================================================================
//
// PROGECT:	SimpleAInEvt
//
// NAME:    SimpleAIn.c
//
// DESCRIPTION:   Simple Analog Input (PD2-MFx, PDL-MF boards) example with events.
//                This example uses eScanDone event to take the data from PowerDAQ board. 
//                   
//
// NOTES:
//
// AUTHOR:  V.S.
//
// DATE:    21-DEC-1999, V.S.
//
// REV:     1.0.0
//
// R DATE:  21-DEC-1999
//
// HISTORY:
//
//      Rev 1.0.0,    21-DEC-1999,  V.S.,   Initial version written.
//      Rev 1.0.1,    20-FEB-2002,  d.k.,   Check adapter type.
//      Rev 2.0.0,    19-SEP-2003,  d.k.,   Check adapter type.
//
//
//---------------------------------------------------------------------------------
//
//      Copyright (C) 1998-2003 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//=================================================================================
//
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
#include "pdfw_def.h" 
#include "pd_hcaps.h"

#define AI_CHLIST_MAX_SIZE	  32		//Max number of channels
#define NUM_CURRENT_ADAPTER	  0		//Logical number od PowerDAQ adapter 0,1,2,3....
#define BUFFER_SIZE		  1024		//Requested samples (buffer size)


#define BASECLOCK             33000000              // 33MHz
#define FRAI                  10                  // Channel list clock,Hz
#define AIN_RATE_DIVISOR      (BASECLOCK/FRAI)-1     


typedef enum _state
{
    closed,
    unconfigured,
    configured,
    running
} tState;

typedef struct _SimpleAIn
{
    int board;                    // board number to be used for the AI operation
    int handle;                   // board handle
    int adapterType;
    int abort;
    tState state;                 // state of the acquisition session
}tSimpleAIn;

static tSimpleAIn G_SimpleAInData;


WORD    AInBuffer[BUFFER_SIZE];         // Analog input buffer
DWORD   dwNumAdapters;                  // number of adapters installed

DWORD   dwSamples;              // Number of returned samples

// Filling channel list
DWORD   dwAInChList[AI_CHLIST_MAX_SIZE] = 
{0,1,2,3,
    0,1,2,3,
    0,1,2,3,
    0,1,2,3,
    0,1,2,3,
    0,1,2,3,
    0,1,2,3,
    0,1,2,3};

int InitSimpleAIn(tSimpleAIn *pSimpleAInData,DWORD dwAInCfg);
void CleanUpSimpleAIn(tSimpleAIn *pSimpleAInData);
int SimpleAIn(tSimpleAIn *pSimpleAInData,DWORD dwAInCfg);
void SimpleAInExitHandler(int status, void *arg);


int InitSimpleAIn( tSimpleAIn *pSimpleAInData,DWORD dwAInCfg)   
{
    Adapter_Info AdapterInfo;
    int retVal = 0;

    //Get adapter information
    retVal =_PdGetAdapterInfo(pSimpleAInData->board, &AdapterInfo);
    if (retVal < 0)
    {
        printf("_PdGetAdapterInfo error #%d\n",retVal);
        exit(EXIT_FAILURE);
    }

    //Check for the adapter type
    if (!(AdapterInfo.atType & atPDLMF))
    {
        printf("\nThis program for PowerDAQ PDL-MFx boards only !!!\n\n");  
        exit(EXIT_FAILURE);
    }

    pSimpleAInData->handle = PdAcquireSubsystem(pSimpleAInData->board,AnalogIn,1);
    if (pSimpleAInData->handle < 0)
    {
        printf("PdAcquireSubsystem failed\n");
        exit(EXIT_FAILURE);
    }

    pSimpleAInData->state = unconfigured;

    //Reset AIn subsystem
    retVal =  _PdAInReset(pSimpleAInData->handle);
    if (retVal < 0 )
    {
        printf("PdAInReset %d.\n", retVal);
        exit(EXIT_FAILURE);
    }

    return 0;
}


int SimpleAIn(tSimpleAIn *pSimpleAInData,DWORD dwAInCfg)
{
    int retVal = 0;
    unsigned int dwEvents;
    unsigned int dwEventsNotify = eScanDone | eTimeout;
    unsigned int dwAInEvents;
    int i=0 ;
    int ct;
    double fDataV;


    //Set AIn Configuration
    retVal = _PdAInSetCfg(pSimpleAInData->handle,dwAInCfg,0,0);
    if (retVal < 0)
    {
        printf("PdAdapterAcquireSubsystem Open Analog Input failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }

    //Set Analog Input Channel List
    retVal = _PdAInSetChList(pSimpleAInData->handle, AI_CHLIST_MAX_SIZE, dwAInChList);
    if (retVal < 0)
    {
        printf("PdAInSetChList failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }
    //Small puse when process will initiated
    usleep(100);

    retVal = _PdAInSetClClk(pSimpleAInData->handle,AIN_RATE_DIVISOR);
    if (retVal < 0)
    {
        printf("_PdAInSetClClk failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }
    // set events we want to receive
    retVal = _PdSetUserEvents(pSimpleAInData->handle, AnalogIn, dwEventsNotify);
    if (retVal < 0)
    {
        printf("_PdSetUserEvents failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }
    // event suff
    retVal = _PdAdapterEnableInterrupt(pSimpleAInData->handle, TRUE);
    if (retVal < 0)
    {
        printf("_PdAdapterEnableInterrupt failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }

    //Perform acquisition  
    retVal = _PdAInEnableConv(pSimpleAInData->handle, TRUE );
    if (retVal < 0)
    {
        printf("PdAInEnableConv failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }
    //Start SW trigger
    retVal = _PdAInSwStartTrig(pSimpleAInData->handle);
    if (retVal < 0)
    {
        printf("PdAInSwStartTrig failed with %d.\n", retVal);
        exit(EXIT_FAILURE);
    }

    printf("\nRequested samples (buffer size): %d\n\n", BUFFER_SIZE);
    printf("# eScanDone\tAval samples\tch0\tch1\tch2\tch3\n");
    printf("--------------------------------------------------------------\n");
    while (!pSimpleAInData->abort)
    {
        // Wait for interrupt.
        dwEvents = _PdWaitForEvent(pSimpleAInData->handle, dwEventsNotify, 1000);
        if (!(dwEvents & eTimeout))
        {
            if ((retVal = _PdGetUserEvents(pSimpleAInData->handle, AnalogIn, &dwAInEvents)) < 0)
                printf("_PdGetUserEvents failed with %d.\n", retVal);

            if (dwEvents & eScanDone)
            {
                //Get AIn data
                if ((retVal = _PdAInGetSamples(pSimpleAInData->handle, BUFFER_SIZE ,AInBuffer, &dwSamples))<0)
                    printf("PdAInSwStartTrig failed with %d.\n", retVal);
                // Print first 4 entries in channel list (or whole channel list if < 4)
                printf("%6d\t\t%6d\t\t",i++,dwSamples);
                for (ct=0;ct<((AI_CHLIST_MAX_SIZE<4)?AI_CHLIST_MAX_SIZE:4);ct++)
                {
                    //Convert RAW Data to Voltage values using current ANalogIn Cfg
                    if ((retVal = PdAInRawToVolts(pSimpleAInData->board, dwAInCfg, &AInBuffer[ct],&fDataV,1)) < 0)
                    {
                        printf("PdAInRawToVolts failed with %d\n", retVal);
                    }
                    printf("%2.1f\t",fDataV); //Show the data   		 
                }
                printf("\r");                   
            }
            //if (dwSamples != AI_CHLIST_MAX_SIZE) 
                printf("\n");
        } 
        
        if(dwEvents & eTimeout)
        {
            printf(".");   // Timed out.
        }

        fflush(stdout);

        if ((retVal = _PdSetUserEvents(pSimpleAInData->handle,AnalogIn, dwEventsNotify))< 0)
                printf("_PdSetUserEvents failed with %d.\n", retVal);
    }

    return 0;
}

void CleanUpSimpleAIn(tSimpleAIn *pSimpleAInData)  
{
    int retVal = 0;

    if (pSimpleAInData->state == running)
    {
        pSimpleAInData->state = configured;
    }

    if (pSimpleAInData->state == configured)
    {
        //Stop SW trigger
        retVal = _PdAInSwStopTrig(pSimpleAInData->handle);
        if (retVal < 0)
        {
            printf("PdAInSwStopTrig failed with %d.\n", retVal);    
            exit(EXIT_FAILURE);
        }
        //Stop acquisition
        retVal = _PdAInEnableConv(pSimpleAInData->handle,FALSE);
        if (retVal < 0)
        {
            printf("PdAInEnableConv failed with %d.\n", retVal);
            exit(EXIT_FAILURE);
        }
        // Clear user events
        retVal = _PdClearUserEvents(pSimpleAInData->handle, AnalogIn, eScanDone);
        if (retVal < 0)
        {
            printf("_PdClearUserEvents failed with %d.\n", retVal);     
            exit(EXIT_FAILURE);
        }
    }
    //Close AIn subsystem
    if (pSimpleAInData->handle > 0 && pSimpleAInData->state == unconfigured)
    {
        retVal = PdAcquireSubsystem(pSimpleAInData->handle,AnalogIn, 0);
        if (retVal < 0)
        {
            printf("PdAdapterAcquireSubsystem Close Analog Input failed with %d.\n", retVal);
            exit(EXIT_FAILURE);
        }
    }

    pSimpleAInData->state = closed;
}



//Signal Crtl-C
void SigInt(int signum)
{
    if (signum == SIGINT)
    {
        printf("Ctrl+C detected. acquisition stopped\n");
        G_SimpleAInData.abort = TRUE;
    }
}

// exit handler
void SimpleAInExitHandler(int status, void *arg)
{
    CleanUpSimpleAIn((tSimpleAIn*)arg);

}


//Main procedure

int main(int argc, char *argv[])
{
    DWORD dwAInCfg = 0;
    int i = 0;

    G_SimpleAInData.handle = 0;
    G_SimpleAInData.board = 1;
    G_SimpleAInData.abort = 0;

    for (i=0;i<AI_CHLIST_MAX_SIZE;i++)
        dwAInChList[i] = dwAInChList[i]+(1<<8);


    dwAInCfg |=       AIB_INPTYPE           // Bipolar Voltage Type
                      | AIB_INPRANGE          // HigVoltage Range 10V
                      //    | AIB_INPMODE           // Differential/Single-Ended)
                      | AIB_INTCVSBASE        // AIn Internal Conv Start Clk Base
                      | AIB_INTCLSBASE        // AIn Internal Ch List Start Clk Base
                      | AIB_CLSTART0          // CL Clock internal
                      | AIB_CVSTART0          // CV Clock Continuous
                      | AIB_CVSTART1;

    printf("PowerDAQ Test Application\n");
    printf("Analog Input (PD2-MFx, PDL-MF boards) example. (C)UEI, 2003\n");

    signal(SIGINT,SigInt);

    on_exit(SimpleAInExitHandler,&G_SimpleAInData);

    //Initializes acquisition session
    InitSimpleAIn(&G_SimpleAInData,dwAInCfg);

    //run the acquisition
    SimpleAIn(&G_SimpleAInData,dwAInCfg);

    //Cleanup acquisition
    CleanUpSimpleAIn(&G_SimpleAInData);

    return 0;   
}

