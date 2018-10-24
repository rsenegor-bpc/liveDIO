//===========================================================================
//
// NAME:   FIFO_SIZE
//
// DESCRIPTION: 
//
// NOTES:  
//          
//
// AUTHOR:  M. Sheeley, F. Villeneuve
//
// DATE:    9-Apr-2003
//
// HISTORY:
//
//      Rev 1.0,     21-Apr-2003,  M. Sheeley.,   Initial version
//      Rev 1.1      26-Jan-2005,  F. Villeneuve, Linux Port
//
//---------------------------------------------------------------------------
//
//      United Electronic Industries Confidential Information.
//
//      Copyright (C) 2005 United Electronic Industries, Inc.
//      All rights reserved.
//      Unauthorized reproduction is strictly prohibited. 
//      Criminal charges may apply.
//
//===========================================================================
//  
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "win_sdk_types.h"
#include "powerdaq.h"
#include "powerdaq32.h"
#include "pdfw_def.h"



/****************************************************************************/
void GetSerialNumber (DWORD dwAdapter)
{
   PD_EEPROM   aEeprom;
   DWORD       dwResult;
   int         ret;
   int         mfhDiag;


   mfhDiag  = PdAcquireSubsystem(dwAdapter, BoardLevel, 1);
   if (mfhDiag < 0)
   {
      printf("GetSerialNumber: PdAcquireSubsystem error %d\n", mfhDiag);
      exit(EXIT_FAILURE);
   }


   ret = _PdAdapterEepromRead(mfhDiag,12, (WORD*)&aEeprom, &dwResult);
   if (ret < 0)
   {
      printf("GetSerialNumber: _PdAdapterEepromRead error %d\n", ret);
      exit(EXIT_FAILURE);
   }

   printf("The board now being tested is board %d, serial number %s\n", dwAdapter+1, aEeprom.u.Header.SerialNumber);

   PdAcquireSubsystem(mfhDiag, BoardLevel, 0);
}

