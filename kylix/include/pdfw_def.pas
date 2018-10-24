unit pdfw_def;

interface

//=======================================================================
//
// NAME:    pdfw_def.h
//
//
// DESCRIPTION:
//
//          Definitions for PowerDAQ Firmware Commands.
//
// NOTES:
//
//
// REV:     0.3
//
//
// HISTORY:
//
//      Rev 0.1,    16-JUN-97,  B.S.,   Initial version.
//      Rev 0.2,     7-JUL-97,  B.S.,   Updated list with new commands.
//      Rev 0.3,    17-JUL-97,  B.S.,   Added AIn commands to list.
//      Rev 0.4,    30-JUL-97,  B.S.,   Remove PCI Test Commands.
//      Rev 0.5,    30-JUL-97,  B.S.,   Added Cfg & Status bit defs.
//      Rev 0.6,     7-AUG-97,  B.S.,   Split interface test function
//                                      defs to new file pdfw_if.h.
//      Rev 0.7,    20-NOV-97,  B.S.,   Revised for new PowerDAQ FW.
//      Rev 0.8,    11-DEC-97,  B.S.,   Added CAL commands.
//      Rev 0.9,    17-JAN-98,  B.S.,   Rearranged PCI defs.
//      Rev 0.91    18-MAR-98,  A.I.,   Add changes for autocalibration.
//      Rev 0.92,   08-MAY-98,  A.I.,   Add bits for buffered I/O. ->removed
//      Rev 0.93,   01-JUN-98,  B.S.,   Reorganized status/event handling.
//      Rev 0.95    03-MAY-99,  A.I.,   PDII series and PDDIO support added
//      Rev 0.951   21-JUL-99,  A.I.,   SSH PGA control added, AOut buffer increased
//      Rev 0.96    22-AUG-99,  A.I.,   FW version flags added
//      Rev 0.97    20-SEP-99,  A.I.,   AO board constants added
//      Rev 0.98    12-MAR-2000,A.I.,   DSP registers r/w added along with XFer size
//      Rev 3.00    12-DEC-2000,A.I.,   Revised to rev.3.0
//
//
//-----------------------------------------------------------------------
//
//      Copyright (C) 1997-2000 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//=======================================================================

{$IFNDEF _INC_PDFW_DEF}
{$DEFINE _INC_PDFW_DEF}

//-----------------------------------------------------------------------
// PowerDAQ PCI Definitions.
//-----------------------------------------------------------------------
type
  DWORD = Cardinal;

const
  PCI_VENDORID = $1057;// Motorola PCI Vendor ID
  PCI_DEVICEID = $1801;// DSP56301 PCI Device ID
  PCI_SUBVENID = $54A9;// 'UEI' PCI Subsystem Vendor ID

//-----------------------------------------------------------------------
// PowerDAQ PCI Definitions.
//-----------------------------------------------------------------------
const
  MOTOROLA_VENDORID = $1057;// Motorola PCI Vendor ID
  DSP56301_DEVICEID = $1801;// DSP56301 PCI Device ID
  UEI_SUBVENID = $54A9;// 'UEI' PCI Subsystem Vendor ID
  ADAPTER_SUBID = $0001;// Board # 0101 PCI Subsystem ID

//-----------------------------------------------------------------------
// PowerDAQ Command IVECs.
//-----------------------------------------------------------------------
const
 PD_SUBSYSTEMID_FIRST =     $101;     // First valid PowerDAQ model
 PD_SUBSYSTEMID_LAST  =     $18F;     // Last valid PowerDAQ model
 ADAPTER_AUTOCAL      =     $0300;    // Boards with autocalibration type 1
 PD_AIB_SELMODE0      =     $01FE;   // Firmware mode0 logic support

