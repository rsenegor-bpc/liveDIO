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
// AUTHOR:  B.S.
//
// DATE:    16-JUN-97
//
// REV:     0.9.2
//
// R DATE:  17-JAN-98
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
//      Rev 0.9.1   3/18/98     A.I.,   Add changes for autocalibration
//      Rev 0.9.2   5/8/98      A.I.,   Add bits for buffered I/O
//
//-----------------------------------------------------------------------
//
//      Copyright (C) 1997, 1998 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//=======================================================================

unit pdfw_bitsdef; // from C:\PowerDAQ\winnt\pdfw_def.h

interface

{$IFDEF MSWINDOWS}
uses
  windows;
{$ENDIF}


//-----------------------------------------------------------------------
// Board Control Subsystem Status (BrdStatus); Bits
//-----------------------------------------------------------------------
const BRDB_HINT       = (1 shl 0);  // PCI Host Interrupt Status
const BRDB_HINTEN     = (1 shl 1);  // PCI Host Interrupt Enabled
const BRDB_HINTASRT   = (1 shl 2);  // PCI Host Interrupt Assert Condition
const BRDB_ERR        = (1 shl 3);  // Board error occured
const BRDB_PCIERR     = (1 shl 4);  // PCI Slave Mode FIFO Error occured
const BRDB_HCVERR     = (1 shl 5);  // PCI Slave Mode HCV Error occured
const BRDB_BMERR      = (1 shl 6);  // PCI Bus Master Error occured
const BRDB_IRQ        = (1 shl 7);  // DSP IRQA|IRQB|IRQC|IRQD Asserted

//-----------------------------------------------------------------------
// Board Subsystem Interrupt Status (BrdIntStat); Bits
//-----------------------------------------------------------------------
const BRDB_IRQA       = (1 shl 0);  // DSP IRQ A Interrupt Status
const BRDB_IRQB       = (1 shl 1);  // DSP IRQ B Interrupt Status
const BRDB_IRQC       = (1 shl 2);  // DSP IRQ C Interrupt Status
const BRDB_IRQD       = (1 shl 3);  // DSP IRQ D Interrupt Status
const BRDB_WDTIMER    = (1 shl 4);  // DSP Watchdog Timer TC
const BRDB_HINTSET    = (1 shl 5);  // (DBG ONLY); PCI Host Interrupt Set
const BRDB_HINTACK    = (1 shl 6);  // (DBG ONLY); PCI Host Interrupt Acknowledged

//-----------------------------------------------------------------------
// AIn Subsystem Configuration (AInCfg); Bits
//-----------------------------------------------------------------------
const AIB_INPMODE     = (1);        // AIn Input Mode (Single-Ended/Differential);
const AIB_INPTYPE     = (1 shl 1);  // AIn Input Type (Unipolar/Bipolar);
const AIB_INPRANGE    = (1 shl 2);  // AIn Input Range (Low/High);
const AIB_CVSTART0    = (1 shl 3);  // AIn Conv Start Clk Source (2 bits);
const AIB_CVSTART1    = (1 shl 4);
const AIB_EXTCVS      = (1 shl 5);  // AIn External Conv Start (Pacer); Clk Edge
const AIB_CLSTART0    = (1 shl 6);  // AIn Ch List Start (Burst); Clk Source (2 bits);
const AIB_CLSTART1    = (1 shl 7);
const AIB_EXTCLS      = (1 shl 8);  // AIn External Ch List Start (Burst); Clk Edge
const AIB_STARTTRIG0  = (1 shl 9);  // AIn Start Trigger Source (2 bits);
const AIB_STARTTRIG1  = (1 shl 10);
const AIB_STOPTRIG0   = (1 shl 11); // AIn Stop Trigger Source (2 bits);
const AIB_STOPTRIG1   = (1 shl 12);
const AIB_BLKBUF      = (1 shl 13); // Use Block Buffer Mode for data transfer
const AIB_BUSMSTR     = (1 shl 14); // Use Bus Master Mode for data transfer

