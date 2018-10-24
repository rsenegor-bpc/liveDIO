//===========================================================================
//
// NAME:    DSPUCTFn.c
//
// DESCRIPTION:
//
//          This file contains implementations for the PowerDAQ DSP
//          Counter/Timer access
//
// AUTHOR:  Dennis L. Kraplin
//
// DATE:    02-FEB-2001
//
// HISTORY:
//
//      Rev 0.1,     02-FEB-2001,  D.K.,   Initial version.
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 2001 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================


//---------------------------------------------------------------------------
//
// PowerDAQ Hardware / Firmware Definitions.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "../include/win_sdk_types.h"
#include "../include/powerdaq.h"
#include "../include/powerdaq32.h"
#include "../include/pd_dsp_ct.h"

           
//---------------------------------------------------------------------------
//
// Function Prototypes:
//

// Load all registers of the selected counter
int _PdDspCtLoad(int hAdapter,
                    DWORD dwCounter, 
                    DWORD dwLoad, 
                    DWORD dwCompare, 
                    DWORD dwMode, 
                    DWORD dwReload, 
                    DWORD dwInverted,
                    DWORD dwUsePrescaler)
{
   int dwError;
   DWORD dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
        
    // Read current status value register 
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            &dwTCSR);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    // Leave Timer/Interrupt enable bits
    dwTCSR = dwTCSR & (M_TE | M_TCIE | M_TOIE);

    // Program counter
    // Load register
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetLoadAddr(dwCounter),
            dwLoad);
    if (dwError < 0) printf("_PdDspRegWrite error\n");
    // Compare register
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetCompareAddr(dwCounter),
            dwCompare);
    if (dwError < 0) printf("_PdDspRegWrite error\n");
    if (dwReload) 
        dwTCSR = dwTCSR | M_TRM | dwMode ;
    else
        dwTCSR = dwTCSR | dwMode;
    if (dwInverted) dwTCSR = dwTCSR | M_INV ;
    if (dwUsePrescaler) dwTCSR = dwTCSR | M_PCE ;
    // Control register
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            dwTCSR);
    if (dwError<0) printf("_PdDspRegWrite error\n");
    return(dwError);    
}


// Enable(1)/Disable(x) counting for the selected counter 
int _PdDspCtEnableCounter(int hAdapter,DWORD dwCounter, DWORD dwEnable)
{
   int dwError;
   DWORD dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
        
    // Read current status value register 
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            &dwTCSR);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    if (dwEnable) 
      dwTCSR = dwTCSR | M_TE;
    else
      dwTCSR = dwTCSR & (~M_TE);

    // Update control register
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            dwTCSR);
    if (dwError < 0) printf("_PdDspRegWrite error\n");
    return(dwError); 
}
   
// Enable(1)/Disable(x) interrupts for the selected events for the selected 
// counter (only one event can be enabled at the time)
int _PdDspCtEnableInterrupts(int hAdapter,DWORD dwCounter, DWORD dwCompare, DWORD dwOverflow)
{
   int dwError;
   DWORD dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Check if both interrupts enabled at the same time
    if ((dwCompare) && (dwOverflow)) return(-1);
        
    // Read current status value register 
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            &dwTCSR);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    if (dwCompare) dwTCSR = dwTCSR | M_TCIE;
    if (dwOverflow) dwTCSR = dwTCSR | M_TOIE;

    // Update control register
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            dwTCSR);
    if (dwError < 0) printf("_PdDspRegWrite error\n");
    return(dwError); 
}

// Get count register value from the selected counter
int _PdDspCtGetCount(int hAdapter,DWORD dwCounter, DWORD *dwCount)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetCountAddr(dwCounter),
            dwCount);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    return(dwError); 
}

// Get compare register value from the selected counter
int _PdDspCtGetCompare(int hAdapter,DWORD dwCounter, DWORD *dwCompare)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetCompareAddr(dwCounter),
            dwCompare);
    if (dwError) printf("_PdDspRegRead error\n");

    return(dwError); 
}

// Get control/status register value from the selected counter
int _PdDspCtGetStatus(int hAdapter,DWORD dwCounter, DWORD *dwStatus)
{
   int dwError;
   
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegRead(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            dwStatus);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    return(dwError); 
}

// Set compare register value from the selected counter
int _PdDspCtSetCompare(int hAdapter,DWORD dwCounter, DWORD dwCompare)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetCompareAddr(dwCounter),
            dwCompare);
    if (dwError < 0) printf("_PdDspRegWrite error\n");

    return(dwError); 
}

// Set load register value from the selected counter
int _PdDspCtSetLoad(int hAdapter,DWORD dwCounter, DWORD dwLoad)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetLoadAddr(dwCounter),
            dwLoad);
    if (dwError < 0) printf("_PdDspRegWrite error\n");

    return(dwError); 
}
// Set control/status register value from the selected counter
int _PdDspCtSetStatus(int hAdapter,DWORD dwCounter, DWORD dwStatus)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = _PdDspRegWrite(
            hAdapter, 
            _PdDspCtGetStatusAddr(dwCounter),
            dwStatus);
    if (dwError < 0) printf("_PdDspRegWrite error\n");

    return(dwError); 
}


// Load prescaler
int _PdDspPSLoad(int hAdapter, DWORD dwLoad, DWORD dwSource)
{
   int dwError;
   DWORD dwTPLR;

    dwTPLR = dwLoad | dwSource ;
    // Program prescaler
    dwError = _PdDspRegWrite(
            hAdapter, 
            M_TPLR,
            dwTPLR);
    if (dwError < 0) printf("_PdDspRegWrite error\n");
    return(dwError);
}

// Get prescaler count register value 
int _PdDspPSGetCount(int hAdapter, DWORD *dwCount)
{
   int dwError;
    
    // Read value
    dwError = _PdDspRegRead(
            hAdapter, 
            M_TPCR,
            dwCount);
    if (dwError < 0) printf("_PdDspRegRead error\n");

    return(dwError); 
}

// Get address of the count register of the selected counter
DWORD _PdDspCtGetCountAddr(DWORD dwCounter)
{
   DWORD dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the load register of the selected counter
DWORD _PdDspCtGetLoadAddr(DWORD dwCounter)
{
   DWORD dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TLR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TLR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TLR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the control/status register of the selected counter
DWORD _PdDspCtGetStatusAddr(DWORD dwCounter)
{
   DWORD dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(0);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCSR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCSR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCSR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the compare register of the selected counter
DWORD _PdDspCtGetCompareAddr(DWORD dwCounter)
{
   DWORD dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCPR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCPR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCPR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// end of DSPUCTFn.c

