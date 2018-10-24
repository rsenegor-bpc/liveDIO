unit pwrdaq;

{$IFDEF VER130}
   {$DEFINE MSWINDOWS}
{$ENDIF}

interface

//===========================================================================
//
// NAME:    pwrdaq.h
//
// DESCRIPTION:
//
//          Definitions for PowerDAQ PCI Device Driver.
//
// NOTES:   This is a common file shared with the Win95 VxD and the
//          Win NT Driver and also included by Win32 DLL and apps.
//
//
//
//
// R DATE:  22-AUG-99
//
// HISTORY:
//
//      Rev 0.1,    15-JUN-97,  A.S.,   Initial version.
//      Rev 0.2,     5-JAN-98,  B.S.,   Revised to final PD Firmware.
//      Rev 0.3,    18-MAR-98   A.I.,   Added changes for autocalibration
//                                      PD_EEPROM support.
//      Rev 0.4,    16-APR-98   A.S.,   Added PD_MAX_SUBSYSTEMS.
//      Rev 0.5,    01-JUN-98,  B.S.,   Reorganized status/event handling.
//      Rev 0.6,    03-MAY-99,  A.I.,   PD2 and DIO256 support added
//      Rev 0.61    21-JUL-99,  A.I.,   SSH control added
//      Rev 0.62    22-AUG-99,  A.I.,   DIn, UCT events added\
//      Rev 2.1.7   30-JAN-01,  A.I.,   A lot of new commands added
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 1997-2001 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================

uses
{$IFDEF MSWINDOWS}
   windows,
   ExtCtrls,
{$ENDIF}
   pdtypes;

{$IFNDEF _INC_PWRDAQ}
  {$DEFINE _INC_PWRDAQ}

const
  INVALID_HANDLE_VALUE = MaxInt;

const
  PWRDAQ_Major = 1;
  PWRDAQ_Minor = 0;

//
// PCI Function Codes for CONFIGMG_Call_Enumerator_Function()
//
const
  PCI_ENUM_FUNC_GET_DEVICE_INFO = 0;
  PCI_ENUM_FUNC_SET_DEVICE_INFO = 1;

//
// PCI Commands:
//
const
  PCI_CMD_IOSPACEENABLE = $0001;
  PCI_CMD_MEMORYSPACEENABLE = $0002;
  PCI_CMD_BUSMASTERENABLE = $0004;

{$ALIGN OFF}

//---------------------------------------------------------------------------
//
// PCI Configuration Space Offsets:
//
const
  PCI_CONFIGREG_VENDORID = $00;
  PCI_CONFIGREG_DEVICEID = $02;
  PCI_CONFIGREG_COMMAND  = $04;
  PCI_CONFIGREG_STATUS   = $06;
  PCI_CONFIGREG_CLASSREV = $08;
  PCI_CONFIGREG_SPACE0   = $10;
  PCI_CONFIGREG_SPACE1   = $14;
  PCI_CONFIGREG_SPACE2   = $18;
  PCI_CONFIGREG_SPACE3   = $1C;
  PCI_CONFIGREG_SPACE4   = $20;
  PCI_CONFIGREG_SPACE5   = $24;
  PCI_CONFIGREG_SUBSYSTEMIDVENDORID = $2C;
  PCI_CONFIGREG_EPROMBASE = $30;

{$IFNDEF PCI_TYPE0_ADDRESSES}
const
  PCI_TYPE0_ADDRESSES = 6;
{$ENDIF} //PCI_TYPE0_ADDRESSES

//---------------------------------------------------------------------------
//
// PCI Configuration Space:
//
type
  PCI_COMMON_CONFIG_0 = record
    VendorID : word;
    DeviceID : word;
    Command : word;
    Status : word;
    RevisionID : byte;
    ProgIf : byte;
    SubClass : byte;
    BaseClass : byte;
    CacheLineSize : byte;
    LatencyTimer : byte;
    HeaderType : byte;
    BIST : byte;
    BaseAddresses : array[0..PCI_TYPE0_ADDRESSES-1] of DWORD;
    CardBusCISPtr : DWORD;
    SubsystemVendorID : word;
    SubsystemID : word;
    ROMBaseAddress : DWORD;
    Reserved2 : array[0..1] of  DWORD;
    InterruptLine : byte;
    InterruptPin : byte;
    MinimumGrant : byte;
    MaximumLatency : byte;
  end;
var
  _PCI_COMMON_CONFIG_0 : PCI_COMMON_CONFIG_0;

