//=======================================================================
//
// NAME:    pxi.c
//
// SYNOPSIS:
//
//      PXI releated functions
//
//
// DESCRIPTION:
//
//      This file contains PXI-releated functions (connect & disconnect lines)
//
// OPTIONS: none
//
//
// DIAGNOSTICS:
//
//
// NOTES:   See notice below.
//
// AUTHOR:  Sergey Svinolobov
//
// DATE:    14-SEP-01
//
//
// REV:     0.1
//
// R DATE:  14-SEP-01
//
// HISTORY:
//
//      Rev 0.1,      14-SEP-02,  S.S.,   Initial version.
//
//-----------------------------------------------------------------------
//
//      Copyright (C) 1998 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//-----------------------------------------------------------------------
//
// Notice:
//
//      To be included into user Visual C++ or Borland C++ application
//
//=======================================================================

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "../include/win_sdk_types.h"
#include "../include/powerdaq.h"
#include "../include/powerdaq32.h"
#include "../include/pd_debug.h"
#include "../include/pd_debug.h"
#include "../include/pxi.h"


// ======================================================================
// Module constants & variables

extern Adapter_Info* G_pAdapterInfo;
extern int G_NbBoards;

const PD_BOARD_PXI_LINES Board_PXI_Info[3] = {
    {
        atMF,
        {"Conv CLK In", "Chan CLK In", "Conv CLK Out", "Chan CLK Out", "Trig In"},
        {pxiInput, pxiInput, pxiOutput, pxiOutput, pxiInput}
    },
    {
        atPD2DIO,
        {"Conv CLK In", "Conv CLK Out", "Update In", "Ext CLK", "Ext Update In"},
        {pxiInOut, pxiInOut, pxiInput, pxiInput, pxiInput}
    },
    {
        atPD2AO,
        {"Conv CLK In", "Conv CLK Out", "Update In", "Ext CLK", "Ext Update In"},
        {pxiInOut, pxiInOut, pxiInput, pxiInput, pxiInput}
    },

};

// working state of the PXI lines
DWORD   dwPXIState[MAX_PXI_LINE] = {0};


//+
// ----------------------------------------------------------------------
// Function:    InternalGetAdapterInfo
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
// Returns:     Pointer to the AdapterInfo structure
//              just for PXI board, otherwise return NULL
//                              
//
// Description: 
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-
Adapter_Info* InternalGetAdapterInfo (int board)
{
    if ((board < G_NbBoards)  && (G_pAdapterInfo[board].atType))
    {
        // check for PXI board
        if (PD_IS_PDXI(G_pAdapterInfo[board].dwBoardID)) 
           return &G_pAdapterInfo[board];
    }

    return NULL;
}

//+
// ----------------------------------------------------------------------
// Function:    _PdPXIGetAdapterInfo
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
// Returns:     Pointer to the PXI info structure or NULL if not a PXI adapter
//                              
//
// Description: 
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-

