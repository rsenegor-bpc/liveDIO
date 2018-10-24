unit pwrdaq32;

//===========================================================================
//
// NAME:    pwrdaq32.h
//
// DESCRIPTION:
//
//          PowerDAQ Win32 DLL header file
//
//          Definitions for PowerDAQ DLL driver interface functions.
//
//
// DATE:    5-MAY-97
//
// REV:     0.6
//
// R DATE:  22-DEC-99
//
// HISTORY:
//
//      Rev 0.1,     5-MAY-97,  A.S.,   Initial version.
//      Rev 0.2,     5-JAN-98,  B.S.,   Revised to final PD Firmware.
//      Rev 0.3,     8-FEB-98,  A.S.,   Added overlapped I/O.
//      Rev 0.4,    01-JUN-98,  B.S.,   Updated to revised firmware.
//      Rev 0.5,     7-MAY-99,  A.I.,   PD DIO support added
//      Rev 0.51,   21-JUL-99,  A.I.,   SSH gain control added
//      Rev 0.52,   13-SEP-99,  A.I.,   AO board added
//      Rev 0.53    06-DEC-99,  A.I.,   Separate private events into subsystems
//      Rev 0.6     22-DEC-99,  A.I.,   Capabilities functions added
//      Rev 2.2     30-MAR-01,  A.I.,   A lot of new commands added
//
//---------------------------------------------------------------------------
//
//      Copyright (C) 1997-2001 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================


interface

{$IFDEF VER130}
   {$DEFINE MSWINDOWS}
{$ENDIF}

uses
  pwrdaq_struct,
{$IFDEF MSWINDOWS}
  pwrdaq_WinAPI;
{$ENDIF}
{$IFDEF LINUX}
  pwrdaq_LinuxAPI;
{$ENDIF}

{$IFNDEF _INC_PWRDAQ32}
{$DEFINE _INC_PWRDAQ32}


//-----------------------------------------------------------------------
// 12-bit multifunction medium speed (330 kHz)
const
    PD_MF_16_330_12L = (PD_BRD + $101);
    PD_MF_16_330_12H = (PD_BRD + $102);
    PD_MF_64_330_12L = (PD_BRD + $103);
    PD_MF_64_330_12H = (PD_BRD + $104);

// 12-bit multifunction high speed (1 MHz)
const
    PD_MF_16_1M_12L = (PD_BRD + $105);
    PD_MF_16_1M_12H = (PD_BRD + $106);
    PD_MF_64_1M_12L = (PD_BRD + $107);
    PD_MF_64_1M_12H = (PD_BRD + $108);

// 16-bit multifunction (250 kHz)
const
    PD_MF_16_250_16L = (PD_BRD + $109);
    PD_MF_16_250_16H = (PD_BRD + $10A);
    PD_MF_64_250_16L = (PD_BRD + $10B);
    PD_MF_64_250_16H = (PD_BRD + $10C);

// 16-bit multifunction (50 kHz)
const
    PD_MF_16_50_16L = (PD_BRD + $10D);
    PD_MF_16_50_16H = (PD_BRD + $10E);

// 12-bit, 6-ch 1M SSH
const
    PD_MFS_6_1M_12  = (PD_BRD + $10F);
    PD_Nothing      = (PD_BRD + $110);
  
  
// PowerDAQ II series -------------------------

// 14-bit multifunction low speed (400 kHz)
const
     PD2_MF_16_400_14L        = (PD_BRD + $111);
     PD2_MF_16_400_14H        = (PD_BRD + $112);
     PD2_MF_64_400_14L        = (PD_BRD + $113);
     PD2_MF_64_400_14H        = (PD_BRD + $114);

// 14-bit multifunction medium speed (800 kHz)
const
     PD2_MF_16_800_14L        = (PD_BRD + $115);
     PD2_MF_16_800_14H        = (PD_BRD + $116);
     PD2_MF_64_800_14L        = (PD_BRD + $117);
     PD2_MF_64_800_14H        = (PD_BRD + $118);

