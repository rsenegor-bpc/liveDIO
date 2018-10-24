unit SimpleAI;

interface

uses
  SysUtils, Types, Classes, Variants, QGraphics, QControls, QForms, QDialogs,
  QStdCtrls,
  pdtypes,
  pwrdaq_struct,
  pwrdaq,
  pwrdaq32,
  pwrdaq_LinuxAPI,
  pd_hcaps,
  pdfw_bitsdef, QExtCtrls, QComCtrls;

type
  TForm1 = class(TForm)
    Start: TButton;
    Quit: TButton;
    DriverVer: TEdit;
    Label5: TLabel;
    AIConfig: TPageControl;
    TabSheetAcquisiton: TTabSheet;
    DeviceList: TComboBox;
    Label7: TLabel;
    Device: TEdit;
    Channel: TEdit;
    Label1: TLabel;
    SamplingFreqEdit: TEdit;
    Frequency: TLabel;
    NumberSamples: TEdit;
    Label2: TLabel;
    TabSheet1: TTabSheet;
    InputMode: TRadioGroup;
    InputRange: TRadioGroup;
    ComboBox1: TComboBox;
    Label3: TLabel;
    TabSheet2: TTabSheet;
    ErrorCode: TListBox;
    Data: TListBox;
    procedure QuitClick(Sender: TObject);
    procedure StartClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure SetError(code : Integer; sender : string);
    procedure ai_async(boardNumber : Integer);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

  TAcqState = (unconfigured, configured, running);
  TPBinaryData = ^TBinaryData;
  TBinaryData = array[0..MaxInt div 64] of WORD;
  TPDoubleData = ^TDoubleData;
  TDoubleData = array[0..MaxInt div 64] of Double;

var
  Form1: TForm1;

implementation

{$R *.xfm}

procedure TForm1.ai_async(boardNumber : Integer);
var  aihandle : Integer;
     aiConfig : Cardinal;
     ret : Integer;
     pAIBuf : TPBinaryData;
     pVoltBuf : TPDoubleData;
     nbOfFrames, nbOfScansPerFrame, nbOfSamplesPerScan : Integer;
     sampleFrequency : double;
     channels : array[0..64] of Integer;
     commaPos, i, j, code : Integer;
     tempStr : string;
     clkDivider : Integer;
     eventsToNotify, events : Integer;
     nbOfValidScans, scanIndex, sampIndex : Integer;
     acqState : TAcqState;