//-----------------------------------------------------------------------
// Board types
//
//-----------------------------------------------------------------------
// PowerDAQ Command IVECs.
//-----------------------------------------------------------------------
// Start of PowerDAQ Command IVEC table.
{!const
 PCI_IVEC = $72;
 PCI_HNMI = (PCI_IVEC or(1 shl HCVR_HNMI)); // Reset board command

// The following commands are valid only during Secondary Bootstrap:
const
  PCI_LOAD = PCI_IVEC+2;  // Bootstrap code/data load command
  PCI_EXEC = PCI_IVEC+4;  // Bootstrap execute code command

// PowerDAQ Board Level Command IVECs.
const
 PD_BRD_BASE     = PCI_IVEC+2;
 PD_BRDHRDRST    = PCI_HNMI;        // Board Hard Reset (HNMI Interrupt)
 PD_BRDFWDNLD    = PD_BRD_BASE;     // Board Code Loader Download
 PD_BRDSFTRST    = PD_BRD_BASE+2;   // Board Soft Reset
 PD_BRDEPRMRD    = PD_BRD_BASE+4;   // Board SPI EEPROM Read
 PD_BRDWRONDATE  = PD_BRD_BASE+6;   // Board Write First Power-On Date
 PD_BRDINTEN     = PD_BRD_BASE+8;   // Board Interrupt Enable/Disable
 PD_BRDINTACK    = ((PD_BRD_BASE+10) or (1 shl HCVR_HNMI)); // Board Intr. Ack.
 PD_BRDSTATUS    = PD_BRD_BASE+12;  // Board Get Board Status Words
 PD_BRDSETEVNTS1 = PD_BRD_BASE+14;  // Board Set ADUEIntrStat Events
 PD_BRDSETEVNTS2 = PD_BRD_BASE+16;  // Board Set AIOIntr Events
 PD_BRDFWLOAD    = PD_BRD_BASE+18;  // Board Firmware Load
 PD_BRDFWEXEC    = PD_BRD_BASE+20;  // Board Firmware Execute
 PD_BRDREGWR     = PD_BRD_BASE+22;  // DSP Register write
 PD_BRDREGRD     = PD_BRD_BASE+24;  // DSP Register read
 PD_BRD_LAST     = PD_BRDREGRD;

// PowerDAQ AIn Subsystem Command IVECs.
const
 PD_AIN_BASE     = PD_BRD_LAST+2;
 PD_AICFG        = PD_AIN_BASE;   // AIn Set Configuration
 PD_AICVCLK      = PD_AIN_BASE+2;   // AIn Set Conv Clock
 PD_AICLCLK      = PD_AIN_BASE+4;   // AIn Set Channel List Clock
 PD_AICHLIST     = PD_AIN_BASE+6;   // AIn Set Channel List
 PD_AISETEVNT    = PD_AIN_BASE+8;   // AIn Set Events
 PD_AISTATUS     = PD_AIN_BASE+10;  // AIn Get Status
 PD_AICVEN       = PD_AIN_BASE+12;  // AIn Conv enable
 PD_AISTARTTRIG  = PD_AIN_BASE+14;  // AIn Start Trigger
 PD_AISTOPTRIG   = PD_AIN_BASE+16;  // AIn Stop Trigger
 PD_AISWCVSTART  = PD_AIN_BASE+18;  // AIn SW Conv Start
 PD_AISWCLSTART  = PD_AIN_BASE+20;  // AIn SW Ch List Start
 PD_AICLRESET    = PD_AIN_BASE+22;  // AIn Reset Channel List
 PD_AICLRDATA    = PD_AIN_BASE+24;  // AIn Clear Data
 PD_AIRESET      = PD_AIN_BASE+26;  // AIn Reset to default
 PD_AIGETVALUE   = PD_AIN_BASE+28;  // AIn Get Single Value
 PD_AIGETSAMPLES = PD_AIN_BASE+30;  // AIn Get Buffered Samples
 PD_AISETSSHGAIN = PD_AIN_BASE+32;  // AIn Set SSH Gain Register
 PD_AIXFERSIZE   = PD_AIN_BASE+34;  // Set size for XFer (sub 1)
 PD_AIN_LAST     = PD_AIXFERSIZE;

// PowerDAQ AOut Subsystem Command IVECs.
const
 PD_AOUT_BASE   = PD_AIN_LAST+2;
 PD_AOCFG       = PD_AOUT_BASE;    // AOut Set Configuration
 PD_AOCVCLK     = PD_AOUT_BASE+2;  // AOut Set Conv Clock
 PD_AOSETEVNT   = PD_AOUT_BASE+4;  // AOut Set Events
 PD_AOSTATUS    = PD_AOUT_BASE+6;  // AOut Get Status
 PD_AOCVEN      = PD_AOUT_BASE+8;  // AOut Conv Enable
 PD_AOSTARTTRIG = PD_AOUT_BASE+10; // AOut Start Trigger
 PD_AOSTOPTRIG  = PD_AOUT_BASE+12; // AOut Stop Trigger
 PD_AOSWCVSTART = PD_AOUT_BASE+14; // AOut SW Conv Start
 PD_AOCLRDATA   = PD_AOUT_BASE+16; // AOut Clear Data
 PD_AORESET     = PD_AOUT_BASE+18; // AOut Reset to default
 PD_AOPUTVALUE  = PD_AOUT_BASE+20; // AOut Put Single Value
 PD_AOPUTBLOCK  = PD_AOUT_BASE+22; // AOut Put Block
 PD_AOUT_LAST   = PD_AOPUTBLOCK;

// PowerDAQ DIn Subsystem Command IVECs.
const
 PD_DIN_BASE    = PD_AOUT_LAST+2;
 PD_DICFG       = PD_DIN_BASE;     // DIn Set Configuration
 PD_DISTATUS    = PD_DIN_BASE+2;   // DIn Get Status
 PD_DIREAD      = PD_DIN_BASE+4;   // DIn Read Input Value
 PD_DICLRDATA   = PD_DIN_BASE+6;   // DIn Clear Data
 PD_DIRESET     = PD_DIN_BASE+8;   // DIn Reset to default
 PD_DIN_LAST    = PD_DIRESET;

// PowerDAQ DOut Subsystem Command IVECs.
const
 PD_DOUT_BASE   = PD_DIN_LAST+2;
 PD_DOWRITE     = PD_DOUT_BASE;    // DOut Write Value
 PD_DORESET     = PD_DOUT_BASE+2;  // DOut Reset to default
 PD_DOUT_LAST   = PD_DORESET;

// PowerDAQ UCT Subsystem Command IVECs.
const
 PD_UCT_BASE    = PD_DOUT_LAST+2;
 PD_UCTCFG      = PD_UCT_BASE;     // UCT Set Configuration
 PD_UCTSTATUS   = PD_UCT_BASE+2;   // UCT Get Status
 PD_UCTWRITE    = PD_UCT_BASE+4;   // UCT Write
 PD_UCTREAD     = PD_UCT_BASE+6;   // UCT Read
 PD_UCTSWGATE   = PD_UCT_BASE+8;   // UCT SW Set Gate
 PD_UCTSWCLK    = PD_UCT_BASE+10;  // UCT SW Clock Strobe
 PD_UCTRESET    = PD_UCT_BASE+12;  // UCT Reset to default
 PD_UCT_LAST    = PD_UCTRESET;

// PowerDAQ DIO-256 Subsystem Command IVECs.
const
 PD_DIO256_BASE  = PD_UCT_LAST+2;
 PD_DI0256RD    = PD_DIO256_BASE;    // DIO 256 Command (Read/Configure)
 PD_DI0256WR    = PD_DIO256_BASE+2;  // DIO 256 Command (Write/Configure)
 PD_DINSETINTRMASK = PD_DIO256_BASE+4;  // Set DIO 256 Interrupt Mask
 PD_DINGETINTRDATA = PD_DIO256_BASE+6;  // Get DIO 256 Interrupt Data
 PD_DININTRREENABLE = PD_DIO256_BASE+8; // (Re-)enable DIO 256 interrupts
 PD_DIODMASET    = PD_DIO256_BASE+10;
 PD_DIO256_LAST  = PD_DIODMASET;

// PowerDAQ Cal Subsystem Command IVECs.
const
 PD_CAL_BASE   =  PD_DIO256_LAST+2;
 PD_CALCFG     =  PD_CAL_BASE;     // CAL Set Configuration
 PD_CALDACWRITE = PD_CAL_BASE+2;   // CAL DAC Write
 PD_CAL_LAST    = PD_CALDACWRITE;

// >>>> NOTE: THIS LIST WILL GROW AS COMMANDS ARE DEFINED <<<<

// PowerDAQ Diag Subsystem Command IVECs.
const
 PD_DIAG_BASE   = PD_CAL_LAST+2;
 PD_DIAGPCIECHO = PD_DIAG_BASE;    // DIAG PCI Echo test
 PD_DIAGPCIINT  = PD_DIAG_BASE+2;  // DIAG PCI Interrupt test
 PD_BRDEPRMWR   = PD_DIAG_BASE+4;  // Board SPI EEPROM Write
 PD_DIAG_LAST   = PD_BRDEPRMWR;

// PowerDAQ Block Transfer Command IVECs.
const
 PD_BLKXFER_BASE  = PD_DIAG_LAST+2;
 PD_AIN_BLK_XFER = PD_BLKXFER_BASE; // AIn Block Transfer IVEC
 PD_BLKXFER_LAST = X_AIN_BLK_XFER;

//-------------------------------------
const
 PD_LAST = PD_BLKXFER_LAST; }