PD_BOARD_PXI_LINES* _PdPXIGetAdapterInfo (int board)
{
    int id;

    // Check the adapter handle
    if (board < G_NbBoards) 
       return FALSE;

    if (PD_IS_PDXI(G_pAdapterInfo[board].dwBoardID))
    {
       id = G_pAdapterInfo[board].dwBoardID - 0x100;
       if (PD_IS_MF(id) || PD_IS_MFS(id))
       {
          return (pPD_BOARD_PXI_LINES) &Board_PXI_Info[0];
       }
       else if (PD_IS_DIO(id))
       {
          return (pPD_BOARD_PXI_LINES) &Board_PXI_Info[1];
       }
       else if ( PD_IS_AO(id) )
       {
          return (pPD_BOARD_PXI_LINES) &Board_PXI_Info[2];
       }
    }

    return NULL;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdPXISaveSettings
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//              
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Saves PXI setting to the EEPROM
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-
int _PdPXISaveSettings (int board)
{
    u32           i, dwResult;
    PD_EEPROM     Eeprom;
    PAdapter_Info pAdInfo;
    int           handle = 0;
    int           result;


    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Open calbration subsystem
    handle = PdAcquireSubsystem(board, CalDiag, 1);
    if (handle < 0) 
    {
        fprintf (stderr, "Error opening CalDiag subsys\n");
        return handle;
    }

    // Read EEPROM
    result = _PdAdapterEepromRead(handle, sizeof(PD_EEPROM)/2, (WORD*)&Eeprom, &dwResult);    
    if (result < 0) 
    {
       fprintf (stderr, "Error reading EEPROM\n");
       PdAcquireSubsystem(handle, CalDiag, 0);
       return result;
    }

    // Store values
    for (i=0; i < MAX_PXI_BOARD_LINE; i++)
       Eeprom.u.Header.PXI_Config[i] = pAdInfo->PXI_Config[i];

    // Write EEPROM
    result = _PdAdapterEepromWrite(handle, (WORD*)&Eeprom, sizeof(PD_EEPROM)/2);
    if (result < 0) 
    {
       fprintf (stderr, "Error writing EEPROM\n");
       PdAcquireSubsystem(handle, CalDiag, 0);
       return result;
    }

    // Close calbration subsystem
    result = PdAcquireSubsystem(handle, CalDiag, 0);
    if (result < 0) 
    {
       fprintf (stderr, "Error Closing CalDiag subsystem\n");
       return result;
    }

    return result;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdPXIRestoreSettings
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Restore PXI setting ftom the EEPROM and reconnect
//              all lines
//
// Notes:       
//
// ----------------------------------------------------------------------
//-
int _PdPXIRestoreSettings (int board)
{
    u32           i, dwResult;
    PD_EEPROM     Eeprom;
    PAdapter_Info pAdInfo;
    int           handle = 0;
    int           result;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Open calbration subsystem
    handle = PdAcquireSubsystem(board, CalDiag, 1);
    if (handle < 0) 
    {
        fprintf (stderr, "Error opening CalDiag subsys\n");
        return handle;
    }

    // Disconnect all lines
    for (i=0; i<MAX_PXI_BOARD_LINE; i++)
        _PdPXIDisconnectLine (board, i);
    
    // Read EEPROM
    result = _PdAdapterEepromRead(handle, sizeof(PD_EEPROM)/2, (WORD*)&Eeprom, &dwResult);    
    if (result < 0) 
    {
       fprintf (stderr, "Error reading EEPROM\n");
       PdAcquireSubsystem(handle, CalDiag, 0);
       return result;
    }

    // Copy PXI lines configuration
    for (i=0; i<MAX_PXI_BOARD_LINE; i++)
        pAdInfo->PXI_Config[i] = (char)Eeprom.u.Header.PXI_Config[i];

    // Close calbration subsystem
    result = PdAcquireSubsystem(handle, CalDiag, 0);
    if (result < 0) 
    {
       fprintf (stderr, "Error Closing CalDiag subsystem\n");
       return result;
    }

    // Connect all lines
    InternalPXIConnectAllLines(board);

    // Reconnect lines
    result = _PdPXIConnect (board);

    return result;
}

//+
//+
// ----------------------------------------------------------------------
// Function:    PdPXIConnect
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Makes a physical connection to PXI lines
//              
//
// Notes:       
//              
// ----------------------------------------------------------------------
//-
int _PdPXIConnect (int board)
{
    u32           dwValue;
    int           result = 0;
    int           i;
    PAdapter_Info pAdInfo;
    int           handleDin;
    int           handleDout;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Open digital subsystem
    handleDin = PdAcquireSubsystem(board, DigitalIn, 1);
    if (handleDin < 0) 
    {
        fprintf (stderr, "Error opening DigitalIn subsys\n");
        return handleDin;
    }
    handleDout = PdAcquireSubsystem(board, DigitalOut, 1);
    if (handleDout < 0) 
    {
        fprintf (stderr, "Error opening DigitalOut subsys\n");
        PdAcquireSubsystem(handleDin, DigitalIn, 0);
        return handleDout;
    }

    // Reset all connections
    result = _PdDIO256CmdRead(handleDin, PD_BrdPDXI, &dwValue);
    if (result < 0)
    {
        fprintf (stderr, "Error Reseting connections\n");
        PdAcquireSubsystem(handleDout, DigitalOut, 0);
        PdAcquireSubsystem(handleDin, DigitalIn, 0);
        return result;
    }

    // Store the new configuration to the adapters
    // code part for MF and MFS PDXI boards
    if ((PD_IS_MF(pAdInfo->dwBoardID) || PD_IS_MFS(pAdInfo->dwBoardID)))
    {
        for (i=0; i<MAX_PXI_BOARD_LINE; i++)
        {
            dwValue = pAdInfo->PXI_Config[i];

            if (dwValue == 0xFF) 
               dwValue = 0;

            result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, dwValue);
            if (result < 0)
            {
                fprintf (stderr, "Error writing connections\n");
                PdAcquireSubsystem(handleDout, DigitalOut, 0);
                PdAcquireSubsystem(handleDin, DigitalIn, 0);
                return result;
            }

            DPRINTK("Line = %d\nPXILine = %d\nResult = %d\n", i, dwValue, result);
        }
    } 
    else 
    // code part for AO and DIO PDXI boards
    {       
        // 1-st write (REG1 PXI configuration)
        dwValue = pAdInfo->PXI_Config[0] ^ 8;

        if ((char) pAdInfo->PXI_Config[2] == (char) 0xFF) 
           dwValue = dwValue & 0xFFFD;
        
        result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, dwValue);
        if (result < 0)
        {
           fprintf (stderr, "Error writing connections\n");
           PdAcquireSubsystem(handleDout, DigitalOut, 0);
           PdAcquireSubsystem(handleDin, DigitalIn, 0);
           return result;
        }


        DPRINTK("Config = %x\n", dwValue);
                
        // 2-nd write
        // check TMR2 line for output
        if ((dwValue & 0x0008) && (pAdInfo->PXI_Config[1] != 0xFF))
        {
           result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, pAdInfo->PXI_Config[1]);
        }
        else 
        {
           result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, 0);
        }
        if (result < 0)
        {
           fprintf (stderr, "Error writing connections\n");
           PdAcquireSubsystem(handleDout, DigitalOut, 0);
           PdAcquireSubsystem(handleDin, DigitalIn, 0);
           return result;
        }


        // 3-rd write
        if ((~dwValue & 8) && (pAdInfo->PXI_Config[1] != 0xFF)) 
           result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, pAdInfo->PXI_Config[1]);
        else 
           result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, 0);
        
        if (result < 0)
        {
           fprintf (stderr, "Error writing connections\n");
           PdAcquireSubsystem(handleDout, DigitalOut, 0);
           PdAcquireSubsystem(handleDin, DigitalIn, 0);
           return result;
        }

        // 4-th write
        result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, pAdInfo->PXI_Config[2]);
        if (result < 0)
        {
           fprintf (stderr, "Error writing connections\n");
           PdAcquireSubsystem(handleDout, DigitalOut, 0);
           PdAcquireSubsystem(handleDin, DigitalIn, 0);
           return result;
        }
 
        // 5-th write
        result = _PdDIO256CmdWrite(handleDout, PD_BrdPDXI, pAdInfo->PXI_Config[4]);
        if (result < 0)
        {
           fprintf (stderr, "Error writing connections\n");
           PdAcquireSubsystem(handleDout, DigitalOut, 0);
           PdAcquireSubsystem(handleDin, DigitalIn, 0);
           return result;
        }
    }

    // Close digital subsystems
    PdAcquireSubsystem(handleDout, DigitalOut, 0);
    PdAcquireSubsystem(handleDin, DigitalIn, 0);

    return 0;
}

