unit pwrdaq_struct;

interface

uses
{$IFDEF MSWINDOWS}
  types,
{$ENDIF}
  pdtypes,
  pwrdaq;
  
//-----------------------------------------------------------------------
// Data Structure Definitions.
//-----------------------------------------------------------------------
type
  PPWRDAQ_PCI_CONFIG = ^PWRDAQ_PCI_CONFIG;
  PWRDAQ_PCI_CONFIG = record
    VendorID : WORD;
    DeviceID : WORD;
    Command : WORD;
    Status : WORD;
    RevisionID : BYTE;
    CacheLineSize : BYTE;
    LatencyTimer : BYTE;
    BaseAddress0 : DWORD;
    SubsystemVendorID : WORD;
    SubsystemID : WORD;
    InterruptLine : BYTE;
    InterruptPin : BYTE;
    MinimumGrant : BYTE;
    MaximumLatency : BYTE;
  end;
var
  _PWRDAQ_PCI_CONFIG : PWRDAQ_PCI_CONFIG;

//
// Driver version and timestamp, along with some system facts
//
type
   PPWRDAQ_VERSION = ^PWRDAQ_VERSION;
   PWRDAQ_VERSION = record
   SystemSize : DWORD;
    NtServerSystem : BOOL;
    NumberProcessors : ULONG;
    MajorVersion : DWORD;
    MinorVersion : DWORD;
    BuildType : char;
    BuildTimeStamp : array[0..40-1] of    char;
  end;
var
  _PWRDAQ_VERSION : PWRDAQ_VERSION;

type
// Event Callback Function
  PD_EVENT_PROC = procedure(pEvents : PPD_EVENTS); stdcall;
  PPD_EVENT_PROC = ^PD_EVENT_PROC;
  
const
  PD_BRD = 0;
  PD_BRD_BASEID = $101;

const
  MAXRANGES = 8;
  MAXGAINS = 4;
  MAXSS = PD_MAX_SUBSYSTEMS;

// Hardware properties definition --------------------------------------
const
  PDHCAPS_BITS = 1;
  PDHCAPS_FIRSTCHAN = 2;
  PDHCAPS_LASTCHAN = 3;
  PDHCAPS_CHANNELS = 4;

// Capabilities structure and table -------------------------------------
type

  PDAQ_Information = ^DAQ_Information;
  DAQ_Information = record
    iBoardID : WORD;           // board ID
    lpBoardName : LPSTR;       // Name of the specified board
    lpBusType : LPSTR;         // Bus type
    lpDSPRAM : LPSTR;          // Type of DSP and volume of RAM
    lpChannels : LPSTR;        // Number of channels of the all types, main string
    lpTrigCaps : LPSTR;        // AIn triggering capabilities
    lpAInRanges : LPSTR;       // AIn ranges
    lpAInGains : LPSTR;        // AIn gains
    lpTransferTypes : LPSTR;   // Types of suported transfer methods
    iMaxAInRate : DWORD;       // Max AIn rate (pacer clock)
    lpAOutRanges : LPSTR;      // AOut ranges
    iMaxAOutRate : DWORD;      // Max AOut rate (pacer clock)
    lpUCTType : LPSTR;         // Type of used UCT
    iMaxUCTRate : DWORD;       // Max UCT rate
    iMaxDIORate : DWORD;       // Max DIO rate
    wXorMask : WORD;           // Xor mask
    wAndMask : WORD;           // And mask
  end;

var
  DAQ_Info: DAQ_Information;

// information structure ----------------------------------------------------
type

  PSUBSYS_INFO_STRUCT = ^TSUBSYS_INFO_STRUCT;
  TSUBSYS_INFO_STRUCT = record
    dwChannels : DWORD;                          // Number of channels of the subsystem type, main string
    dwChBits : DWORD;                            //*NEW* how wide is the channel
    dwRate : DWORD;                              // Maximum output rate
    dwMaxGains : DWORD;                          // = MAXGAINS
    fGains : array[0..MAXGAINS-1] of Single;     // Array of gains
    // Information to convert values
    dwMaxRanges : DWORD;                         // = MAXRANGES
    fRangeLow : array[0..MAXRANGES-1] of Single; // Low part of range
    fRangeHigh : array[0..MAXRANGES-1] of Single;// High part of the range
    fFactor : array[0..MAXRANGES-1] of Single;   // What to mult value to
    fOffset : array[0..MAXRANGES-1] of Single;   // What to substruct from value
    wXorMask : WORD;                             // Xor mask
    wAndMask : WORD;                             // And mask
    dwFifoSize : DWORD;                          // FIFO Size (in samples) for subsystem
    dwChListSize : DWORD;                        // Max number of entries in channel list
     // Variable filled from application side for proper conversion
    dwMode : DWORD;                              // Input/Output Mode
    pdwChGainList : PDWORD;                      // Pointer to Active Channel Gain List
    dwChGainListSize : DWORD;                    // Size of channel list
  end;

const
    atPD2MF  = 1 shl 0;
    atPD2MFS = 1 shl 1;
    atPDMF   = 1 shl 2;
    atPDMFS  = 1 shl 3;
    atPD2AO  = 1 shl 4;
    atPD2DIO = 1 shl 5;
    atPXI    = 1 shl 6;
    atPDLMF  = 1 shl 7;
    atMF     = atPD2MF + atPD2MFS + atPDMF + atPDMFS + atPDLMF;

type
  PADAPTER_INFO_STRUCT = ^TADAPTER_INFO_STRUCT;
  TADAPTER_INFO_STRUCT = record
    dwBoardID: DWORD;                               // board ID
    atType: DWORD;                                  // Adapter type
    lpBoardName: array[0..19] of Char;              // Name of the specified board
    lpSerialNum: array[0..19] of Char;              // Serial Number
    SSI: array[0..MAXSS] of TSUBSYS_INFO_STRUCT;  // Subsystem description array
  end;

var
  AdapterInfo: TADAPTER_INFO_STRUCT;


implementation

end.
 