//***********************************************************************

//***********************************************************************

//-----------------------------------------------------------------------
// Return value constants.
//-----------------------------------------------------------------------
const
  CMD_VALID = 1;// Success return value
  ERR_RET = $00800000;// Error return value

//-----------------------------------------------------------------------
// DAQ Constants.
//-----------------------------------------------------------------------
const
 AIN_BLKSIZE = 512;     // AIn Transfer Block Size
 AOUT_BLKSIZE = 1024;     // AOut Transfer Block Size
//-----------------------------------------------------------------------
// Board Control Subsystem Status (BrdStatus) Bits
//-----------------------------------------------------------------------
const
  BRDB_HINT = (1 shl 0);// PCI Host Interrupt Status
  BRDB_HINTEN = (1 shl 1);// PCI Host Interrupt Enabled
  BRDB_HINTASRT = (1 shl 2);// PCI Host Interrupt Assert Condition
  BRDB_ERR = (1 shl 3);// Board error occured
  BRDB_DSPERR = (1 shl 4);// DSP error occured
  BRDB_PCIERR = (1 shl 5);// PCI Slave Mode FIFO Error occured
  BRDB_HCVERR = (1 shl 6);// PCI Slave Mode HCV Error occured
  BRDB_BMERR = (1 shl 7);// PCI Bus Master Error occured

//-----------------------------------------------------------------------
// Board Subsystem Interrupt Status (BrdIntStat) Debug Only Bits
//-----------------------------------------------------------------------
const
  BRDB_IRQA = (1 shl 0);// (DBG ONLY) DSP IRQ A Interrupt Status
  BRDB_IRQB = (1 shl 1);// (DBG ONLY) DSP IRQ B Interrupt Status
  BRDB_IRQC = (1 shl 2);// (DBG ONLY) DSP IRQ C Interrupt Status
  BRDB_IRQD = (1 shl 3);// (DBG ONLY) DSP IRQ D Interrupt Status
  BRDB_WDTIMER = (1 shl 4);// (DBG ONLY) DSP Watchdog Timer TC
  BRDB_HINTSET = (1 shl 5);// (DBG ONLY) PCI Host Interrupt Set
  BRDB_HINTACK = (1 shl 6);// (DBG ONLY) PCI Host Interrupt Acknowledged

//-----------------------------------------------------------------------
// ADUEIntrStat: AIn/AOut/DIn/UCT/ExTrig Interrupt/Status Register
//
// Note: All Interrupt Mask Bits:
//
//          Write '1' to enable, '0' to disable
//          Read current bit setting
//
//       All Interrupt Status/Clear Bits:
//
//          Write '0' to clear interrupt
//          Read '1' cause active, '0' inactive
//
//       1. UCTxIntr interrupts are active on either edge transition.
//
//       2. ExTrigIm enables both rising edge and falling edge External
//          Trigger input signal interrupts.
//
//       3. B_AInCVDone and B_AInCLDone are used by firmware only in SW Strobe
//          Start clocks.
//
//-----------------------------------------------------------------------
const
  UTB_Uct0Im = (1 shl 0);// UCT 0 Interrupt mask
  UTB_Uct1Im = (1 shl 1);// UCT 1 Interrupt mask
  UTB_Uct2Im = (1 shl 2);// UCT 2 Interrupt mask

const
  UTB_Uct0IntrSC = (1 shl 3);// UCT 0 Interrupt Status/Clear
  UTB_Uct1IntrSC = (1 shl 4);// UCT 1 Interrupt Status/Clear
  UTB_Uct2IntrSC = (1 shl 5);// UCT 2 Interrupt Status/Clear

const
  DIB_IntrIm = (1 shl 6);// DIn Interrupt mask
  DIB_IntrSC = (1 shl 7);// DIn Interrupt Status/Clear

const
  BRDB_ExTrigIm = (1 shl 8);// External Trigger Interrupt mask
  BRDB_ExTrigReSC = (1 shl 9);// Ext Trigger Rising Edge Interrupt Status/Clear
  BRDB_ExTrigFeSC = (1 shl 10);// Ext Trigger Falling Edge Interrupt Status/Clear
//----------------------------------
// Status only bits:
const
  AIB_FNE = (1 shl 11);// 1 = ADC FIFO Not Empty
  AIB_FHF = (1 shl 12);// 1 = ADC FIFO Half Full
  AIB_FF = (1 shl 13);// 1 = ADC FIFO Full
  AIB_CVDone = (1 shl 14);// 1 = ADC Conversion Done
  AIB_CLDone = (1 shl 15);// 1 = ADC Channel List Done
  UTB_Uct0Out = (1 shl 16);// Current state of UCT0 output
  UTB_Uct1Out = (1 shl 17);// Current state of UCT1 output
  UTB_Uct2Out = (1 shl 18);// Current state of UCT2 output

const
  BRDB_ExTrigLevel = (1 shl 19);// Current state of External Trigger input
//#define AOB_CVDone    (1 shl 20) // 1 = DAC CV Done, 0 = DAC Conversion in progress

//-----------------------------------------------------------------------
// AIOIntr:  AIn/AOut Interrupt Registers #1 & #2
//
// Note: All Interrupt Mask Bits:
//
//          Write '1' to enable, '0' to disable
//          Read current bit setting
//
//       All Interrupt Status/Clear Bits:
//
//          Write '0' to clear interrupt
//          Read '1' cause active, '0' inactive
//
//       AOutCVDoneIm, AOutCVDoneSC, AOutCVStrtErrIm, and AOutCVStrtErrSC
//       will be implemented in future board versions with parallel DAC.
//
//       Interrupt bits AIB_CLDoneIm/AIB_CLDoneSC must not be modified
//       by driver or user software.
//
//-----------------------------------------------------------------------
//#define AIB_FNEIm     (1 shl 0)  // AIn FIFO Not Empty Interrupt mask
const
  AIB_FHFIm = (1 shl 1);// AIn FIFO Half Full Interrupt mask
  AIB_CLDoneIm = (1 shl 2);// AIn CL Done Interrupt mask