//+
// ----------------------------------------------------------------------
// Function:    _PdPXIDisconnect
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: 
//              
//
// Notes:       
//              
// ----------------------------------------------------------------------
//-
int _PdPXIDisconnect (int board)
{
    u32         dwValue;
    PAdapter_Info pAdInfo;
    int result;
    int handleDin;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Open digital subsystem
    handleDin = PdAcquireSubsystem(board, DigitalIn, 1);
    if (handleDin < 0) 
    {
        fprintf (stderr, "Error opening DigitalIn subsys\n");
        return handleDin;
    }

    // Reset all connections
    result = _PdDIO256CmdRead(handleDin, PD_BrdPDXI, &dwValue);
    if (result < 0)
    {
        fprintf (stderr, "Error Reseting connections\n");
        PdAcquireSubsystem(handleDin, DigitalIn, 0);
        return result;
    }

    // Close digital subsystems
    PdAcquireSubsystem(handleDin, DigitalIn, 0);

    return 0;
} 

//+
// ----------------------------------------------------------------------
// Function:    _PdPXIConnectLine 
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//              DWORD  dwBoardLine -- Board line number
//              DWORD  dwPXILine   -- PXI line number
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: 
//              
//
// Notes:       
//              
// ----------------------------------------------------------------------
//-
int _PdPXIConnectLine (int board, u32 dwBoardLine, u32 dwPXILine)
{
    PAdapter_Info           pAdInfo;
    pPD_BOARD_PXI_LINES     pBoardLines;
    int                     iLineType;
    BOOL                    bIsMFBoard;
//  DWORD                   dwPrevConnection;

    // Check the adapter handle
//  if ( (dwPXILine >= MAX_PXI_LINE) || (dwBoardLine >= MAX_PXI_BOARD_LINE)) return FALSE;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Get the board PXI info
    if ((pBoardLines = _PdPXIGetAdapterInfo (board)) == NULL) 
       return -1;

    iLineType = pBoardLines->LineTypes[dwBoardLine];

    bIsMFBoard = PD_IS_MF(pAdInfo->dwBoardID) | PD_IS_MFS(pAdInfo->dwBoardID);

    // if it's try to connect two or more output lines prevent connection
    if (bIsMFBoard)
    {        
        if (((pBoardLines->LineTypes[dwBoardLine] != pxiInput) && (dwPXIState[dwPXILine] >= pxiOutput)) ||
            ((pBoardLines->LineTypes[dwBoardLine] != pxiInput) && (dwPXILine == PXI_STAR)))
           return FALSE;
    }
    // special part for AO and DIO adapters (for bi-directional lines)
    else
    {
        if ((dwBoardLine == 1) && (pAdInfo->PXI_Config [0] & 8))
        {
            iLineType = pxiOutput;
            if (dwPXIState[dwPXILine] >= pxiOutput) return FALSE;
        }
        else iLineType = pxiInput;
    }

    // Remove previous connection
/*  dwPrevConnection = pAdInfo->PXI_Config[dwBoardLine];
      if ( dwPrevConnection != 0xFF)
        if (dwPXIState[dwPrevConnection])
            dwPXIState[dwPrevConnection] -= pBoardLines->LineTypes[dwBoardLine]; */

    if ( (bIsMFBoard) || (dwBoardLine == 2))
         dwPXIState[dwPXILine] += (char) iLineType;

    if ((~bIsMFBoard) && ((dwBoardLine == 0) || (dwBoardLine == 4)))
        pAdInfo->PXI_Config [dwBoardLine] = (char) dwPXILine;
    else 
        pAdInfo->PXI_Config [dwBoardLine] = (char) dwPXILine | 8;

    return 0;
}

