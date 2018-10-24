unit DAQDefs;

interface

uses Windows, SysUtils;

const
  BaseFrequency: array [Boolean] of Longint = (11000000, 33000000);

type

  Sample  = short;
  PSample = ^Sample;

  PChartBuffer = ^TChartBuffer;
  TChartBuffer = array [1..40] of Cardinal;

  TSampleBuffer = array [0..MaxInt div 64] of Sample;
  PSampleBuffer = ^TSampleBuffer;

  PAnalogInputBuffer = ^TAnalogInputBuffer;
  TAnalogInputBuffer = array [0..MaxInt div 64] of Word;

  TOutputBuffer = record
     SampleBuffer : array [0..200] of Word;
     Position : Word;
     BlockSize : Word;
  end;

  PAnalogOutputBuffer = ^TAnalogOutputBuffer;
  TAnalogOutputBuffer = record
     Block: array [0..1] of TOutputBuffer;
  end;

  TDataFunction = function: Cardinal of object;
  TUpdateViewProc = procedure(Value: Integer) of object;
  TGetDataProc = procedure(Buffer: PSampleBuffer; Count: DWORD; Skipped: DWORD) of object;

  // Simple exception
  TPwrDaqException = class(Exception)
    constructor Create(Msg: String; Error: Longint);
  end;

  TChannelList = array [0..511] of DWORD;

  PFIFOBuffer = ^TFIFOBuffer;
  TFIFOBuffer = record
    FIFO:  PSampleBuffer;            // FIFO
    Head:  Integer;                  // stored data
    Tail:  Integer;                  // working data
    Delta: Integer;                  // Head - Tail
    Size:  Integer;                  // in samples
    Wrap:  Boolean;                  // wrap around flag
    Overflow: Boolean;               // overflow flag
  end;

const
  AnalogIn     = 1;
  AnalogOut    = 2;
  DigitalIn    = 3;
  DigitalOut   = 4;
  CounterTimer = 5;
  CalDiag      = 6;

// Subsystem Events

  eStartTrig          = (1 shl 0);   // start trigger / operation started
  eStopTrig           = (1 shl 1);   // stop trigger / operation stopped
  eInputTrig          = (1 shl 2);   // subsystem specific input trigger

  eDataAvailable      = (1 shl 3);   // new data / points available
  eScanDone           = (1 shl 4);   // scan done
  eFrameDone          = (1 shl 5);   // logical frame done
  eFrameRecycled      = (1 shl 6);   // cyclic buffer frame recycled
  eBlockDone          = (1 shl 7);   // logical block done (FUTURE)
  eBufferDone         = (1 shl 8);   // buffer done
  eBufListDone        = (1 shl 9);   // buffer list done (FUTURE)
  eBufferWrapped      = (1 shl 10);  // cyclic buffer / list wrapped

  eConvError          = (1 shl 11);  // conversion clock error
  eScanError          = (1 shl 12);  // scan clock error
  eDataError          = (1 shl 13);  // data error (out-of-range)
  eBufferError        = (1 shl 14);  // buffer over/under run error
  eTrigError          = (1 shl 15);  // trigger error
  eStopped            = (1 shl 16);  // operation stopped
  eTimeout            = (1 shl 17);  // operation timed-out
  eAllEvents          = ($FFFFF);    // set/clear all events

implementation

constructor TPwrDaqException.Create(Msg: String; Error: Longint);
begin
  if Error > 0 then
    inherited Create(Msg+' failed with error '+IntToStr(Error))
  else if Error = 0
    then inherited Create(Msg)
    else inherited Create(Msg+' failed with unknown error');
end;

end.