//-----------------------------------------------------------------------
// AIn Subsystem Status & Events (AInStatus/AInEvent); Bits
//-----------------------------------------------------------------------
const AIB_ACTIVE      = (1 shl 0);  // AIn Active (Enabled);
const AIB_BMACTIVE    = (1 shl 1);  // AIn Bus Master Active (Enabled);
const AIB_START       = (1 shl 2);  // AIn Sample Acquisition Started
const AIB_STOP        = (1 shl 3);  // AIn Sample Acquisition Stopped
const AIB_ASAMPLE     = (1 shl 4);  // AIn One or More Samples Acquired
const AIB_CLDONE      = (1 shl 5);  // AIn One or More CL Blocks Acquired
const AIB_FHFDONE     = (1 shl 6);  // AIn Acquired Over Half FIFO Full
const AIB_BMDONE      = (1 shl 7);  // AIn Bus Master Blocks Transferred
const AIB_ERR         = (1 shl 8);  // AIn Subsystem Error
const AIB_OVRUNERR    = (1 shl 9);  // AIn FIFO/Buffer Overrun Error
const AIB_CVSTRTERR   = (1 shl 10); // AIn Conversion Start Error
const AIB_CLSTRTERR   = (1 shl 11); // AIn Channel List Start Error
const AIB_OTRLOW      = (1 shl 12); // ADC Out of Range Low Error
const AIB_OTRHIGH     = (1 shl 13); // ADC Out of Range High Error
const AIB_BMERR       = (1 shl 14); // Bus Master Data Transfer Error
const AIB_BMEMPTY     = (1 shl 15); // Bus Master PRD Table Empty Error
//------------
const AIB_FNE         = (1 shl 16); // (DBG ONLY); AIn ADC FIFO Not Empty
const AIB_FHF         = (1 shl 17); // (DBG ONLY); AIn ADC FIFO Half Full
const AIB_FF          = (1 shl 18); // (DBG ONLY); AIn ADC FIFO Full
const AIB_CVDONE      = (1 shl 19); // (DBG ONLY); AIn ADC Conversion Done
const AIB_CLDONEF     = (1 shl 20); // (DBG ONLY); AIn ADC Channel List Done
//------------
const AIB_QEMPTY      = (1 shl 21); // (DBG ONLY); AIn Queue Empty
const AIB_QHF         = (1 shl 22); // (DBG ONLY); AIn Queue Half Full
const AIB_QFULL       = (1 shl 23); // (DBG ONLY); AIn Queue Full
//------------ Driver events
const AIB_DFRMDONE    = (1 shl 24); // Frame done. Controlled by driver
const AIB_DBUFDONE    = (1 shl 25); // Buffer done. Controlled by driver

//-----------------------------------------------------------------------
// AOut Subsystem Configuration (AOutCfg); Bits
//-----------------------------------------------------------------------
const AOB_CVSTART0    = (1 shl 0);  // AOut Conv (Pacer); Start Clk Source (2 bits);
const AOB_CVSTART1    = (1 shl 1);
const AOB_EXTCVS      = (1 shl 2);  // AOut External Conv (Pacer); Clock Edge
const AOB_DACBLK0     = (1 shl 3);  // DACs Enabled in Block Mode (4 bits);
const AOB_DACBLK1     = (1 shl 4);
const AOB_DACBLK2     = (1 shl 5);
const AOB_DACBLK3     = (1 shl 6);
const AOB_STARTTRIG0  = (1 shl 7);  // AOut Start Trigger Source (2 bits);
const AOB_STARTTRIG1  = (1 shl 8);
const AOB_STOPTRIG0   = (1 shl 9);  // AOut Stop Trigger Source (2 bits);
const AOB_STOPTRIG1   = (1 shl 10);
const AOB_BUFMODE0    = (1 shl 11); // AOut Buf Mode (Single-Value/Block); (2 bits);
const AOB_BUFMODE1    = (1 shl 12);

//-----------------------------------------------------------------------
// AOut Subsystem Status & Events (AOutStatus/AOutEvent); Bits
//-----------------------------------------------------------------------
const AOB_ACTIVE      = (1 shl 0);  // AOut Active (Enabled);
const AOB_START       = (1 shl 1);  // AOut Conversion Started
const AOB_STOP        = (1 shl 2);  // AOut Conversion Stopped
const AOB_CVDONE      = (1 shl 3);  // AOut Single Conversion Done
const AOB_HALFDONE    = (1 shl 4);  // AOut Half Block Conversion Done
const AOB_BLKDONE     = (1 shl 5);  // AOut Block Conversion Done
const AOB_ERR         = (1 shl 6);  // AOut Subsystem Error
const AOB_UNDRUNERR   = (1 shl 7);  // AOut Buffer Underrun Error
const AOB_CVSTRTERR   = (1 shl 8);  // AOut Conversion Start Error
//------------
const AOB_QEMPTY      = (1 shl 9);  // (DBG ONLY); AOut Queue Empty
const AOB_QHF         = (1 shl 10); // (DBG ONLY); AOut Queue Half Full
const AOB_QFULL       = (1 shl 11); // (DBG ONLY); AOut Queue Full

//-----------------------------------------------------------------------
// DIn Subsystem Configuration (DInCfg); Bits
//-----------------------------------------------------------------------
const DIB_0CFG0       = (1 shl 0);  // DIn Bit 0 Intr Cfg bit 0
const DIB_0CFG1       = (1 shl 1);  // DIn Bit 0 Intr Cfg bit 1
const DIB_1CFG0       = (1 shl 2);  // DIn Bit 1 Intr Cfg bit 0
const DIB_1CFG1       = (1 shl 3);  // DIn Bit 1 Intr Cfg bit 1
const DIB_2CFG0       = (1 shl 4);  // DIn Bit 2 Intr Cfg bit 0
const DIB_2CFG1       = (1 shl 5);  // DIn Bit 2 Intr Cfg bit 1
const DIB_3CFG0       = (1 shl 6);  // DIn Bit 3 Intr Cfg bit 0
const DIB_3CFG1       = (1 shl 7);  // DIn Bit 3 Intr Cfg bit 1
const DIB_4CFG0       = (1 shl 8);  // DIn Bit 4 Intr Cfg bit 0
const DIB_4CFG1       = (1 shl 9);  // DIn Bit 4 Intr Cfg bit 1
const DIB_5CFG0       = (1 shl 10); // DIn Bit 5 Intr Cfg bit 0
const DIB_5CFG1       = (1 shl 11); // DIn Bit 5 Intr Cfg bit 1
const DIB_6CFG0       = (1 shl 12); // DIn Bit 6 Intr Cfg bit 0
const DIB_6CFG1       = (1 shl 13); // DIn Bit 6 Intr Cfg bit 1
const DIB_7CFG0       = (1 shl 14); // DIn Bit 7 Intr Cfg bit 0
const DIB_7CFG1       = (1 shl 15); // DIn Bit 7 Intr Cfg bit 1