// 12-bit multifunction high speed (1.25 MHz)
const
     PD2_MF_16_1M_12L         = (PD_BRD + $119);
     PD2_MF_16_1M_12H         = (PD_BRD + $11A);
     PD2_MF_64_1M_12L         = (PD_BRD + $11B);
     PD2_MF_64_1M_12H         = (PD_BRD + $11C);

// 16-bit multifunction (50 kHz)
const
     PD2_MF_16_50_16L        = (PD_BRD + $11D);
     PD2_MF_16_50_16H        = (PD_BRD + $11E);
     PD2_MF_64_50_16L        = (PD_BRD + $11F);
     PD2_MF_64_50_16H        = (PD_BRD + $120);

// 16-bit multifunction (333 kHz)
const
     PD2_MF_16_333_16L        = (PD_BRD + $121);
     PD2_MF_16_333_16H        = (PD_BRD + $122);
     PD2_MF_64_333_16L        = (PD_BRD + $123);
     PD2_MF_64_333_16H        = (PD_BRD + $124);

// 14-bit multifunction high speed (2.2 MHz)
const
     PD2_MF_16_2M_14L         = (PD_BRD + $125);
     PD2_MF_16_2M_14H         = (PD_BRD + $126);
     PD2_MF_64_2M_14L         = (PD_BRD + $127);
     PD2_MF_64_2M_14H         = (PD_BRD + $128);

// 16-bit multifunction high speed (500 kHz)
const
     PD2_MF_16_500_16L        = (PD_BRD + $129);
     PD2_MF_16_500_16H        = (PD_BRD + $12A);
     PD2_MF_64_500_16L        = (PD_BRD + $12B);
     PD2_MF_64_500_16H        = (PD_BRD + $12C);


// 12-bit multifunction high speed (3 MHz)
const
     PD2_MF_16_3M_12L         = (PD_BRD + $129);
     PD2_MF_16_3M_12H         = (PD_BRD + $12A);
     PD2_MF_64_3M_12L         = (PD_BRD + $12B);
     PD2_MF_64_3M_12H         = (PD_BRD + $12C);

// PowerDAQ II SSH section ---------------------
// 14-bit multifunction SSH low speed (500 kHz)
const
     PD2_MFS_4_500_14         = (PD_BRD + $12D);
     PD2_MFS_8_500_14         = (PD_BRD + $12E);
     PD2_MFS_4_500_14DG       = (PD_BRD + $12F);
     PD2_MFS_4_500_14H        = (PD_BRD + $130);
     PD2_MFS_8_500_14DG       = (PD_BRD + $131);
     PD2_MFS_8_500_14H        = (PD_BRD + $132);

// 14-bit multifunction SSH medium speed (800 kHz)
const
     PD2_MFS_4_800_14         = (PD_BRD + $133);
     PD2_MFS_8_800_14         = (PD_BRD + $134);
     PD2_MFS_4_800_14DG       = (PD_BRD + $135);
     PD2_MFS_4_800_14H        = (PD_BRD + $136);
     PD2_MFS_8_800_14DG       = (PD_BRD + $137);
     PD2_MFS_8_800_14H        = (PD_BRD + $138);

// 12-bit multifunction SSH high speed (1.25 MHz)
const
     PD2_MFS_4_1M_12          = (PD_BRD + $139);
     PD2_MFS_8_1M_12          = (PD_BRD + $13A);
     PD2_MFS_4_1M_12DG        = (PD_BRD + $13B);
     PD2_MFS_4_1M_12H         = (PD_BRD + $13C);
     PD2_MFS_8_1M_12DG        = (PD_BRD + $13D);
     PD2_MFS_8_1M_12H         = (PD_BRD + $13E);

