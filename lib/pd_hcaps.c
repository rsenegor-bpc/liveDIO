//=======================================================================
//
// NAME:    pd_hcaps.c
//
// SYNOPSIS:
//
//      Capabilities functions file of UEI PowerDAQ DLL
//
//
// DESCRIPTION:
//
//      This file contains easy-to-use caps functions 
//
// OPTIONS: none
//
//
// DIAGNOSTICS:
//
//
// NOTES:   See notice below.
//
// AUTHOR:  Alex Ivchenko 
//
// DATE:    14-JAN-98
//
// DATE:    14-JAN-98
//
// REV:     1.0.6
//
// R DATE:  14-SEP-99
//
// HISTORY:
//
//      Rev 0.1,      14-JAN-98,  A.I.,   Initial version.
//      Rev 1.0.1,    14-MAY-98,  A.I.,   PD_MF family members added
//      Rev 1.0.2,    25-JAN-99,  A.I.,   PD_MF_16_50 added
//      Rev 1.0.3,    10-MAR-99,  A.I.,   PD_MFS_6_1M added
//      Rev 1.0.4,    12-APR-99,  A.I.,   PD2_MF series added
//      Rev 1.0.4a,   16-APR-99,  A.I.,   16-bit compatibility added
//      Rev 1.0.5,    06-MAY-99,  A.I.,   PD2_MF series added
//      Rev 1.0.6,    14-SEP-99,  A.I.,   Integrated into DLL
//
//-----------------------------------------------------------------------
//
//      Copyright (C) 1998 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//-----------------------------------------------------------------------
//
//=======================================================================

#ifdef _PD_RTLPRO
#include "../include/powerdaq_kernel.h"
#define PD_STRTOK rtl_strtok_r
#define PD_STRCPY rtl_strcpy
#define PD_STRSTR rtl_strstr
#define PD_MEMCPY rtl_memcpy
#define PD_MEMSET rtl_memset
#define PD_ATOI   rtl_atoi
#else

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../include/win_sdk_types.h"
#define PD_STRTOK strtok_r
#define PD_STRCPY strcpy
#define PD_STRSTR strstr
#define PD_MEMCPY memcpy
#define PD_MEMSET memset
#define PD_ATOI   atoi
#endif

#include "../include/powerdaq.h"
#include "../include/powerdaq32.h"
#include "../include/pd_hcaps.h"

extern Adapter_Info *G_pAdapterInfo;
extern int G_NbBoards;

//=======================================================================
// Function returns pointer to DAQ_Information structure for dwBoardID 
// board (stored in PCI Configuration Space
//
// If dwBoardID is incorrect function returns NULL
//
DAQ_Information* _PdGetCapsPtr(DWORD dwBoardID)
{
   if ((dwBoardID > PD_BRD_BASEID + PD_BRD_LST)|| 
       (dwBoardID < PD_BRD_BASEID)) 
   {
      return NULL;
   }
   return(DAQ_Information*) &DAQ_Info[dwBoardID - PD_BRD_BASEID];
}

//=======================================================================
// Function returns pointer to DAQ_Information structure for dwBoardID 
// board (stored in PCI Configuration Space) using handle to adapter
//
// Returns TRUE if success and FALSE if failure
//
int _PdGetCapsPtrA(int hAdapter, PDAQ_Information* pDaqInf)
{
   PWRDAQ_PCI_CONFIG PciConfig;
   int    iRes;   

   iRes = PdGetPciConfiguration(hAdapter, &PciConfig);
   if (iRes < 0)
   {
      pDaqInf = NULL;
      return iRes;
   }

   *pDaqInf = _PdGetCapsPtr(PciConfig.SubsystemID & 0xFFF);

   return iRes;
}