//+
// ----------------------------------------------------------------------
// Function:    _PdPXIDisconnectLine 
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//              DWORD  dwBoardLine -- Board line number
//              DWORD  dwPXILine   -- PXI line number
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: 
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-
int _PdPXIDisconnectLine (int board, u32 dwBoardLine)
{
    u32                 dwPXILine;                 
    PAdapter_Info       pAdInfo;
    pPD_BOARD_PXI_LINES pBoardLines;

    // Check the adapter handle
    if (dwBoardLine >= MAX_PXI_BOARD_LINE) 
       return -1;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Get the board PXI info
    if (( pBoardLines = _PdPXIGetAdapterInfo (board)) == NULL) 
       return -1;

    // Get PXI line number
    dwPXILine = pAdInfo->PXI_Config[dwBoardLine];

    // Remove connection into the internal variable
    if ((dwPXILine >=0) && (dwPXILine < MAX_PXI_LINE) && (dwPXIState[dwPXILine] > 0))
        dwPXIState[dwPXILine] -= pBoardLines->LineTypes[dwBoardLine];

    // Remove connection into the EEPROM space
    pAdInfo->PXI_Config [dwBoardLine] = (char) 0xFF;

    return 0;
}

//+
// ----------------------------------------------------------------------
// Function:    _PdPXIGetLineState 
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//              DWORD  dwBoardLine -- Board line number
//              DWORD* dwLineState -- connected PXI line number 
//                                    or 0xFF if not connected
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Returns status of the PXI line
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-

int _PdPXIGetLineState (int board, u32 dwBoardLine)
{
    PAdapter_Info pAdInfo;

    // Check the adapter handle
    if (dwBoardLine >= MAX_PXI_BOARD_LINE) 
       return 0xFF;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return 0xFF;
    
    // Remove connection
    if (pAdInfo->PXI_Config [dwBoardLine] == 0xFF)
        return pAdInfo->PXI_Config [dwBoardLine];
    else 
        if ((~ (PD_IS_MF(pAdInfo->dwBoardID) || PD_IS_MFS(pAdInfo->dwBoardID))) && 
            ((dwBoardLine == 0) || (dwBoardLine == 4)))
            return pAdInfo->PXI_Config [dwBoardLine];
        else 
            return pAdInfo->PXI_Config [dwBoardLine]^8;
}

//+
// ----------------------------------------------------------------------
// Function:    InternalPXIConnectAllLines
//
// Parameters:  HANDLE hAdapter    -- handle to adapter
//
//
// Description: Connect all PXI lines (for internal settings)
//              
//
// Notes:       
//
// ----------------------------------------------------------------------
//-
int InternalPXIConnectAllLines(int board)
{
    int i;
    PAdapter_Info pAdInfo;

    // Get pointer to adapter info structure
    if ((pAdInfo = InternalGetAdapterInfo(board)) == NULL) 
       return -1;

    // Connect lines
    for (i=0; i<MAX_PXI_BOARD_LINE; i++)
        _PdPXIConnectLine (board, i, pAdInfo->PXI_Config[i]);

    return 0;
}
 