// 14-bit multifunction SSH high speed (2.2 MHz)
const
     PD2_MFS_4_2M_14          = (PD_BRD + $13F);
     PD2_MFS_8_2M_14          = (PD_BRD + $140);
     PD2_MFS_4_2M_14DG        = (PD_BRD + $141);
     PD2_MFS_4_2M_14H         = (PD_BRD + $142);
     PD2_MFS_8_2M_14DG        = (PD_BRD + $143);
     PD2_MFS_8_2M_14H         = (PD_BRD + $144);

// 16-bit multifunction SSH high speed (333 kHz)
const
     PD2_MFS_4_300_16         = (PD_BRD + $145);
     PD2_MFS_8_300_16         = (PD_BRD + $146);
     PD2_MFS_4_300_16DG       = (PD_BRD + $147);
     PD2_MFS_4_300_16H        = (PD_BRD + $148);
     PD2_MFS_8_300_16DG       = (PD_BRD + $149);
     PD2_MFS_8_300_16H        = (PD_BRD + $14A);

// DIO series
const
     PD2_DIO_64                = (PD_BRD + $14B);
     PD2_DIO_128               = (PD_BRD + $14C);
     PD2_DIO_256               = (PD_BRD + $14D);

// AO series
const
     PD2_AO_8_16               = (PD_BRD + $14E);
     PD2_AO_16_16              = (PD_BRD + $14F);
     PD2_AO_32_16              = (PD_BRD + $150);
     PD2_AO_96_16              = (PD_BRD + $151);

const
     PD2_AO_R1                 = (PD_BRD + $152);
     PD2_AO_R2                 = (PD_BRD + $153);
     PD2_AO_R3                 = (PD_BRD + $154);
     PD2_AO_R4                 = (PD_BRD + $155);
     PD2_AO_R5                 = (PD_BRD + $156);
     PD2_AO_R6                 = (PD_BRD + $157);
     PD2_AO_R7                 = (PD_BRD + $158);
                                                
// extended DIO series
const
     PD2_DIO_64CE             = (PD_BRD + $159);
     PD2_DIO_128CE            = (PD_BRD + $15A);

const
     PD2_DIO_64ST             = (PD_BRD + $15B);
     PD2_DIO_128ST            = (PD_BRD + $15C);

const
     PD2_DIO_64HS             = (PD_BRD + $15D);
     PD2_DIO_128HS            = (PD_BRD + $15E);

const
     PD2_DIO_64CT             = (PD_BRD + $15F);
     PD2_DIO_128CT            = (PD_BRD + $160);

const                         
     PD2_DIO_R0              = (PD_BRD + $161);
     PD2_DIO_R1              = (PD_BRD + $162);
     PD2_DIO_R2              = (PD_BRD + $163);
     PD2_DIO_R3              = (PD_BRD + $164);
     PD2_DIO_R4              = (PD_BRD + $165);
     PD2_DIO_R5              = (PD_BRD + $166);
     PD2_DIO_R6              = (PD_BRD + $167);
     PD2_DIO_R7              = (PD_BRD + $168);

// 16-bit multifunction SSH high speed (500 kHz)
const
     PD2_MFS_4_500_16        = (PD_BRD + $169);
     PD2_MFS_8_500_16        = (PD_BRD + $16A);
     PD2_MFS_4_500_16DG      = (PD_BRD + $16B);
     PD2_MFS_4_500_16H       = (PD_BRD + $16C);
     PD2_MFS_8_500_16DG      = (PD_BRD + $16D);
     PD2_MFS_8_500_16H       = (PD_BRD + $16E);

// 16-bit multifunction (100 kHz)
const
     PD2_MF_16_100_16L       = (PD_BRD + $16F);
     PD2_MF_16_100_16H       = (PD_BRD + $170);
     PD2_MF_64_100_16L       = (PD_BRD + $171);
     PD2_MF_64_100_16H       = (PD_BRD + $172);