//#define AIB_FNESC     (1 shl 3)  // AIn FIFO Not Empty Interrupt Status/Clear
const
  AIB_FHFSC = (1 shl 4);// AIn FIFO Half Full Interrupt Status/Clear
  AIB_CLDoneSC = (1 shl 5);// AIn CL Done Interrupt Status/Clear
//#define AOB_CVDoneIm  (1 shl 6)  // AOut CV Done Interrupt mask
//#define AOB_CVDoneSC  (1 shl 7)  // AOut CV Done Interrupt Status/Clear
//----------------------------------
const
  AIB_FFIm = (1 shl 8);// AIn FIFO Full Interrupt mask
  AIB_CVStrtErrIm = (1 shl 9);// AIn CV Start Error Interrupt mask
  AIB_CLStrtErrIm = (1 shl 10);// AIn CL Start Error Interrupt mask
  AIB_OTRLowIm = (1 shl 11);// AIn OTR Low Error Interrupt mask
  AIB_OTRHighIm = (1 shl 12);// AIn OTR High Error Interrupt mask
  AIB_FFSC = (1 shl 13);// AIn FIFO Full Interrupt Status/Clear
  AIB_CVStrtErrSC = (1 shl 14);// AIn CV Start Error Interrupt Status/Clear
  AIB_CLStrtErrSC = (1 shl 15);// AIn CL Start Error Interrupt Status/Clear
  AIB_OTRLowSC = (1 shl 16);// AIn OTR Low Error Interrupt Status/Clear
  AIB_OTRHighSC = (1 shl 17);// AIn OTR High Error Interrupt Status/Clear
//#define AOB_CVStrtErrIm (1 shl 18)// AOut CV Start Error Interrupt mask
//#define AOB_CVStrtErrSC (1 shl 19)// AOut CV Start Error Interrupt Status/Clear

//-----------------------------------------------------------------------
// AIn Subsystem Configuration (AInCfg) Bits
//-----------------------------------------------------------------------
const
  AIB_INPMODE = (1 shl 0);// AIn Input Mode (Single-Ended/Differential)
  AIB_INPTYPE = (1 shl 1);// AIn Input Type (Unipolar/Bipolar)
  AIB_INPRANGE = (1 shl 2);// AIn Input Range (Low/High)
  AIB_CVSTART0 = (1 shl 3);// AIn Conv Start Clk Source (2 bits)
  AIB_CVSTART1 = (1 shl 4);
  AIB_EXTCVS = (1 shl 5);// AIn External Conv Start (Pacer) Clk Edge
  AIB_CLSTART0 = (1 shl 6);// AIn Ch List Start (Burst) Clk Source (2 bits)
  AIB_CLSTART1 = (1 shl 7);
  AIB_EXTCLS = (1 shl 8);// AIn External Ch List Start (Burst) Clk Edge
  AIB_INTCVSBASE = (1 shl 9);// AIn Internal Conv Start Clk Base
  AIB_INTCLSBASE = (1 shl 10);// AIn Internal Ch List Start Clk Base
  AIB_STARTTRIG0 = (1 shl 11);// AIn Start Trigger Source (2 bits)
  AIB_STARTTRIG1 = (1 shl 12);
  AIB_STOPTRIG0 = (1 shl 13);// AIn Stop Trigger Source (2 bits)
  AIB_STOPTRIG1 = (1 shl 14);
  AIB_PRETRIG = (1 shl 15);// Use AIn Pre-Trigger Scan Count
  AIB_POSTTRIG = (1 shl 16);// Use AIn Post-Trigger Scan Count
  //AIB_BLKBUF = (1 shl 17);// Use Block Buffer Mode for data transfer
  AIB_EXTGATE = (1 shl 17);// Use AIn trigger for scan clock gate
  AIB_BUSMSTR = (1 shl 18);// Use Bus Master Mode for data transfer
  AIB_SELMODE0 = (1 shl 19);// Use Bus Master Mode for data transfer
  AIB_SELMODE1 = (1 shl 20);// Use Bus Master Mode for data transfer
  AIB_SELMODE2 = (1 shl 21);// Use Bus Master Mode for data transfer
  AIB_SELMODE3 = (1 shl 22);// Use Bus Master Mode for data transfer
  AIB_SELMODE4 = (1 shl 23);// Use Bus Master Mode for data transfer

const
 AIB_MODEBITS = AIB_INPTYPE or AIB_INPRANGE;  // autocal needed on mode chages

 //-----------------------------------------------------------------------
// DIn Subsystem Configuration (DInCfg) Bits (PD2-DIO boards)
//-----------------------------------------------------------------------
const
 DIB_CVSTART0    = (1 shl 3);  // DIn Conv Start Clk Source (2 bits)
 DIB_CVSTART1    = (1 shl 4);
 DIB_EXTCVS      = (1 shl 5);  // DIn External Conv Start (Pacer) Clk Edge
 DIB_INTCVSBASE  = (1 shl 9);  // DIn Internal Conv Start Clk Base
 DIB_STARTTRIG0  = (1 shl 11); // DIn Start Trigger Source (2 bits)
 DIB_STARTTRIG1  = (1 shl 12);
 DIB_STOPTRIG0   = (1 shl 13); // DIn Stop Trigger Source (2 bits)
 DIB_STOPTRIG1   = (1 shl 14);
 DIB_BLKBUF      = (1 shl 17); // Use Block Buffer Mode for data transfer
 DIB_BUSMSTR     = (1 shl 18); // Use Bus Master Mode for data transfer

//-----------------------------------------------------------------------
// CT Subsystem Configuration (CTCfg) Bits (PD2-DIO boards)
//-----------------------------------------------------------------------
const
 CTB_CVSTART0   = (1 shl 3);  // CT Conv Start Clk Source (2 bits)
 CTB_CVSTART1   = (1 shl 4);
 CTB_EXTCVS     = (1 shl 5);  // CT External Conv Start (Pacer) Clk Edge
 CTB_INTCVSBASE = (1 shl 9);  // CT Internal Conv Start Clk Base
 CTB_STARTTRIG0 = (1 shl 11); // CT Start Trigger Source (2 bits)
 CTB_STARTTRIG1 = (1 shl 12);
 CTB_STOPTRIG0  = (1 shl 13); // CT Stop Trigger Source (2 bits)
 CTB_STOPTRIG1   =(1 shl 14);