//=======================================================================
// Function parse channel definition string in DAQ_Information structure
// 
// Parameters:  dwBoardID -- board ID from PCI Config.Space
//              dwSubsystem -- subsystem enum from pwrdaq.h
//              dwProperty -- property of subsystem to retrieve:
//                  PDHCAPS_BITS       -- subsystem bit width
//                  PDHCAPS_FIRSTCHAN  -- first channel available
//                  PDHCAPS_LASTCHAN   -- last channel available
//                  PDHCAPS_CHANNELS   -- number of channels available
//
DWORD _PdParseCaps(DWORD dwBoardID, DWORD dwSubsystem, DWORD dwProperty)
{
   char    cSS[10];
   char    cST[100];
   char*   pcTok1;
   char*   pcTok2;
   char*   pSave;
   BOOL    bDone = FALSE;
   DWORD   dwBits = 0;
   DWORD   dwFirstCh = 0;
   DWORD   dwLastCh = 0;

   if ((dwBoardID > PD_BRD_BASEID + PD_BRD_LST)|| 
       (dwBoardID < PD_BRD_BASEID)) 
   {
      return FALSE;
   }

   // What subsystem is requested
   switch (dwSubsystem)
   {
   case AnalogIn:      PD_STRCPY(cSS, "AI"); break;
   case AnalogOut:     PD_STRCPY(cSS, "AO"); break;
   case DigitalIn:     PD_STRCPY(cSS, "DI"); break;
   case DigitalOut:    PD_STRCPY(cSS, "DO"); break;
   case CounterTimer:  PD_STRCPY(cSS, "CT"); break;
   case DSPCounter:    PD_STRCPY(cSS, "DSPCT"); break;
   default: return FALSE; 
   }

   // First, find needed subsystem
   PD_STRCPY(cST, DAQ_Info[dwBoardID - PD_BRD_BASEID].lpChannels);
   pcTok1 = PD_STRTOK(cST, " ", &pSave);
   while ((pcTok1)&&(!bDone))
   {
      // check, is it our subsystem
      if (PD_STRSTR(pcTok1, cSS))     // subsystem found
      {
         pcTok2 = PD_STRTOK(pcTok1, ":", &pSave);
         if (pcTok2) dwBits = PD_ATOI(pcTok2);
         pcTok2 = PD_STRTOK(NULL, ":", &pSave);
         pcTok2 = PD_STRTOK(NULL, ":", &pSave);
         if (pcTok2) dwFirstCh = PD_ATOI(pcTok2);
         pcTok2 = PD_STRTOK(NULL, ":", &pSave);
         if (pcTok2) dwLastCh = PD_ATOI(pcTok2);
         bDone = TRUE;
      }
      pcTok1 = PD_STRTOK(NULL, " ", &pSave);
   }
   if (!bDone) return FALSE;

   // what to return
   switch (dwProperty)
   {
   case PDHCAPS_BITS: return dwBits; break;
   case PDHCAPS_FIRSTCHAN: return dwFirstCh; break;
   case PDHCAPS_LASTCHAN: return dwLastCh; break;
   case PDHCAPS_CHANNELS: return(dwLastCh - dwFirstCh + 1); break;
   default: return FALSE;
   }

}

int _PdGetAdapterInfo(DWORD dwBoardNum,       // Number of board
                      PAdapter_Info pAdInfo   // Pointer to the structure to
                      // store data (allocated by app)
                     )
{
   if (G_pAdapterInfo == NULL)
   {
      return __PdGetAdapterInfo(dwBoardNum, pAdInfo);
   }

   if (dwBoardNum >= G_NbBoards)
   {
      return -ENODEV;
   }

   PD_MEMCPY(pAdInfo, &(G_pAdapterInfo[dwBoardNum]), sizeof(Adapter_Info));

   return 0;
}


