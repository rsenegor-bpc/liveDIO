unit pd32hdr;

// --------------------------------------------------------
// converted from D:\Work\Delphi\headerconvertor\pd32hdr.h
// 4/13/00 1:29:44 PM
// --------------------------------------------------------
//===========================================================================
//
// NAME:    pd32hdr.h
//
// DESCRIPTION:
//
//          PowerDAQ Win32 DLL header file
//
//          Definitions for PowerDAQ DLL driver interface functions.
//
// AUTHOR:  Alex Ivchenko
//
// DATE:    22-DEC-99
//
// REV:     0.11
//
// R DATE:  12-MAR-2000
//
// HISTORY:
//
//      Rev 0.1,     26-DEC-99,  A.I.,   Initial version.
//      Rev 0.11,    12-MAR-2000,A.I.,   DSP reghister r/w added
//
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 1999,2000 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================

interface

{$IFNDEF _INC_PWRDAQ32HDR}
  {$DEFINE _INC_PWRDAQ32HDR}
{$ENDIF}

uses
  windows,pwrdaq;

  // move this constant below uses clause
const
  DLLName = 'pwrdaq32.dll';


// Adapter information structure ---------------------------------------
type
  DWORD = Cardinal;

  EVENT_PROC = procedure(pAllEvents : PPD_ALL_EVENTS); stdcall;
  PD_EVENT_PROC = ^EVENT_PROC;

  PPD_ADAPTER_INFO = ^PD_ADAPTER_INFO;
  PD_ADAPTER_INFO = record
    dwAdapterId : DWORD;
    hAdapter : THANDLE;
    bTerminate : BOOL;
    hTerminateEvent : THANDLE;
    hEventThread : THANDLE;
    dwProcessId : DWORD;
    pfn_EventProc : PD_EVENT_PROC;
    csAdapter : THANDLE;
  end;
var
  _PD_ADAPTER_INFO : PD_ADAPTER_INFO;

//
// Private functions
//
procedure _PdFreeBufferList; stdcall; EXTERNAL DLLName;

function _PdDevCmd(hDevice : THANDLE;pError : PDWORD;dwIoControlCode : DWORD;
  lpInBuffer : POINTER;nInBufferSize : DWORD;lpOutBuffer : POINTER;
  nOutBufferSize : DWORD;bOverlapped : BOOL) : BOOL; stdcall; EXTERNAL DLLName;

function _PdDevCmdEx(hDevice : THANDLE;pError : PDWORD;dwIoControlCode : DWORD;
  lpInBuffer : POINTER;nInBufferSize : DWORD;
  lpOutBuffer : POINTER;nOutBufferSize : DWORD;
  pdwRetBytes : PDWORD;bOverlapped : BOOL) : DWORD; stdcall; EXTERNAL DLLName;

//?CHECK THIS:
function _PdAInEnableClCount(hAdapter : THANDLE;pError : PDWORD;bEnable : BOOL) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInEnableTimer(hAdapter : THANDLE;pError : PDWORD;bEnable : BOOL;dwMilliSeconds : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function PdEventThreadProc(lpParameter : POINTER) : DWORD; stdcall; EXTERNAL DLLName;

function _PdDIO256CmdWrite(hAdapter : THANDLE;pError : PDWORD;dwCmd : DWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIO256CmdRead(hAdapter : THANDLE;pError : PDWORD;dwCmd : DWORD;pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDspRegWrite(hAdapter : THANDLE;pError : PDWORD;dwReg : DWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDspRegRead(hAdapter : THANDLE;pError : PDWORD;dwReg : DWORD;pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

//----------------------------------
function PdRegisterForEvents(dwAdapter : DWORD;pError : PDWORD;pEventProc : PD_EVENT_PROC) : BOOL; stdcall; EXTERNAL DLLName;
function PdUnregisterForEvents(dwAdapter : DWORD;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterTestInterrupt(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterBoardReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterEepromWriteOnDate(hAdapter : THANDLE;pError : PDWORD;wValue : WORD) : BOOL; stdcall; EXTERNAL DLLName;

//AIN
function _PdAInFlushFifo(hAdapter : THANDLE;pError : PDWORD;pdwSamples : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetXferSize(hAdapter : THANDLE;pError : PDWORD;dwSize : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

// Events
function _PdAdapterGetBoardStatus(hAdapter : THANDLE;pError : PDWORD;pdwStatusBuf : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterSetBoardEvents1(hAdapter : THANDLE;pError : PDWORD;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterSetBoardEvents2(hAdapter : THANDLE;pError : PDWORD;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetEvents(hAdapter : THANDLE;pError : PDWORD;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

// SSH
function _PdAInSetSSHGain(hAdapter : THANDLE;pError : PDWORD;dwCfg : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSetEvents(hAdapter : THANDLE;pError : PDWORD;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

// Diag
function _PdDiagPCIEcho(hAdapter : THANDLE;pError : PDWORD;dwValue : DWORD;pdwReply : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDiagPCIInt(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

implementation
begin
end.