//-----------------------------------------------------------------------
// AIn Subsystem Interrupt/Status (AInIntrStat) Bits
//
// Note: Status bits AIB_Enabled, AIB_Active, AIB_BMEnabled, and
//       AIB_BMActive must not be modified by driver or user software.
//
//-----------------------------------------------------------------------
const
  AIB_StartIm = (1 shl 0);// AIn Sample Acquisition Started Int mask
  AIB_StopIm = (1 shl 1);// AIn Sample Acquisition Stopped Int mask
  AIB_SampleIm = (1 shl 2);// AIn One or More Samples Acquired Int mask
  AIB_ScanDoneIm = (1 shl 3);// AIn One or More CL Scans Acquired Int mask
  AIB_ErrIm = (1 shl 4);// AIn Subsystem Error Int mask
  AIB_BMDoneIm = (1 shl 5);// AIn Bus Master Blocks Transferred Int mask
  AIB_BMErrIm = (1 shl 6);// Bus Master Data Transfer Error Int mask
  AIB_BMEmptyIm = (1 shl 7);// Bus Master PRD Table Empty Error Int mask
//----------------------------------
const
  AIB_StartSC = (1 shl 8);// AIn Sample Acquisition Started Status/Clear
  AIB_StopSC = (1 shl 9);// AIn Sample Acquisition Stopped Status/Clear
  AIB_SampleSC = (1 shl 10);// AIn One or More Samples Acquired Status/Clear
  AIB_ScanDoneSC = (1 shl 11);// AIn One or More CL Scans Acquired Status/Clear
  AIB_ErrSC = (1 shl 12);// AIn Subsystem Error Status/Clear
  AIB_BMDoneSC = (1 shl 13);// AIn Bus Master Blocks Transferred Status/Clear
  AIB_BMErrSC = (1 shl 14);// Bus Master Data Transfer Error Status/Clear
  AIB_BMEmptySC = (1 shl 15);// Bus Master PRD Table Empty Error Status/Clear
//----------------------------------
// Status only bits:
const
  AIB_Enabled = (1 shl 16);// AIn Enabled Status
  AIB_Active = (1 shl 17);// AIn Active (Started) Status
  AIB_BMEnabled = (1 shl 18);// AIn Bus Master Enabled Status
  AIB_BMActive = (1 shl 19);// AIn Bus Master Active (Started) Status

//-----------------------------------------------------------------------
// AOut Subsystem Configuration (AOutCfg) Bits
//-----------------------------------------------------------------------
const
  AOB_CVSTART0 = (1 shl 0);// AOut Conv (Pacer) Start Clk Source (2 bits)
  AOB_CVSTART1 = (1 shl 1);
  AOB_INTCVSBASE=  (1 shl 6) ; // 11Mhz (0) or 33MHz (1) internal base clock
  AOB_STARTTRIG0=  (1 shl 7) ; // AOut Start Trigger Source (2 bits)
  AOB_STARTTRIG1=  (1 shl 8) ;
  AOB_STOPTRIG0 =  (1 shl 9) ; // AOut Stop Trigger Source (2 bits)
  AOB_STOPTRIG1 =  (1 shl 10);
  AOB_REGENERATE=  (1 shl 13); // Switch to regenerate mode
  AOB_AOUT32    =  (1 shl 14); // switch to PD2-AO board (format: (channel<<16)|(value & 0xFFFF))
  AOB_DOUT32    =  (1 shl 14); // switch to PD2-DIO board (format: (channel<<16)|(value & 0xFFFF))
  AOB_DMAEN     =  (1 shl 15); // enable analog output DMA mode

const
  AOB_BUFMODE0  =  (1 shl 30); // AOut Buf Mode (Single-Value/Block) (2 bits)
  AOB_BUFMODE1  =  (1 shl 30);
  AOB_EXTCVS    =  (1 shl 30);  // AOut External Conv (Pacer) Clock Edge
  AOB_DACBLK0   =  (1 shl 30);  // DACs Enabled in Block Mode (4 bits)
  AOB_DACBLK1   =  (1 shl 30);
  AOB_DACBLK2   =  (1 shl 30);
  AOB_DACBLK3   =  (1 shl 30);

//-----------------------------------------------------------------------
// DOut Subsystem Configuration (DOutCfg) Bits (PD2-DIO boards)
//-----------------------------------------------------------------------
const
 DOB_CVSTART0    = (1 shl 0);  // DOut Conv (Pacer) Start Clk Source (2 bits)
 DOB_CVSTART1    = (1 shl 1);
 DOB_EXTCVS      = (1 shl 2);  // DOut External Conv (Pacer) Clock Edge
 DOB_STARTTRIG0  = (1 shl 7);  // DOut Start Trigger Source (2 bits)
 DOB_STARTTRIG1  = (1 shl 8);
 DOB_STOPTRIG0   = (1 shl 9);  // DOut Stop Trigger Source (2 bits)
 DOB_STOPTRIG1   = (1 shl 10);
 DOB_BUFMODE0    = (1 shl 11); // DOut Buf Mode (Single-Value/Block) (2 bits)
 DOB_BUFMODE1    = (1 shl 12);
 DOB_REGENERATE  = (1 shl 13); // Switch to regenerate mode

//-----------------------------------------------------------------------
// AOut Subsystem Interrupt/Status (AOutIntrStat) Bits
//
// Note: Status bits AOB_Enabled, AOB_Active, AIB_BMEnabled, AOB_BufFull,
//       AOB_QEMPTY, AOB_QHF, and AOB_QFULL must not be modified by driver
//       or user software.
//
//-----------------------------------------------------------------------
const
  AOB_StartIm = (1 shl 0);// AOut Conversion Started Int mask
  AOB_StopIm = (1 shl 1);// AOut Conversion Stopped Int mask
  AOB_ScanDoneIm = (1 shl 2);// AOut Single Conversion/Scan Done Int mask
  AOB_HalfDoneIm = (1 shl 3);// AOut Half Buffer Done Int mask
  AOB_BufDoneIm = (1 shl 4);// AOut Buffer Done Int mask
  AOB_BlkXDoneIm = (1 shl 5);// AOut Subsystem 
  AOB_BlkYDoneIm = (1 shl 6);// AOut Subsystem 
  AOB_UndRunErrIm = (1 shl 7);// AOut Buffer Underrun Error Int mask
  AOB_CVStrtErrIm = (1 shl 8);// AOut Conversion Start Error Int mask
