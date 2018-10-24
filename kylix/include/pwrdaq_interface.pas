unit pwrdaq_interface;

{$IFDEF VER130}
   {$DEFINE MSWINDOWS}
{$ENDIF}

interface

uses
{$IFDEF MSWINDOWS}
   Windows,
   pwrdaq_WinAPI,
{$ENDIF}
{$IFDEF LINUX}
   pwrdaq_LinuxAPI,
{$ENDIF}
   pwrdaq,
   pdtypes,
   pwrdaq_struct,
   pwrdaq_exception;

type
  CPwrDAQAPI = class
  public
     procedure PwdDriverOpen(phDriver : PHANDLE;pNumAdapters : PDWORD);
     procedure PwdDriverClose(hDriver : THANDLE);
     procedure PwdAdapterOpen(dwAdapter : DWORD;phAdapter : PHANDLE);
     procedure PwdAdapterClose(hAdapter : THANDLE);

     //----------------------------------
     procedure PwdGetVersion(hDriver : THANDLE;pVersion : PPWRDAQ_VERSION);
     procedure PwdGetPciConfiguration(hDriver : THANDLE;pPciConfig : PPWRDAQ_PCI_CONFIG);
     procedure PwdAdapterAcquireSubsystem(phAdapter : PHANDLE; dwBoardNum : DWORD; dwSubsystem : DWORD;dwAcquire : DWORD);
     //----------------------------------
     procedure PwdSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     //----------------------------------
     procedure PwdSetUserEvents(hAdapter : THANDLE;Subsystem : DWORD;dwEvents : DWORD);
     procedure PwdClearUserEvents(hAdapter : THANDLE;Subsystem : DWORD;dwEvents : DWORD);
     procedure PwdGetUserEvents(hAdapter : THANDLE;Subsystem : DWORD;pdwEvents : PDWORD);
     procedure PwdImmediateUpdate(hAdapter : THANDLE);
     procedure PwdWaitForEvent(handle : THANDLE; dwNotifyEvents : DWORD; dwTimeOut : DWORD);
     //----------------------------------
     procedure PwdAInAsyncInit(hAdapter : THANDLE;dwAInCfg : ULONG;
                                dwAInPreTrigCount : ULONG;dwAInPostTrigCount : ULONG;
                                dwAInCvClkDiv : ULONG;dwAInClClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG;pdwChList : PULONG);
     procedure PwdAInAsyncTerm(hAdapter : THANDLE);
     procedure PwdAInAsyncStart(hAdapter : THANDLE);
     procedure PwdAInAsyncStop(hAdapter : THANDLE);
     procedure PwdGetDaqBufStatus(hAdapter : THANDLE;pDaqBufStatus : PPD_DAQBUF_STATUS_INFO);
     procedure PwdAInGetScans(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
     procedure PwdClearDaqBuf(hAdapter : THANDLE;Subsystem : DWORD);

     procedure PwdAOutAsyncInit(hAdapter : THANDLE;dwAOutCfg : ULONG;dwAOutCvClkDiv : ULONG;dwEventsNotify : ULONG);
     procedure PwdAOutAsyncTerm(hAdapter : THANDLE);
     procedure PwdAOutAsyncStart(hAdapter : THANDLE);
     procedure PwdAOutAsyncStop(hAdapter : THANDLE);
     procedure PwdAOutGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);


     procedure PwdAOAsyncInit(hAdapter : THANDLE;dwAOCfg : ULONG;dwAOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG);
     procedure PwdAOAsyncTerm(hAdapter : THANDLE);
     procedure PwdAOAsyncStart(hAdapter : THANDLE);
     procedure PwdAOAsyncStop(hAdapter : THANDLE);
     procedure PwdAOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
     //procedure PwdAODMASet(hAdapter : THANDLE; dwOffset:DWORD;dwCount:DWORD;dwSource:DWORD);
     procedure PwdAOSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdAOClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     procedure PwdDIAsyncInit(hAdapter : THANDLE;dwDICfg : ULONG;dwDICvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;dwFirstChan : ULONG);
     procedure PwdDIAsyncTerm(hAdapter : THANDLE);
     procedure PwdDIAsyncStart(hAdapter : THANDLE);
     procedure PwdDIAsyncStop(hAdapter : THANDLE);
     procedure PwdDIGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
     procedure PwdDISetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdDIClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     procedure PwdDOAsyncInit(hAdapter : THANDLE;dwDOCfg : ULONG;dwDOCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG;pdwChList : PULONG);
     procedure PwdDOAsyncTerm(hAdapter : THANDLE);
     procedure PwdDOAsyncStart(hAdapter : THANDLE);
     procedure PwdDOAsyncStop(hAdapter : THANDLE);
     procedure PwdDOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
     procedure PwdDOSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdDOClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     procedure PwdCTAsyncInit(hAdapter : THANDLE;dwCTCfg : ULONG;dwCTCvClkDiv : ULONG;dwEventsNotify : ULONG;dwChListChan : ULONG);
     procedure PwdCTAsyncTerm(hAdapter : THANDLE);
     procedure PwdCTAsyncStart(hAdapter : THANDLE);
     procedure PwdCTAsyncStop(hAdapter : THANDLE);
     procedure PwdCTGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
     procedure PwdCTSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdCTClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     // Functions defined in pwrdaqll.c:
     // Board Level Commands:
     procedure PwdAdapterEepromRead(hAdapter : THANDLE;dwMaxSize : DWORD;pwReadBuf : PWORD;pdwWords : PDWORD);
     procedure PwdAdapterEnableInterrupt(hAdapter : THANDLE;dwEnable : DWORD);

     // AIn Subsystem Commands:
     procedure PwdAInSetCfg(hAdapter : THANDLE;dwAInCfg : DWORD;dwAInPreTrig : DWORD;dwAInPostTrig : DWORD);
     procedure PwdAInSetCvClk(hAdapter : THANDLE;dwClkDiv : DWORD);
     procedure PwdAInSetClClk(hAdapter : THANDLE;dwClkDiv : DWORD);
     procedure PwdAInSetChList(hAdapter : THANDLE;dwCh : DWORD;pdwChList : PDWORD);

     procedure PwdAInGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
     procedure PwdAInEnableConv(hAdapter : THANDLE;dwEnable : DWORD);
     procedure PwdAInSwStartTrig(hAdapter : THANDLE);
     procedure PwdAInSwStopTrig(hAdapter : THANDLE);
     procedure PwdAInSwCvStart(hAdapter : THANDLE);
     procedure PwdAInSwClStart(hAdapter : THANDLE);
     procedure PwdAInResetCl(hAdapter : THANDLE);
     procedure PwdAInClearData(hAdapter : THANDLE);
     procedure PwdAInReset(hAdapter : THANDLE);

     procedure PwdAInGetValue(hAdapter : THANDLE;pwSample : PWORD);
     procedure PwdAInGetSamples(hAdapter : THANDLE;dwMaxBufSize : DWORD;pwBuf : PWORD;pdwSamples : PDWORD);
     procedure PwdAInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdAInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     // AOut Subsystem Commands:
     procedure PwdAOutSetCfg(hAdapter : THANDLE;dwAOutCfg : DWORD;dwAOutPostTrig : DWORD);
     procedure PwdAOutSetCvClk(hAdapter : THANDLE;dwClkDiv : DWORD);

     procedure PwdAOutGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
     procedure PwdAOutEnableConv(hAdapter : THANDLE;dwEnable : DWORD);
     procedure PwdAOutSwStartTrig(hAdapter : THANDLE);
     procedure PwdAOutSwStopTrig(hAdapter : THANDLE);
     procedure PwdAOutSwCvStart(hAdapter : THANDLE);
     procedure PwdAOutClearData(hAdapter : THANDLE);
     procedure PwdAOutReset(hAdapter : THANDLE);
     procedure PwdAOutPutValue(hAdapter : THANDLE;dwValue : DWORD);
     procedure PwdAOutPutBlock(hAdapter : THANDLE;dwValues : DWORD;pdwBuf : PDWORD;pdwCount : PDWORD);
     procedure PwdAOutSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdAOutClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdAOutVoltsToRaw(hAdapter: THANDLE; dwMode: DWORD; pfVoltage: PDouble;
                                 pdwRawData: PDWORD; dwCount: DWORD);

     // DIn Subsystem Commands:
     procedure PwdDInSetCfg(hAdapter : THANDLE;dwDInCfg : DWORD);
     procedure PwdDInGetStatus(hAdapter : THANDLE;pdwEvents : PDWORD);
     procedure PwdDInRead(hAdapter : THANDLE;pdwValue : PDWORD);
     procedure PwdDInClearData(hAdapter : THANDLE);
     procedure PwdDInReset(hAdapter : THANDLE);
     procedure PwdDInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdDInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     // DOut Subsystem Commands:
     procedure PwdDOutWrite(hAdapter : THANDLE;dwValue : DWORD);
     procedure PwdDOutReset(hAdapter : THANDLE);

     // DIO256 Subsystem Commands:
     procedure PwdDIOReset(hAdapter : THANDLE);
     procedure PwdDIOEnableOutput(hAdapter : THANDLE; dwRegMask : DWORD);
     procedure PwdDIOLatchAll(hAdapter : THANDLE; dwRegister : DWORD);
     procedure PwdDIOSimpleRead(hAdapter : THANDLE; dwRegister : DWORD; pdwValue : PDWORD);
     procedure PwdDIORead(hAdapter : THANDLE; dwRegister : DWORD; pdwValue : PDWORD);
     procedure PwdDIOWrite(hAdapter : THANDLE; dwRegister : DWORD; dwValue : DWORD);
     procedure PwdDIOPropEnable(hAdapter : THANDLE; dwRegMask : DWORD);
     procedure PwdDIOExtLatchEnable(hAdapter : THANDLE; dwRegister :  DWORD; bEnable : Integer);
     procedure PwdDIOExtLatchRead(hAdapter : THANDLE; dwRegister : DWORD; bLatch : PInteger);

     procedure PwdDIOSetIntrMask(hAdapter : THANDLE; dwIntMask : PDWORD);
     procedure PwdDIOGetIntrData(hAdapter : THANDLE; dwIntData : PDWORD; dwEdgeData : PDWORD);
     procedure PwdDIOIntrEnable(hAdapter : THANDLE; dwEnable : PDWORD);
     procedure PwdDIOSetIntCh(hAdapter : THANDLE; dwChannels :  PDWORD);
     procedure PwdDIODMASet(hAdapter : THANDLE; dwOffset : PDWORD; dwCount : PDWORD; dwSource : PDWORD);

     procedure PwdDIO256CmdWrite(hAdapter : THANDLE;dwCmd : DWORD;dwValue : DWORD);
     procedure PwdDIO256CmdRead(hAdapter : THANDLE;dwCmd : DWORD;pdwValue : PDWORD);
     procedure PwdDspRegWrite(hAdapter : THANDLE;dwReg : DWORD;dwValue : DWORD);
     procedure PwdDspRegRead(hAdapter : THANDLE;dwReg : DWORD;pdwValue : PDWORD);

     // AO32 Subsystem Commands
     procedure PwdAO32Reset(hAdapter : THANDLE);
     procedure PwdAO32Write(hAdapter : THANDLE; wChannel : WORD; wValue : WORD);
     procedure PwdAO32WriteHold(hAdapter : THANDLE; wChannel : WORD; wValue : WORD);
     procedure PwdAO32Update(hAdapter : THANDLE);
     procedure PwdAO32SetUpdateChannel(hAdapter : THANDLE; wChannel : WORD; bEnable : Integer);

     // UCT Subsystem Commands:
     procedure PwdUctSetCfg(hAdapter : THANDLE;dwUctCfg : DWORD);
     procedure PwdUctGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
     procedure PwdUctWrite(hAdapter : THANDLE;dwUctWord : DWORD);
     procedure PwdUctRead(hAdapter : THANDLE;dwUctReadCfg : DWORD;pdwUctWord : PDWORD);
     procedure PwdUctSwSetGate(hAdapter : THANDLE;dwGateLevels : DWORD);
     procedure PwdUctSwClkStrobe(hAdapter : THANDLE);
     procedure PwdUctReset(hAdapter : THANDLE);
     procedure PwdUctSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
     procedure PwdUctClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);

     //----------------------------------
     procedure PwdAInGetDataCount(hAdapter : THANDLE;pdwSamples : PDWORD);
     //----------------------------------

     // Calibration Commands:
     procedure PwdCalSet(hAdapter : THANDLE;dwCalCfg : DWORD);
     procedure PwdCalDACWrite(hAdapter : THANDLE;dwCalDACValue : DWORD);
     procedure PwdCalDACSet(hAdapter : THANDLE;nDAC : Integer;nOut : Integer;nValue : Integer);
     procedure PwdAdapterEepromWrite(hAdapter : THANDLE;pwWriteBuf : PWORD;dwSize : DWORD);

     // Functions defined in pddrvmem.c:
     procedure PwdAllocateBuffer(hAdapter: THANDLE; pBuffer : PPWORD;dwFrames : DWORD;dwScans : DWORD;dwScanSize : DWORD;
                                 dwSubsystem : DWORD;bWrapAround : DWORD);
     procedure PwdFreeBuffer(hAdapter : THANDLE; pBuffer : PWORD; dwSubSystem : DWORD);
     
     procedure PwdGetAdapterInfo(dwBoardNum: DWORD; Adp_Info: PADAPTER_INFO_STRUCT);
     //function PdGetCapsPtr(dwBoardID: DWORD): PDAQ_Information;
     //procedure PdParseCaps(dwBoardID: DWORD; dwSubsystem : DWORD; dwProperty : DWORD);
     procedure PwdGetSerialNumber(handle : THANDLE; serial : PChar);
     procedure PwdGetManufactureDate(handle : THANDLE; manfctrDate : PChar);
     procedure PwdGetCalibrationDate(handle : THANDLE; calDate : PChar);