/****************************************************************************/
int main(int argc, char *argv[])
{
   int  mfhAi, mfhAo, mfhDiag;
   int  diohDi, diohDo, diohDiag;
   int  aohDi, aohDo, aohDiag;
   Adapter_Info AdapterInfo;
   DWORD  dwWords;
   DWORD dwAInCfg;
   WORD  AInBuffer[65536];
   DWORD   dwArr2[65536];
   DWORD   dwCnt;
   DWORD dwData;
   DWORD   dwNumAdapters;;
   PD_EEPROM EEPROM;
   DWORD   dwSamples;
   DWORD numSamples;
   DWORD size;
   int retVal;
   int i;
   DWORD dwAInChList[1] = {0};
      
   // Print hello world
   printf("EEPROM FIFO size fix application. (C)UEI, 2005\n");

   // Open driver.
   dwNumAdapters = PdGetNumberAdapters();
   if (dwNumAdapters < 0)
   {
      printf("PdGetNumberAdapters failed with %d.\n", dwNumAdapters);
      exit(EXIT_FAILURE);
   }

   printf("PowerDAQ Test - %d boards detected\n\n", dwNumAdapters);

   //loop through all board installed on the pc
   for (i = 0; i < (int)dwNumAdapters; i++)
   {
      GetSerialNumber(i);

      retVal = _PdGetAdapterInfo(i, &AdapterInfo);
      if (retVal < 0 )
      {
         printf("_PdGetAdapterInfo error #%d\n",retVal);
         exit(EXIT_FAILURE);
      }


      //Check for the adapter type
      if (AdapterInfo.atType & atMF)
      {
         // Get subsystem in use
         mfhAi = PdAcquireSubsystem(i, AnalogIn, 1);
         if (mfhAi<0) 
         {
            printf("PdAcquireSubsystem error %d\n", mfhAi);
            exit(EXIT_FAILURE);
         }

         mfhDiag  = PdAcquireSubsystem(i,BoardLevel,1);
         if (mfhDiag<0) 
         {
            printf("PdAcquireSubsystem error %d\n", mfhDiag);
            exit(EXIT_FAILURE);
         }

         mfhAo  = PdAcquireSubsystem(i, AnalogOut, 1);
         if (mfhAo<0) 
         {
            printf("PdAcquireSubsystem error %d\n", mfhAo);
            exit(EXIT_FAILURE);
         }
     
         retVal = _PdAdapterEepromRead(mfhDiag,PD_EEPROM_SIZE,(WORD*)&EEPROM,&dwWords);
         if (retVal < 0)
         {
            printf("_PdAdapterEepromRead error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         if (EEPROM.u.Header.CLFifoSize == 1)
         {
            printf("CL FIFO Size Passed!\n");
         } 
         else
         {
            printf("CL FIFO Size FAILED!\n");
            EEPROM.u.Header.CLFifoSize = 1;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(mfhDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("CL FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }
            
            printf("CL FIFO Size updated!\n");
         }

         //Check for the adapter type
         if ((AdapterInfo.atType & atPDLMF))
         {/*

            // set configuration - _PdAOutReset is called inside _PdAOutSetCfg
            retVal = _PdAOutSetCfg(mfhAo, AOB_CVSTART0 , 0); 
            if (retVal < 0)
            {
               printf("_PdAOutSetCfg error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            retVal = _PdAOutSetCvClk(mfhAo, (11000000/100000)-1);
            if (retVal < 0)
            {
               printf("_PdAOutSetCvClk error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            retVal = _PdAOutPutBlock(mfhAo, (DWORD) 65536, (DWORD*) &dwArr2, &dwCnt); 
            if (retVal < 0)
            {
               printf("_PdAOutPutBlock error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            usleep(2000);

            if (EEPROM.u.Header.DACFifoSize == dwCnt/1024)
            {
               printf("DAC FIFO Size Passed!\n");
            } 
            else
            {
               printf("DAC FIFO Size FAILED!\n");
            }
*/
         } 
         else
         {
            if (EEPROM.u.Header.DACFifoSize == 2)
            {
               printf("DAC FIFO Size Passed!\n");
            } 
            else
            {
               printf("DAC FIFO Size FAILED!\n");
               EEPROM.u.Header.DACFifoSize = 2;
               dwWords = PD_EEPROM_SIZE;
               retVal = _PdAdapterEepromWrite(mfhDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
               if (retVal < 0)
               {
                  printf("DAC FIFO Size update FAILED!\n");
                  printf("_PdAdapterEepromWrite error %d\n", retVal);
                  exit(EXIT_FAILURE);
               }

               printf("DAC FIFO Size updated!\n");
            }
         }

         if ((AdapterInfo.atType & atPDLMF))
         {

            if (EEPROM.u.Header.ADCFifoSize == 1)
            {
               printf("ADC FIFO Size Passed!\n");
            } 
            else
            {
               printf("ADC FIFO Size FAILED!\n");
               EEPROM.u.Header.ADCFifoSize = 1;
               retVal = _PdAdapterEepromWrite(mfhDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
               if (retVal < 0)
               {
                  printf("ADC FIFO Size update FAILED!\n");
                  printf("_PdAdapterEepromWrite error %d\n", retVal);
                  exit(EXIT_FAILURE);
               }

               printf("ADC FIFO Size updated!\n");
            }
         }
         else
         {
            retVal = _PdAInReset(mfhAi);
            if (retVal < 0)
            {
               printf("_PdAInReset failed with %d.\n", retVal);
               exit(EXIT_FAILURE);
            }

            dwAInCfg = AIB_CVSTART0 | AIB_CVSTART1| AIB_CLSTART0 | AIB_CLSTART1;
            retVal =  _PdAInSetCfg(mfhAi, dwAInCfg, 0, 0);
            if (retVal < 0)
            {
               printf("_PdAInSetCfg Open Analog Input failed with %d.\n", retVal);
               exit(EXIT_FAILURE);
            }

            retVal = _PdAInSetChList(mfhAi,1, dwAInChList);
            if (retVal < 0)
            {
               printf("_PdAInSetChList failed with %d.\n",retVal);
               exit(EXIT_FAILURE);
            }

            retVal = _PdAInEnableConv(mfhAi, TRUE);
            if (retVal < 0)
            {
               printf("_PdAInEnableConv failed with %d.\n", retVal);
               exit(EXIT_FAILURE);
            }
            retVal = _PdAInSwStartTrig(mfhAi);
            if (retVal < 0)
            {
               printf("_PdAInSwStartTrig failed with %d.\n",retVal);
               exit(EXIT_FAILURE);
            }
            fflush(stdout);
            usleep(5000);
            retVal = _PdAInSwStopTrig(mfhAi);
            if (retVal < 0)
            {
               printf("_PdAInSwStopTrig failed with %d.\n", retVal);
               exit(EXIT_FAILURE);
            }

	    // Read 512 samples at a time until we reach the end of the FIFO
	    // This will give us the real FIFO size
            dwSamples = 0;
            do
            {
               retVal = _PdAInGetSamples(mfhAi,512, AInBuffer, &numSamples);
               
               if(numSamples > 0)
                  dwSamples = dwSamples+numSamples;
            }
            while(retVal >= 0 && numSamples > 0);

            if (retVal < 0)
            {
               printf("_PdAInGetSamples failed with %d.\n", retVal);
            }

            printf("SingleAI: detected %d samples.\n", dwSamples);
            printf("SingleAI: EEPROM ADC Fifo Size=%d\n",(DWORD)EEPROM.u.Header.ADCFifoSize);

            if (dwSamples == (DWORD) EEPROM.u.Header.ADCFifoSize * (DWORD) 1024)
            {
               printf("ADC FIFO Size Passed!\n");
            } 
            else
            {
               printf("ADC FIFO Size FAILED!\n");
               EEPROM.u.Header.ADCFifoSize = (DWORD) dwSamples/1024;
               dwWords = PD_EEPROM_SIZE;

               retVal = _PdAdapterEepromWrite(mfhDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
               if (retVal < 0)
               {
                  printf("ADC FIFO Size update FAILED!\n");
                  printf("_PdAdapterEepromWrite error %d\n", retVal);
                  exit(EXIT_FAILURE);
               }
                                 
               printf("eeprom's ADC FIFO Size successfully updated\n");
            }
         }  


         // return subsystem
         retVal = PdAcquireSubsystem(mfhDiag, BoardLevel, 0);
         if (retVal < 0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }

         //Open AIn subsystem
         retVal = PdAcquireSubsystem(mfhAi,AnalogIn, 0);
         if(retVal < 0)
         {
            printf("PdAdapterAcquireSubsystem error %d.\n", retVal);
         }

         retVal = PdAcquireSubsystem(mfhAo, AnalogOut, 0);
         if (retVal < 0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }
      } 
      else if (AdapterInfo.atType & atPD2DIO)
      {
         // Get subsystem in use
         diohDo = PdAcquireSubsystem(i, DigitalOut, 1);
         if (diohDo < 0)
         {
             printf("PdAdapterAcquireSubsystem error %d\n", diohDo);
             exit(EXIT_FAILURE);
         }

         diohDi = PdAcquireSubsystem(i,DigitalIn, 1);
         if (diohDi < 0)
         {
             printf("PdAdapterAcquireSubsystem error %d\n", diohDi);
             exit(EXIT_FAILURE);
         }

         diohDiag = PdAcquireSubsystem(i,BoardLevel,1);
         if (diohDiag < 0)
         {
             printf("PdAdapterAcquireSubsystem error %d\n", diohDiag);
             exit(EXIT_FAILURE);
         }

         //	EEPROM = &m_PdEeprom;
         retVal = _PdAdapterEepromRead(diohDiag, PD_EEPROM_SIZE,(WORD*)&EEPROM, &dwWords);
         if(retVal < 0)
         {
            printf("_PdAdapterEepromRead error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         if (EEPROM.u.Header.CLFifoSize == 1)
         {
            printf("CL FIFO Size Passed!\n");
         } 
         else
         {
            printf("CL FIFO Size FAILED!\n");
            EEPROM.u.Header.CLFifoSize = 1;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(diohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("CL FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("CL FIFO Size updated!\n");
         } 

         if (EEPROM.u.Header.ADCFifoSize == 1)
         {
            printf("ADC FIFO Size Passed!\n");
         } 
         else
         {
            printf("ADC FIFO Size FAILED!\n");
            EEPROM.u.Header.ADCFifoSize = 1;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(diohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("ADC FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("ADC FIFO Size updated!\n");
         }

         // Reset DIO Board (all ports)
         retVal = _PdDIOReset(diohDi);
         if (retVal<0) 
         {
            printf("_PdDIOReset error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         //Setup Port mode
         retVal = _PdDIOEnableOutput(diohDi, 0xFF);
         if (retVal<0) 
         {
            printf("_PdDIOEnableOutput error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         retVal = _PdDIO256CmdWrite(diohDo, 0x100000, 0x555555);
         if(retVal < 0)
         {
            printf("_PdDIO256CmdWrite error %d\n", retVal);
            exit(EXIT_FAILURE);
         }
         usleep(50000);
         retVal = _PdDIO256CmdRead(diohDi, 0x100000,&dwData);
         if(retVal < 0)
         {
            printf("_PdDIO256CmdRead error %d\n", retVal);
            exit(EXIT_FAILURE);
         }
         
         size = 2;
         if (dwData == 0x555555)
         {
            size = 64;
         }

         if (EEPROM.u.Header.DACFifoSize == size)
         {
            printf("DAC FIFO Size Passed!\n");
         } 
         else
         {
            printf("DAC FIFO Size FAILED!\n");
            EEPROM.u.Header.DACFifoSize = size;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(diohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("DAC FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("DAC FIFO Size updated!\n");
         }

         // return subsystem
         retVal = PdAcquireSubsystem(diohDiag, BoardLevel, 0);
         if (retVal < 0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }

         retVal = PdAcquireSubsystem(diohDi,DigitalIn, 0);
         if(retVal < 0)
         {
            printf("PdAdapterAcquireSubsystem error %d.\n", retVal);
         }

         retVal = PdAcquireSubsystem(diohDo, DigitalOut, 0);
         if (retVal < 0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }


      } 
      else if ((AdapterInfo.atType & atPD2AO))
      {
         aohDo = PdAcquireSubsystem(i, DigitalOut, 1);
         if (aohDo < 0)
         {
             printf("PdAdapterAcquireSubsystem error %d\n", aohDo);
             exit(EXIT_FAILURE);
         }

         aohDi = PdAcquireSubsystem(i,DigitalIn, 1);
         if (aohDi < 0)
         {
             printf("PdAdapterAcquireSubsystem error %d\n", aohDi);
             exit(EXIT_FAILURE);
         } 
         aohDiag = PdAcquireSubsystem(i,BoardLevel,1);
         if (aohDiag<0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", aohDiag);
            exit(EXIT_FAILURE);
         }

         retVal = _PdAdapterEepromRead(aohDiag,PD_EEPROM_SIZE,(WORD*)&EEPROM,  &dwWords);
         if(retVal < 0)
         {
            printf("_PdAdapterEepromRead error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

         printf("the value in EEPROM is %d\n",EEPROM.u.Header.CLFifoSize);       
         if (EEPROM.u.Header.CLFifoSize == 1)
         {
            printf("CL FIFO Size Passed!\n");
         } 
         else
         {
            printf("CL FIFO Size FAILED!\n");
            EEPROM.u.Header.CLFifoSize = 1;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(aohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("CL FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("CL FIFO Size updated!\n");
         } 

         if (EEPROM.u.Header.ADCFifoSize == 1)
         {
            printf("ADC FIFO Size Passed!\n");
         } 
         else
         {
            printf("ADC FIFO Size FAILED!\n");
            EEPROM.u.Header.ADCFifoSize = 1;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(aohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);
            if (retVal < 0)
            {
               printf("ADC FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("ADC FIFO Size updated!\n");
         }

         // Detecting current size of DACFifoSize								
         retVal = _PdDIO256CmdWrite(aohDo, 0x100000, 0xA5A5A5);
	 if (retVal < 0)
         {
            printf("DAC FIFO Size update FAILED!\n");
            printf("_PdDIO256CmdWrite error %d\n", retVal);
            exit(EXIT_FAILURE);
         }

	 usleep(50000);
			
         retVal = _PdDIO256CmdRead(aohDi, 0x100000,&dwData);
         if (retVal < 0)
         {
            printf("DAC FIFO Size update FAILED!\n");
            printf("_PdDIO256CmdRead error %d\n", retVal);
            exit(EXIT_FAILURE);
         }
	
         size = 2;
         if(dwData == 0xA5A5A5) size = 64;				
			
         if (EEPROM.u.Header.DACFifoSize == size)
         {
            printf("DAC FIFO Size Passed!\n");
         } 
         else
         {
            printf("DAC FIFO Size FAILED!\n");
            EEPROM.u.Header.DACFifoSize = size;
            dwWords = PD_EEPROM_SIZE;
            retVal = _PdAdapterEepromWrite(aohDiag,(WORD*)&EEPROM, PD_EEPROM_SIZE-1);            
            if (retVal < 0)
            {
               printf("DAC FIFO Size update FAILED!\n");
               printf("_PdAdapterEepromWrite error %d\n", retVal);
               exit(EXIT_FAILURE);
            }

            printf("DAC FIFO Size updated!\n");
         }

         retVal = PdAcquireSubsystem(aohDiag,BoardLevel, 0);
         if (retVal<0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }

         retVal = PdAcquireSubsystem(aohDi,DigitalIn, 0);
         if(retVal < 0)
         {
            printf("PdAdapterAcquireSubsystem error %d.\n", retVal);
         }

         retVal = PdAcquireSubsystem(aohDo, DigitalOut, 0);
         if (retVal < 0) 
         {
            printf("PdAdapterAcquireSubsystem error %d\n", retVal);
         }
      }
      printf("\n");
   }

   return 0;
}
