unit pwrdaq_LinuxAPI;

interface

uses
   pdtypes,
   pwrdaq,
   pwrdaq_struct;


const
   AIN_SCANRETMODE_MMAP = 0;
   AIN_SCANRETMODE_RAW = 1;
   AIN_SCANRETMODE_VOLTS = 2;

const
   DLLName = 'libpowerdaq32.so';

//----------------------------------
function PdGetVersion(pVersion : PPWRDAQ_VERSION) : Integer; cdecl; EXTERNAL DLLName;
function PdGetNumberAdapters() : Integer; cdecl; EXTERNAL DLLName;
function PdGetPciConfiguration(hAdapter : Integer;pPciConfig : PPWRDAQ_PCI_CONFIG) : Integer; cdecl; EXTERNAL DLLName;
function PdAcquireSubsystem(hAdapter : Integer;dwSubsystem : DWORD;dwAcquire : DWORD) : Integer; cdecl; EXTERNAL DLLName;
//----------------------------------
function _PdSetUserEvents(hAdapter : Integer;Subsystem : PD_SUBSYSTEM;dwEvents : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdClearUserEvents(hAdapter : Integer;Subsystem : PD_SUBSYSTEM;dwEvents : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdGetUserEvents(hAdapter : Integer;Subsystem : PD_SUBSYSTEM;pdwEvents : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdImmediateUpdate(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAdapterEnableInterrupt(hAdapter : Integer; dwEnable : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdWaitForEvent(hAdapter : Integer; events : Integer; timeout : Integer) : Integer; cdecl; EXTERNAL DLLName;
//----------------------------------
function _PdAInAsyncInit(hAdapter : Integer;dwAInCfg : ULONG;
                        dwAInPreTrigCount : ULONG;dwAInPostTrigCount : ULONG;dwAInCvClkDiv : ULONG;
                        dwAInClClkDiv : ULONG;dwEventsNotify : ULONG;
                        dwChListChan : ULONG;pdwChList : PULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInAsyncTerm(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInAsyncStart(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInAsyncStop(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdGetDaqBufStatus(hAdapter : Integer;pDaqBufStatus : PPD_DAQBUF_STATUS_INFO) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInGetScans(hAdapter : Integer;NumScans : DWORD;RetMod : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdClearDaqBuf(hAdapter : Integer;Subsystem : PD_SUBSYSTEM) : Integer; cdecl; EXTERNAL DLLName;

function _PdAOutAsyncInit(hAdapter : THANDLE;dwAOutCfg : ULONG;dwAOutCvClkDiv : ULONG;dwEventsNotify : ULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutAsyncTerm(hAdapter : THANDLE) : Integer; EXTERNAL DLLName;
function _PdAOutAsyncStart(hAdapter : THANDLE) : Integer; EXTERNAL DLLName;
function _PdAOutAsyncStop(hAdapter : THANDLE) : Integer; EXTERNAL DLLName;
function _PdAOutGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdAOAsyncInit(hAdapter : THANDLE;dwAOutCfg : ULONG;dwAOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOAsyncTerm(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOAsyncStart(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOAsyncStop(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAODMASet(hAdapter : THANDLE; dwOffset:DWORD;dwCount:DWORD;dwSource:DWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdDIAsyncInit(hAdapter : THANDLE;dwDICfg : ULONG;dwAInCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;dwFirstChan : ULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIAsyncTerm(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIAsyncStart(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIAsyncStop(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdDOAsyncInit(hAdapter : THANDLE;dwDOutCfg : ULONG;dwDOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdDOAsyncTerm(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDOAsyncStart(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDOAsyncStop(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdDOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdCTAsyncInit(hAdapter : THANDLE;dwCTCfg : ULONG;dwCTCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG) : Integer; cdecl; EXTERNAL DLLName;
function _PdCTAsyncTerm(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdCTAsyncStart(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdCTAsyncStop(hAdapter : THANDLE) : Integer; cdecl; EXTERNAL DLLName;
function _PdCTGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD) : Integer; cdecl; EXTERNAL DLLName;


// Functions defined in pwrdaqll.c:
// Board Level Commands:
function _PdAdapterEepromRead(hAdapter : Integer;dwMaxSize : DWORD;pwReadBuf : PWORD;pdwWords : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAdapterEnableIntegererrupt(hAdapter : Integer;dwEnable : DWORD) : Integer; cdecl; EXTERNAL DLLName;

// AIn Subsystem Commands:
function _PdAInSetCfg(hAdapter : Integer;dwAInCfg : DWORD;dwAInPreTrig : DWORD;dwAInPostTrig : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSetCvClk(hAdapter : Integer;dwClkDiv : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSetClClk(hAdapter : Integer;dwClkDiv : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSetChList(hAdapter : Integer;dwCh : DWORD;pdwChList : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdAInGetStatus(hAdapter : Integer;pdwStatus : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInEnableConv(hAdapter : Integer;dwEnable : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSwStartTrig(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSwStopTrig(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSwCvStart(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInSwClStart(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInResetCl(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInClearData(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInGetValue(hAdapter : Integer;pwSample : PWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInGetDataCount(hAdapter : Integer; pdwSamples : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAInGetSamples(hAdapter : Integer;dwMaxBufSize : DWORD;pwBuf : PWORD;pdwSamples : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

// AOut Subsystem Commands:
function _PdAOutSetCfg(hAdapter : Integer;dwAOutCfg : DWORD;dwAOutPostTrig : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutSetCvClk(hAdapter : Integer;dwClkDiv : DWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdAOutGetStatus(hAdapter : Integer;pdwStatus : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutEnableConv(hAdapter : Integer;dwEnable : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutSwStartTrig(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutSwStopTrig(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutSwCvStart(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutClearData(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutPutValue(hAdapter : Integer;dwValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAOutPutBlock(hAdapter : Integer;dwValues : DWORD;pdwBuf : PDWORD;pdwCount : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

// DIn Subsystem Commands:
function _PdDInSetCfg(hAdapter : Integer;dwDInCfg : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDInGetStatus(hAdapter : Integer;pdwEvents : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDInRead(hAdapter : Integer;pdwValue : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDInClearData(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdDInReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;

// DOut Subsystem Commands:
function _PdDOutWrite(hAdapter : Integer;dwValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDOutReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;

// DIO256 Subsystem Commands:
function _PdDIOReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOEnableOutput(hAdapter : Integer;  dwRegMask : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOLatchAll(hAdapter : Integer;  dwRegister : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOSimpleRead(hAdapter : Integer;  dwRegister : DWORD; pdwValue : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIORead(hAdapter : Integer;  dwRegister : DWORD; pdwValue : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOWrite(hAdapter : Integer;  dwRegister : DWORD; dwValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOPropEnable(hAdapter : Integer;  dwRegMask : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOExtLatchEnable(hAdapter : Integer;  dwRegister :  DWORD; bEnable : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOExtLatchRead(hAdapter : Integer;  dwRegister : DWORD; bLatch : PInteger) : Integer; cdecl; EXTERNAL DLLName;

function _PdDIOSetIntrMask(hAdapter : Integer;  dwIntegerMask : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOGetIntrData(hAdapter : Integer;  dwIntegerData : PDWORD; dwEdgeData : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOIntrEnable(hAdapter : Integer;  dwEnable : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIOSetIntCh(hAdapter : Integer;  dwChannels :  PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIODMASet(hAdapter : Integer;  dwOffset : PDWORD; dwCount : PDWORD; dwSource : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdDIO256CmdWrite(hAdapter : Integer;dwCmd : DWORD;dwValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDIO256CmdRead(hAdapter : Integer;dwCmd : DWORD;pdwValue : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDspRegWrite(hAdapter : Integer;dwReg : DWORD;dwValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdDspRegRead(hAdapter : Integer;dwReg : DWORD;pdwValue : PDWORD) : Integer; cdecl; EXTERNAL DLLName;

// AO32 Subsystem Commands
function _PdAO32Reset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAO32Write(hAdapter : Integer;  wChannel : WORD; wValue : WORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAO32WriteHold(hAdapter : Integer;  wChannel : WORD; wValue : WORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdAO32Update(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAO32SetUpdateChannel(hAdapter : Integer;  wChannel : WORD; bEnable : Integer) : Integer; cdecl; EXTERNAL DLLName;

// UCT Subsystem Commands:
function _PdUctSetCfg(hAdapter : Integer;dwUctCfg : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctGetStatus(hAdapter : Integer;pdwStatus : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctWrite(hAdapter : Integer;dwUctWord : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctRead(hAdapter : Integer;dwUctReadCfg : DWORD;pdwUctWord : PDWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctSwSetGate(hAdapter : Integer; dwGateLevels : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctSwClkStrobe(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdUctReset(hAdapter : Integer) : Integer; cdecl; EXTERNAL DLLName;

// Calibration Commands:
function _PdCalSet(hAdapter : Integer;dwCalCfg : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdCalDACWrite(hAdapter : Integer;dwCalDACValue : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _CalDACSet(hAdapter : Integer;nDAC : Integer;nOut : Integer;nValue : Integer) : Integer; cdecl; EXTERNAL DLLName;
function _PdAdapterEepromWrite(hAdapter : Integer;pwWriteBuf : PWORD;dwSize : DWORD) : Integer; cdecl; EXTERNAL DLLName;

// Functions defined in pddrvmem.c:
function _PdRegisterBuffer(hAdapter : Integer;pBuffer : PWORD;dwSubsystem : DWORD;dwFrames : DWORD;dwScans : DWORD;dwScanSize : DWORD;dwCircMode : DWORD) : Integer; cdecl; EXTERNAL DLLName;
function _PdUnregisterBuffer(hAdapter : Integer;pBuffer : PWORD;dwSubsystem : DWORD) : Integer; cdecl; EXTERNAL DLLName;

function _PdGetSerialNumber(hAdapter : Integer; serialNumber : Pchar) : integer; cdecl; EXTERNAL DLLName;
function _PdGetManufactureDate(hAdapter : Integer; manfctrDate : Pchar) : integer; cdecl; EXTERNAL DLLName;
function _PdGetCalibrationDate(hAdapter : Integer; calDate : Pchar) : integer; cdecl; EXTERNAL DLLName;
function __PdGetAdapterInfo(dwBoardNum: DWORD; Adp_Info: PADAPTER_INFO_STRUCT): Integer cdecl; EXTERNAL DLLName;
function PdAInRawToVolts( dwBoardNum: DWORD;
                          dwMode: DWORD;              // Mode used
                          pwRawData: PWORD;           // Raw data
                          pfVoltage: PDouble;         // Engineering unit
                          dwCount: DWORD              // Number of samples to convert
                         ): Integer; cdecl; EXTERNAL DLLName;

// Function convers raw values to volts
function PdAOutVoltsToRaw( dwBoardNum: DWORD;
                           dwMode: DWORD;             // Mode used
                           pfVoltage: PDouble;        // Engineering unit
                           pdwRawData: PDWORD;        // Raw data
                           dwCount: DWORD             // Number of samples to convert
                         ): Integer; cdecl; EXTERNAL DLLName;

function _PdDspCtLoad(handle : Integer; // Load all registers of the selected counter
                           dwCounter : DWORD; // Counter register
                           dwLoad : DWORD;    // Load register
                           dwCompare : DWORD; // Compare register
                           dwMode : DWORD;    // Counting mode
                           bReload : DWORD;    // Reload flag (0/1)
                           bInverted : DWORD;  // Input/Output polarity
                           bUsePrescaler : DWORD): Integer; cdecl; EXTERNAL DLLName; // Use prescaler

// Enable(1)/Disable(x) counting for the selected counter 
function _PdDspCtEnableCounter(handle : Integer;
                          dwCounter : DWORD;
                          bEnable : DWORD): Integer; cdecl; EXTERNAL DLLName; //r3

// Enable(1)/Disable(x) interrupts for the selected events for the selected 
// counter (HANDLE hAdapter,only one event can be enabled at the time)
function _PdDspCtEnableInterrupts(handle : Integer;
                             dwCounter : DWORD;
                             bCompare : DWORD;
                             bOverflow : DWORD): Integer; cdecl; EXTERNAL DLLName;

// Get count register value from the selected counter
function _PdDspCtGetCount(handle  : Integer; dwCounter : DWORD; dwCount : PDWORD): Integer; cdecl; EXTERNAL DLLName;

// Get compare register value from the selected counter
function _PdDspCtGetCompare(handle : Integer; dwCounter : DWORD; dwCompare : PDWORD): Integer; cdecl; EXTERNAL DLLName;
// Get control/status register value from the selected counter
function _PdDspCtGetStatus(handle : Integer; dwCounter : DWORD; dwStatus : PDWORD): Integer; cdecl; EXTERNAL DLLName;

// Set compare register value from the selected counter
function _PdDspCtSetCompare(handle : Integer; dwCounter : DWORD; dwCompare : DWORD): Integer; cdecl; EXTERNAL DLLName;
// Set load register value from the selected counter
function _PdDspCtSetLoad(handle : Integer; dwCounter : DWORD; dwLoad : DWORD): Integer; cdecl; EXTERNAL DLLName;
// Set control/status register value from the selected counter
function _PdDspCtSetStatus(handle : Integer; dwCounter : DWORD; dwStatus : DWORD): Integer; cdecl; EXTERNAL DLLName;

// Load prescaler
function _PdDspPSLoad(handle : Integer; dwLoad : DWORD; dwSource : DWORD): Integer; cdecl; EXTERNAL DLLName;
// Get prescaler count register value 
function _PdDspPSGetCount(handle : Integer; dwCount : PDWORD): Integer; cdecl; EXTERNAL DLLName;

// Get address of the count register of the selected counter
function _PdDspCtGetCountAddr(dwCounter : DWORD): DWORD; cdecl; EXTERNAL DLLName;

// Get address of the load register of the selected counter
function _PdDspCtGetLoadAddr(dwCounter : DWORD): DWORD; cdecl; EXTERNAL DLLName;

// Get address of the control/status register of the selected counter
function _PdDspCtGetStatusAddr(dwCounter : DWORD): DWORD; cdecl; EXTERNAL DLLName;

// Get address of the compare register of the selected counter
function _PdDspCtGetCompareAddr(dwCounter : DWORD): DWORD; cdecl; EXTERNAL DLLName;


implementation

end.