begin
   acqState := unconfigured;
   // Get number of channels and build an array of channels
   tempStr := Channel.Text;
   aihandle := 0;
   i := 0;
   repeat
      commapos := LastDelimiter(',', tempStr);
      Val(Copy(tempStr, commaPos+1, Length(tempStr)-commaPos), channels[i], code);
      tempStr := Copy(tempStr, 0, commaPos-1);
      i := i + 1;
   until commapos = 0;

   nbOfSamplesPerScan := i;
   Val(NumberSamples.Text, nbOfScansPerFrame, code);
   if (code <> 0) then
   Begin
      SetError(code, 'syntax for nb of samples');
      Exit;
   End;
   nbOfFrames:=4;
   Val(SamplingFreqEdit.Text, sampleFrequency, code);
   if (code <> 0) then
   Begin
      SetError(code, 'syntax for frequency');
      Exit;
   End;
   clkDivider := Trunc((11000000.0 / sampleFrequency) - 1.0);
   eventsToNotify := eFrameDone or eBufferDone or
                    eBufferWrapped or eTimeout or
                    eBufferError or eStopped or
                    eStartTrig or eStopTrig;

   aiConfig := 0 or AIB_CLSTART0 or AIB_CVSTART0 or AIB_CVSTART1;
   // set input mode bit if user requests differential mode
   if(InputMode.ItemIndex = 1) then
      aiConfig := aiConfig or AIB_INPMODE;

   // set input range bit if user requests a 10V range
   if((InputRange.ItemIndex = 1) or (InputRange.ItemIndex = 3)) then
      aiConfig := aiConfig or AIB_INPRANGE;

   // set input type bit if user requests a bipolar type
   if((InputRange.ItemIndex = 2) or (InputRange.ItemIndex = 3)) then
      aiConfig := aiConfig or AIB_INPTYPE;


   try
      aihandle := PdAcquireSubsystem(boardNumber, AnalogIn, 1);
      if (aihandle < 0) then
      Begin
         SetError(aihandle, 'PdAcquireSubsystem');
         Exit;
      End;

      pAIBuf:=nil;
      ret := _PdRegisterBuffer(aihandle, @pAIBuf, AnalogIn,
                     nbOfFrames, nbOfScansPerFrame, nbOfSamplesPerScan,
                     BUF_BUFFERRECYCLED or BUF_BUFFERWRAPPED);
      if (ret < 0) then
      Begin
         SetError(ret, '_PdRegisterBuffer');
         Exit;
      End;

      ret := _PdAInAsyncInit(aihandle, aiConfig, 0, 0,
                              clkDivider, clkDivider,
                              eventsToNotify,
                              nbOfSamplesPerScan, @channels);
      if (ret < 0) then
      Begin
         SetError(ret, '_PdAInAsyncInit');
         Exit;
      End;

      acqState := configured;

      ret := _PdAInAsyncStart(aihandle);
      if (ret < 0) then
      Begin
         SetError(ret, '_PdAInAsyncStart');
         Exit;
      End;

      acqState := running;

      events := _PdWaitForEvent(aihandle, eventsToNotify, 5000);

      if ((events and eBufferError) or (events and eStopped) <> 0) then
      begin
         SetError(-103, 'Buffer');
         Exit;
      end;

      if((events and eTimeout) <> 0) then
      begin
         SetError(-104, 'Timeout');
         Exit;
      end;

      // deal with the events
      // Get frames using PdAInGetScans
      if ((events and eFrameDone) or (events and eBufferDone) <> 0) then
      begin
         ret := _PdAInGetScans(aihandle, nbOfScansPerFrame,
                              AIN_SCANRETMODE_MMAP,
                              @scanIndex, @nbOfValidScans );
         if (ret<0) then
         begin
            SetError(ret, '_PdAInGetScans');
            Exit;
         end;

         if (nbOfValidScans < nbOfScansPerFrame) then
         begin
            SetError(-104, ' can''t read enough scans');
            Exit;
         end;

         // convert binary data to voltage
         GetMem(pVoltBuf, nbOfValidScans * nbOfSamplesPerScan * SizeOf(Double));

         ret := PdAInRawToVolts(boardNumber, aiConfig, PWORD(pAiBuf),
                         PDouble(pVoltBuf), nbOfValidScans * nbOfSamplesPerScan);

         Data.Clear;
         for i:=0 to nbOfValidScans-1 do
         begin
            tempStr := '';
            for j:=0 to nbOfSamplesPerScan-1 do
            begin
               sampIndex := (ScanIndex+i)*nbOfSamplesPerScan+j;
               if(sampIndex < (nbOfScansPerFrame*nbOfSamplesPerScan)) then
                  //tempStr := tempStr + FloatToStrF(pVoltBuf[sampIndex], ffFixed, 15, 4) + ' ';
                  tempStr := tempStr + IntToStr(pAiBuf[sampIndex]);
            end;
            Data.Items.Add(tempStr);
         end;
      end;

   finally
      if(acqState = running) then
      begin
         ret := _PdAInAsyncStop(aihandle);
         if (ret<0) then
            SetError(ret, '_PdAInAsyncStop');
         acqState := configured;
      end;

      if(acqState = configured) then
      begin
        ret := _PdClearUserEvents(aihandle, AnalogIn, eAllEvents);
        if (ret<0) then
           SetError(ret, '_PdClearUserEvents');

        ret := _PdAInAsyncTerm(aihandle);
        if (ret<0) then
           SetError(ret, '_PdAInAsyncTerm');

        ret := _PdUnregisterBuffer(aihandle, PWORD(pAIBuf), AnalogIn);
        if (ret<0) then
           SetError(ret, '_PdUnregisterBuffer');
        acqState := unconfigured;
      end;

      if(aihandle > 0) then
         PdAcquireSubsystem(aihandle, AnalogIn, 0);

      if(acqState <> unconfigured) then
         SetError(-102, 'bad state');
   end;
end;

procedure TForm1.QuitClick(Sender: TObject);
begin
   Application.Terminate();
end;

procedure TForm1.StartClick(Sender: TObject);
var device : Integer;
begin
   device := DeviceList.ItemIndex;
   ai_async(device);
end;

procedure TForm1.FormCreate(Sender: TObject);
var pdversion : PWRDAQ_VERSION;
    pciConfig : PWRDAQ_PCI_CONFIG;
    devList : TStrings;
    ret, i, j, handle : Integer;
    nbAdapters : Integer;
begin
   devList := TStringList.Create;

   try
      ret := PdGetVersion(@pdversion);
      if(ret < 0) then
         SetError(ret, 'PdGetVersion');

      DriverVer.Text := 'v ' + IntToStr(pdversion.MajorVersion) +
                     '.' + IntToStr(pdversion.MinorVersion);

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
               devList.Add(DAQ_Info[j].lpBoardName);
      end;

      DeviceList.Items := devList;
      DeviceList.ItemIndex := 0;
   finally
      devList.Free;
   end;
end;

procedure TForm1.SetError(code : integer; sender : string);
begin
   ErrorCode.Items.Add('Error ' + IntToStr(code) + ' occured in ' + sender);
end;

end.