// reserved for MF/MFS boards
const
     PD2_MF_R0               = (PD_BRD + $173);
     PD2_MF_R1               = (PD_BRD + $174);
     PD2_MF_R2               = (PD_BRD + $175);
     PD2_MF_R3               = (PD_BRD + $176);
     PD2_MF_R4               = (PD_BRD + $177);
     PD2_MF_R5               = (PD_BRD + $178);
     PD2_MF_R6               = (PD_BRD + $179);
     PD2_MF_R7               = (PD_BRD + $17A);
     PD2_MF_R8               = (PD_BRD + $17B);
     PD2_MF_R9               = (PD_BRD + $17C);
     PD2_MF_RA               = (PD_BRD + $17D);
     PD2_MF_RB               = (PD_BRD + $17E);
     PD2_MF_RC               = (PD_BRD + $17F);
     PD2_MF_RD               = (PD_BRD + $180);
     PD2_MF_RE               = (PD_BRD + $181);
     PD2_MF_RF               = (PD_BRD + $182);

// reserved for LAB board
const
     PDL_MF_16               = (PD_BRD + $183);
     PDL_MF_16_1             = (PD_BRD + $184);
     PDL_MF_16_2             = (PD_BRD + $185);
     PDL_MF_16_3             = (PD_BRD + $186);

const
     PDL_DIO_64              = (PD_BRD + $187);
     PDL_DIO_64_1            = (PD_BRD + $188);
     PDL_DIO_64_2            = (PD_BRD + $189);
     PDL_DIO_64_3            = (PD_BRD + $18A);

const
     PDL_AO_64               = (PD_BRD + $18B);
     PDL_AO_64_1             = (PD_BRD + $18C);
     PDL_AO_64_2             = (PD_BRD + $18D);
     PDL_AO_64_3             = (PD_BRD + $18E);

// new PCI model goes from $18B to $1FF
//
//.....
//
// PDXI series -------------------------
//
// 14-bit multifunction low speed (400 kHz)
const
     PDXI_MF_16_400_14L       = (PD_BRD + $211);
     PDXI_MF_16_400_14H       = (PD_BRD + $212);
     PDXI_MF_64_400_14L       = (PD_BRD + $213);
     PDXI_MF_64_400_14H       = (PD_BRD + $214);

// 14-bit multifunction medium speed (800 kHz)
const
     PDXI_MF_16_800_14L       = (PD_BRD + $215);
     PDXI_MF_16_800_14H       = (PD_BRD + $216);
     PDXI_MF_64_800_14L       = (PD_BRD + $217);
     PDXI_MF_64_800_14H       = (PD_BRD + $218);

// 12-bit multifunction high speed (1.25 MHz)
const
     PDXI_MF_16_1M_12L        = (PD_BRD + $219);
     PDXI_MF_16_1M_12H        = (PD_BRD + $21A);
     PDXI_MF_64_1M_12L        = (PD_BRD + $21B);
     PDXI_MF_64_1M_12H        = (PD_BRD + $21C);

// 16-bit multifunction (50 kHz)
const
     PDXI_MF_16_50_16L       = (PD_BRD + $21D);
     PDXI_MF_16_50_16H       = (PD_BRD + $21E);
     PDXI_MF_64_50_16L       = (PD_BRD + $21F);
     PDXI_MF_64_50_16H       = (PD_BRD + $220);

// 16-bit multifunction (333 kHz)
const
     PDXI_MF_16_333_16L       = (PD_BRD + $221);
     PDXI_MF_16_333_16H       = (PD_BRD + $222);
     PDXI_MF_64_333_16L       = (PD_BRD + $223);
     PDXI_MF_64_333_16H       = (PD_BRD + $224);

// 14-bit multifunction high speed (2.2 MHz)
const
     PDXI_MF_16_2M_14L       = (PD_BRD + $225);
     PDXI_MF_16_2M_14H       = (PD_BRD + $226);
     PDXI_MF_64_2M_14L       = (PD_BRD + $227);
     PDXI_MF_64_2M_14H       = (PD_BRD + $228);

// 16-bit multifunction high speed (500 kHz)
const
     PDXI_MF_16_500_16L        = (PD_BRD + $229);
     PDXI_MF_16_500_16H        = (PD_BRD + $22A);
     PDXI_MF_64_500_16L        = (PD_BRD + $22B);
     PDXI_MF_64_500_16H        = (PD_BRD + $22C);