//=======================================================================
// Function fills up Adapter_Info structure distributed by DLL
//
// Returns TRUE if success
//
int __PdGetAdapterInfo(DWORD dwBoardNum,       // Number of board
                       PAdapter_Info pAdInfo   // Pointer to the structure to
                       // store data (allocated by app)
                      )
{
   int                 hAdapter;
   PD_EEPROM           Eeprom;
   PWRDAQ_PCI_CONFIG   PciConfig;
   DWORD               dwResult;
   PDAQ_Information    pDaqInfo;
   DWORD               dwID, dwRangeVal;
   char                cST[80];
   char*               pcTok;
   char*               pc;
   char*               pSave;
   BOOL                bUP, bBP;
   int                 retVal;

   if ((hAdapter = PdAcquireSubsystem(dwBoardNum, BoardLevel, 1)) < 0)
   {
      return -1;
   }

   if ((retVal = PdGetPciConfiguration(hAdapter, &PciConfig)) < 0)
   {
      PdAcquireSubsystem(hAdapter, BoardLevel, 0); return retVal;
   }

   if ((retVal = _PdAdapterEepromRead(hAdapter, sizeof(PD_EEPROM)/2, (WORD*)&Eeprom, &dwResult)) < 0)
   {
      PdAcquireSubsystem(hAdapter, BoardLevel, 0); return retVal;
   }

   // Start filling the structure

   // Get pointer to board caps
   pDaqInfo = _PdGetCapsPtr(PciConfig.SubsystemID & 0xFFF);
   if (!pDaqInfo)
   {
      PdAcquireSubsystem(hAdapter, BoardLevel, 0); 
      return -1;
   }

   // Clear structure
   PD_MEMSET( pAdInfo, 0, sizeof(Adapter_Info));


   // Fill board level stuff -----------------------------------------
   pAdInfo->dwBoardID = dwID = PciConfig.SubsystemID & 0xFFF;

   if (PD_IS_PDXI(dwID))
   {
      dwID = dwID - 0x100;
      pAdInfo->atType |= atPXI;
   }

   // Find out what type of board is it
   if ((dwID >= 0x101) && (dwID <= 0x10E))
      pAdInfo->atType |= atPDMF;

   if ((dwID >= 0x10F) && (dwID <= 0x110))
      pAdInfo->atType |= atPDMFS;



   if (PD_IS_MF(dwID))
      pAdInfo->atType |= atPD2MF;

   if (PD_IS_MFS(dwID))
      pAdInfo->atType |= atPD2MFS;

   if (PD_IS_DIO(dwID))
      pAdInfo->atType |= atPD2DIO;

   if (PD_IS_AO(dwID))
      pAdInfo->atType |= atPD2AO;

   if (PDL_IS_MFX(dwID))
      pAdInfo->atType |= atPDLMF;

   if (PDL_IS_DIO(dwID))
      pAdInfo->atType |= atPD2DIO;

   if (PDL_IS_AO(dwID))
      pAdInfo->atType |= atPD2AO;

   PD_STRCPY(pAdInfo->lpBoardName, pDaqInfo->lpBoardName);
   PD_STRCPY(pAdInfo->lpSerialNum, (char*)Eeprom.u.Header.SerialNumber);

   // Fill Subsystem level stuff
   //
   //
   // All types of multifunctional boards
   if ((pAdInfo->atType & atMF))
   {
      // Analog input section MF, MFS
      pAdInfo->SSI[AnalogIn].dwChannels = _PdParseCaps(dwID, AnalogIn, PDHCAPS_CHANNELS);
      pAdInfo->SSI[AnalogIn].dwChBits = _PdParseCaps(dwID, AnalogIn, PDHCAPS_BITS);
      pAdInfo->SSI[AnalogIn].dwRate = pDaqInfo->iMaxAInRate;

      // gains    
      pAdInfo->SSI[AnalogIn].dwMaxGains = 0;
      PD_STRCPY(cST, pDaqInfo->lpAInGains);
      pcTok = PD_STRTOK(cST, " ", &pSave);
      while (pcTok)
      {
         pAdInfo->SSI[AnalogIn].fGains[pAdInfo->SSI[AnalogIn].dwMaxGains] = (float)PD_ATOI(pcTok);
         pAdInfo->SSI[AnalogIn].dwMaxGains++;
         pcTok = PD_STRTOK(NULL, " ", &pSave);        // get next token
      }

      // ranges
      pAdInfo->SSI[AnalogIn].dwMaxRanges = 0; 
      PD_STRCPY(cST, pDaqInfo->lpAInRanges);
      pcTok = PD_STRTOK(cST, " ", &pSave);
      while (pcTok)
      {
         pc = pcTok; // get token
         bUP = FALSE;
         bBP = FALSE;

         while ( *pc != 0 )
         {
            if (*pc == 'U') bUP = TRUE;
            if (*pc == 'B') bBP = TRUE;
            pc++;
            if (bUP || bBP)
               break;
         }
         dwRangeVal = PD_ATOI(pc);
         pAdInfo->SSI[AnalogIn].fRangeHigh[pAdInfo->SSI[AnalogIn].dwMaxRanges] = (float)(WORD)dwRangeVal;
         pAdInfo->SSI[AnalogIn].fRangeLow[pAdInfo->SSI[AnalogIn].dwMaxRanges] = 
         (bUP) ? 0 : (- pAdInfo->SSI[AnalogIn].fRangeHigh[pAdInfo->SSI[AnalogIn].dwMaxRanges]);

         pAdInfo->SSI[AnalogIn].fFactor[pAdInfo->SSI[AnalogIn].dwMaxRanges] = 
         (pAdInfo->SSI[AnalogIn].fRangeHigh[pAdInfo->SSI[AnalogIn].dwMaxRanges] * ((bUP)?1:2)) /
         ((pDaqInfo->wAndMask | 0xF)+1);
         pAdInfo->SSI[AnalogIn].fOffset[pAdInfo->SSI[AnalogIn].dwMaxRanges] =
         - pAdInfo->SSI[AnalogIn].fRangeLow[pAdInfo->SSI[AnalogIn].dwMaxRanges];

         pAdInfo->SSI[AnalogIn].dwMaxRanges++;
         pcTok = PD_STRTOK(NULL, " ", &pSave);        // get next token
      }
      pAdInfo->SSI[AnalogIn].dwMaxRanges /= 2;
      pAdInfo->SSI[AnalogIn].wXorMask = pDaqInfo->wXorMask;
      pAdInfo->SSI[AnalogIn].wAndMask = pDaqInfo->wAndMask;
      pAdInfo->SSI[AnalogIn].dwFifoSize = Eeprom.u.Header.ADCFifoSize * 1024;
      pAdInfo->SSI[AnalogIn].dwChListSize = Eeprom.u.Header.CLFifoSize * 256;

      // Analog output section - MF, MFS
      pAdInfo->SSI[AnalogOut].dwChannels = _PdParseCaps(dwID, AnalogOut, PDHCAPS_CHANNELS);
      pAdInfo->SSI[AnalogOut].dwChBits = _PdParseCaps(dwID, AnalogOut, PDHCAPS_BITS);
      pAdInfo->SSI[AnalogOut].dwRate = pDaqInfo->iMaxAOutRate;

      pAdInfo->SSI[AnalogOut].fGains[0] = 1;
      pAdInfo->SSI[AnalogOut].dwMaxGains = 1;

      pAdInfo->SSI[AnalogOut].fRangeLow[0] = -10.0;
      pAdInfo->SSI[AnalogOut].fRangeHigh[0] = 10.0;

      pAdInfo->SSI[AnalogOut].fFactor[0] = (0xfff / 20);
      pAdInfo->SSI[AnalogOut].fOffset[0] = 10.0;
      pAdInfo->SSI[AnalogOut].dwMaxRanges = 1;

      pAdInfo->SSI[AnalogOut].wXorMask = 0;
      pAdInfo->SSI[AnalogOut].wAndMask = 0xFFF;
      pAdInfo->SSI[AnalogOut].dwFifoSize = 2048;
      pAdInfo->SSI[AnalogOut].dwChListSize = 2;

      // Digital Input/Output section - MF, MFS
      pAdInfo->SSI[DigitalIn].dwChannels = _PdParseCaps(dwID, DigitalIn, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalIn].dwChBits = _PdParseCaps(dwID, DigitalIn, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalIn].dwRate = pDaqInfo->iMaxDIORate;

      pAdInfo->SSI[DigitalOut].dwChannels = _PdParseCaps(dwID, DigitalOut, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalOut].dwChBits = _PdParseCaps(dwID, DigitalOut, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalOut].dwRate = pDaqInfo->iMaxDIORate;

      // Counter-timer section - MF, MFS
      pAdInfo->SSI[CounterTimer].dwChannels = _PdParseCaps(dwID, CounterTimer, PDHCAPS_CHANNELS);
      pAdInfo->SSI[CounterTimer].dwChBits = _PdParseCaps(dwID, CounterTimer, PDHCAPS_BITS);
      pAdInfo->SSI[CounterTimer].dwRate = pDaqInfo->iMaxUCTRate;

   }

   //
   // Analog output boards
   //
   if ((pAdInfo->atType & atPD2AO))
   {
      // Analog output section - AO
      pAdInfo->SSI[AnalogOut].dwChannels = _PdParseCaps(dwID, AnalogOut, PDHCAPS_CHANNELS);
      pAdInfo->SSI[AnalogOut].dwChBits = _PdParseCaps(dwID, AnalogOut, PDHCAPS_BITS);
      pAdInfo->SSI[AnalogOut].dwRate = pDaqInfo->iMaxAOutRate;

      pAdInfo->SSI[AnalogOut].fGains[0] = 1;
      pAdInfo->SSI[AnalogOut].dwMaxGains = 1;

      pAdInfo->SSI[AnalogOut].fRangeLow[0] = -10.0;
      pAdInfo->SSI[AnalogOut].fRangeHigh[0] = 10.0;

      pAdInfo->SSI[AnalogOut].fFactor[0] = (0xffff / 20);
      pAdInfo->SSI[AnalogOut].fOffset[0] = 10.0;
      pAdInfo->SSI[AnalogOut].dwMaxRanges = 1;

      pAdInfo->SSI[AnalogOut].wXorMask = 0x0;
      pAdInfo->SSI[AnalogOut].wAndMask = 0xFFFF;
      pAdInfo->SSI[AnalogOut].dwFifoSize = 2048;
      pAdInfo->SSI[AnalogOut].dwChListSize = 2;

      // Digital Input/Output section - AO
      pAdInfo->SSI[DigitalIn].dwChannels = _PdParseCaps(dwID, DigitalIn, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalIn].dwChBits = _PdParseCaps(dwID, DigitalIn, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalIn].dwRate = pDaqInfo->iMaxDIORate;

      pAdInfo->SSI[DigitalOut].dwChannels = _PdParseCaps(dwID, DigitalOut, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalOut].dwChBits = _PdParseCaps(dwID, DigitalOut, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalOut].dwRate = pDaqInfo->iMaxDIORate;

      // Counter-timer section - AO
      pAdInfo->SSI[CounterTimer].dwChannels = _PdParseCaps(dwID, CounterTimer, PDHCAPS_CHANNELS);
      pAdInfo->SSI[CounterTimer].dwChBits = _PdParseCaps(dwID, CounterTimer, PDHCAPS_BITS);
      pAdInfo->SSI[CounterTimer].dwRate = pDaqInfo->iMaxUCTRate;

   }

   //
   // Digital input/output boards
   //
   if ((pAdInfo->atType & atPD2DIO))
   {
      // Digital Input/Output section - DIO
      pAdInfo->SSI[DigitalIn].dwChannels = _PdParseCaps(dwID, DigitalIn, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalIn].dwChBits = _PdParseCaps(dwID, DigitalIn, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalIn].dwRate = pDaqInfo->iMaxDIORate;

      pAdInfo->SSI[DigitalOut].dwChannels = _PdParseCaps(dwID, DigitalOut, PDHCAPS_CHANNELS);
      pAdInfo->SSI[DigitalOut].dwChBits = _PdParseCaps(dwID, DigitalOut, PDHCAPS_BITS);
      pAdInfo->SSI[DigitalOut].dwRate = pDaqInfo->iMaxDIORate;

      // Counter-timer section - DIO
      pAdInfo->SSI[CounterTimer].dwChannels = _PdParseCaps(dwID, CounterTimer, PDHCAPS_CHANNELS);
      pAdInfo->SSI[CounterTimer].dwChBits = _PdParseCaps(dwID, CounterTimer, PDHCAPS_BITS);
      pAdInfo->SSI[CounterTimer].dwRate = pDaqInfo->iMaxUCTRate;

   }

   retVal = PdAcquireSubsystem(hAdapter, BoardLevel, 0); 
   
   return retVal;
}


