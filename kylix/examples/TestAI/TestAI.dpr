program TestAI;

{$APPTYPE CONSOLE}

uses Classes,
     pwrdaq_struct,
     pwrdaq,
     pwrdaq32,
     pwrdaq_LinuxAPI,
     pd_hcaps;

procedure SetError(code : Integer; sender : string);
begin
   //ErrorCode.Text := 'Error ' + IntToStr(code) + ' occured in ' + sender;
end;

procedure Test();
var pdversion : PWRDAQ_VERSION;
    pciConfig : PWRDAQ_PCI_CONFIG;
    devList : TStrings;
    nbAdapters : Integer;
    ret, i, j, handle : Integer;
begin
   devList := TStringList.Create;

   try
      ret := PdGetVersion(@pdversion);
      if(ret < 0) then
         SetError(ret, 'PdGetVersion');

      //DriverVer.Text := 'v ' + IntToStr(pdversion.MajorVersion) +
      //               '.' + IntToStr(pdversion.MinorVersion);

      nbAdapters := PdGetNumberAdapters();
      if(nbAdapters < 0) then
         SetError(ret, 'PdGetNumberAdapters');

      for i := 0 to nbAdapters-1 do
      begin
         handle := PdAcquireSubSystem(i, BoardLevel, 1);
         if(handle < 0) then
            SetError(ret, 'PdAcquireSubSystem');

         ret := PdGetPciConfiguration(handle, @pciConfig);
         if(ret < 0) then
            SetError(ret, 'PdGetPciConfiguration');

         ret := PdAcquireSubSystem(handle, BoardLevel, 0);
         if(ret < 0) then
            SetError(ret, 'PdAcquireSubSystem');

         // look-up in the board list for the corresponding
         // subsystem Id
         for j:=0 to PD_BRD_LST-1 do
            if (DAQ_Info[j].iBoardID = pciConfig.SubsystemID) then
            begin
               devList.Add(DAQ_Info[j].lpBoardName);
               writeln('found board: ', DAQ_Info[j].lpBoardName);
            end;
      end;

      //DeviceList.Items := devList;

   finally
      devList.Free;
   end;
end;

begin
   Test();
end.

