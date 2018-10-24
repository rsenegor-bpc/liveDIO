//=======================================================================
//
// NAME:    pwrdaqes.c
//
// DESCRIPTION:
//
//      PowerDAQ DLL Low-Level Driver Interface functions.
//                       (ESSI port)
// NOTES:  
//
// AUTHOR:  dk
//
// DATE:    07-JUN-2001
//
//
// HISTORY:
//
//      Rev 0.1,    07-JUN-2001,  dk , Initial version
//      Rev 0.2,    24-AUG-2001,  dk , In PdEssiRegWrite end PdEssiRegRead  no support move
//                                     pdwValue to left/rait. (in rev 0.1 - If number of 
//                                     bits/word (WL[2:0]) = 8, 12, 16, that it have to move 
//                                     pdwValue on 16, 12, 8 bit to left/right
//
// ----------------------------------------------------------------------
//
//      Copyright (C) 2001 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//=======================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "../include/win_sdk_types.h"
#include "../include/powerdaq.h"
#include "../include/powerdaq32.h"
#include "../include/pd_dsp_es.h"

//-----------------------------------------------------------------------
// Function:    BOOL CALLBACK PdEssiReset(HANDLE hAdapter, DWORD *pError)
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
// Returns:     BOOL status     -- TRUE:  ESSI reset succeeded
//                                 FALSE: ESSI reset failed
// Description: Reset ESSI. 
//
// Notes:      
//-----------------------------------------------------------------------
int _PdEssiReset(int hAdapter)
{
    int retVal;

    retVal = _PdDspRegWrite(hAdapter, M_PCRC, M_PCRC0); //PCRC
    if(retVal < 0)
        return retVal; 
    retVal = _PdDspRegWrite(hAdapter, M_PRRC, M_PRRC0); //PRRC
    if(retVal < 0)
        return retVal; 
    retVal = _PdDspRegWrite(hAdapter, M_PDRC, M_PDRC0); //PDRC
    if(retVal < 0)
        return retVal; 
    
    retVal = _PdDspRegWrite(hAdapter, M_PCRD, M_PCRD0); //PCRD
    if(retVal < 0)
        return retVal; 
    retVal = _PdDspRegWrite(hAdapter, M_PRRD, M_PRRD0); //PRRD
    if(retVal < 0)
        return retVal; 
    retVal = _PdDspRegWrite(hAdapter, M_PDRD, M_PDRD0); //PDRD
    if(retVal < 0)
        return retVal; 
    
    return TRUE;
}

//-----------------------------------------------------------------------
// Function:    BOOL CALLBACK PdEssi0PinsEnable(HANDLE hAdapter, DWORD *pError)
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
// Returns:     BOOL status     -- TRUE:  ESSI0 enable succeeded
//                                 FALSE: ESSI0 enable failed
// Description: ESSI0 pins enable 
// Notes:      
//-----------------------------------------------------------------------
int _PdEssi0PinsEnable(int hAdapter)
{
    // ESSI0 Pins Enable
    return _PdDspRegWrite(hAdapter, M_PCRC, 0x2F); //PCRC
}

//-----------------------------------------------------------------------
// Function:    BOOL CALLBACK PdEssi1PinsEnable(HANDLE hAdapter, DWORD *pError)
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
// Returns:     BOOL status     -- TRUE:  ESSI1 enable succeeded
//                                 FALSE: ESSI1 enable failed
// Description: ESSI1 pins enable 
// Notes:      
//-----------------------------------------------------------------------
int _PdEssi1PinsEnable(int hAdapter)
{
    // ESSI1 Pins Enable	
    return _PdDspRegWrite(hAdapter, M_PCRD, 0x2F); //PCRD
}

//-----------------------------------------------------------------------
// Function:    BOOL CALLBACK PdEssiRegWrite(HANDLE hAdapter, DWORD *pError,
//					  DWORD dwReg, DWORD dwValue);
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//		DWORD dwReg     -- register to write
//              DWORD *pdwValue -- write value
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: write the data into ESSI registers
//
// Notes:
//
//-----------------------------------------------------------------------
int _PdEssiRegWrite(int hAdapter, DWORD dwReg, DWORD dwValue)
{	
    return _PdDspRegWrite(hAdapter, dwReg, dwValue);
}

//-----------------------------------------------------------------------
// Function:    BOOL CALLBACK PdEssiRegRead(HANDLE hAdapter, DWORD *pError, 
// 							  DWORD dwReg, DWORD *pdwValue)
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//		DWORD dwReg     -- register to read
//              DWORD *pdwValue -- read value
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: read the data from ESSI registers
//
// Notes: 
//	
//-----------------------------------------------------------------------
int _PdEssiRegRead(int hAdapter, DWORD dwReg, DWORD *pdwValue)
{	
    return _PdDspRegRead(hAdapter, dwReg, pdwValue); 
}