#if !defined(_PD_RTLPRO)
//
// Converts analog input data from raw format to volts (for MF/MFS boards)
//
//
int PdAInRawToVolts(int boardNumber,
                       DWORD dwMode,          // Mode used
                       WORD* wRawData,          // Raw data
                       double* fVoltage,        // Engineering unit
                       DWORD dwCount            // Number of samples to convert
                     )
{
    Adapter_Info AdpInfo;
    int retVal;
    int i;
    int modeIndex;
    int polarModes[10];
    int nbOfPolarModes;
    int range;

    // check parameters
    if (!(wRawData && fVoltage && dwCount)) return -1;

    retVal = _PdGetAdapterInfo(boardNumber, &AdpInfo);
    if(retVal <0)
       return retVal;

    // Get a list of modes
    modeIndex = 0;
    for(i=0; i<AdpInfo.SSI[AnalogIn].dwMaxRanges*2; i++)
    {
       // if we are in bipolar mode look for bipolar modes
       // in board caps
       if(dwMode & AIB_INPTYPE)
       { 
          if(AdpInfo.SSI[AnalogIn].fRangeLow[i] < 0)
             polarModes[modeIndex++] = i;
       }
       else 
       {
          if(AdpInfo.SSI[AnalogIn].fRangeLow[i] == 0)
             polarModes[modeIndex++] = i;
       }
    }
    nbOfPolarModes = modeIndex;

    // if we are in high range, look for the highest range
    // else look for the lowest
    range = AdpInfo.SSI[AnalogIn].fRangeHigh[polarModes[0]] - AdpInfo.SSI[AnalogIn].fRangeLow[polarModes[0]];
    modeIndex = polarModes[0];
    for(i=0; i<nbOfPolarModes; i++)
    {
       if(dwMode & AIB_INPRANGE)
       {
          if((AdpInfo.SSI[AnalogIn].fRangeHigh[polarModes[i]] - 
              AdpInfo.SSI[AnalogIn].fRangeLow[polarModes[i]]) > range)
          {
             range = AdpInfo.SSI[AnalogIn].fRangeHigh[polarModes[i]] - AdpInfo.SSI[AnalogIn].fRangeLow[polarModes[i]];
             modeIndex = polarModes[i];
          }
       }
       else
       {
          if((AdpInfo.SSI[AnalogIn].fRangeHigh[polarModes[i]] - 
              AdpInfo.SSI[AnalogIn].fRangeLow[polarModes[i]]) < range)
          {
             range = AdpInfo.SSI[AnalogIn].fRangeHigh[polarModes[i]] - AdpInfo.SSI[AnalogIn].fRangeLow[polarModes[i]];
             modeIndex = polarModes[i];
          }
       }
    }

    // Perform one-channel conversion
    for (i = 0; i < dwCount; i++)
    {
        *(fVoltage + i) = ((*(wRawData + i) & AdpInfo.SSI[AnalogIn].wAndMask) ^ AdpInfo.SSI[AnalogIn].wXorMask) *
                          AdpInfo.SSI[AnalogIn].fFactor[modeIndex] -
                          AdpInfo.SSI[AnalogIn].fOffset[modeIndex];

    }   

    return 0;
}