const
  MAX_PWRDAQ_ADAPTERS = 8;// max number of PowerDAQ PCI cards in the system
  MAX_PCI_SPACES = 1;// number of PCI Type 0 base address registers

//---------------------------------------------------------------------------
//
// Define the unique device types for PowerDAQ device objects:
//
//!const
//! FILE_DEVICE_PWRDAQ = FILE_DEVICE_UNKNOWN;
//! FILE_DEVICE_PWRDAQX =FILE_DEVICE_UNKNOWN+1;

//---------------------------------------------------------------------------
//
// PowerDAQ IOCTL:
//
//!const
//! IOCTL_PWRDAQ_VERSION =   PWRDAQ_CONTROL_CODE(0, METHOD_BUFFERED);
//! IOCTL_PWRDAQ_ADAPTERS =  PWRDAQ_CONTROL_CODE(1, METHOD_BUFFERED);

{const
 IOCTL_PWRDAQ_PRIVATE_MAP_DEVICE   = PWRDAQX_CONTROL_CODE(0, METHOD_BUFFERED);
 IOCTL_PWRDAQ_PRIVATE_UNMAP_DEVICE = PWRDAQX_CONTROL_CODE(1, METHOD_BUFFERED);
 IOCTL_PWRDAQ_PRIVATE_GETCFG       = PWRDAQX_CONTROL_CODE(2, METHOD_BUFFERED);
 IOCTL_PWRDAQ_PRIVATE_SETCFG       = PWRDAQX_CONTROL_CODE(3, METHOD_BUFFERED);
 IOCTL_PWRDAQ_PRIVATE_SET_EVENT    = PWRDAQX_CONTROL_CODE(4, METHOD_BUFFERED);
 IOCTL_PWRDAQ_PRIVATE_CLR_EVENT    = PWRDAQX_CONTROL_CODE(5, METHOD_BUFFERED);

//
// Win9X only.
//
const
 IOCTL_PWRDAQ_ADAPTER_OPEN   =    PWRDAQX_CONTROL_CODE(6, METHOD_BUFFERED);
 IOCTL_PWRDAQ_ADAPTER_CLOSE  =    PWRDAQX_CONTROL_CODE(7, METHOD_BUFFERED);

// PowerDAQ Generic Operations.
const
  IOCTL_PWRDAQ_ACQUIRESUBSYSTEM =  PWRDAQX_CONTROL_CODE(10, METHOD_BUFFERED);
  IOCTL_PWRDAQ_REGISTER_BUFFER  =  PWRDAQX_CONTROL_CODE(11, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UNREGISTER_BUFFER=  PWRDAQX_CONTROL_CODE(12, METHOD_BUFFERED);
  IOCTL_PWRDAQ_REGISTER_EVENTS  =  PWRDAQX_CONTROL_CODE(13, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UNREGISTER_EVENTS=  PWRDAQX_CONTROL_CODE(14, METHOD_BUFFERED);
  IOCTL_PWRDAQ_GET_EVENTS       =  PWRDAQX_CONTROL_CODE(15, METHOD_BUFFERED);

const
  IOCTL_PWRDAQ_SET_USER_EVENTS  =  PWRDAQX_CONTROL_CODE(20, METHOD_BUFFERED);
  IOCTL_PWRDAQ_CLEAR_USER_EVENTS=  PWRDAQX_CONTROL_CODE(21, METHOD_BUFFERED);
  IOCTL_PWRDAQ_GET_USER_EVENTS  =  PWRDAQX_CONTROL_CODE(22, METHOD_BUFFERED);
  IOCTL_PWRDAQ_IMMEDIATE_UPDATE =  PWRDAQX_CONTROL_CODE(23, METHOD_BUFFERED);
  IOCTL_PWRDAQ_SET_TIMED_UPDATE =  PWRDAQX_CONTROL_CODE(24, METHOD_BUFFERED);

// PowerDAQ Asynchronous Buffered AIn/AOut Operations.
const
  IOCTL_PWRDAQ_AIN_ASYNC_INIT   =  PWRDAQX_CONTROL_CODE(30, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIN_ASYNC_TERM   =  PWRDAQX_CONTROL_CODE(31, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIN_ASYNC_START  =  PWRDAQX_CONTROL_CODE(32, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIN_ASYNC_STOP   =  PWRDAQX_CONTROL_CODE(33, METHOD_BUFFERED);
const
  IOCTL_PWRDAQ_AO_ASYNC_INIT   =  PWRDAQX_CONTROL_CODE(34, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AO_ASYNC_TERM   =  PWRDAQX_CONTROL_CODE(35, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AO_ASYNC_START  =  PWRDAQX_CONTROL_CODE(36, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AO_ASYNC_STOP   =  PWRDAQX_CONTROL_CODE(37, METHOD_BUFFERED);
const
  IOCTL_PWRDAQ_GET_DAQBUF_STATUS = PWRDAQX_CONTROL_CODE(40, METHOD_BUFFERED);
  IOCTL_PWRDAQ_GET_DAQBUF_SCANS  = PWRDAQX_CONTROL_CODE(41, METHOD_BUFFERED);
  IOCTL_PWRDAQ_CLEAR_DAQBUF      = PWRDAQX_CONTROL_CODE(42, METHOD_BUFFERED);

// Low Level PowerDAQ Board Level Commands.
const
  IOCTL_PWRDAQ_BRDRESET          = PWRDAQX_CONTROL_CODE(100, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDEEPROMREAD     = PWRDAQX_CONTROL_CODE(101, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDEEPROMWRITEDATE= PWRDAQX_CONTROL_CODE(102, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDENABLEINTERRUPT= PWRDAQX_CONTROL_CODE(103, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDTESTINTERRUPT  = PWRDAQX_CONTROL_CODE(104, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDSTATUS         = PWRDAQX_CONTROL_CODE(105, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDSETEVNTS1      = PWRDAQX_CONTROL_CODE(106, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDSETEVNTS2      = PWRDAQX_CONTROL_CODE(107, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDFWLOAD         = PWRDAQX_CONTROL_CODE(108, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDFWEXEC         = PWRDAQX_CONTROL_CODE(109, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDREGWR          = PWRDAQX_CONTROL_CODE(110, METHOD_BUFFERED); //AI00312
  IOCTL_PWRDAQ_BRDREGRD          = PWRDAQX_CONTROL_CODE(111, METHOD_BUFFERED); //AI00312

// Low Level PowerDAQ AIn Subsystem Commands.
const
  IOCTL_PWRDAQ_AISETCFG         =  PWRDAQX_CONTROL_CODE(200, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISETCVCLK       =  PWRDAQX_CONTROL_CODE(201, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISETCLCLK       =  PWRDAQX_CONTROL_CODE(202, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISETCHLIST      =  PWRDAQX_CONTROL_CODE(203, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISETEVNT        =  PWRDAQX_CONTROL_CODE(204, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISTATUS         =  PWRDAQX_CONTROL_CODE(205, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AICVEN           =  PWRDAQX_CONTROL_CODE(206, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISTARTTRIG      =  PWRDAQX_CONTROL_CODE(207, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISTOPTRIG       =  PWRDAQX_CONTROL_CODE(208, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISWCVSTART      =  PWRDAQX_CONTROL_CODE(209, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISWCLSTART      =  PWRDAQX_CONTROL_CODE(210, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AICLRESET        =  PWRDAQX_CONTROL_CODE(211, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AICLRDATA        =  PWRDAQX_CONTROL_CODE(212, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIRESET          =  PWRDAQX_CONTROL_CODE(213, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIGETVALUE       =  PWRDAQX_CONTROL_CODE(214, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIGETSAMPLES     =  PWRDAQX_CONTROL_CODE(215, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AISETSSHGAIN     =  PWRDAQX_CONTROL_CODE(216, METHOD_BUFFERED); //AI90721
  IOCTL_PWRDAQ_AI_SET_EVENT     =  PWRDAQX_CONTROL_CODE(217, METHOD_BUFFERED); //AI91206
  IOCTL_PWRDAQ_AI_CLR_EVENT     =  PWRDAQX_CONTROL_CODE(218, METHOD_BUFFERED); //AI91206
  IOCTL_PWRDAQ_AIXFERSIZE       =  PWRDAQX_CONTROL_CODE(219, METHOD_BUFFERED); //AI00312

//?ALEX:
const
  IOCTL_PWRDAQ_AIENABLECLCOUNT  =  PWRDAQX_CONTROL_CODE(120, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIENABLETIMER    =  PWRDAQX_CONTROL_CODE(121, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIFLUSHFIFO      =  PWRDAQX_CONTROL_CODE(122, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AIGETSAMPLECOUNT =  PWRDAQX_CONTROL_CODE(123, METHOD_BUFFERED);

// Low Level PowerDAQ AOut Subsystem Commands.
const
  IOCTL_PWRDAQ_AOSETCFG         =  PWRDAQX_CONTROL_CODE(300, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSETCVCLK       =  PWRDAQX_CONTROL_CODE(301, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSETEVNT        =  PWRDAQX_CONTROL_CODE(302, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSTATUS         =  PWRDAQX_CONTROL_CODE(303, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOCVEN           =  PWRDAQX_CONTROL_CODE(304, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSTARTTRIG      =  PWRDAQX_CONTROL_CODE(305, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSTOPTRIG       =  PWRDAQX_CONTROL_CODE(306, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOSWCVSTART      =  PWRDAQX_CONTROL_CODE(307, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOCLRDATA        =  PWRDAQX_CONTROL_CODE(308, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AORESET          =  PWRDAQX_CONTROL_CODE(309, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOPUTVALUE       =  PWRDAQX_CONTROL_CODE(310, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AOPUTBLOCK       =  PWRDAQX_CONTROL_CODE(311, METHOD_BUFFERED);
  IOCTL_PWRDAQ_AO_SET_EVENT     =  PWRDAQX_CONTROL_CODE(312, METHOD_BUFFERED); //AI91206
  IOCTL_PWRDAQ_AO_CLR_EVENT     =  PWRDAQX_CONTROL_CODE(313, METHOD_BUFFERED); //AI91206

// Low Level PowerDAQ DIn Subsystem Commands.
const
  IOCTL_PWRDAQ_DISETCFG         =  PWRDAQX_CONTROL_CODE(400, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DISTATUS         =  PWRDAQX_CONTROL_CODE(401, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DIREAD           =  PWRDAQX_CONTROL_CODE(402, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DICLRDATA        =  PWRDAQX_CONTROL_CODE(403, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DIRESET          =  PWRDAQX_CONTROL_CODE(404, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DI_SET_EVENT     =  PWRDAQX_CONTROL_CODE(405, METHOD_BUFFERED); //AI91206
  IOCTL_PWRDAQ_DI_CLR_EVENT     =  PWRDAQX_CONTROL_CODE(406, METHOD_BUFFERED); //AI91206

// Low Level PowerDAQ DOut Subsystem Commands.
const
  IOCTL_PWRDAQ_DOWRITE          =  PWRDAQX_CONTROL_CODE(500, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DORESET          =  PWRDAQX_CONTROL_CODE(501, METHOD_BUFFERED);

// Low Level PowerDAQ DIO-256 Subsystem Commands.
const
  IOCTL_PWRDAQ_DIO256CMDWR       = PWRDAQX_CONTROL_CODE(502, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DIO256CMDRD       = PWRDAQX_CONTROL_CODE(503, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DIO256SETINTRMASK = PWRDAQX_CONTROL_CODE(504, METHOD_BUFFERED); //AI00316
  IOCTL_PWRDAQ_DIO256GETINTRDATA = PWRDAQX_CONTROL_CODE(505, METHOD_BUFFERED); //AI00316
  IOCTL_PWRDAQ_DIO256INTRREENABLE= PWRDAQX_CONTROL_CODE(506, METHOD_BUFFERED); //AI00316
  IOCTL_PWRDAQ_DIODMASET         = PWRDAQX_CONTROL_CODE(507, METHOD_BUFFERED); //AI00502

// Low Level PowerDAQ UCT Subsystem Commands.
const
  IOCTL_PWRDAQ_UCTSETCFG        =  PWRDAQX_CONTROL_CODE(600, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTSTATUS        =  PWRDAQX_CONTROL_CODE(601, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTWRITE         =  PWRDAQX_CONTROL_CODE(602, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTREAD          =  PWRDAQX_CONTROL_CODE(603, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTSWGATE        =  PWRDAQX_CONTROL_CODE(604, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTSWCLK         =  PWRDAQX_CONTROL_CODE(605, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCTRESET         =  PWRDAQX_CONTROL_CODE(606, METHOD_BUFFERED);
  IOCTL_PWRDAQ_UCT_SET_EVENT    =  PWRDAQX_CONTROL_CODE(607, METHOD_BUFFERED); //AI91206
  IOCTL_PWRDAQ_UCT_CLR_EVENT    =  PWRDAQX_CONTROL_CODE(608, METHOD_BUFFERED); //AI91206

// Low Level PowerDAQ Cal Subsystem Commands.
const
  IOCTL_PWRDAQ_CALSETCFG        =  PWRDAQX_CONTROL_CODE(700, METHOD_BUFFERED);
  IOCTL_PWRDAQ_CALDACWRITE      =  PWRDAQX_CONTROL_CODE(701, METHOD_BUFFERED);

// Low Level PowerDAQ Diag Subsystem Commands.
const
  IOCTL_PWRDAQ_DIAGPCIECHO      =  PWRDAQX_CONTROL_CODE(800, METHOD_BUFFERED);
  IOCTL_PWRDAQ_DIAGPCIINT       =  PWRDAQX_CONTROL_CODE(801, METHOD_BUFFERED);
  IOCTL_PWRDAQ_BRDEEPROMWRITE   =  PWRDAQX_CONTROL_CODE(802, METHOD_BUFFERED);}


//---------------------------------------------------------------------------
//
// Interface Data Structures:
//
const
  PD_EEPROM_SIZE = 256;// EEPROM size in 16-bit words
  PD_SERIALNUMBER_SIZE = 10;// serial number length in bytes
  PD_DATE_SIZE = 12;// date string length in bytes
  PD_CAL_AREA_SIZE = 32;// EEPROM calibration area size in 16-bit words
  PD_SST_AREA_SIZE = 96;// Startup-state area size in 16-bit words
  DIO_REGS_NUM = 8 ;

type
  PWRDAQ_EEPROM_HDR = record
    ADC_Fifo_Size : byte;       // Size of data FIFOs in kS blocks = 1 for standard
    CL_Fifo_Size  : byte;       // Size of Channel list FIFO in 256 channel blocks = 1 for standard
    SerialNumber : array [0..PD_SERIALNUMBER_SIZE-1] of char;
    ManufactureDate : array [0..PD_DATE_SIZE-1] of char;
    CalibrationDate : array [0..PD_DATE_SIZE-1] of char;
    Revision  : longint;
    FirstUseDate : word;
    CalibrArea : array [0..PD_CAL_AREA_SIZE-1] of word;
    FWModeSelect : word;
    StartupArea : array [0..PD_SST_AREA_SIZE-1] of word;
  end;
  PPWRDAQ_EEPROM  = ^PWRDAQ_EEPROM;
  PWRDAQ_EEPROM = record
    Header : PWRDAQ_EEPROM_HDR;
    WordValues : array[1..1] of word;
  end;


//---------------------------------------------------------------------------
//
// Interface Data Structures:
//

const
  PD_PARAMS16_SIGNATURE = $CADDBABE;

type
   PPD_PARAMS16 = ^PPD_PARAMS16;
   PD_PARAMS16 = record
   dwSignature: DWORD;
   dwFunctionCode: DWORD;
   dwErrorCode: DWORD;

   hTask: DWORD;
   hWnd: DWORD;

   dwAdapterId: DWORD;

   wValue: DWORD;
   dwValue: DWORD;

   wInBufSelector: DWORD;
   wInBufOffset: DWORD;
   dwInBufSize: DWORD;
   wOutBufSelector: DWORD;
   wOutBufOffset: DWORD;
   dwOutBufSize: DWORD;
   dwBytesReturned: DWORD;
  end;
 var
  _PD_PARAMS16 : PD_PARAMS16;

type
  PD_VERSION = record
    SystemSize : DWORD;
    NtServerSystem : BYTE; //BOOLEAN in C++
    NumberProcessors : DWORD;
    MajorVersion : DWORD;
    MinorVersion : DWORD;
    BuildType : char;
    BuildTimeStamp : array[0..40-1] of char;
  end;
var
  _PD_VERSION : PD_VERSION;

type
   PPD_EEPROM = ^PPD_EEPROM;
   PD_EEPROM = record
   ADCFifoSize: CHAR;
   CLFifoSize: CHAR;
   SerialNumber : array[0..PD_SERIALNUMBER_SIZE-1] of CHAR;
   ManufactureDate : array[0..PD_DATE_SIZE-1] of CHAR;
   CalibrationDate : array[0..PD_DATE_SIZE-1] of CHAR;
   Revision: DWORD;
   FirstUseDate : DWORD;
   CalibrArea : array[0..PD_CAL_AREA_SIZE-1] of DWORD;
   FWModeSelect:  DWORD;
   StartupArea  : array[0..PD_SST_AREA_SIZE-1] of DWORD;
 end;
 Header = PD_EEPROM;

type
   PPD_EEPROM_REG = ^PPD_EEPROM_REG;
   PD_EEPROM_REG = record
    dwAdapterId: DWORD;
    Eeeprom: PD_EEPROM;
  end;
 var
  _PD_EEPROM_REG : PD_EEPROM_REG;

const
 PD_MAX_SUBSYSTEMS  = 7;

const
    BoardLevel  = 0;
    AnalogIn    = 1;
    AnalogOut   = 2;
    DigitalIn   = 3;
    DigitalOut  = 4;
    CounterTimer =5;
    CalDiag      =6;
    DspCounter   =7;
type
  PD_SUBSYSTEM=BoardLevel..CalDiag;

type
   PPD_SUBSYSTEM_REQ = ^PPD_SUBSYSTEM_REQ;
   PD_SUBSYSTEM_REQ=record
   dwAdapterId: DWORD;
   Subsystem: PD_SUBSYSTEM;
   dwAcquire: DWORD;
  end;
  var
  _PD_SUBSYSTEM_REQ : PD_SUBSYSTEM_REQ;

type
   PPD_GETCFG = ^PPD_GETCFG;
   PD_GETCFG=record
   dwAdapterId: DWORD;
   ConfigData:    PCI_COMMON_CONFIG_0;
  end;
 var
  _PD_GETCFG : PD_GETCFG;

type
   PPD_SETCFG = ^PPD_SETCFG;
   PD_SETCFG = record
   dwAdapterId: DWORD;
   ConfigData: PCI_COMMON_CONFIG_0;
  end;
 var
  _PD_SETCFG : PD_SETCFG;

type
   PPD_ADAPTER = ^PPD_ADAPTER;
   PD_ADAPTER = record
   dwAdapterId: DWORD;
  end;
 var
  _PD_ADAPTER : PD_ADAPTER;

type
   PPD_VALUE = ^PPD_VALUE;
   PD_VALUE = record
   dwAdapterId: DWORD;
   wValue: Pword;
   dwValue: DWORD;
  end;
 var
  _PD_VALUE : PD_VALUE;

type
   PPD_TWO_DWORDS = ^PPD_TWO_DWORDS;
   PD_TWO_DWORDS = record
   dwAdapterId: DWORD;
   dwCommand: DWORD;
   dwValue: DWORD;
  end;
 var
  _PD_TWO_DWORDS : PD_TWO_DWORDS;

type
   PPD_WORD_ARRAY = ^PPD_WORD_ARRAY;
   PD_WORD_ARRAY =record
   dwAdapterId: DWORD;
   dwNumberWords: DWORD;
   pwValues:  DWORD;
  end;
 var
  _PD_WORD_ARRAY : PD_WORD_ARRAY;

type
  PPD_TIMER = ^PD_TIMER;
  PD_TIMER = record
    dwAdapterId : DWORD;
    bEnable : BOOLEAN;
    dwDelay : DWORD;        // timer delay interval in milliseconds
  end;
var
  _PD_TIMER : PD_TIMER;

type
   PPD_AISETCFG = ^PPD_AISETCFG;
   PD_AISETCFG = record
   dwAdapterId: DWORD;
   dwAInCfg: DWORD;
   dwAInPreTrig: DWORD;
   dwAInPostTrig: DWORD;
  end;
 var
  _PD_AISETCFG : PD_AISETCFG;

type
   PPD_AOSETCFG = ^PPD_AOSETCFG;
   PD_AOSETCFG = record
   dwAdapterId: DWORD;
   dwAOutCfg: DWORD;
   dwAOutPostTrig: DWORD;
  end;
 var
  _PD_AOSETCFG : PD_AOSETCFG;

type
   PPD_NOTIFY_EVENT = ^PPD_NOTIFY_EVENT;
   PD_NOTIFY_EVENT = record
   dwAdapterId: DWORD;
   hNotifyEvent: THANDLE;
   hRing0Event: THANDLE;
  end;
 var
  _PD_NOTIFY_EVENT : PD_NOTIFY_EVENT;

//---------------------------------------------------------------------------
//
type
  PPD_BUFFER = ^PD_BUFFER;
  PD_BUFFER = record
    dwAdapterId : DWORD;    // Adapter ID
    dwSubsystem : PD_SUBSYSTEM;    // subsystem which uses buffer
    dwStartAddress : DWORD; // buffer location
    dwFrames : DWORD;       // number of frames
    dwFrameValues : DWORD;  // Number of samples in frame = dwScans * dwScanSize
    dwScans : DWORD;        // number of scans in the frame, after which AIB_DFRMDONE will be fired
    dwScanSize : DWORD;     // scan size = number of chennels in channel list
    bWrapAround : BOOL;
  end;
var
  _PD_BUFFER : PD_BUFFER;

const
   AIB_BUFFERWRAPPED = 1;
   AIB_BUFFERRECYCLED = 2;
   BUF_BUFFERWRAPPED  = 1;  // now applied to AIn/AOut/DIn/DOut/CT buffers
   BUF_BUFFERRECYCLED = 2;
   BUF_DWORDVALUES    = $10;
   BUF_FIXEDDMA       = $20;


//---------------------------------------------------------------------------
//
// DAQ Buffer Configuration and Status structures.          (BS 15-JUN-98)
//
// *REPLACING PD_BUFFER STRUCT, IMBEDDED VARIABLES, ETC.*
//
type
  PPD_DAQBUF_CONFIG = ^PD_DAQBUF_CONFIG;
  PD_DAQBUF_CONFIG = record
    dwAdapterId : DWORD;    // Adapter ID
    Subsystem : PD_SUBSYSTEM;      // subsystem which uses buffer
    //-----------------------
    pBuf : Pword;                   // ptr to buffer memory
    dwBufSizeInBytes : DWORD;       // buffer size in bytes
    //-----------------------
    dwDataWidth : DWORD;            // size of single value in bytes
    dwMaxValues : DWORD;            // max buffer values
    dwScanValues : DWORD;           // scan list size in values
    dwFrameValues : DWORD;          // frame size in values
    bWrap : byte;                   // wrap buffer  BOOLEAN in C++
    bRecycle : byte;                // (AIn) wrap and recycle frames in buffer BOOLEAN in C++
    dwValidValues : DWORD;          // (AOut) number of valid values in buffer
  end;
var
  _PD_DAQBUF_CONFIG : PD_DAQBUF_CONFIG;

type
  PPD_DAQBUF_STATUS = ^PD_DAQBUF_STATUS;
  PD_DAQBUF_STATUS = record
    dwAdapterId : DWORD;    // Adapter ID
    Subsystem : PD_SUBSYSTEM;      // subsystem which uses buffer
    //-----------------------
    pBuf : Pword;                   // ptr to buffer memory
    dwMaxValues : DWORD;            // max buffer values
    //-----------------------
    dwValueIndex : DWORD;           // value read/written index
    dwScanIndex : DWORD;            // scan read/written index
    dwFrameIndex : DWORD;           // frame read/written index
    //-----------------------
    dwValueCount : DWORD;           // number of values in buffer
    dwScanCount : DWORD;            // number of complete scans in buffer
    dwFrameCount : DWORD;           // number of complete frames in buffer
    //-----------------------
    dwWrapCount : DWORD;            // total num times buffer wrapped
    dwFirstTimestamp : DWORD;       // first sample timestamp
    dwLastTimestamp : DWORD;        // last sample timestamp
  end;
var
  _PD_DAQBUF_STATUS : PD_DAQBUF_STATUS;

//---------------------------------------------------------------------------
// DEFINITIONS FROM PDAPI.H.                                (BS 20-JUN-98)
// Subsystem States (State):
const
    ssConfig            = 0;        // configuration state (default)
    ssStandby           = 1;        // on standby ready to start
    ssRunning           = 2;        // running
    ssPaused            = 3;        // paused
    ssStopped           = 4;        // stopped
    ssDone              = 5;        // operation done, stopped
    ssError             = 6;        // error condition; stopped
type
   PDSubsystemState=ssConfig..ssError;

// Subsystem Events (Events):
//
const
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
  eAllEvents          = ($FFFFF);// set/clear all events
type
  PDEvent=eStartTrig..eAllEvents;

const
  eDInEvent           = (1 shl 0);   // Digital Input event 
  eUct0Event          = (1 shl 1);   // Uct0 countdown event
  eUct1Event          = (1 shl 2);   // Uct1 countdown event
  eUct2Event          = (1 shl 3);   // Uct2 countdown event
type
  PDigEvent=eDInEvent..eUct2Event;

//---------------------------------------------------------------------------
//
// Board hardware event words.
//
type
  PPD_EVENTS = ^PD_EVENTS;
  PD_EVENTS = record
    Board : DWORD;
    ADUIntr : DWORD;
    AIOIntr : DWORD;
    AInIntr : DWORD;
    AOutIntr : DWORD;
  end;
var
  _PD_EVENTS : PD_EVENTS;

type
  PPD_ALL_EVENTS = ^PD_ALL_EVENTS;
  PD_ALL_EVENTS = record
    Board : DWORD;                  // Board status
    ADUIntr : DWORD;
    AIOIntr : DWORD;
    AInIntr : DWORD;
    AOutIntr : DWORD;
    //-----------------------
    AInDriverEvents : DWORD;        // AIn driver generated events
    AOutDriverEvents : DWORD;       // AOut driver generated events
    UctDriverEvents : DWORD;        // UCT driver generated events
  end;
var
  _PD_ALL_EVENTS : PD_ALL_EVENTS;

//---------------------------------------------------------------------------
//                                                      (27-JUN-98)
// Moved the following two event struct definitions from "pdevents.h".
//
//?CHECK THIS:
type

  PD_UCT_EVENT = record
    Uct0 : byte;
    Uct1 : byte;
    Uct2 : byte;
  end;
var
  _PD_UCT_EVENT : PD_UCT_EVENT;

//---------------------------------------------------------------------------
//
// DAQ Subsystem DLL/User Events.                           (BS 20-JUN-98)
// (Hardware / Firmware and Driver generated events for which DLL / User
// application can request to be notified).
//
type
  PPD_USER_EVENTS = ^PD_USER_EVENTS;
  PD_USER_EVENTS = record
    dwAdapterId : DWORD;    // Adapter ID
    Subsystem : PD_SUBSYSTEM;      // subsystem
    //-----------------------
    Events : DWORD;         // subsystem user events
    dwTimestamp : DWORD;    // timestamp (FUTURE)
  end;
var
  _PD_USER_EVENTS : PD_USER_EVENTS;

//---------------------------------------------------------------------------
//
// DaqBuf Get DaqBuf Status Info Struct.                    (BS 28-JUN-98)
//
type
  PPD_DAQBUF_STATUS_INFO = ^PD_DAQBUF_STATUS_INFO;
  PD_DAQBUF_STATUS_INFO = record
    dwAdapterId : DWORD;        // Adapter ID
    Subsystem : PD_SUBSYSTEM;          // subsystem
    //-----------------------
    dwSubsysState : DWORD;      // OUT: current subsystem state
    dwScanIndex : DWORD;        // OUT: buffer index of first scan
    dwNumValidValues : DWORD;   // OUT: number of valid values available
    dwNumValidScans : DWORD;    // OUT: number of valid scans available
    dwNumValidFrames : DWORD;   // OUT: number of valid frames available
    dwWrapCount : DWORD;        // OUT: total num times buffer wrapped

    dwFirstTimestamp : DWORD;   // OUT: first sample timestamp
    dwLastTimestamp : DWORD;    // OUT: last sample timestamp
  end;
var
  _PD_DAQBUF_STATUS_INFO : PD_DAQBUF_STATUS_INFO;

//---------------------------------------------------------------------------
//
// DaqBuf Get Scan Info Struct used in PdAInGetScans.       (BS 27-JUN-98)
//
type
  PPD_DAQBUF_SCAN_INFO = ^PD_DAQBUF_SCAN_INFO;
  PD_DAQBUF_SCAN_INFO = record
    dwAdapterId : DWORD;    // Adapter ID
    Subsystem : PD_SUBSYSTEM;      // subsystem
    //-----------------------
    NumScans : DWORD;       // IN:  number of scans to get
    ScanIndex : DWORD;      // OUT: buffer index of first scan
    NumValidScans : DWORD;  // OUT: number of valid scans available
  end;
var
  _PD_DAQBUF_SCAN_INFO : PD_DAQBUF_SCAN_INFO;


//---------------------------------------------------------------------------
// DEFINITIONS FROM PDAPI.H.                                (BS 20-JUN-98)
const
  PD_MAX_AIN_CHLIST_ENTRIES = 256;

//---------------------------------------------------------------------------
//
// AIn Async Config Struct used in PdAInAsyncInit.          (BS 28-JUN-98)
//
type
  PPD_AIN_ASYNC_CONFIG = ^PD_AIN_ASYNC_CONFIG;
  PD_AIN_ASYNC_CONFIG = record
    dwAdapterId : DWORD;    // Adapter ID
    Subsystem : PD_SUBSYSTEM;      // subsystem
    //-----------------------
    dwAInCfg : DWORD;               // IN: AIn configuration word
    dwAInPreTrigCount : DWORD;      // IN: pre-trigger scan count
    dwAInPostTrigCount : DWORD;     // IN: post-trigger scan count
    dwAInCvClkDiv : DWORD;          // IN: conversion start clock divider
    dwAInClClkDiv : DWORD;          // IN: channel list start clock divider
    dwEventsNotify : DWORD;         // IN: subsystem user events notification
    dwChListChan : DWORD;           // IN: number of channels in list
    ChList : array[0..PD_MAX_AIN_CHLIST_ENTRIES-1] of    DWORD; // IN: channel list data buffer
  end;
var
  _PD_AIN_ASYNC_CONFIG : PD_AIN_ASYNC_CONFIG;

{$ALIGN ON}

{$ENDIF} //_INC_PWRDAQ

//---------------------------------------------------------------------------
// end of pwrdaq.h
implementation
begin
end.







