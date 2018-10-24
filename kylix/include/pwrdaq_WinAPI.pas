unit pwrdaq_WinAPI;

{$IFDEF VER130}
   {$DEFINE MSWINDOWS}
{$ENDIF}

interface

uses
     Windows,
     pdtypes,
     pwrdaq,
     pwrdaq_struct;

const
  DLLName = 'pwrdaq32.dll';

{$IFDEF MSWINDOWS}
//-----------------------------------------------------------------------
// Function Prototypes:
//-----------------------------------------------------------------------
// Functions defined in pwrdaq.c:
function PdDriverOpen(phDriver : PHANDLE;pError : PDWORD;pNumAdapters : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function PdDriverClose(hDriver : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterOpen(dwAdapter : DWORD;pError : PDWORD;phAdapter : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterClose(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

//----------------------------------
function PdGetVersion(hDriver : THANDLE;pError : PDWORD;pVersion : PPWRDAQ_VERSION) : BOOL; stdcall; EXTERNAL DLLName;
function PdGetPciConfiguration(hDriver : THANDLE;pError : PDWORD;pPciConfig : PPWRDAQ_PCI_CONFIG) : BOOL; stdcall; EXTERNAL DLLName;
function PdAdapterAcquireSubsystem(hAdapter : THANDLE;pError : PDWORD;dwSubsystem : DWORD;dwAcquire : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
//----------------------------------
function _PdSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
//----------------------------------
function _PdSetUserEvents(hAdapter : THANDLE;pError : PDWORD;Subsystem : PD_SUBSYSTEM;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdClearUserEvents(hAdapter : THANDLE;pError : PDWORD;Subsystem : PD_SUBSYSTEM;dwEvents : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdGetUserEvents(hAdapter : THANDLE;pError : PDWORD;Subsystem : PD_SUBSYSTEM;pdwEvents : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdImmediateUpdate(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
//----------------------------------
function _PdAInAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwAInCfg : ULONG;
dwAInPreTrigCount : ULONG;dwAInPostTrigCount : ULONG;dwAInCvClkDiv : ULONG;dwAInClClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdGetDaqBufStatus(hAdapter : THANDLE;pError : PDWORD;pDaqBufStatus : PPD_DAQBUF_STATUS_INFO) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInGetScans(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdClearDaqBuf(hAdapter : THANDLE;pError : PDWORD;Subsystem : PD_SUBSYSTEM) : BOOL; stdcall; EXTERNAL DLLName;

function _PdAOutAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwAOutCfg : ULONG;dwAOutCvClkDiv : ULONG;dwEventsNotify : ULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

function _PdAOAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwAOutCfg : ULONG;dwAOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAODMASet(hAdapter : THANDLE; pError : PDWORD; dwOffset:DWORD;dwCount:DWORD;dwSource:DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOSetPrivateEvent(hAdapter : THANDLE; phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3
function _PdAOClearPrivateEvent(hAdapter : THANDLE; phNotifyEvent :  PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3

function _PdDIAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwDICfg : ULONG;dwAInCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;dwFirstChan : ULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDISetPrivateEvent(hAdapter : THANDLE; phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3
function _PdDIClearPrivateEvent(hAdapter : THANDLE; phNotifyEvent :  PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3


function _PdDOAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwDOutCfg : ULONG;dwDOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOSetPrivateEvent(hAdapter : THANDLE; phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3
function _PdDOClearPrivateEvent(hAdapter : THANDLE; phNotifyEvent :  PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3


function _PdCTAsyncInit(hAdapter : THANDLE;pError : PDWORD;dwCTCfg : ULONG;dwCTCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCTAsyncTerm(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCTAsyncStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCTAsyncStop(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCTGetBufState(hAdapter : THANDLE;pError : PDWORD;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCTSetPrivateEvent(hAdapter : THANDLE; phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3
function _PdCTClearPrivateEvent(hAdapter : THANDLE; phNotifyEvent :  PHANDLE) : BOOL; stdcall; EXTERNAL DLLName; //r3


// Functions defined in pwrdaqll.c:
// Board Level Commands:
function _PdAdapterEepromRead(hAdapter : THANDLE;pError : PDWORD;dwMaxSize : DWORD;pwReadBuf : PWORD;pdwWords : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAdapterEnableInterrupt(hAdapter : THANDLE;pError : PDWORD;dwEnable : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

// AIn Subsystem Commands:
function _PdAInSetCfg(hAdapter : THANDLE;pError : PDWORD;dwAInCfg : DWORD;dwAInPreTrig : DWORD;dwAInPostTrig : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetCvClk(hAdapter : THANDLE;pError : PDWORD;dwClkDiv : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetClClk(hAdapter : THANDLE;pError : PDWORD;dwClkDiv : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetChList(hAdapter : THANDLE;pError : PDWORD;dwCh : DWORD;pdwChList : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

function _PdAInGetStatus(hAdapter : THANDLE;pError : PDWORD;pdwStatus : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInEnableConv(hAdapter : THANDLE;pError : PDWORD;dwEnable : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSwStartTrig(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSwStopTrig(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSwCvStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSwClStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInResetCl(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInClearData(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInGetValue(hAdapter : THANDLE;pError : PDWORD;pwSample : PWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInGetSamples(hAdapter : THANDLE;pError : PDWORD;dwMaxBufSize : DWORD;pwBuf : PWORD;pdwSamples : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;

// AOut Subsystem Commands:
function _PdAOutSetCfg(hAdapter : THANDLE;pError : PDWORD;dwAOutCfg : DWORD;dwAOutPostTrig : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSetCvClk(hAdapter : THANDLE;pError : PDWORD;dwClkDiv : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

function _PdAOutGetStatus(hAdapter : THANDLE;pError : PDWORD;pdwStatus : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutEnableConv(hAdapter : THANDLE;pError : PDWORD;dwEnable : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSwStartTrig(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSwStopTrig(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSwCvStart(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutClearData(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutPutValue(hAdapter : THANDLE;pError : PDWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutPutBlock(hAdapter : THANDLE;pError : PDWORD;dwValues : DWORD;pdwBuf : PDWORD;pdwCount : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAOutClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;

// DIn Subsystem Commands:
function _PdDInSetCfg(hAdapter : THANDLE;pError : PDWORD;dwDInCfg : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInGetStatus(hAdapter : THANDLE;pError : PDWORD;pdwEvents : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInRead(hAdapter : THANDLE;pError : PDWORD;pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInClearData(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;

// DOut Subsystem Commands:
function _PdDOutWrite(hAdapter : THANDLE;pError : PDWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDOutReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

// DIO256 Subsystem Commands:
function _PdDIOReset(hAdapter : THANDLE; pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOEnableOutput(hAdapter : THANDLE; pError : PDWORD; dwRegMask : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOLatchAll(hAdapter : THANDLE; pError : PDWORD; dwRegister : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOSimpleRead(hAdapter : THANDLE; pError : PDWORD; dwRegister : DWORD; pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIORead(hAdapter : THANDLE; pError : PDWORD; dwRegister : DWORD; pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOWrite(hAdapter : THANDLE; pError : PDWORD; dwRegister : DWORD; dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOPropEnable(hAdapter : THANDLE; pError : PDWORD; dwRegMask : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOExtLatchEnable(hAdapter : THANDLE; pError : PDWORD; dwRegister :  DWORD; bEnable : BOOL) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOExtLatchRead(hAdapter : THANDLE; pError : PDWORD; dwRegister : DWORD; bLatch : PBOOL) : BOOL; stdcall; EXTERNAL DLLName;

function _PdDIOSetIntrMask(hAdapter : THANDLE; pError : PDWORD; dwIntMask : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOGetIntrData(hAdapter : THANDLE; pError : PDWORD; dwIntData : PDWORD; dwEdgeData : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOIntrEnable(hAdapter : THANDLE; pError : PDWORD; dwEnable : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIOSetIntCh(hAdapter : THANDLE; pError : PDWORD; dwChannels :  PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIODMASet(hAdapter : THANDLE; pError : PDWORD; dwOffset : PDWORD; dwCount : PDWORD; dwSource : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

function _PdDIO256CmdWrite(hAdapter : THANDLE;pError : PDWORD;dwCmd : DWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDIO256CmdRead(hAdapter : THANDLE;pError : PDWORD;dwCmd : DWORD;pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDspRegWrite(hAdapter : THANDLE;pError : PDWORD;dwReg : DWORD;dwValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdDspRegRead(hAdapter : THANDLE;pError : PDWORD;dwReg : DWORD;pdwValue : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;

// AO32 Subsystem Commands
function _PdAO32Reset(hAdapter : THANDLE; pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAO32Write(hAdapter : THANDLE; pError : PDWORD; wChannel : WORD; wValue : WORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAO32WriteHold(hAdapter : THANDLE; pError : PDWORD; wChannel : WORD; wValue : WORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAO32Update(hAdapter : THANDLE; pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAO32SetUpdateChannel(hAdapter : THANDLE; pError : PDWORD; wChannel : WORD; bEnable : BOOL) : BOOL; stdcall; EXTERNAL DLLName;

// UCT Subsystem Commands:
function _PdUctSetCfg(hAdapter : THANDLE;pError : PDWORD;dwUctCfg : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctGetStatus(hAdapter : THANDLE;pError : PDWORD;pdwStatus : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctWrite(hAdapter : THANDLE;pError : PDWORD;dwUctWord : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctRead(hAdapter : THANDLE;pError : PDWORD;dwUctReadCfg : DWORD;pdwUctWord : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctSwSetGate(hAdapter : THANDLE; pError : PDWORD;dwGateLevels : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctSwClkStrobe(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctReset(hAdapter : THANDLE;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUctClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE) : BOOL; stdcall; EXTERNAL DLLName;

//----------------------------------
function _PdAInGetDataCount(hAdapter : THANDLE;pError : PDWORD;pdwSamples : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
//----------------------------------

// Calibration Commands:
function _PdCalSet(hAdapter : THANDLE;pError : PDWORD;dwCalCfg : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdCalDACWrite(hAdapter : THANDLE;pError : PDWORD;dwCalDACValue : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _CalDACSet(hAdapter : THANDLE;pError : PDWORD;nDAC : Integer;nOut : Integer;nValue : Integer) : Integer; stdcall; EXTERNAL DLLName;
function _PdAdapterEepromWrite(hAdapter : THANDLE;pError : PDWORD;pwWriteBuf : PWORD;dwSize : DWORD) : BOOL; stdcall; EXTERNAL DLLName;

// Functions defined in pddrvmem.c:
function _PdAllocateBuffer(pBuffer : PPWORD;dwFrames : DWORD;dwScans : DWORD;dwScanSize : DWORD;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdFreeBuffer(pBuffer : PWORD;pError : PDWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdRegisterBuffer(hAdapter : THANDLE;pError : PDWORD;pBuffer : PWORD;dwSubsystem : DWORD;bWrapAround : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdUnregisterBuffer(hAdapter : THANDLE;pError : PDWORD;pBuffer : PWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdAcquireBuffer(hAdapter : THANDLE;pError : PDWORD;pBuffer : PPWORD;dwFrames : DWORD;dwScans : DWORD;dwScanSize : DWORD;dwSubsystem : DWORD;bWrapAround : DWORD) : BOOL; stdcall; EXTERNAL DLLName;
function _PdReleaseBuffer(hAdapter : THANDLE;pError : PDWORD;subsystem : DWORD; pBuffer : PWORD) : BOOL; stdcall; EXTERNAL DLLName;
{$ENDIF}

// function prototypes
//
//=======================================================================
// Function fills up Adapter_Info structure distributed by application
//
// dwBoardNum : Number of boards
// dwError:     Error if any
// Adp_Info:    Pointer to the structure to
//
// Returns
//
function _PdGetAdapterInfo(dwBoardNum: DWORD; pdwError: PDWORD; Adp_Info: PADAPTER_INFO_STRUCT): BOOL stdcall; EXTERNAL DLLName;

// The function is similar to previopus but takes data directly from EEPROM and Primary Boot
// instead of structures stored in DLL
function __PdGetAdapterInfo(dwBoardNum: DWORD; pdwError: PDWORD; Adp_Info: PADAPTER_INFO_STRUCT): BOOL stdcall; EXTERNAL DLLName;

//=======================================================================
// Function returns pointer to DAQ_Information structure for dwBoardID
// board (stored in PCI Configuration Space) using handle to adapter
//
// Returns TRUE if success and FALSE if failure
//
function _PdGetCapsPtrA(hAdapter: THANDLE; pdwError: PDWORD; pDaqInf: PDAQ_Information) : BOOL; stdcall; EXTERNAL DLLName;

//=======================================================================
// Function returns pointer to DAQ_information structure for dwBoardID
// board (stored in PCI Configuration Space
//
// If dwBoardID is incorrect function returns NULL
//
function _PdGetCapsPtr(dwBoardID: DWORD): PDAQ_Information; stdcall; EXTERNAL DLLName;

//=======================================================================
// Function parses channel definition string in DAQ_Information structure
//
// Parameters:  dwBoardID -- board ID from PCI Config.Space
//              dwSubsystem -- subsystem enum from pwrdaq.h
//              dwProperty -- property of subsystem to retrieve:
//                  PDHCAPS_BITS       -- subsystem bit width
//                  PDHCAPS_FIRSTCHAN  -- first channel available
//                  PDHCAPS_LASTCHAN   -- last channel available
//                  PDHCAPS_CHANNELS   -- number of channels available
//
function _PdParseCaps(dwBoardID: DWORD; dwSubsystem : DWORD; dwProperty : DWORD) : DWORD; stdcall; EXTERNAL DLLName;

// Function convers raw values to volts
function PdAInRawToVolts( hAdapter: THANDLE;
                          dwMode: DWORD;              // Mode used
                          pwRawData: PWORD;           // Raw data
                          pfVoltage: PDouble;         // Engineering unit
                          dwCount: DWORD              // Number of samples to convert
                         ): BOOL; stdcall; EXTERNAL DLLName;

// Function convers raw values to volts
function PdAOutVoltsToRaw( hAdapter: THANDLE;
                           dwMode: DWORD;             // Mode used
                           pfVoltage: PDouble;        // Engineering unit
                           pdwRawData: PDWORD;        // Raw data
                           dwCount: DWORD             // Number of samples to convert
                         ): BOOL; stdcall; EXTERNAL DLLName;

implementation

end.