//----------------------------------
const
  AOB_StartSC = (1 shl 9);// AOut Conversion Started Status/Clear
  AOB_StopSC = (1 shl 10);// AOut Conversion Stopped Status/Clear
  AOB_ScanDoneSC = (1 shl 11);// AOut Single Conversion/Scan Done Status/Clear
  AOB_HalfDoneSC = (1 shl 12);// AOut Half Buffer Done Status/Clear
  AOB_BufDoneSC = (1 shl 13);// AOut Buffer Done Status/Clear
  AOB_BlkXDoneSC = (1 shl 14);// AOut Subsystem
  AOB_BlkYDoneSC = (1 shl 15);// AOut Subsystem
  AOB_UndRunErrSC = (1 shl 16);// AOut Buffer Underrun Error Status/Clear
  AOB_CVStrtErrSC = (1 shl 17);// AOut Conversion Start Error Status/Clear
//----------------------------------
// Status only bits:
const
  AOB_Enabled = (1 shl 18);// AOut Enabled Status
  AOB_Active = (1 shl 19);// AOut Active (Started) Status
  AOB_BufFull = (1 shl 20);// AOut Buffer Full Error Status
  AOB_QEMPTY = (1 shl 21);// AOut Queue Empty Status
  AOB_QHF = (1 shl 22);// AOut Queue Half Full Status
  AOB_QFULL = (1 shl 23);// AOut Queue Full Status

//-----------------------------------------------------------------------
// DIn Subsystem Configuration (DInCfg) Bits
//-----------------------------------------------------------------------
const
  DIB_0CFG0 = (1 shl 0);// DIn Bit 0 Intr Cfg bit 0
  DIB_0CFG1 = (1 shl 1);// DIn Bit 0 Intr Cfg bit 1
  DIB_1CFG0 = (1 shl 2);// DIn Bit 1 Intr Cfg bit 0
  DIB_1CFG1 = (1 shl 3);// DIn Bit 1 Intr Cfg bit 1
  DIB_2CFG0 = (1 shl 4);// DIn Bit 2 Intr Cfg bit 0
  DIB_2CFG1 = (1 shl 5);// DIn Bit 2 Intr Cfg bit 1
  DIB_3CFG0 = (1 shl 6);// DIn Bit 3 Intr Cfg bit 0
  DIB_3CFG1 = (1 shl 7);// DIn Bit 3 Intr Cfg bit 1
  DIB_4CFG0 = (1 shl 8);// DIn Bit 4 Intr Cfg bit 0
  DIB_4CFG1 = (1 shl 9);// DIn Bit 4 Intr Cfg bit 1
  DIB_5CFG0 = (1 shl 10);// DIn Bit 5 Intr Cfg bit 0
  DIB_5CFG1 = (1 shl 11);// DIn Bit 5 Intr Cfg bit 1
  DIB_6CFG0 = (1 shl 12);// DIn Bit 6 Intr Cfg bit 0
  DIB_6CFG1 = (1 shl 13);// DIn Bit 6 Intr Cfg bit 1
  DIB_7CFG0 = (1 shl 14);// DIn Bit 7 Intr Cfg bit 0
  DIB_7CFG1 = (1 shl 15);// DIn Bit 7 Intr Cfg bit 1

//-----------------------------------------------------------------------
// DIn Subsystem Level/Latch (DInStatus) Bits
//-----------------------------------------------------------------------
// Bits 0 - 7:  Digital Input Bit Level
const
  DIB_LEVEL0 = (1 shl 0);// DIn Bit 0 Input Level
  DIB_LEVEL1 = (1 shl 1);// DIn Bit 1 Input Level
  DIB_LEVEL2 = (1 shl 2);// DIn Bit 2 Input Level
  DIB_LEVEL3 = (1 shl 3);// DIn Bit 3 Input Level
  DIB_LEVEL4 = (1 shl 4);// DIn Bit 4 Input Level
  DIB_LEVEL5 = (1 shl 5);// DIn Bit 5 Input Level
  DIB_LEVEL6 = (1 shl 6);// DIn Bit 6 Input Level
  DIB_LEVEL7 = (1 shl 7);// DIn Bit 7 Input Level

// Bits 8 - 15: Digital Input Bit Trigger Status
const
  DIB_INTR0 = (1 shl 8);// DIn Bit 8 Latched Interrupt
  DIB_INTR1 = (1 shl 9);// DIn Bit 9 Latched Interrupt
  DIB_INTR2 = (1 shl 10);// DIn Bit 10 Latched Interrupt
  DIB_INTR3 = (1 shl 11);// DIn Bit 11 Latched Interrupt
  DIB_INTR4 = (1 shl 12);// DIn Bit 12 Latched Interrupt
  DIB_INTR5 = (1 shl 13);// DIn Bit 13 Latched Interrupt
  DIB_INTR6 = (1 shl 14);// DIn Bit 14 Latched Interrupt
  DIB_INTR7 = (1 shl 15);// DIn Bit 15 Latched Interrupt

//-----------------------------------------------------------------------
// DOut Subsystem Configuration (DOutCfg) Bits
//-----------------------------------------------------------------------
// N/A

//-----------------------------------------------------------------------
// DOut Subsystem Interrupt/Status (DOutIntrStat) Bits
//-----------------------------------------------------------------------
// N/A
//-----------------------------------------------------------------------
// DIO 256 Subsystem Configuration (DOI256) Bits
//-----------------------------------------------------------------------
const
   DIO_REG0   = $FFFF00;       // DIO 64 I/O Bank 0 mask 
   DIO_REG1   = $FFFF01;       // DIO 64 I/O Bank 1 mask
   DIO_REG2   = $FFFF02;       // DIO 64 I/O Bank 2 mask
   DIO_REG3   = $FFFF03;       // DIO 64 I/O Bank 3 mask
   DIO_REG4   = $FFFF04;       // DIO 16 I/O Register 0 mask 
   DIO_REG5   = $FFFF05;       // DIO 16 I/O Register 1 mask
   DIO_REG6   = $FFFF06;       // DIO 16 I/O Register 2 mask
   DIO_REG7   = $FFFF07;       // DIO 16 I/O Register 3 mask

   DIO_SRD     = $000000;       // Read register command 
   DIO_SWR     = $000000;       // Write register command 
   DIO_LRD     = $FFFF08;       // Read latch command
   DIO_LWR     = $FFFF08;       // Write latch enable command 
   DIO_PWR     = $FFFF20;       // Write propagate mask command 
   DIO_LAL     = $FFFF40;       // Latch all registers command 
   DIO_LAR     = $000060;       // Latch and read register command 
   DIO_WOE     = $FFFF60;       // Write output enable state command 

   DIO_DIS_0_3 = $FFFF7B;       // Disable reg 0 to 3 outputs
   DIO_DIS_4_7 = $FFFF7F;       // Disable reg 4 to 7 outputs