// PowerDAQ II SSH section ---------------------
// 14-bit multifunction SSH low speed (500 kHz)
const
     PDXI_MFS_4_500_14        = (PD_BRD + $22D);
     PDXI_MFS_8_500_14        = (PD_BRD + $22E);
     PDXI_MFS_4_500_14DG      = (PD_BRD + $22F);
     PDXI_MFS_4_500_14H       = (PD_BRD + $230);
     PDXI_MFS_8_500_14DG      = (PD_BRD + $231);
     PDXI_MFS_8_500_14H       = (PD_BRD + $232);

// 14-bit multifunction SSH medium speed (800 kHz)
const
     PDXI_MFS_4_800_14        = (PD_BRD + $233);
     PDXI_MFS_8_800_14        = (PD_BRD + $234);
     PDXI_MFS_4_800_14DG      = (PD_BRD + $235);
     PDXI_MFS_4_800_14H       = (PD_BRD + $236);
     PDXI_MFS_8_800_14DG      = (PD_BRD + $237);
     PDXI_MFS_8_800_14H       = (PD_BRD + $238);

// 12-bit multifunction SSH high speed (1.25 MHz)
const
     PDXI_MFS_4_1M_12         = (PD_BRD + $239);
     PDXI_MFS_8_1M_12         = (PD_BRD + $23A);
     PDXI_MFS_4_1M_12DG       = (PD_BRD + $23B);
     PDXI_MFS_4_1M_12H        = (PD_BRD + $23C);
     PDXI_MFS_8_1M_12DG       = (PD_BRD + $23D);
     PDXI_MFS_8_1M_12H        = (PD_BRD + $23E);

// 14-bit multifunction SSH high speed (2.2 MHz)
const
     PDXI_MFS_4_2M_14         = (PD_BRD + $23F);
     PDXI_MFS_8_2M_14         = (PD_BRD + $240);
     PDXI_MFS_4_2M_14DG       = (PD_BRD + $241);
     PDXI_MFS_4_2M_14H        = (PD_BRD + $242);
     PDXI_MFS_8_2M_14DG       = (PD_BRD + $243);
     PDXI_MFS_8_2M_14H        = (PD_BRD + $244);

// 16-bit multifunction SSH high speed (333 kHz)
const
     PDXI_MFS_4_300_16        = (PD_BRD + $245);
     PDXI_MFS_8_300_16        = (PD_BRD + $246);
     PDXI_MFS_4_300_16DG      = (PD_BRD + $247);
     PDXI_MFS_4_300_16H       = (PD_BRD + $248);
     PDXI_MFS_8_300_16DG      = (PD_BRD + $249);
     PDXI_MFS_8_300_16H       = (PD_BRD + $24A);

// DIO series
const
     PDXI_DIO_64              = (PD_BRD + $24B);
     PDXI_DIO_128             = (PD_BRD + $24C);
     PDXI_DIO_256             = (PD_BRD + $24D);
                                              
// AO series
const
     PDXI_AO_8_16             = (PD_BRD + $24E);
     PDXI_AO_16_16            = (PD_BRD + $24F);
     PDXI_AO_32_16            = (PD_BRD + $250);
     PDXI_AO_96_16            = (PD_BRD + $251);

const
     PDXI_AO_R1               = (PD_BRD + $252);
     PDXI_AO_R2               = (PD_BRD + $253);
     PDXI_AO_R3               = (PD_BRD + $254);
     PDXI_AO_R4               = (PD_BRD + $255);
     PDXI_AO_R5               = (PD_BRD + $256);
     PDXI_AO_R6               = (PD_BRD + $257);
     PDXI_AO_R7               = (PD_BRD + $258);
                                              
// extended DIO series
const
     PDXI_DIO_64CE            = (PD_BRD + $259);
     PDXI_DIO_128CE           = (PD_BRD + $25A);