//
// Converts volts to analog ouput raw data (for MF/MFS/AO boards)
//
//
int PdAOutVoltsToRaw( int boardNumber,
                      DWORD dwMode,            // Mode used
                      double* fVoltage,        // Engineering unit
                      DWORD* dwRawData,        // Raw data
                      DWORD dwCount            // Number of samples to convert
                    )
{
   Adapter_Info AdpInfo;
   int retVal;
   int i;

   // check parameters
   if (!(dwRawData && fVoltage && dwCount)) return -1;

   retVal = _PdGetAdapterInfo(boardNumber, &AdpInfo);
   if (retVal <0)
      return retVal;


   if (AdpInfo.atType & atMF)
   {
      for (i = 0; i < dwCount; i++)
      {
         *(dwRawData + i) = (DWORD)(( *(fVoltage+i) + AdpInfo.SSI[AnalogOut].fOffset[0]) * AdpInfo.SSI[AnalogOut].fFactor[0]);
      }

   }
   else
   {
      for (i = 0; i < dwCount; i++)
      {
         *(dwRawData + i) = (DWORD)(( *(fVoltage+i) + AdpInfo.SSI[AnalogOut].fOffset[0]) * AdpInfo.SSI[AnalogOut].fFactor[0]);
      }
   }

//    if (pAdInfo->atType & atMF)
//    { 
//        // Do MF AOut conversion
//        for (i = 0, j = 0; i < dwCount; i+2, j++)
//        {
//            dwCh0 = (DWORD)(*(fVoltage+i) * pAdInfo->SSI[AnalogOut].fFactor[0] - pAdInfo->SSI[AnalogOut].fOffset[0]);
//            dwCh1 = (DWORD)(*(fVoltage+i+1) * pAdInfo->SSI[AnalogOut].fFactor[0] - pAdInfo->SSI[AnalogOut].fOffset[0]);
//            *(dwRawData + j) = dwCh0 | dwCh1;
//        }
//    } else {
//        // Do AO32 conversion
//        for (i = 0; i < dwCount; i++)
//        {
//            *(dwRawData + i) = (DWORD)(*(fVoltage+i) * pAdInfo->SSI[AnalogOut].fFactor[0] - pAdInfo->SSI[AnalogOut].fOffset[0]);
//            *(dwRawData + i) ^= pAdInfo->SSI[AnalogOut].wXorMask;
//        }
//    }

    return 0;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdGetAdapterClBaseClock
//
// Parameters:  int hAdapter -- handle to adapter
//              int dwMode -- Mode used
//              int *pdwClBaseClock -- CL Base clock
//
// Returns:     int status, 0 = success, <0 = error  
//
// Description: Returns actual value of channel list base clock (11/16,6/33 or 50 MHz)
//              (for All PowerDAQ boards)
//
// Notes:       dwMode is configuration DWORD
//
// ----------------------------------------------------------------------
//-
int _PdGetAdapterClBaseClock(int handle, int mode, int *pClBaseClock) 
{
   PWRDAQ_PCI_CONFIG   PCIConfig;
   int   iBoardID;
   int ClBaseClock[4]={11000000, 16666666, 33000000, 50000000};
   int lowClBaseClock = 0;
   int highClBaseClock= 0;
   int status;

   *pClBaseClock=0; 

   // check parameters
   if (handle<0) 
   {
      return -EINVAL;
   }

   if ((status = PdGetPciConfiguration(handle, &PCIConfig)) < 0) 
   {
      return status;
   }

   iBoardID=(PCIConfig.SubsystemID & 0x3FF);

   // check up high speed boards (with 25MHz oscillator) 
   if (PD_IS_HS(iBoardID))
   {
      // high speed boards
      lowClBaseClock = ClBaseClock[1];
      highClBaseClock= ClBaseClock[3];
   } 
   else
   {
      // regular boards
      lowClBaseClock = ClBaseClock[0];
      highClBaseClock= ClBaseClock[2];
   }

   // return CL Base Clock actual value 
   *pClBaseClock=lowClBaseClock;
   if (mode & AIB_INTCLSBASE) 
      *pClBaseClock=highClBaseClock;

   return 0;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdGetAdapterCvBaseClock
//
// Parameters:  int hAdapter -- handle to adapter
//              int dwMode -- Mode used
//              int *pdwCvBaseClock -- CV Base clock
//
// Returns:     int status     -- 0:  command succeeded
//                                 <0: command failed
//
// Description: Returns actual value of conversion base clock (11/16,6/33 or 50 MHz)
//              (for All PowerDAQ boards)
//
// Notes:       mode is configuration DWORD
//
// ----------------------------------------------------------------------
//-
int _PdGetAdapterCvBaseClock(int hAdapter, int mode, int *pCvBaseClock) 
{  
   PWRDAQ_PCI_CONFIG   PCIConfig;
   int   iBoardID;
   int CvBaseClock[4]={11000000, 16666666, 33000000, 50000000};
   int lowCvBaseClock = 0;
   int highCvBaseClock= 0;
   int status;

   *pCvBaseClock=0; 

   // check parameters
   if (hAdapter<0) 
   {
      return -EINVAL;
   }

   if ((status=PdGetPciConfiguration(hAdapter, &PCIConfig))<0) 
   {
      return status;
   }
   iBoardID=(PCIConfig.SubsystemID & 0x3FF);

   // check up high speed boards (with 25MHz oscillator) 
   if (PD_IS_HS(iBoardID))
   {
      // high speed boards
      lowCvBaseClock = CvBaseClock[1];
      highCvBaseClock= CvBaseClock[3];
   } 
   else
   {
      // regular boards
      lowCvBaseClock = CvBaseClock[0];
      highCvBaseClock= CvBaseClock[2];
   }

   // return CV Base Clock actual value 
   *pCvBaseClock=lowCvBaseClock;
   if (mode & AIB_INTCVSBASE) 
      *pCvBaseClock=highCvBaseClock;
   if (mode & DIB_INTCVSBASE) 
      *pCvBaseClock=highCvBaseClock;
   if (mode & CTB_INTCVSBASE) 
      *pCvBaseClock=highCvBaseClock;
   if (mode & AOB_INTCVSBASE) 
      *pCvBaseClock=highCvBaseClock;

   return 0;
}

#endif

