//-----------------------------------------------------------------------
// AO 32 Subsystem Configuration (AO32) Bits
//-----------------------------------------------------------------------
const
  AO32_WRPR      = $0;         // Write value to the DAC and set it
  AO32_WRH       = $20;        // Write value to the DAC but hold it
  AO32_UPDALL    = $20;        // Read to update all holded DACs
  AO32_SETUPDMD  = $40;        // Read to set last channel autoupdate
  AO32_BASE      = $FFFF00;    // Base address

//-----------------------------------------------------------------------
// UCT Subsystem Configuration (UctCfg) Bits
//-----------------------------------------------------------------------
const
  UTB_CLK0 = (1 shl 0);// UCT 0 Clock Source (2 bits)  00 - sw bit strobe
  UTB_CLK0_1 = (1 shl 1);// 01 - 1 MHz;                11 - external clock
  UTB_CLK1 = (1 shl 2);// UCT 1 Clock Source (2 bits)
  UTB_CLK1_1 = (1 shl 3);//
  UTB_CLK2 = (1 shl 4);// UCT 2 Clock Source (2 bits)
  UTB_CLK2_1 = (1 shl 5);//
  UTB_GATE0 = (1 shl 6);// UCT 0 Gate Source bit  (0 - sw gate; 1- external)
  UTB_GATE1 = (1 shl 7);// UCT 1 Gate Source bit
  UTB_GATE2 = (1 shl 8);// UCT 2 Gate Source bit
  UTB_SWGATE0 = (1 shl 9);// UCT 0 SW Gate Setting bit  (sw gate 1 - enable)
  UTB_SWGATE1 = (1 shl 10);// UCT 1 SW Gate Setting bit
  UTB_SWGATE2 = (1 shl 11);// UCT 2 SW Gate Setting bit
  UTB_INTR0MSK = (1 shl 12);// UCT 0 Output Event Interrupt Mask bit(1-enable)
  UTB_INTR1MSK = (1 shl 13);// UCT 1 Output Event Interrupt Mask bit
  UTB_INTR2MSK = (1 shl 14);// UCT 2 Output Event Interrupt Mask bit
  UTB_FREQMODE0 = (1 shl 15);// UCT 0 freq-count mode set/status
  UTB_FREQMODE1 = (1 shl 16);// UCT 1 freq-count mode set/status
  UTB_FREQMODE2 = (1 shl 17);// UCT 2 freq-count mode set/status

//-----------------------------------------------------------------------
// UCT Subsystem Level/Latch (UctStatus) Bits
//-----------------------------------------------------------------------
const
  UTB_LEVEL0 = (1 shl 0);// UCT 0 Output Level
  UTB_LEVEL1 = (1 shl 1);// UCT 1 Output Level
  UTB_LEVEL2 = (1 shl 2);// UCT 2 Output Level
  UTB_INTR0 = (1 shl 3);// UCT 0 Latched Interrupt
  UTB_INTR1 = (1 shl 4);// UCT 1 Latched Interrupt
  UTB_INTR2 = (1 shl 5);// UCT 2 Latched Interrupt

//-----------------------------------------------------------------------
// Calibration Subsystem Status (CalStatus) Bits
//-----------------------------------------------------------------------
const
  CAL_ACTIVE = (1 shl 0);// CAL Routine Executed flag

//-----------------------------------------------------------------------
// Diagnostics Subsystem Status (DiagStatus) Bits
//-----------------------------------------------------------------------
const
  DIAG_ACTIVE = (1 shl 0);// DIAG Routine Executed flag
  //clock type arrays continuos/internal/external
  AIcontinuos = 0;
  AIinternal  = 1;
  AIexternal =  2;

//---------------------------------------------------------------------------
// Function:    int test_uct1(void)
//
// Description: Testing User Counter/Timer.
//
//---------------------------------------------------------------------------
const
  UCT_BCD          =(1 shl 0);
  UCT_M0           =(1 shl 1);
  UCT_M1           =(1 shl 2);
  UCT_M2           =(1 shl 3);
  UCT_RW0          =(1 shl 4);
  UCT_RW1          =(1 shl 5);
  UCT_SC0          =(1 shl 6);
  UCT_SC1          =(1 shl 7);
  UCT_CNT0         =(1 shl 1);
  UCT_CNT1         =(1 shl 2);
  UCT_CNT2         =(1 shl 3);
  UCT_STATUS       =(1 shl 4);
  UCT_COUNT        =(1 shl 5);
  UCT_NULLCNT      =(1 shl 6);
  UCT_OUTPUT       =(1 shl 7);

  UCT_SelCtr0      =(0);
  UCT_SelCtr1      =(UCT_SC0);
  UCT_SelCtr2      =(UCT_SC1);
  UCT_ReadBack     =(UCT_SC0 OR UCT_SC1);
  UCT_Mode0        =(0);
  UCT_Mode1        =(UCT_M0);
  UCT_Mode2        =(UCT_M1);
  UCT_Mode3        =(UCT_M0 OR UCT_M1);
  UCT_Mode4        =(UCT_M2);
  UCT_Mode5        =(UCT_M0 OR UCT_M2);
  UCT_CtrLatch     =(0);
  UCT_RWlsb        =(UCT_RW0);
  UCT_RWmsb        =(UCT_RW1);
  UCT_RW16bit      =(UCT_RW0 OR UCT_RW1);

  UCTREAD_CFW      =(1 shl 8);
  UCTREAD_UCT0     =(0);
  UCTREAD_UCT1     =(1 shl 9);
  UCTREAD_UCT2     =(2 shl 9);
  UCTREAD_0BYTES   =(0);
  UCTREAD_1BYTE    =(1 shl 11);
  UCTREAD_2BYTES   =(2 shl 11);
  UCTREAD_3BYTES   =(3 shl 11);

  UCT_Mode_Low_High    = UCT_Mode0 OR UCT_RW16Bit;
  UCT_Mode_One_Shot    = UCT_Mode1 OR UCT_RW16Bit;
  UCT_Mode_Rate        = UCT_Mode2 OR UCT_RW16Bit;
  UCT_Mode_Square_Wave = UCT_Mode3 OR UCT_RW16Bit;
  UCT_Mode_Sw_Strobe   = UCT_Mode4 OR UCT_RW16Bit;
  UCT_Mode_Hw_Strobe   = UCT_Mode5 OR UCT_RW16Bit;
  AIClClock : array [AIcontinuos..AIexternal] of DWORD =
    (AIB_CLSTART0+AIB_CLSTART1,
     AIB_CLSTART0,
     AIB_CLSTART1);
  AICvClock : array [AIcontinuos..AIexternal] of DWORD =
    (AIB_CVSTART0+AIB_CVSTART1,
     AIB_CVSTART0,
     AIB_CVSTART1);
  UCTModes: array [0..5] of DWORD =
     (UCT_Mode_Low_High,UCT_Mode_One_Shot,UCT_Mode_Rate,
      UCT_Mode_Square_Wave,UCT_Mode_Sw_Strobe,UCT_Mode_Hw_Strobe);
  UCT0 = 0;
  UCT1 = 1;
  UCT2 = 2;
  UCTClkSW = 0;
  UCTClk1M = 1;
  UCTClkEx = 2;
  UCTClocks  : array [UCT0..UCT2,UCTClkSW..UCTClkEx] of DWORD =
    ((0,UTB_CLK0, UTB_CLK0 or UTB_CLK0_1),
     (0,UTB_CLK1, UTB_CLK1 or UTB_CLK1_1),
     (0,UTB_CLK2, UTB_CLK2 or UTB_CLK2_1));
  UCTGateSW = false;
  UCTGateEx = true;
  UCTGates   : array [UCT0..UCT2,Boolean] of DWORD =
    ((UTB_SWGATE0, UTB_GATE0),
     (UTB_SWGATE1, UTB_GATE1),
     (UTB_SWGATE2, UTB_GATE2));