//-----------------------------------------------------------------------
// DIn Subsystem Status & Events (DInStatus/DInEvent); Bits
//-----------------------------------------------------------------------
// Bits 0 - 7:  Digital Input Bit Level
const DIB_LEVEL0      = (1 shl 0);  // DIn Bit 0 Input Level
const DIB_LEVEL1      = (1 shl 1);  // DIn Bit 1 Input Level
const DIB_LEVEL2      = (1 shl 2);  // DIn Bit 2 Input Level
const DIB_LEVEL3      = (1 shl 3);  // DIn Bit 3 Input Level
const DIB_LEVEL4      = (1 shl 4);  // DIn Bit 4 Input Level
const DIB_LEVEL5      = (1 shl 5);  // DIn Bit 5 Input Level
const DIB_LEVEL6      = (1 shl 6);  // DIn Bit 6 Input Level
const DIB_LEVEL7      = (1 shl 7);  // DIn Bit 7 Input Level

// Bits 8 - 15: Digital Input Bit Trigger Status
const DIB_INTR0       = (1 shl 8);  // DIn Bit 8 Latched Interrupt
const DIB_INTR1       = (1 shl 9);  // DIn Bit 9 Latched Interrupt
const DIB_INTR2       = (1 shl 10); // DIn Bit 10 Latched Interrupt
const DIB_INTR3       = (1 shl 11); // DIn Bit 11 Latched Interrupt
const DIB_INTR4       = (1 shl 12); // DIn Bit 12 Latched Interrupt
const DIB_INTR5       = (1 shl 13); // DIn Bit 13 Latched Interrupt
const DIB_INTR6       = (1 shl 14); // DIn Bit 14 Latched Interrupt
const DIB_INTR7       = (1 shl 15); // DIn Bit 15 Latched Interrupt

//-----------------------------------------------------------------------
// DOut Subsystem Configuration (DOutCfg); Bits
//-----------------------------------------------------------------------
// N/A

//-----------------------------------------------------------------------
// DOut Subsystem Status & Events (DOutStatus/DOutEvent); Bits
//-----------------------------------------------------------------------
// N/A

//-----------------------------------------------------------------------
// UCT Subsystem Configuration (UctCfg); Bits
//-----------------------------------------------------------------------
const UTB_CLK0        = (1 shl 0);  // UCT 0 Clock Source (2 bits);
const UTB_CLK0_1      = (1 shl 1);  //
const UTB_CLK1        = (1 shl 2);  // UCT 1 Clock Source (2 bits);
const UTB_CLK1_1      = (1 shl 3);  //
const UTB_CLK2        = (1 shl 4);  // UCT 2 Clock Source (2 bits);
const UTB_CLK2_1      = (1 shl 5);  //
const UTB_GATE0       = (1 shl 6);  // UCT 0 Gate Source bit
const UTB_GATE1       = (1 shl 7);  // UCT 1 Gate Source bit
const UTB_GATE2       = (1 shl 8);  // UCT 2 Gate Source bit
const UTB_SWGATE0     = (1 shl 9);  // UCT 0 SW Gate Setting bit
const UTB_SWGATE1     = (1 shl 10); // UCT 1 SW Gate Setting bit
const UTB_SWGATE2     = (1 shl 11); // UCT 2 SW Gate Setting bit
const UTB_INTR0MSK    = (1 shl 12); // UCT 0 Output Event Interrupt Mask bit
const UTB_INTR1MSK    = (1 shl 13); // UCT 1 Output Event Interrupt Mask bit
const UTB_INTR2MSK    = (1 shl 14); // UCT 2 Output Event Interrupt Mask bit

//-----------------------------------------------------------------------
// UCT Subsystem Status & Events (UctStatus/UctEvent); Bits
//-----------------------------------------------------------------------
const UTB_LEVEL0      = (1 shl 0);  // UCT 0 Output Level
const UTB_LEVEL1      = (1 shl 1);  // UCT 1 Output Level
const UTB_LEVEL2      = (1 shl 2);  // UCT 2 Output Level
const UTB_INTR0       = (1 shl 3);  // UCT 0 Latched Interrupt
const UTB_INTR1       = (1 shl 4);  // UCT 1 Latched Interrupt
const UTB_INTR2       = (1 shl 5);  // UCT 2 Latched Interrupt

implementation
begin
end.