const
     PDXI_DIO_64ST            = (PD_BRD + $25B);
     PDXI_DIO_128ST           = (PD_BRD + $25C);

const
     PDXI_DIO_64HS            = (PD_BRD + $25D);
     PDXI_DIO_128HS           = (PD_BRD + $25E);

const
     PDXI_DIO_64CT            = (PD_BRD + $25F);
     PDXI_DIO_128CT           = (PD_BRD + $260);

const
     PDXI_DIO_R0              = (PD_BRD + $261);
     PDXI_DIO_R1              = (PD_BRD + $262);
     PDXI_DIO_R2              = (PD_BRD + $263);
     PDXI_DIO_R3              = (PD_BRD + $264);
     PDXI_DIO_R4              = (PD_BRD + $265);
     PDXI_DIO_R5              = (PD_BRD + $266);
     PDXI_DIO_R6              = (PD_BRD + $267);
     PDXI_DIO_R7              = (PD_BRD + $268);

// 16-bit multifunction SSH high speed (500 kHz)
const
     PDXI_MFS_4_500_16        = (PD_BRD + $269);
     PDXI_MFS_8_500_16        = (PD_BRD + $26A);
     PDXI_MFS_4_500_16DG      = (PD_BRD + $26B);
     PDXI_MFS_4_500_16H       = (PD_BRD + $26C);
     PDXI_MFS_8_500_16DG      = (PD_BRD + $26D);
     PDXI_MFS_8_500_16H       = (PD_BRD + $26E);

// 16-bit multifunction (200 kHz)
const
     PDXI_MF_16_100_16L       = (PD_BRD + $26F);
     PDXI_MF_16_100_16H       = (PD_BRD + $270);
     PDXI_MF_64_100_16L       = (PD_BRD + $271);
     PDXI_MF_64_100_16H       = (PD_BRD + $272);

// reserved for MF/MFS boards
const
     PDXI_MF_R0               = (PD_BRD + $273);
     PDXI_MF_R1               = (PD_BRD + $274);
     PDXI_MF_R2               = (PD_BRD + $275);
     PDXI_MF_R3               = (PD_BRD + $276);
     PDXI_MF_R4               = (PD_BRD + $277);
     PDXI_MF_R5               = (PD_BRD + $278);
     PDXI_MF_R6               = (PD_BRD + $279);
     PDXI_MF_R7               = (PD_BRD + $27A);
     PDXI_MF_R8               = (PD_BRD + $27B);
     PDXI_MF_R9               = (PD_BRD + $27C);
     PDXI_MF_RA               = (PD_BRD + $27D);
     PDXI_MF_RB               = (PD_BRD + $27E);
     PDXI_MF_RC               = (PD_BRD + $27F);
     PDXI_MF_RD               = (PD_BRD + $280);
     PDXI_MF_RE               = (PD_BRD + $281);
     PDXI_MF_RF               = (PD_BRD + $282);

// reserved for LAB board
const
     PDXL_MF_16               = (PD_BRD + $283);
     PDXL_MF_16_1             = (PD_BRD + $284);
     PDXL_MF_16_2             = (PD_BRD + $285);
     PDXL_MF_16_3             = (PD_BRD + $286);

const
     PDXL_DIO_64              = (PD_BRD + $287);
     PDXL_DIO_64_1            = (PD_BRD + $288);
     PDXL_DIO_64_2            = (PD_BRD + $289);
     PDXL_DIO_64_3            = (PD_BRD + $28A);

const
     PDXL_AO_64               = (PD_BRD + $28B);
     PDXL_AO_64_1             = (PD_BRD + $28C);
     PDXL_AO_64_2             = (PD_BRD + $28D);
     PDXL_AO_64_3             = (PD_BRD + $28E);



const
     PD_BRD_LST = (PDXL_AO_64_3 - $101 - PD_BRD);

// end of pwrdaq32.h
{$ENDIF}

implementation
begin
end.