end;

implementation


procedure CPwrDAQAPI.PwdDriverOpen(phDriver : PHANDLE;pNumAdapters : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdDriverOpen(phDriver, @error, pNumAdapters);
{$ENDIF}
{$IFDEF LINUX}
   error := PdGetNumberAdapters();
   if(error < 0) then
      bResult := False
   else
      pNumAdapters^ := error;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDriverOpen', error);
end;

procedure CPwrDAQAPI.PwdDriverClose(hDriver : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdDriverClose(hDriver, @error);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDriverClose', error);
end;

procedure CPwrDAQAPI.PwdAdapterOpen(dwAdapter : DWORD;phAdapter : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAdapterOpen(dwAdapter, @error, phAdapter);
{$ENDIF}
{$IFDEF LINUX}
   phAdapter^ := INVALID_HANDLE_VALUE;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterOpen', error);
end;

procedure CPwrDAQAPI.PwdAdapterClose(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAdapterClose(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterClose', error);
end;

//----------------------------------
procedure CPwrDAQAPI.PwdGetVersion(hDriver : THANDLE;pVersion : PPWRDAQ_VERSION);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdGetVersion(hDriver, @error, pVersion);
{$ENDIF}
{$IFDEF LINUX}
   error := PdGetVersion(pVersion);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdGetVersion', error);
end;

procedure CPwrDAQAPI.PwdGetPciConfiguration(hDriver : THANDLE;pPciConfig : PPWRDAQ_PCI_CONFIG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdGetPciConfiguration(hDriver, @error, pPciConfig);
{$ENDIF}
{$IFDEF LINUX}
   error := PdGetPciConfiguration(hDriver, pPciConfig);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdGetPciConfiguration', error);
end;

procedure CPwrDAQAPI.PwdAdapterAcquireSubsystem(phAdapter : PHANDLE;dwBoardNum : DWORD; dwSubsystem : DWORD;dwAcquire : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdAdapterAcquireSubsystem(phAdapter^, @error, dwSubsystem, dwAcquire);
{$ENDIF}
{$IFDEF LINUX}
   if dwAcquire = 1 then
      phAdapter^ := PdAcquireSubsystem(dwBoardNum, dwSubsystem, dwAcquire)
   else
      PdAcquireSubSystem(phAdapter^, dwSubSystem, dwAcquire);
   if(phAdapter^ < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterAcquireSubsystem', error);
end;

//----------------------------------
procedure CPwrDAQAPI.PwdSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdClearPrivateEvent', error);
end;

//----------------------------------
procedure CPwrDAQAPI.PwdSetUserEvents(hAdapter : THANDLE;Subsystem : DWORD;dwEvents : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdSetUserEvents(hAdapter, @error, Subsystem, dwEvents);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdSetUserEvents(hAdapter, Subsystem, dwEvents);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdSetUserEvents', error);
end;

procedure CPwrDAQAPI.PwdWaitForEvent(handle : THANDLE; dwNotifyEvents : DWORD; dwTimeOut : DWORD);
var
   events : DWORD;
   bTimeOut : Boolean;
begin
   bTimeOut := false;
{$IFDEF MSWINDOWS}
   if (WaitForSingleObject(handle, dwTimeOut)=WAIT_TIMEOUT) then
      bTimeOut := true;
{$ENDIF}
{$IFDEF LINUX}
   events := _PdWaitForEvent(handle, dwNotifyEvents, dwTimeOut);
   if((events and eTimeout) <> 0) then
      bTimeOut := true;
{$ENDIF}
   if bTimeout then
      raise EPwrDAQ.Create('PdWaitForEvent', -1);
end;


procedure CPwrDAQAPI.PwdClearUserEvents(hAdapter : THANDLE;Subsystem : DWORD;dwEvents : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdClearUserEvents(hAdapter, @error, Subsystem, dwEvents);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdClearUserEvents(hAdapter, Subsystem, dwEvents);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdClearUserEvents', error);
end;

procedure CPwrDAQAPI.PwdGetUserEvents(hAdapter : THANDLE;Subsystem : DWORD;pdwEvents : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdGetUserEvents(hAdapter, @error, Subsystem, pdwEvents);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdGetUserEvents(hAdapter, Subsystem, pdwEvents);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdGetUserEvents', error);
end;

procedure CPwrDAQAPI.PwdImmediateUpdate(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdImmediateUpdate(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdImmediateUpdate(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdImmediateUpdate', error);
end;

//----------------------------------
procedure CPwrDAQAPI.PwdAInAsyncInit(hAdapter : THANDLE;dwAInCfg : ULONG;
                                dwAInPreTrigCount : ULONG;dwAInPostTrigCount : ULONG;
                                dwAInCvClkDiv : ULONG;dwAInClClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG;pdwChList : PULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInAsyncInit(hAdapter, @error, dwAInCfg, dwAInPreTrigCount,
                              dwAInPostTrigCount, dwAInCvClkDiv, dwAInClClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInAsyncInit(hAdapter, dwAInCfg, dwAInPreTrigCount,
                              dwAInPostTrigCount, dwAInCvClkDiv, dwAInClClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdAInAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdAInAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdAInAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInAsyncStop', error);
end;

procedure CPwrDAQAPI.PwdGetDaqBufStatus(hAdapter : THANDLE;pDaqBufStatus : PPD_DAQBUF_STATUS_INFO);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdGetDaqBufStatus(hAdapter, @error, pDaqBufStatus);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdGetDaqBufStatus(hAdapter, pDaqBufStatus);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdGetDaqBufStatus', error);
end;

procedure CPwrDAQAPI.PwdAInGetScans(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInGetScans(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInGetScans(hAdapter, NumScans, AIN_SCANRETMODE_MMAP,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInGetScans', error);
end;

procedure CPwrDAQAPI.PwdClearDaqBuf(hAdapter : THANDLE;Subsystem : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdClearDaqBuf(hAdapter, @error, Subsystem);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdClearDaqBuf(hAdapter, Subsystem);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdClearDaqBuf', error);
end;


procedure CPwrDAQAPI.PwdAOutAsyncInit(hAdapter : THANDLE;dwAOutCfg : ULONG;
                                dwAOutCvClkDiv : ULONG;dwEventsNotify : ULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutAsyncInit(hAdapter, @error, dwAOutCfg, dwAOutCvClkDiv,
                              dwEventsNotify);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutAsyncInit(hAdapter, dwAOutCfg, dwAOutCvClkDiv,
                              dwEventsNotify);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdAOutAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdAOutAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdAOutAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutAsyncStop', error);
end;


procedure CPwrDAQAPI.PwdAOutGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutGetBufState(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutGetBufState(hAdapter, NumScans,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutGetBufState', error);
end;


procedure CPwrDAQAPI.PwdAOAsyncInit(hAdapter : THANDLE;dwAOCfg : ULONG;
                                dwAOCvClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG;pdwChList : PULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOAsyncInit(hAdapter, @error, dwAOCfg, dwAOCvClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOAsyncInit(hAdapter, dwAOCfg, dwAOCvClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdAOAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdAOAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdAOAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOAsyncStop', error);
end;


procedure CPwrDAQAPI.PwdAOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOGetBufState(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOGetBufState(hAdapter, NumScans,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOGetBufState', error);
end;


procedure CPwrDAQAPI.PwdDIAsyncInit(hAdapter : THANDLE;dwDICfg : ULONG;
                                dwDICvClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG;dwFirstChan : ULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIAsyncInit(hAdapter, @error, dwDICfg, dwDICvClkDiv,
                              dwEventsNotify, dwChListChan, dwFirstChan);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIAsyncInit(hAdapter, dwDICfg, dwDICvClkDiv,
                              dwEventsNotify, dwChListChan, dwFirstChan);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdDIAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdDIAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdDIAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIAsyncStop', error);
end;


procedure CPwrDAQAPI.PwdDIGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIGetBufState(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIGetBufState(hAdapter, NumScans,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIGetBufState', error);
end;


procedure CPwrDAQAPI.PwdDOAsyncInit(hAdapter : THANDLE;dwDOCfg : ULONG;
                                dwDOCvClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG;pdwChList : PULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOAsyncInit(hAdapter, @error, dwDOCfg, dwDOCvClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOAsyncInit(hAdapter, dwDOCfg, dwDOCvClkDiv,
                              dwEventsNotify, dwChListChan, pdwChList);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdDOAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdDOAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdDOAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOAsyncStop', error);
end;


procedure CPwrDAQAPI.PwdDOGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOGetBufState(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOGetBufState(hAdapter, NumScans,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOGetBufState', error);
end;


procedure CPwrDAQAPI.PwdCTAsyncInit(hAdapter : THANDLE;dwCTCfg : ULONG;
                                dwCTCvClkDiv : ULONG;dwEventsNotify : ULONG;
                                dwChListChan : ULONG);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTAsyncInit(hAdapter, @error, dwCTCfg, dwCTCvClkDiv,
                              dwEventsNotify, dwChListChan);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCTAsyncInit(hAdapter, dwCTCfg, dwCTCvClkDiv,
                              dwEventsNotify, dwChListChan);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTAsyncInit', error);
end;

procedure CPwrDAQAPI.PwdCTAsyncTerm(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTAsyncTerm(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCTAsyncTerm(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTAsyncTerm', error);
end;

procedure CPwrDAQAPI.PwdCTAsyncStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTAsyncStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCTAsyncStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTAsyncStart', error);
end;

procedure CPwrDAQAPI.PwdCTAsyncStop(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTAsyncStop(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCTAsyncStop(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTAsyncStop', error);
end;


procedure CPwrDAQAPI.PwdCTGetBufState(hAdapter : THANDLE;NumScans : DWORD;pScanIndex : PDWORD;pNumValidScans : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTGetBufState(hAdapter, @error, NumScans,
                                 pScanIndex, pNumValidScans);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCTGetBufState(hAdapter, NumScans,
                                 pScanIndex, pNumValidScans);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTGetBufState', error);
end;



// Functions defined in pwrdaqll.c:
// Board Level Commands:
procedure CPwrDAQAPI.PwdAdapterEepromRead(hAdapter : THANDLE;dwMaxSize : DWORD;pwReadBuf : PWORD;pdwWords : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAdapterEepromRead(hAdapter, @error, dwMaxSize,
                                   pwReadBuf, pdwWords);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAdapterEepromRead(hAdapter, dwMaxSize,
                                   pwReadBuf, pdwWords);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterEepromRead', error);
end;

procedure CPwrDAQAPI.PwdAdapterEnableInterrupt(hAdapter : THANDLE;dwEnable : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAdapterEnableInterrupt(hAdapter, @error, dwEnable);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAdapterEnableInterrupt(hAdapter, dwEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterEnableInterrupt', error);
end;

// AIn Subsystem Commands:
procedure CPwrDAQAPI.PwdAInSetCfg(hAdapter : THANDLE;dwAInCfg : DWORD;dwAInPreTrig : DWORD;dwAInPostTrig : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSetCfg(hAdapter, @error, dwAInCfg, dwAInPreTrig, dwAInPostTrig);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSetCfg(hAdapter, dwAInCfg, dwAInPreTrig, dwAInPostTrig);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSetCfg', error);
end;

procedure CPwrDAQAPI.PwdAInSetCvClk(hAdapter : THANDLE;dwClkDiv : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSetCvClk(hAdapter, @error, dwClkDiv);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSetCvClk(hAdapter, dwClkDiv);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSetCvClk', error);
end;

procedure CPwrDAQAPI.PwdAInSetClClk(hAdapter : THANDLE;dwClkDiv : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSetClClk(hAdapter, @error, dwClkDiv);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSetClClk(hAdapter, dwClkDiv);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSetClClk', error);
end;

procedure CPwrDAQAPI.PwdAInSetChList(hAdapter : THANDLE;dwCh : DWORD;pdwChList : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSetChList(hAdapter, @error, dwCh, pdwChList);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSetChList(hAdapter, dwCh, pdwChList);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSetChList', error);
end;

procedure CPwrDAQAPI.PwdAInGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInGetStatus(hAdapter, @error, pdwStatus);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInGetStatus(hAdapter, pdwStatus);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PwdAInGetStatus', error);
end;

procedure CPwrDAQAPI.PwdAInEnableConv(hAdapter : THANDLE;dwEnable : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInEnableConv(hAdapter, @error, dwEnable);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInEnableConv(hAdapter, dwEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PwdAInEnableConv', error);
end;

procedure CPwrDAQAPI.PwdAInSwStartTrig(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSwStartTrig(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSwStartTrig(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSwStartTrig', error);
end;

procedure CPwrDAQAPI.PwdAInSwStopTrig(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSwStopTrig(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSwStopTrig(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSwStopTrig', error);
end;

procedure CPwrDAQAPI.PwdAInSwCvStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSwCvStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSwCvStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSwCvStart', error);
end;

procedure CPwrDAQAPI.PwdAInSwClStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSwClStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInSwClStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSwClStart', error);
end;

procedure CPwrDAQAPI.PwdAInResetCl(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInResetCl(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInResetCl(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInResetCl', error);
end;

procedure CPwrDAQAPI.PwdAInClearData(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInClearData(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInClearData(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInClearData', error);
end;

procedure CPwrDAQAPI.PwdAInReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInReset', error);
end;

procedure CPwrDAQAPI.PwdAInGetValue(hAdapter : THANDLE;pwSample : PWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInGetValue(hAdapter, @error, pwSample);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInGetValue(hAdapter, pwSample);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInGetValue', error);
end;

procedure CPwrDAQAPI.PwdAInGetSamples(hAdapter : THANDLE;dwMaxBufSize : DWORD;pwBuf : PWORD;pdwSamples : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInGetSamples(hAdapter, @error, dwMaxBufSize, pwBuf, pdwSamples);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInGetSamples(hAdapter, dwMaxBufSize, pwBuf, pdwSamples);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInGetSamples', error);
end;

procedure CPwrDAQAPI.PwdAInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdAInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}

{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInClearPrivateEvent', error);
end;

// AOut Subsystem Commands:
procedure CPwrDAQAPI.PwdAOutSetCfg(hAdapter : THANDLE;dwAOutCfg : DWORD;dwAOutPostTrig : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSetCfg(hAdapter, @error, dwAOutCfg, dwAOutPostTrig);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutSetCfg(hAdapter, dwAOutCfg, dwAOutPostTrig);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSetCfg', error);
end;

procedure CPwrDAQAPI.PwdAOutSetCvClk(hAdapter : THANDLE;dwClkDiv : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSetCvClk(hAdapter, @error, dwClkDiv);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutSetCvClk(hAdapter, dwClkDiv);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSetCvClk', error);
end;

procedure CPwrDAQAPI.PwdAOutGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutGetStatus(hAdapter, @error, pdwStatus);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutGetStatus(hAdapter, pdwStatus);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutGetStatus', error);
end;

procedure CPwrDAQAPI.PwdAOutEnableConv(hAdapter : THANDLE;dwEnable : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutEnableConv(hAdapter, @error, dwEnable);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutEnableConv(hAdapter, dwEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutEnableConv', error);
end;

procedure CPwrDAQAPI.PwdAOutSwStartTrig(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSwStartTrig(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutSwStartTrig(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSwStartTrig', error);
end;

procedure CPwrDAQAPI.PwdAOutSwStopTrig(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSwStopTrig(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutSwStopTrig(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSwStopTrig', error);
end;

procedure CPwrDAQAPI.PwdAOutSwCvStart(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSwCvStart(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutSwCvStart(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSwCvStart', error);
end;

procedure CPwrDAQAPI.PwdAOutClearData(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutClearData(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutClearData(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutClearData', error);
end;

procedure CPwrDAQAPI.PwdAOutReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutReset', error);
end;

procedure CPwrDAQAPI.PwdAOutPutValue(hAdapter : THANDLE;dwValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutPutValue(hAdapter, @error, dwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutPutValue(hAdapter, dwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutPutValue', error);
end;

procedure CPwrDAQAPI.PwdAOutPutBlock(hAdapter : THANDLE;dwValues : DWORD;pdwBuf : PDWORD;pdwCount : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutPutBlock(hAdapter, @error, dwValues, pdwBuf, pdwCount);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAOutPutBlock(hAdapter, dwValues, pdwBuf, pdwCount);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutPutBlock', error);
end;

procedure CPwrDAQAPI.PwdAOutSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdAOutClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOutClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutClearPrivateEvent', error);
end;


procedure CPwrDAQAPI.PwdAOSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdAOClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAOClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOClearPrivateEvent', error);
end;


procedure CPwrDAQAPI.PwdDISetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDISetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDISetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdDIClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIClearPrivateEvent', error);
end;


procedure CPwrDAQAPI.PwdDOSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdDOClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOClearPrivateEvent', error);
end;


procedure CPwrDAQAPI.PwdCTSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdCTClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCTClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCTClearPrivateEvent', error);
end;


procedure CPwrDAQAPI.PwdAOutVoltsToRaw(hAdapter: THANDLE; dwMode: DWORD; pfVoltage: PDouble;
                                 pdwRawData: PDWORD; dwCount: DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := PdAOutVoltsToRaw(hAdapter, dwMode, pfVoltage, pdwRawData, dwCount);
{$ENDIF}
{$IFDEF LINUX}
   bResult := PdAOutVoltsToRaw(hAdapter, dwMode, pfVoltage, pdwRawData, dwCount);
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAOutVoltsToRaw', error);
end;

// DIn Subsystem Commands:
procedure CPwrDAQAPI.PwdDInSetCfg(hAdapter : THANDLE;dwDInCfg : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInSetCfg(hAdapter, @error, dwDInCfg);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDInSetCfg(hAdapter, dwDInCfg);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInSetCfg', error);
end;

procedure CPwrDAQAPI.PwdDInGetStatus(hAdapter : THANDLE;pdwEvents : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInGetStatus(hAdapter, @error, pdwEvents);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDInGetStatus(hAdapter, pdwEvents);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInGetStatus', error);
end;

procedure CPwrDAQAPI.PwdDInRead(hAdapter : THANDLE;pdwValue : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInRead(hAdapter, @error, pdwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDInRead(hAdapter, pdwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInRead', error);
end;

procedure CPwrDAQAPI.PwdDInClearData(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInClearData(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDInClearData(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInClearData', error);
end;

procedure CPwrDAQAPI.PwdDInReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDInReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInReset', error);
end;

procedure CPwrDAQAPI.PwdDInSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdDInClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDInClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDInClearPrivateEvent', error);
end;

// DOut Subsystem Commands:
procedure CPwrDAQAPI.PwdDOutWrite(hAdapter : THANDLE;dwValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOutWrite(hAdapter, @error, dwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOutWrite(hAdapter, dwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOutWrite', error);
end;

procedure CPwrDAQAPI.PwdDOutReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDOutReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDOutReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDOutReset', error);
end;

// DIO256 Subsystem Commands:
procedure CPwrDAQAPI.PwdDIOReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOReset', error);
end;

procedure CPwrDAQAPI.PwdDIOEnableOutput(hAdapter : THANDLE; dwRegMask : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOEnableOutput(hAdapter, @error, dwRegMask);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOEnableOutput(hAdapter, dwRegMask);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOEnableOutput', error);
end;

procedure CPwrDAQAPI.PwdDIOLatchAll(hAdapter : THANDLE; dwRegister : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOLatchAll(hAdapter, @error, dwRegister);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOLatchAll(hAdapter, dwRegister);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOLatchAll', error);
end;

procedure CPwrDAQAPI.PwdDIOSimpleRead(hAdapter : THANDLE; dwRegister : DWORD; pdwValue : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOSimpleRead(hAdapter, @error, dwRegister, pdwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOSimpleRead(hAdapter, dwRegister, pdwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOSimpleRead', error);
end;

procedure CPwrDAQAPI.PwdDIORead(hAdapter : THANDLE; dwRegister : DWORD; pdwValue : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIORead(hAdapter, @error, dwRegister, pdwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIORead(hAdapter, dwRegister, pdwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIORead', error);
end;

procedure CPwrDAQAPI.PwdDIOWrite(hAdapter : THANDLE; dwRegister : DWORD; dwValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOWrite(hAdapter, @error, dwRegister, dwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOWrite(hAdapter, dwRegister, dwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOWrite', error);
end;

procedure CPwrDAQAPI.PwdDIOPropEnable(hAdapter : THANDLE; dwRegMask : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOPropEnable(hAdapter, @error, dwRegMask);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOPropEnable(hAdapter, dwRegMask);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOPropEnable', error);
end;

procedure CPwrDAQAPI.PwdDIOExtLatchEnable(hAdapter : THANDLE; dwRegister :  DWORD; bEnable : Integer);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOExtLatchEnable(hAdapter, @error, dwRegister, LongBool(bEnable));
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOExtLatchEnable(hAdapter, dwRegister, bEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOExtLatchEnable', error);
end;

procedure CPwrDAQAPI.PwdDIOExtLatchRead(hAdapter : THANDLE; dwRegister : DWORD; bLatch : PInteger);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOExtLatchRead(hAdapter, @error, dwRegister, PBOOL(bLatch));
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOExtLatchRead(hAdapter, dwRegister, bLatch);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOExtLatchRead', error);
end;

procedure CPwrDAQAPI.PwdDIOSetIntrMask(hAdapter : THANDLE; dwIntMask : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOSetIntrMask(hAdapter, @error, dwIntMask);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOSetIntrMask(hAdapter, dwIntMask);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOSetIntrMask', error);
end;

procedure CPwrDAQAPI.PwdDIOGetIntrData(hAdapter : THANDLE; dwIntData : PDWORD; dwEdgeData : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOGetIntrData(hAdapter, @error, dwIntData, dwEdgeData);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOGetIntrData(hAdapter, dwIntData, dwEdgeData);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOGetIntrData', error);
end;

procedure CPwrDAQAPI.PwdDIOIntrEnable(hAdapter : THANDLE; dwEnable : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOIntrEnable(hAdapter, @error, dwEnable);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOIntrEnable(hAdapter, dwEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOIntrEnable', error);
end;

procedure CPwrDAQAPI.PwdDIOSetIntCh(hAdapter : THANDLE; dwChannels :  PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIOSetIntCh(hAdapter, @error, dwChannels);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIOSetIntCh(hAdapter, dwChannels);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIOSetIntCh', error);
end;

procedure CPwrDAQAPI.PwdDIODMASet(hAdapter : THANDLE; dwOffset : PDWORD; dwCount : PDWORD; dwSource : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIODMASet(hAdapter, @error, dwOffset, dwCount, dwSource);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIODMASet(hAdapter, dwOffset, dwCount, dwSource);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIODMASet', error);
end;

procedure CPwrDAQAPI.PwdDIO256CmdWrite(hAdapter : THANDLE;dwCmd : DWORD;dwValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIO256CmdWrite(hAdapter, @error, dwCmd, dwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIO256CmdWrite(hAdapter, dwCmd, dwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIO256CmdWrite', error);
end;

procedure CPwrDAQAPI.PwdDIO256CmdRead(hAdapter : THANDLE;dwCmd : DWORD;pdwValue : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDIO256CmdRead(hAdapter, @error, dwCmd, pdwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDIO256CmdRead(hAdapter, dwCmd, pdwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDIO256CmdRead', error);
end;

procedure CPwrDAQAPI.PwdDspRegWrite(hAdapter : THANDLE;dwReg : DWORD;dwValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDspRegWrite(hAdapter, @error, dwReg, dwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDspRegWrite(hAdapter, dwReg, dwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDspRegWrite', error);
end;

procedure CPwrDAQAPI.PwdDspRegRead(hAdapter : THANDLE;dwReg : DWORD;pdwValue : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdDspRegRead(hAdapter, @error, dwReg, pdwValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdDspRegRead(hAdapter, dwReg, pdwValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdDspRegRead', error);
end;


// AO32 Subsystem Commands
procedure CPwrDAQAPI.PwdAO32Reset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAO32Reset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAO32Reset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAO32Reset', error);
end;

procedure CPwrDAQAPI.PwdAO32Write(hAdapter : THANDLE; wChannel : WORD; wValue : WORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAO32Write(hAdapter, @error, wChannel, wValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAO32Write(hAdapter, wChannel, wValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAO32Write', error);
end;

procedure CPwrDAQAPI.PwdAO32WriteHold(hAdapter : THANDLE; wChannel : WORD; wValue : WORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAO32WriteHold(hAdapter, @error, wChannel, wValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAO32WriteHold(hAdapter, wChannel, wValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAO32Write', error);
end;

procedure CPwrDAQAPI.PwdAO32Update(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAO32Update(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAO32Update(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAO32Update', error);
end;

procedure CPwrDAQAPI.PwdAO32SetUpdateChannel(hAdapter : THANDLE; wChannel : WORD; bEnable : Integer);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAO32SetUpdateChannel(hAdapter, @error, wChannel, BOOL(bEnable));
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAO32SetUpdateChannel(hAdapter, wChannel, bEnable);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAO32SetUpdateChannel', error);
end;

// UCT Subsystem Commands:
procedure CPwrDAQAPI.PwdUctSetCfg(hAdapter : THANDLE;dwUctCfg : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctSetCfg(hAdapter, @error, dwUctCfg);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctSetCfg(hAdapter, dwUctCfg);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctSetCfg', error);
end;

procedure CPwrDAQAPI.PwdUctGetStatus(hAdapter : THANDLE;pdwStatus : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctGetStatus(hAdapter, @error, pdwStatus);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctGetStatus(hAdapter, pdwStatus);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctGetStatus', error);
end;

procedure CPwrDAQAPI.PwdUctWrite(hAdapter : THANDLE;dwUctWord : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctWrite(hAdapter, @error, dwUctWord);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctWrite(hAdapter, dwUctWord);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctWrite', error);
end;

procedure CPwrDAQAPI.PwdUctRead(hAdapter : THANDLE;dwUctReadCfg : DWORD;pdwUctWord : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctRead(hAdapter, @error, dwUctReadCfg, pdwUctWord);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctRead(hAdapter, dwUctReadCfg, pdwUctWord);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctRead', error);
end;

procedure CPwrDAQAPI.PwdUctSwSetGate(hAdapter : THANDLE;dwGateLevels : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctSwSetGate(hAdapter, @error, dwGateLevels);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctSwSetGate(hAdapter, dwGateLevels);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctSwSetGate', error);
end;

procedure CPwrDAQAPI.PwdUctSwClkStrobe(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctSwClkStrobe(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctSwClkStrobe(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctSwClkStrobe', error);
end;

procedure CPwrDAQAPI.PwdUctReset(hAdapter : THANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctReset(hAdapter, @error);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUctReset(hAdapter);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctReset', error);
end;

procedure CPwrDAQAPI.PwdUctSetPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctSetPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}
   error := 0;
   phNotifyEvent^ := hAdapter;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctSetPrivateEvent', error);
end;

procedure CPwrDAQAPI.PwdUctClearPrivateEvent(hAdapter : THANDLE;phNotifyEvent : PHANDLE);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdUctClearPrivateEvent(hAdapter, phNotifyEvent);
{$ENDIF}
{$IFDEF LINUX}

{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdUctClearPrivateEvent', error);
end;

//----------------------------------
procedure CPwrDAQAPI.PwdAInGetDataCount(hAdapter : THANDLE;pdwSamples : PDWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAInGetDataCount(hAdapter, @error, pdwSamples);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAInGetDataCount(hAdapter, pdwSamples);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAInGetDataCount', error);
end;
//----------------------------------

// Calibration Commands:
procedure CPwrDAQAPI.PwdCalSet(hAdapter : THANDLE;dwCalCfg : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCalSet(hAdapter, @error, dwCalCfg);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCalSet(hAdapter, dwCalCfg);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCalSet', error);
end;

procedure CPwrDAQAPI.PwdCalDACWrite(hAdapter : THANDLE;dwCalDACValue : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdCalDACWrite(hAdapter, @error, dwCalDACValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdCalDACWrite(hAdapter, dwCalDACValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdCalDACWrite', error);
end;

procedure CPwrDAQAPI.PwdCalDACSet(hAdapter : THANDLE;nDAC : Integer;nOut : Integer;nValue : Integer);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   error := _CalDACSet(hAdapter, @error, nDAC, nOut, nValue);
{$ENDIF}
{$IFDEF LINUX}
   error := _CalDACSet(hAdapter, nDAC, nOut, nValue);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('CalDACSet', error);
end;

procedure CPwrDAQAPI.PwdAdapterEepromWrite(hAdapter : THANDLE;pwWriteBuf : PWORD;dwSize : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdAdapterEepromWrite(hAdapter, @error, pwWriteBuf, dwSize);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdAdapterEepromWrite(hAdapter, pwWriteBuf, dwSize);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAdapterEepromWrite', error);
end;

// Functions defined in pddrvmem.c:
procedure CPwrDAQAPI.PwdAllocateBuffer(hAdapter: THANDLE; pBuffer : PPWORD;dwFrames : DWORD;dwScans : DWORD;dwScanSize : DWORD;
                                       dwSubsystem : DWORD;bWrapAround : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   {bResult := __PdAllocateBuffer(pBuffer, dwFrames, dwScans, dwScanSize, @error, bWrapAround);
   if bResult then
      bResult := _PdRegisterBuffer(hAdapter, @error, PWORD(pBuffer^), dwSubsystem, bWrapAround); }
   bResult := _PdAcquireBuffer(hAdapter,  @error, pBuffer, dwFrames,
                                dwScans, dwScanSize, dwSubsystem, bWrapAround);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdRegisterBuffer(hAdapter, PWORD(pBuffer), dwSubsystem, dwFrames,
                              dwScans, dwScanSize, bWrapAround);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdAllocateBuffer', error);
end;

procedure CPwrDAQAPI.PwdFreeBuffer(hAdapter : THANDLE; pBuffer : PWORD; dwSubSystem : DWORD);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   {bResult := _PdUnRegisterBuffer(hAdapter, @error, pBuffer);
   if bResult then
      bResult := _PdFreeBuffer(pBuffer, @error); }
   bResult := _PdReleaseBuffer(hAdapter, @error, dwSubSystem, pBuffer);
{$ENDIF}
{$IFDEF LINUX}
   error := _PdUnRegisterBuffer(hAdapter, pBuffer, dwSubSystem);
   if error < 0 then
      bResult := false;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdFreeBuffer', error);
end;


procedure CPwrDAQAPI.PwdGetAdapterInfo(dwBoardNum: DWORD; Adp_Info: PADAPTER_INFO_STRUCT);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   bResult := _PdGetAdapterInfo(dwBoardNum, @error, Adp_Info);
{$ENDIF}
{$IFDEF LINUX}
   error := __PdGetAdapterInfo(dwBoardNum, Adp_Info);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('PdGetAdapterInfo', error);
end;

procedure CPwrDAQAPI.PwdGetSerialNumber(handle : THANDLE; serial : PChar);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   error := 0;
{$ENDIF}
{$IFDEF LINUX}
   error := _PdGetSerialNumber(handle, serial);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('_PdGetSerialNumber', error);
end;

procedure CPwrDAQAPI.PwdGetManufactureDate(handle : THANDLE; manfctrDate : PChar);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   error := 0;
{$ENDIF}
{$IFDEF LINUX}
   error := _PdGetManufactureDate(handle, manfctrDate);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('_PdGetSerialNumber', error);
end;

procedure CPwrDAQAPI.PwdGetCalibrationDate(handle : THANDLE; calDate : PChar);
var
   error : DWORD;
   bResult : BOOL;
begin
   bResult := True;
{$IFDEF MSWINDOWS}
   error := 0;
{$ENDIF}
{$IFDEF LINUX}
   error := _PdGetCalibrationDate(handle, calDate);
   if(error < 0) then
      bResult := False;
{$ENDIF}
   if not bResult then
      raise EPwrDAQ.Create('_PdGetCalibrationDate', error);
end;

end.