const
  M_TCSR0    = $FFFF8F;      // TIMER0 Control/Status Register
  M_TLR0     = $FFFF8E;      // TIMER0 Load Reg
  M_TCPR0    = $FFFF8D;      // TIMER0 Compare Register
  M_TCR0     = $FFFF8C;      // TIMER0 Count Register
//
//       Register Addresses Of TIMER1
  M_TCSR1    = $FFFF8B;      // TIMER1 Control/Status Register
  M_TLR1     = $FFFF8A;      // TIMER1 Load Reg
  M_TCPR1    = $FFFF89;      // TIMER1 Compare Register
  M_TCR1     = $FFFF88;      // TIMER1 Count Register
//
//       Register Addresses Of TIMER2
  M_TCSR2    = $FFFF87;     // TIMER2 Control/Status Register
  M_TLR2     = $FFFF86;      // TIMER2 Load Reg
  M_TCPR2    = $FFFF85;      // TIMER2 Compare Register
  M_TCR2     = $FFFF84;      // TIMER2 Count Register
//
//       Prescaler Registers
  M_TPLR     = $FFFF83;      // TIMER Prescaler Load Register
  M_TPCR     = $FFFF82;      // TIMER Prescalar Count Register
//
//       Timer Control/Status Register Bit Flags
  M_TE       = (1 shl 0);      // Timer Enable
  M_TOIE     = (1 shl 1);      // Timer Overflow Interrupt Enable
  M_TCIE     = (1 shl 2);      // Timer Compare Interrupt Enable
  M_TC0      = (1 shl 4);      // Timer Control 0
  M_TC1      = (1 shl 5);      // Timer Control 1
  M_TC2      = (1 shl 6);      // Timer Control 2
  M_TC3      = (1 shl 7);      // Timer Control 3
  M_TC       = $F0;          // Timer Control Mask   (TC0-TC3)
  M_INV      = (1 shl 8);      // Inverter Bit
  M_TRM      = (1 shl 9);      // Timer Restart Mode
  M_DIR      = (1 shl 11);     // Direction Bit
  M_DI       = (1 shl 12);     // Data Input
  M_DO       = (1 shl 13);     // Data Output
  M_PCE      = (1 shl 15);     // Prescaled Clock Enable
  M_TOF      = (1 shl 20);     // Timer Overflow Flag
  M_TCF      = (1 shl 21);     // Timer Compare Flag
//
//       Timer Prescaler Register Bit Flags
  M_PS       = $600000;      // Prescaler Source Mask
  M_PS0      = (1 shl 21);
  M_PS1      = (1 shl 22);
  M_PS_CLK   = 0;
  M_PS_CLK2  = 0;                 // Use internal clock / 2
  M_PS_TIO0  = (M_PS0);           // Use timer IO 0
  M_PS_TIO1  = (M_PS1);           // Use timer IO 1
  M_PS_TIO2  = (M_PS0 or M_PS1);   // Use timer IO 2

//
//       Timer Mode Masks
  DCT_Timer              = $0;
  DCT_TimerPulse         = $10;
  DCT_TimerToggle        = $20;
  DCT_EventCounter       = $30;
  DCT_InputWidth         = $40;
  DCT_InputPeriod        = $50;
  DCT_Capture            = $60;
  DCT_PWM                = $70;
  DCT_WatchdogPulse      = $90;
  DCT_WatchdogToggle     = $A0;
//
//       DCT_Status bits
  DCTB_TE0       = (1 shl 0);   // Timer 0 Enable
  DCTB_TE1       = (1 shl 1);   // Timer 1 Enable
  DCTB_TE2       = (1 shl 2);   // Timer 2 Enable
  DCTB_TA0       = (1 shl 3);   // Timer 0 Available
  DCTB_TA1       = (1 shl 4);   // Timer 1 Available
  DCTB_TA2       = (1 shl 5);   // Timer 2 Available
  DCTB_FMODE     = (1 shl 6);   // Frequency Mode On
  DCTB_FDONE     = (1 shl 7);   // Frequency Measurement Done
  DCTB_TFME0     = (1 shl 8);   // Timer 0 Frequency Mode Enable
  DCTB_TFME1     = (1 shl 9);   // Timer 1 Frequency Mode Enable
//
// Counter IDs
  DCT_UCT0       = $0;    // counter 0
  DCT_UCT1       = $1;    // counter 1
  DCT_UCT2       = $2;    // counter 2




{$ENDIF}

//-----------------------------------------------------------------------
// end of pdfw_def.h
implementation
begin
end.

