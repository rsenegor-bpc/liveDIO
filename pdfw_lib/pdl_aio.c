//===========================================================================
//
// NAME:    pdl_aio.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to Analog Input buffering functions
//
// AUTHOR: Frederic Villeneuve 
//         Alex Ivchenko
//
//---------------------------------------------------------------------------
//      Copyright (C) 2000,2004 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------
// For more informations on using and distributing this software, please see
// the accompanying "LICENSE" file.
//
// this file is not to be compiled independently
// but to be included into pdfw_lib.c



//
// Function:    pd_register_daq_buffer
//
// Parameters:  int board
//
// Returns:     1 = SUCCESS
//
//
// Description: register daq buffer.
//
// Notes: To unregister daq buffer call with databuf = NULL
//

int pd_register_daq_buffer(int board, u32 SubSystem, u32 ScanSize, u32 FrameSize, u32 NumFrames,
                           uint16_t* databuf, int bWrap)
{
    void* buf;
    PTBuf_Info pDaqBuf = NULL;
    

    if ((SubSystem == AnalogIn) ||
        (SubSystem == DigitalIn) ||
        (SubSystem == CounterTimer) ||
        (SubSystem == DSPCounter))
    {
        pDaqBuf = &pd_board[board].AinSS.BufInfo;
        pDaqBuf->SampleSize = sizeof(u16);
    }
    if ((SubSystem == AnalogOut) ||
        (SubSystem == DigitalOut))
    {
        pDaqBuf = &pd_board[board].AoutSS.BufInfo;
        pDaqBuf->SampleSize = sizeof(u32);
    }
    if (!pDaqBuf) return 0;

    // parameters check
    if (!(ScanSize && FrameSize && NumFrames))
    {
	DPRINTK_I("pd_register_daq_buffer: bad parameters\n");
        return 0;
    }

    pDaqBuf->ScanSize = ScanSize;
    pDaqBuf->FrameSize = FrameSize;
    pDaqBuf->NumFrames = NumFrames;
    pDaqBuf->userdatabuf = databuf;  // it can be NULL, then we don't work with it
    pDaqBuf->bWrap = bWrap & BUF_BUFFERWRAPPED;
    pDaqBuf->bRecycle = bWrap & BUF_BUFFERRECYCLED;
    pDaqBuf->bDWValues = bWrap & BUF_DWORDVALUES;
    pDaqBuf->bFixedDMA = bWrap & BUF_FIXEDDMA;

    pDaqBuf->FirstTimestamp = 0;
    pDaqBuf->LastTimestamp = 0;

    pDaqBuf->FrameValues = pDaqBuf->FrameSize * pDaqBuf->ScanSize;
    pDaqBuf->ScanValues = pDaqBuf->ScanSize;
    pDaqBuf->MaxValues = pDaqBuf->FrameValues * pDaqBuf->NumFrames;

    if (pDaqBuf->bDWValues)
    {
       pDaqBuf->BufSizeInBytes = pDaqBuf->NumFrames * pDaqBuf->FrameValues * sizeof(u32);
       pDaqBuf->DataWidth = 4;
       pDaqBuf->SampleSize = sizeof(u32);
    }
    else
    {
       pDaqBuf->BufSizeInBytes = pDaqBuf->NumFrames * pDaqBuf->FrameValues * sizeof(u16);
       pDaqBuf->DataWidth = 2;
       pDaqBuf->SampleSize = sizeof(u16);
    }

    // try to allocate buffer memory
    DPRINTK_I("Trying to allocate %d bytes\n", pDaqBuf->BufSizeInBytes);
    buf = pd_alloc_bigbuf(pDaqBuf->BufSizeInBytes);

    if (buf) {
        pDaqBuf->databuf = (u16*)buf;
    } else {
        pDaqBuf->databuf = NULL;
        DPRINTK_F("rvmalloc/kmalloc failed...\n");
        return 0;
    }

    DPRINTK_I("pd_register_daq_buffer: Allocated buffer for SS %d, size %d\n", SubSystem, pDaqBuf->BufSizeInBytes);

    return (pDaqBuf->BufSizeInBytes);
}

int pd_unregister_daq_buffer(int board, u32 SubSystem)
{
    PTBuf_Info pDaqBuf = NULL;

    if ((SubSystem == AnalogIn) ||
        (SubSystem == DigitalIn) ||
        (SubSystem == CounterTimer) ||
        (SubSystem == DSPCounter))
        pDaqBuf = &pd_board[board].AinSS.BufInfo;

    if ((SubSystem == AnalogOut) ||
        (SubSystem == DigitalOut))
        pDaqBuf = &pd_board[board].AoutSS.BufInfo;

    if (!pDaqBuf) return 0;

    if (pDaqBuf->databuf)
        pd_free_bigbuf(pDaqBuf->databuf,
                       pDaqBuf->BufSizeInBytes);
    pDaqBuf->databuf = NULL;

    DPRINTK_I("pd_unregister_daq_buf: Freed buffer for SS %d, size %d\n", SubSystem, pDaqBuf->BufSizeInBytes);

    // return size of deallocated buffer
    return (pDaqBuf->BufSizeInBytes);
}
//
// Function:    pd_clear_daq_buffer
//
// Parameters:  int board
//              int subsystem - subsystem type
//
// Returns:     1 = SUCCESS
//
//
// Description: The Clear DaqBuf function clears the DAQ Buffer and sets it
//              to the initialized state.
// Notes:
//
//u16 XFBufTemp[0x1000];  //FIXME: temp buffer for debug only: remove after debugging
int pd_clear_daq_buffer(int board, int subsystem)
{
    PTBuf_Info pDaqBuf = NULL;
    u32* pXferBuf = NULL;

    // AIn
    if (subsystem == AnalogIn)
    {
        if (pd_board[board].AinSS.SubsysState != ssConfig)
        {
    	    DPRINTK_F("pd_clear_daq_buffer error: SubsysState != ssConfig\n");
            return 0;
        }
        pDaqBuf = &pd_board[board].AinSS.BufInfo;
        pXferBuf = pd_board[board].AinSS.pXferBuf;
    }

    if (subsystem == AnalogOut)
    {
        pDaqBuf = &pd_board[board].AoutSS.BufInfo;
        pXferBuf = pd_board[board].AoutSS.pXferBuf;
    }  

    if (!pDaqBuf) return 0;

    if (!pXferBuf)
    {
  	    DPRINTK_F("pd_clear_daq_buffer error: pDaqBuf->XferBuf == NULL\n");
        return 0;
    }
    if (!pDaqBuf->databuf)
    {
  		DPRINTK_F("pd_clear_daq_buffer error: pDaqBuf->databuf == NULL\n");
        return 0;
    }

    DPRINTK_T("pd_clear_daq_buffer: buffer is prepared to run!\n");

    // Initialize DaqBuf status.
    pDaqBuf->Count = 0;
    pDaqBuf->Head = 0;
    pDaqBuf->Tail = 0;
    pDaqBuf->ValueCount = 0;
    pDaqBuf->WrapCount = 0;
    pDaqBuf->FirstTimestamp = 0;
    pDaqBuf->LastTimestamp = 0;
    pDaqBuf->ValueIndex = 0;
    pDaqBuf->ScanIndex = 0;
    
    // do clear events
    if (subsystem == AnalogIn)
    {
       pd_board[board].AinSS.bCheckHalfDone = TRUE;
       pd_board[board].AinSS.bCheckFifoError = TRUE;

       pd_board[board].AinSS.dwEventsNotify = 0;
       pd_board[board].AinSS.dwEventsStatus = 0;
       pd_board[board].AinSS.dwEventsNew = 0;

       pd_board[board].AinSS.XferBufValueCount = 0;
    }

    if (subsystem == AnalogOut)
    {
       pd_board[board].AoutSS.bCheckHalfDone = TRUE;
       pd_board[board].AoutSS.bAsyncMode = TRUE;
   
       if (pd_board[board].AoutSS.SubsysConfig & AOB_BUFMODE1) // stop if not enough performance
           pd_board[board].AoutSS.bCheckFifoError = TRUE;
       else
           pd_board[board].AoutSS.bCheckFifoError = FALSE;

       pd_board[board].AoutSS.dwEventsNotify = 0;
       pd_board[board].AoutSS.dwEventsStatus = 0;
       pd_board[board].AoutSS.dwEventsNew = 0;
       pd_board[board].AoutSS.XferBufValueCount = 0;
    } 

    return 1;
}

int pd_get_daq_buffer_size(int board, int SubSystem)
{
    PTBuf_Info pDaqBuf = NULL;

    if ((SubSystem == AnalogIn) ||
        (SubSystem == DigitalIn) ||
        (SubSystem == CounterTimer) ||
        (SubSystem == DSPCounter))
        pDaqBuf = &pd_board[board].AinSS.BufInfo;

    if ((SubSystem == AnalogOut) ||
        (SubSystem == DigitalOut))
        pDaqBuf = &pd_board[board].AoutSS.BufInfo;

    if (!pDaqBuf) return 0;

    DPRINTK_I("pd_get_daq_buffer_size: buffer for SS %d, size %d\n", SubSystem, pDaqBuf->BufSizeInBytes);

    return pDaqBuf->BufSizeInBytes;
}

//--------------------------------------------------------
// DIO Digital Input
// Aux function - returns offset register value (input)
u32 get_din_offs_reg(u32 Channels) 
{
   switch (Channels)
   {
   case 0:  return 0x00000000;
   case 1:  return 0x00000000;
   case 2:  return 0x00ffffff;
   default: return (0xffffff - (Channels-2));
   }
}

// Aux function - returns count register value (input)
u32 get_din_cnt_reg(u32 Channels, u32 TranSize) 
{
   u32 cl,ch,res;
   if (!Channels) 
      return 0xfff000;
   
   ch = (TranSize)/Channels-1;
   cl = Channels - 1;
   res = (ch << 12)|cl;
   return (res>0xffffff)?0xfff007:res;
}


// Aux function - returns destination register value (input)
u32 get_din_dest_reg(u32 Channels) 
{
   if (Channels < 8)
      return((DIO_REG0 + Channels)|DIO_LAR);
   else
      return(DIO_REG0|DIO_LAR);
}

//--------------------------------------------------------
// DIO Digital Output
// Aux function - returns offset register value (output)
u32 get_dout_offs_reg(u32 Channels) 
{
   switch (Channels) 
   {
       case 0:  return 0x00000000;
       case 1:  return 0x00000000;
       case 2:  return 0x00ffffff;
       default: return (0xffffff - (Channels-2));
   }
}

// Aux function - returns count register value (output)
u32 get_dout_cnt_reg(u32 Channels, u32 TranSize) 
{
   u32 cl, ch, res;
   if (!Channels) return 0xfff000;

   ch = (TranSize)/Channels-1;
   cl = Channels - 1;
   res = (ch << 12)|cl;
   return (res>0xffffff)?0xfff007:res;
}

// Aux function - returns destination register value (output)
u32 get_dout_dest_reg(u32 Channels) 
{
   if (Channels < 8)
      return((DIO_REG0 + Channels)|DIO_SWR);
   else
      return(DIO_REG0|DIO_SWR);
}

//--------------------------------------------------------
// AO DMA Output
// Aux function - returns offset register value (output)
u32 get_aout_offs_reg(u32 Channels) 
{
   u32 result;

   switch (Channels) 
   {
       case 0:  result = 0x00000000; break;
       case 1:  result = 0x00000000; break;
       case 2:  result = 0x00ffffff; break;
       default: result = 0xffffff - (Channels-2); break;
   }

   DPRINTK_F("get_aout_offs_reg result = 0x%x\n", result);

   return result;
}

// Aux function - returns count register value (output)
u32 get_aout_cnt_reg(u32 Channels, u32 TranSize) 
{
   u32 cl, ch, res, result;
   
   //if ((Channels == 96)&&(TranSize == 64*1024)) return 0x2A905F;
   if (!Channels) return 0xfff000;

   ch = (TranSize)/Channels-1;
   cl = Channels - 1;
   res = (ch << 12)|cl;
   result = (res>0xffffff)?0xfff007:res;

   DPRINTK_F("get_aout_cnt_reg result = 0x%x\n", result);

   return result;
}

// Aux function - returns destination register value (output)
u32 get_aout_dest_reg(u32 Channels) 
{
   u32 result = (AO_REG0 + Channels)|AO_WR;
   DPRINTK_F("get_aout_dest_reg: result = 0x%x\n", result);
   return(result);
}



//
//
// Function:    pd_ain_async_init
//
// Parameters:  int board
//              PTAinAsyncCfg pAInCfg  -- AIn async config struct:
//
// Returns:     1 = SUCCESS
//
// Description: The AIn Initialize Asynchronous Buffered Acquisition function
//              initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
//
int pd_ain_async_init(int board, tAsyncCfg* pAInCfg)
{
   u32 offset, count, dest;
   u32 UserEvents;
   int i;

   // Check for ongoing process
   if (pd_board[board].AinSS.SubsysState == ssRunning)
   {
      DPRINTK_F("pd_ain_async_init bails!  SubsysState == ssRunning!\n");
      return 0;
   }

   pd_board[board].AinSS.SubsysState = ssConfig;

   // Verify that a buffer has been properly registered.
   if (!pd_board[board].AinSS.BufInfo.databuf)
   {
      DPRINTK_F("pd_ain_async_init bails!  no databuf!\n");
      return 0;
   }

   // Initialize DaqBuf status.
   if (!pd_clear_daq_buffer(board, AnalogIn))
   {
      DPRINTK_F("pd_ain_async_init bails!  pd_clear_daq_buffer failed!\n");
      return 0;
   }

   // Program AIn subsystem:
   pd_board[board].AinSS.BufInfo.Count = 0;
   pd_board[board].AinSS.BufInfo.Head = 0;
   pd_board[board].AinSS.BufInfo.Tail = 0;
   pd_board[board].AinSS.BufInfo.FirstTimestamp = 0;
   pd_board[board].AinSS.BufInfo.LastTimestamp = 0;
   pd_board[board].AinSS.BufInfo.ValueCount = 0;
   pd_board[board].AinSS.BufInfo.WrapCount = 0;
   pd_board[board].AinSS.BufInfo.ScanIndex = 0;

   pd_board[board].AinSS.bCheckHalfDone = TRUE;
   pd_board[board].AinSS.bCheckFifoError = TRUE;
   pd_board[board].AinSS.bAsyncMode = TRUE;

   pd_board[board].AinSS.dwEventsNotify = 0;
   pd_board[board].AinSS.dwEventsStatus = 0;
   pd_board[board].AinSS.dwEventsNew = 0;

   pd_board[board].AinSS.BlkXferValues = AIN_BLKSIZE;
   pd_board[board].AinSS.XferBufValueCount = 0;

   pd_board[board].AinSS.dwAInCfg = pAInCfg->dwAInCfg;
   pd_board[board].AinSS.dwAInPreTrigCount = pAInCfg->dwAInPreTrigCount;
   pd_board[board].AinSS.dwAInPostTrigCount = pAInCfg->dwAInPostTrigCount;
   pd_board[board].AinSS.dwCvRate = pAInCfg->dwCvRate;
   pd_board[board].AinSS.dwClRate = pAInCfg->dwClRate;
   pd_board[board].AinSS.dwChListChan = pAInCfg->dwChListSize;

   // limit number of XFer cycles if not in BM mode
   if (pd_board[board].AinSS.FifoXFerCycles > 8)
       pd_board[board].AinSS.FifoXFerCycles = 8;

   if (pAInCfg->dwChListSize)
      memcpy(pd_board[board].AinSS.ChList, pAInCfg->dwChList, PD_MAX_CL_SIZE * sizeof(uint32_t));

   // Configure AIn subsystem.
   if ((pd_board[board].dwXFerMode == XFERMODE_BM) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM8WORD))
      pd_board[board].AinSS.dwAInCfg |= AIB_SELMODE1;

   
   // Configure AIn subsystem
   if (!pd_ain_set_config(board,
                          pd_board[board].AinSS.dwAInCfg,
                          pd_board[board].AinSS.dwAInPreTrigCount,
                          pd_board[board].AinSS.dwAInPostTrigCount))
   {
      DPRINTK_F("pd_ain_async_init bails!  pd_ain_set_config failed!\n");
      return 0;
   }

   // Set internal CV clock if used.
   if ( (pAInCfg->dwAInCfg & (AIB_CVSTART0 | AIB_CVSTART1)) == AIB_CVSTART0 )
   {
      if (!pd_ain_set_cv_clock(board, pd_board[board].AinSS.dwCvRate))
      {
         DPRINTK_F("pd_ain_async_init bails!  pd_ain_set_cv_clock failed!\n");
         return 0;
      }
   }

   // Set internal CL clock if used.
   if ( (pAInCfg->dwAInCfg & (AIB_CLSTART0 | AIB_CLSTART1)) == AIB_CLSTART0 )
   {
      if (!pd_ain_set_cl_clock(board, pd_board[board].AinSS.dwClRate))
      {
         DPRINTK_F("pd_ain_async_init bails!  pd_ain_set_cl_clock failed!\n");
         return 0;
      }
   }

   if (!pd_ain_set_channel_list(board, pd_board[board].AinSS.dwChListChan,
                                pd_board[board].AinSS.ChList))
   {
      DPRINTK_F("pd_ain_async_init bails!  pd_ain_set_channel_list failed!\n");
      return 0;
   }

#define RTModeAIBM_SAMPLENUM   320
#define RTModeAIBM_NUMCHLIST   10
#define RTModeAIBM_DEFDMASIZE  31
#define RTModeAIBM_DEFBURSTS   1*RTModeAIBM_NUMCHLIST
#define RTModeAIBM_TRANSFERSIZE  (RTModeAIBM_DEFDMASIZE+1)*(RTModeAIBM_DEFBURSTS-1)


   // Set up bus mastering parameters
   if ((pd_board[board].dwXFerMode == XFERMODE_BM) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM8WORD)) 
   {
      u32 BMList[4];
      u32 aibmDefDMASize;    // Default DMA burst size -1
      u32 aibmDefBursts;     // Default number of BM bursts per DMA transfer
      u32 aibmDefBurstSize;  // Default BM burst size
      u32 statMemAddr;       // Address of Status memory in DSP X memory

      switch (pd_board[board].Eeprom.u.Header.ADCFifoSize) 
      { 
         case 1:
         case 2:     
            if (pd_board[board].dwXFerMode == XFERMODE_BM8WORD)
            {      
               aibmDefDMASize = 7;
               aibmDefBursts = 64;
            }
            else
            {
               aibmDefDMASize = 15;
               aibmDefBursts = 32;
            }
            break;
         case 4:
         case 8:
         case 16:
         case 32:
         case 64:
            if (pd_board[board].dwXFerMode == XFERMODE_BM8WORD)
            {      
               aibmDefDMASize = 7;
               aibmDefBursts = 128;
            }
            else
            {
               aibmDefDMASize = 15;
               aibmDefBursts = 64;
            }
            break;
         default:
            aibmDefDMASize = 15;
            aibmDefBursts = 32;
            break;
      }

      if (pd_board[board].dwXFerMode == XFERMODE_RTBM)
      {      
         aibmDefDMASize = RTModeAIBM_DEFDMASIZE;
         aibmDefBursts = RTModeAIBM_DEFBURSTS;
      }
            
      aibmDefBurstSize = aibmDefDMASize << 16;

      // Read address of status memory
      if(!pd_dsp_reg_read(board, 0x2, &statMemAddr))
      {
         DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_read failed!\n");
         return 0;
      }

      // Enable/Disable real-time bus master mode
      if(!pd_dsp_reg_write(board, statMemAddr + ADRREALTIMEBMMODE, (pd_board[board].dwXFerMode == XFERMODE_RTBM) ? 1 : 0))
      {
         DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
         return 0;
      }

      // Writes def DMA size, bursts and burst size to DSP memory
      if(!pd_dsp_reg_write(board, statMemAddr + ADRAIBM_DEFDMASIZE, aibmDefDMASize))
      {
         DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
         return 0;
      }
      if(!pd_dsp_reg_write(board, statMemAddr + ADRAIBM_DEFBURSTS, aibmDefBursts))
      {
         DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
         return 0;
      }
      if(!pd_dsp_reg_write(board, statMemAddr + ADRAIBM_DEFBURSTSIZE, aibmDefBurstSize))
      {
         DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
         return 0;
      }

      if(pd_board[board].dwXFerMode == XFERMODE_RTBM)
      {
         if(!pd_dsp_reg_write(board, statMemAddr + ADRRTModeAIBM_TRANSFERSIZE, RTModeAIBM_TRANSFERSIZE))
         {
            DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
            return 0;
         }

         if(!pd_dsp_reg_write(board, statMemAddr + ADRRRTModeAIBM_SAMPLENUM, RTModeAIBM_SAMPLENUM))
         {
            DPRINTK_F("pd_ain_async_init failed: pd_dsp_reg_write failed!\n");
            return 0;
         }
      }

      BMList[0] = (u32)pd_board[board].DMAHandleBMB[AI_PAGE0] >> 8;
      BMList[1] = (u32)pd_board[board].DMAHandleBMB[AI_PAGE1] >> 8;
      BMList[2] = pd_board[board].AinSS.BmFHFXFers;
      BMList[3] = pd_board[board].AinSS.BmPageXFers;

      if (!pd_ain_set_BM_ctr(board, AIBM_SET, BMList))
      {
          DPRINTK_F("pd_ain_async_init: cannot set BM ctr\n");
          return 0;
      }
   }

   // Program DMA if we are in DMA mode - this is possible
   if ((pAInCfg->dwAInCfg & AIB_SELMODE3)&&(pAInCfg->dwAInCfg & AIB_SELMODE4))
   {
      // AIn used for DIn or CT
      // check whether channel list size is a power of two
      for (i = 0; i <= 3; i++) 
      { //(add dk 05.11.2002)
        if (pAInCfg->dwChListSize == (u32)(1<<i))
            break;
      }

      if (i < 8)
      {
         offset = get_din_offs_reg(pAInCfg->dwChListSize);  // Offset register
         count = get_din_cnt_reg(pAInCfg->dwChListSize, 512);   // Count register
         dest = get_din_dest_reg(*(pAInCfg->dwChList));  // Destination register (stored in the first channel)
         
         if (!pd_dio_dmaSet(board, offset, count, dest))
         {
            DPRINTK_F("pd_ain_async_init: cannot set up DMA for DI operation\n");
            return 0;
         }
      }
      else
      {
         DPRINTK_F("pd_ain_async_init: channel list is not suitable for DMA\n");
         return 0;
      }
   }

   // Configure AIn firmware event interrupts.
   pd_board[board].FwEventsConfig.AIOIntr = AIB_FHFIm | AIB_FHFSC;
   if ((pd_board[board].dwXFerMode == XFERMODE_BM) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM8WORD))
   {
        pd_board[board].FwEventsConfig.AInIntr = AIB_BMPgDoneIm | AIB_BMErrIm; //| AIB_BMPg0DoneSC | AIB_BMPg0DoneSC | AIB_BMErrSC;
   }

   if (!pd_enable_events(board, &pd_board[board].FwEventsConfig))
   {
      DPRINTK_F("pd_ain_async_init bails!  pd_enable_events failed!\n");
      return 0;
   }

   // Set AIn user events.
   UserEvents = eAllEvents;
   if (!pd_clear_user_events(board, AnalogIn, UserEvents))
   {
      DPRINTK_F("pd_ain_async_init bails!  pd_clear_user_events failed!\n");
      return 0;
   }

   if (pAInCfg->dwEventsNotify)
   {
      UserEvents = pAInCfg->dwEventsNotify;
      if (!pd_set_user_events(board, AnalogIn, UserEvents))
      {
         DPRINTK_F("pd_ain_async_init bails!  pd_set_user_events failed! UserEvents = 0x%08X\n", UserEvents);
         return 0;
      }
   }

   pd_board[board].AinSS.SubsysState = ssStandby;

   return 1;
}


//
//
// Function:    pd_aout_async_init
//
// Parameters:  int board
//              PTAinAsyncCfg pAInCfg  -- AIn async config struct:
//
// Returns:     1 = SUCCESS
//
// Description: The AOut Initialize Asynchronous Buffered Output function
//              initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//              This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
//              This routine may also be called for DIO DOut
//
//              This routine uses Analog Input configuration structure :-)
//
//
int pd_aout_async_init(int board, tAsyncCfg* pAOutCfg)
{
   u32 UserEvents;
   u32 BufVal;
   u32 offset;
   u32 count = 0;
   u32 dest = 0;
   int i;
   
   // Check for ongoing process
   if (pd_board[board].AoutSS.SubsysState == ssRunning)
   {
      DPRINTK_F("pd_aout_async_init bails!  SubsysState == ssRunning!\n");
      return 0;
   }

   pd_board[board].AoutSS.SubsysState = ssConfig;

   // Verify that a buffer has been properly registered.
   if (!pd_board[board].AoutSS.BufInfo.databuf)
   {
      DPRINTK_F("pd_aout_async_init bails!  no databuf!\n");
      return 0;
   }

   if (!pd_clear_daq_buffer(board, AnalogOut))
   {
      DPRINTK_F("pd_aout_async_init bails!  pd_clear_daq_buffer failed!\n");
      return 0;
   }

   pd_board[board].AoutSS.BufInfo.Count = 0;
   pd_board[board].AoutSS.BufInfo.Head = 0;
   pd_board[board].AoutSS.BufInfo.Tail = 0;
   pd_board[board].AoutSS.BufInfo.FirstTimestamp = 0;
   pd_board[board].AoutSS.BufInfo.LastTimestamp = 0;
   pd_board[board].AoutSS.BufInfo.ValueCount = 0;
   pd_board[board].AoutSS.BufInfo.WrapCount = 0;
   pd_board[board].AoutSS.BufInfo.ScanIndex = 0;


   //-----------------------------------------------------------------------
   // Initialize AOut event status.
   pd_board[board].AoutSS.bCheckHalfDone = TRUE;
   pd_board[board].AoutSS.bAsyncMode = TRUE;

   if (pAOutCfg->dwAInCfg & AOB_BUFMODE1) // stop if not enough performance
      pd_board[board].AoutSS.bCheckFifoError = TRUE;
   else
      pd_board[board].AoutSS.bCheckFifoError = FALSE;

   pd_board[board].AoutSS.dwEventsNotify = 0;
   pd_board[board].AoutSS.dwEventsStatus = 0;
   pd_board[board].AoutSS.dwEventsNew = 0;

   //-----------------------------------------------------------------------
   // Program AOut subsystem:
   // about "magic" config bits - do it in DLL
   pd_board[board].AoutSS.SubsysConfig = pAOutCfg->dwAInCfg;
   pd_board[board].AoutSS.dwAOutPreTrigCount = pAOutCfg->dwAInPreTrigCount;
   pd_board[board].AoutSS.dwAOutPostTrigCount = pAOutCfg->dwAInPostTrigCount;
   pd_board[board].AoutSS.dwCvRate = pAOutCfg->dwCvRate;
   pd_board[board].AoutSS.dwClRate = pAOutCfg->dwClRate;
   pd_board[board].AoutSS.dwChListChan = pAOutCfg->dwChListSize;

   if (pd_board[board].AoutSS.SubsysConfig & AOB_EXTM) 
   {
      // Switch to normal mode if external memory is used on AO and DIO boards
      pd_board[board].dwXFerMode = XFERMODE_NORMAL;

      switch (pd_board[board].AoutSS.SubsysConfig & (AOB_EXTMSIZE0|AOB_EXTMSIZE1)) 
      {
      case (AOB_EXTMSIZE0):
         pd_board[board].AoutSS.TranSize = 32 * 1024;
         break;
      case (AOB_EXTMSIZE1):
         pd_board[board].AoutSS.TranSize = 16 * 1024;
         break;
      case (AOB_EXTMSIZE0|AOB_EXTMSIZE1):
         pd_board[board].AoutSS.TranSize = 8 * 1024;
         break;
      default:
         pd_board[board].AoutSS.TranSize = pd_board[board].AoutSS.FifoValues;
      }
   } 
   else 
   {
      pd_board[board].AoutSS.TranSize = PD_AOUT_MAX_FIFO_VALUES;
   }

   DPRINTK("pd_aout_async_init: subsysconfig = 0x%x\n", pd_board[board].AoutSS.SubsysConfig);
   DPRINTK("pd_aout_async_init: TranSize = 0x%x\n", pd_board[board].AoutSS.TranSize);

   pd_board[board].AoutSS.BlkXferValues = pd_board[board].AoutSS.TranSize >> 1; // on FHF should
   pd_board[board].AoutSS.XferBufValueCount = 0;
      
   // We need to store channel list to combine it on the fly
   if (pAOutCfg->dwChListSize)
   {
      //memcpy(pd_board[board].AoutSS.ChList, pAOutCfg->dwChList, PD_MAX_CL_SIZE * sizeof(u32));
      for (i = 0; (i < pAOutCfg->dwChListSize)&&(i<PD_MAX_CL_SIZE); i++)
            pd_board[board].AoutSS.ChList[i] = *(pAOutCfg->dwChList + i);
   }
   
   // Check - do we need to use AOB_REGENERATE
   BufVal = pd_board[board].AoutSS.BufInfo.BufSizeInBytes >> (pd_board[board].AoutSS.BufInfo.DataWidth >> 1);

   if ((BufVal <= pd_board[board].AoutSS.TranSize) && (pd_board[board].AoutSS.BufInfo.bRecycle))
   {
      DPRINTK_F("pd_aout_async_init: automatically turning on AOB_REGENERATE!\n");
      pd_board[board].AoutSS.SubsysConfig |= AOB_REGENERATE;
   }

   // Configure AOut subsystem
   if(!pd_aout_set_config(board, pd_board[board].AoutSS.SubsysConfig, pAOutCfg->dwAInPostTrigCount))
   {
      DPRINTK_F("pd_aout_async_init bails!  pd_aout_set_config failed!\n");
      return 0;
   }

   // Set internal CV clock if used.
   if ( (pd_board[board].AoutSS.SubsysConfig & (AOB_CVSTART0 | AOB_CVSTART1)) == AOB_CVSTART0 )
   {
      if(!pd_aout_set_cv_clk(board, pAOutCfg->dwCvRate))
      {
         DPRINTK_F("pd_aout_async_init bails!  pd_aout_set_cv_clk failed!\n");
         return 0;
      }
   }

   // Program DMA if we are in DMA mode
   if ((pd_board[board].AoutSS.BufInfo.bFixedDMA)||(pd_board[board].AoutSS.SubsysConfig & AOB_DMAEN))
   {
      DPRINTK_F("pd_aout_async_init: programming DMA mode\n");

      // check whether channel list size is a power of two
      for (i = 0; i <= 7; i++) 
      {  //(add dk 05.11.2002)
        if (pAOutCfg->dwChListSize == (ULONG)(1<<i))
            break;
      }

      if(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE)
         i = 1;

      if (i < 8)
      {
         if (pAOutCfg->dwAInCfg & AOB_AOUT32)
         {
            offset = get_aout_offs_reg(pAOutCfg->dwChListSize);  // Offset register
            count = get_aout_cnt_reg(pAOutCfg->dwChListSize, 
                        (pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE)?
                         (BufVal):(pd_board[board].AoutSS.TranSize>>1));   // Count register
            dest = get_aout_dest_reg(*(pAOutCfg->dwChList));  // Destination register (stored in the first channel)
         }
         else
         {
            offset = get_dout_offs_reg(pAOutCfg->dwChListSize);  // Offset register
            count = get_dout_cnt_reg(pAOutCfg->dwChListSize, 
                        (pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE)?
                         (BufVal):(pd_board[board].AoutSS.TranSize>>1));   // Count register
            dest = get_dout_dest_reg(*(pAOutCfg->dwChList));  // Destination register (stored in the first channel)
         }
         if (!pd_aout_dmaSet(board, offset, count, dest))
         {
            DPRINTK_F("pd_aout_async_init: cannot set up DMA for AO operation");
            return 0;
         }
      }
      else
      {
         DPRINTK_F("pd_aout_async_init: channel list is not suitable for DMA");
         return 0;
      }
   }

   //-----------------------------------------------------------------------
   // Set AOut user events.
   UserEvents = eAllEvents;
   if (!pd_clear_user_events(board, AnalogOut, UserEvents))
   {
      DPRINTK_F("pd_aout_async_init bails!  pd_clear_user_events failed!\n");
      return 0;
   }

   if (pAOutCfg->dwEventsNotify && (!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE)))
   {
      UserEvents = pAOutCfg->dwEventsNotify;
      if (!pd_set_user_events(board, AnalogOut, UserEvents))
      {
         DPRINTK_F("pd_aout_async_init bails!  pd_set_user_events failed! UserEvents = 0x%08X\n", UserEvents);
         return 0;
      }
   }

   //-----------------------------------------------------------------------
   // Configure AOut firmware event interrupts.
   pd_board[board].FwEventsConfig.AOutIntr = AOB_HalfDoneIm | AOB_HalfDoneSC |
                                             AOB_BufDoneIm | AOB_BufDoneSC;

   if(!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE))
   {
      if (!pd_enable_events(board, &pd_board[board].FwEventsConfig))
      {
         DPRINTK_F("pd_ain_async_init bails!  pd_enable_events failed!\n");
         return 0;
      }
   }

   pd_board[board].AoutSS.SubsysState = ssStandby;

   return 1;
}
//
//
// Function:    pd_ain_async_term
//
// Parameters:  int board
//
// Returns:     1 = SUCCESS
//
// Description: The AIn Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered
//              acquisition.
//
// Notes:
//
int pd_ain_async_term(int board)
{
    tEvents Events={0};

    if (!pd_ain_sw_stop_trigger(board)) return 0;
    if (!pd_ain_set_enable_conversion(board, 0)) return 0;

    Events.AIOIntr = AIB_FHFIm;
    if (!pd_disable_events(board, &Events)) return 0;

    pd_board[board].AinSS.SubsysState = ssConfig;
    pd_board[board].AinSS.bCheckHalfDone = FALSE;
    pd_board[board].AinSS.bCheckFifoError = FALSE;
    pd_board[board].AinSS.bAsyncMode = FALSE;

    return 1;
}

//---------------------------------------------------------------------------
// Function:    pd_aout_async_term
//
// Parameters:  int board
//
// Returns:     1 = SUCCESS
//
// Description: The AOut Terminate Asynchronous Buffered Output function
//              terminates and releases memory allocated for buffered
//              output.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
int pd_aout_async_term(int board)
{
   tEvents Events={0};

   if (!pd_aout_sw_stop_trigger(board)) return 0;
   if (!pd_aout_set_enable_conversion(board, 0)) return 0;

   
   // Configure AIn firmware event interrupts.
   // (Disable event interrupt bits and clear event status bits).
   Events.AOutIntr = AOB_HalfDoneIm | AOB_BufDoneIm;
     
   if (!pd_disable_events(board, &Events)) return 0;

   pd_board[board].AoutSS.SubsysState = ssConfig;

   pd_board[board].AoutSS.bAsyncMode = FALSE;

   pd_board[board].AoutSS.bCheckHalfDone = FALSE;
   pd_board[board].AoutSS.bCheckFifoError = FALSE;

   return 1;
}


//
// Function:    pd_ain_async_start
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//
// Returns:     1 = SUCCESS
//
// Description: The AIn Start Asynchronous Buffered Acquisition function starts
//              buffered acquisition.
//
// Notes:
//
int pd_ain_async_start(int board)
{
   DPRINTK_P("pd_ain_async_start...\n");
   
   if (pd_board[board].AinSS.SubsysState != ssStandby)
   {
      DPRINTK_F("pd_ain_async_start fails!  SubsysState != ssStandby!\n");
      return 0;
   }

   pd_board[board].AinSS.bAsyncMode = TRUE;

   if (!pd_board[board].intMutex)
   {
      if (!pd_adapter_enable_interrupt(board, 1))
      {
         DPRINTK_F("pd_ain_async_start: pd_adapter_enable_interrupt fails");
         return 0;
      }
   }
   else
   {
      DPRINTK_F("pd_ain_async_start: interrupt already enabled");
   }

   pd_board[board].intMutex++;

   if (!pd_ain_set_enable_conversion(board, 1))
   {
      DPRINTK_F("pd_ain_async_start: pd_ain_set_enable_conversion fails");
      return 0;
   }

   if ((pd_board[board].AinSS.dwAInCfg & (AIB_STARTTRIG0 | AIB_STARTTRIG1)) == 0)
      if (!pd_ain_sw_start_trigger(board))
      {
         DPRINTK_F("pd_ain_async_start: pd_ain_sw_start_trigger fails");
         return 0;
      }

   pd_board[board].AinSS.SubsysState = ssRunning;

   return 1;
}


//---------------------------------------------------------------------------
// Function:    pd_aout_async_start
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//
// Returns:     1 SUCCESS
//              
// Description: The AOut Start Asynchronous Buffered Acquisition function starts
//              buffered acquisition.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
int pd_aout_async_start(int board)
{
   u32 Count, BufVal, UserBufCopied;
   PTBuf_Info pDaqBuf = &pd_board[board].AoutSS.BufInfo;
   u32 dwReply;

   //-----------------------------------------------------------------------
   // Check to verify that AOut buffered acquisition was initialized.
   if (pd_board[board].AoutSS.SubsysState != ssStandby)
   {
      DPRINTK_F("pd_aout_async_start fails!  SubsysState != ssStandby!\n");
      return 0;
   }

   // Enable the DSP interrupts.
   if (!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE))     // we don't need interrupts on regenerate
   {
      if (!pd_board[board].intMutex)
      {
         if (!pd_adapter_enable_interrupt(board, 1))
         {
            DPRINTK_F("pd_aout_async_start: pd_adapter_enable_interrupt fails");
            return 0;
         }
      }
      pd_board[board].intMutex++;
   }

   //-------------------------------------------------------------------
   // here we need to give some data for AOut - depends on mode and buffer size
   // we can use one of three methods
   BufVal = pDaqBuf->BufSizeInBytes >> (pDaqBuf->DataWidth >> 1);
   if (BufVal <= pd_board[board].AoutSS.TranSize)
      Count = BufVal;                   // put whatever we have in the buffer
   else
      Count = pd_board[board].AoutSS.TranSize/2;  // put amount fit in the DAC FIFO

   //--------------------------------------------------------------------
   // Output values - head and count will be adjusted automatically
   pd_board[board].AoutSS.bRev3Mode = TRUE;
   if (!pd_aout_put_xbuf(board, Count, &UserBufCopied))
   {
      DPRINTK_F("pd_aout_async_start: cannot pd_aout_put_xbuf");
      return 0;
   }

   //-------------------------------------------------------------------
   // Enable D/A conversions.
   dwReply = pd_dsp_cmd_write_ack(board, PD_AOCVEN, 1);

   //-------------------------------------------------------------------
   // Issue start trigger if it's set to software
   if ( (pd_board[board].AoutSS.SubsysConfig & (AOB_STARTTRIG0 | AOB_STARTTRIG1)) == 0 )
      dwReply = pd_dsp_cmd_ret_ack(board, PD_AOSTARTTRIG);

   //-------------------------------------------------------------------
   pd_board[board].AoutSS.SubsysState = ssRunning;

   return 1;
}


//
// Function:    pd_ain_async_stop
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//
// Returns:     1 = SUCCESS
//
// Description: The AIn Start Asynchronous Buffered Acquisition function stops
//              buffered acquisition.
//
// Notes:
//
int pd_ain_async_stop(int board)
{
   if (pd_board[board].AinSS.SubsysState != ssRunning)
   {
      DPRINTK_F("pd_ain_async_stop: not in ssRunning state\n");
      //return 0;
   }


   if (!pd_ain_sw_stop_trigger(board))
   {
      DPRINTK_F("pd_ain_async_stop: not pd_ain_sw_stop_trigger\n");
      return 0;
   }

   if (!pd_ain_set_enable_conversion(board, 0))
   {
      DPRINTK_F("pd_ain_async_stop: not pd_ain_set_enable_conversion\n");
      return 0;
   }

   if (pd_board[board].intMutex == 1)
   {
      if (!pd_adapter_enable_interrupt(board, 0))
      {
         DPRINTK_F("pd_ain_async_stop: not pd_adapter_enable_interrupt\n");
         return 0;
      }
   }
   
   if (pd_board[board].intMutex > 0)
   {
      pd_board[board].intMutex--;
   }

   pd_board[board].AinSS.SubsysState = ssStopped;
   pd_board[board].AinSS.bAsyncMode = FALSE;

   return 1;
}


//---------------------------------------------------------------------------
// Function:    pd_ain_async_retrieve
//
// Parameters:  int board -- board id
//
// Returns:     int    1 -- STATUS_SUCCESS
//                     0 -- STATUS_UNSUCCESSFUL
//
// Description: This routine retrieves whatever data is available in the buffer
//              If mode is not bus-mastering it GetSamples whatever left in the
//              FIFO by calling PdProcessAInGetSamples()
//              If mode is bus mastering it calls PdAInGetBMCtr() and then
//              PdProcessAInMoveSamples()
//              This routine cannot be called if acquisition continues
//
// Notes:       * This routine must be called with device spinlock held! *
//---------------------------------------------------------------------------
int pd_ain_async_retrieve(int board) 
{
   u32   dwBurstCtr, dwFrameCtr;
   u32   dwPage, dwAmount;

   if ((pd_board[board].dwXFerMode == XFERMODE_BM) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM8WORD))
   {
        // 1. make sure bus master is stopped
        pd_dsp_cmd_write_ack(board, PD_AISETEVNT, 0);

        // 2. get information
        if (!pd_dsp_cmd_ret_ack(board, PD_AICHLIST))
            return 0;

        pd_dsp_write(board, AIBM_GET);
        dwBurstCtr = pd_dsp_read(board); // read burst counter
        dwFrameCtr = pd_dsp_read(board); // read frame counter
        pd_dsp_read_ack(board);

        // 3. retrieve exisiting data and put into into the user buffer
        if (pd_board[board].cp == AI_PAGE0) 
           dwPage = AI_PAGE1; 
        else 
           dwPage = AI_PAGE0;
        dwAmount = (dwBurstCtr << 5) + (dwFrameCtr << 9);
        pd_process_ain_move_samples(board, dwPage, dwAmount);

        // 4. Issue event?

   } 
   else 
   {
        return pd_immediate_update(board);
   }
   
   return 1;
}


//---------------------------------------------------------------------------
// Function:    pd_aout_async_stop
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//
// Returns:     NTSTATUS    -- STATUS_SUCCESS
//                          -- STATUS_UNSUCCESSFUL
//
// Description: The AOut Stop Asynchronous Buffered Acquisition function stops
//              perviously started buffered acquisition.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
int pd_aout_async_stop(int board)
{
   // Check to verify that AIn buffered acquisition was running.
   if (pd_board[board].AoutSS.SubsysState != ssRunning)
   {
      pd_board[board].AoutSS.SubsysState = ssStopped;
      DPRINTK_F("pd_aout_async_stop: not in ssRunning state\n");
      return 0;
   }


   if (!pd_aout_sw_stop_trigger(board))
   {
      DPRINTK_F("pd_aout_async_stop: not pd_aout_sw_stop_trigger\n");
      return 0;
   }

   if (!pd_aout_set_enable_conversion(board, 0))
   {
      DPRINTK_F("pd_aout_async_stop: not pd_aout_set_enable_conversion\n");
      return 0;
   }


   //FIXME: INTERRUPTS SHALL BE CONTROLLED IN ONE PLACE FOR AIN AND AOUT
   // Enable the DSP interrupts.
   if (!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE))
   {
      if (pd_board[board].intMutex == 1)
      {
         if (!pd_adapter_enable_interrupt(board, 0))
         {
            DPRINTK_F("pd_aout_async_stop: not pd_adapter_enable_interrupt\n");
            return 0;
         }
      }
      
      if (pd_board[board].intMutex > 0)
      {
         pd_board[board].intMutex--;
      }
   }

   pd_board[board].AoutSS.bRev3Mode = FALSE;
   pd_board[board].AoutSS.SubsysState = ssStopped;

   return 1;
}


//
// Function:    pd_ain_get_scans
//
// Parameters:  int board
//              PTScan_Info pScanInfo -- get scan info struct
//                  u32 NumScans      -- IN:  number of scans to get
//                  u32 ScanIndex     -- OUT: buffer index of first scan
//                  u32 NumValidScans -- OUT: number of valid scans available
//
// Returns:     1 = SUCCESS
//
// Description: The AIn Get Scans function returns the oldest scan index
//              in the DAQ buffer and releases (recycles) frame(s) of scans
//              that had been obtained previously.
//
// Notes:
//
int pd_ain_get_scans(int board, tScanInfo* pScanInfo)
{
    PTBuf_Info pDaqBuf = &pd_board[board].AinSS.BufInfo;
    u32   HeadScan;
    u32   TailScan;
    u32   MaxScans;
    u32   FrameScans;
    u32   ScanIndex;
    u32   NumFramesToInc;
    u32   NewTail;
    u32   AvailScans = 0;

    DPRINTK_P("pd_ain_get_scans:\n");

    // Get new scan index.
    pScanInfo->ScanIndex = ScanIndex = pDaqBuf->ScanIndex;
    HeadScan = pDaqBuf->Head / pDaqBuf->ScanValues;
    TailScan = pDaqBuf->Tail / pDaqBuf->ScanValues;
    MaxScans = pDaqBuf->MaxValues / pDaqBuf->ScanValues;
    FrameScans = pDaqBuf->FrameValues / pDaqBuf->ScanValues;

    DPRINTK_P("pd_ain_get_scans recalc: ScanIdx=0x%x, HeadScan=0x%x, TailScan=0x%x, MaxScans=0x%x, FrameScans=0x%x\n",
            pScanInfo->ScanIndex,
            HeadScan,
            TailScan,
            MaxScans,
            FrameScans);


    // Get the maximum number of valid scans available up to the end of
    // the buffer.
    if ( ScanIndex == HeadScan )
        AvailScans = 0;
    else if ( ScanIndex < HeadScan )
        AvailScans = HeadScan - ScanIndex;
    else if ( HeadScan < ScanIndex )
        AvailScans = MaxScans - ScanIndex;

    pScanInfo->NumValidScans = (pScanInfo->NumScans < AvailScans)?
                                pScanInfo->NumScans:AvailScans;

    // Update the next scan index, (this value is used the next time this function is called).
    pDaqBuf->ScanIndex = (ScanIndex + pScanInfo->NumValidScans) % MaxScans;

    // Check if we should recycle frames(s).
    if ( pDaqBuf->bWrap || pDaqBuf->bRecycle )
    {
        // Did the scan index cross one or more frame boundries?
        if ( (ScanIndex / FrameScans) > (TailScan / FrameScans) )
        {
            // Increment Tail to the last frame boundry preceeding the new scan index.
            NumFramesToInc = (ScanIndex / FrameScans) - (TailScan / FrameScans);
            NewTail = ((TailScan / FrameScans) + NumFramesToInc) * pDaqBuf->FrameValues;
            pDaqBuf->Count -= NewTail - pDaqBuf->Tail;

            // Check if we wrapped and notify user.
            if ( NewTail >= pDaqBuf->MaxValues )
            {
                pd_board[board].AinSS.dwEventsNew |= eBufferWrapped;
            }
            
            pDaqBuf->Tail = NewTail % pDaqBuf->MaxValues;
            DPRINTK("pd_ain_get_scans: setting tail to 0x%x\n", pDaqBuf->Tail);
        }
        else if ( (ScanIndex / FrameScans) < (TailScan / FrameScans) )
        {
            // Current scan index wrapped and is behind the tail.
            // Wrap tail and set it to 0.
            pDaqBuf->Count -= pDaqBuf->MaxValues - pDaqBuf->Tail;
            pDaqBuf->Tail = 0;
            DPRINTK("pd_ain_get_scans: resetting tail to 0\n");
        }
    }

    //
    DPRINTK_T("pd_ain_get_scans: Tail=0x%x, Head=0x%x, Count=0x%x, ScanIdx=0x%x, NxtScanIdx=0x%x, AvlScans=0x%x, NumScans=0x%x\n",
        	pDaqBuf->Tail,
        	pDaqBuf->Head,
        	pDaqBuf->Count,
        	ScanIndex,
        	pDaqBuf->ScanIndex,
        	AvailScans,
        	pScanInfo->NumScans
    );
    //

    // Check if buffer is empty and notify user.
    if (pDaqBuf->Count == 0)
    {
        DPRINTK("pd_ain_get_scans: Error, Count = 0\n");
        return 0;
    }

    return 1;
}

//---------------------------------------------------------------------------
// Function:    pd_aout_get_scans
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//              PPD_DAQBUF_SCAN_INFO pDaqBufScanInfo -- get scan info struct
//                  ULONG NumScans      -- IN:  number of scans to get
//                  ULONG ScanIndex     -- OUT: buffer index of first scan
//                  ULONG NumValidScans -- OUT: number of valid scans available
//
// Returns:     NTSTATUS    -- STATUS_SUCCESS
//                          -- STATUS_UNSUCCESSFUL
//
// Description: The AOut Get Scans function returns the oldest scan index
//              avilable for user to put new scans and how many scans user
//              can put from this index.
//              Additionally it releases (recycles) frame(s) of scans
//              and move tail of the buffer toward to its head.
//              At the beginning of output buffer is assumed full and Aout
//              can output data until the head reaches the tail. By calling
//              this function user moves the tail - adds data to the buffer.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
int pd_aout_get_scans(int board, tScanInfo* pScanInfo)
{
    PTBuf_Info pDaqBuf = &pd_board[board].AoutSS.BufInfo;
    u32   HeadScan;
    u32   TailScan;
    u32   MaxScans;
    u32   FrameScans;
    u32   ScanIndex;
    u32   NumFramesToInc;
    u32   NewTail;
    u32   AvailScans = 0;
    
    
    //-----------------------------------------------------------------------
    // Get new scan index.
    pScanInfo->ScanIndex = ScanIndex = pDaqBuf->ScanIndex;
    HeadScan = pDaqBuf->Head / pDaqBuf->ScanValues;
    TailScan = pDaqBuf->Tail / pDaqBuf->ScanValues;
    MaxScans = pDaqBuf->MaxValues / pDaqBuf->ScanValues;
    FrameScans = pDaqBuf->FrameValues / pDaqBuf->ScanValues;

    //-----------------------------------------------------------------------
    // Get the maximum number of valid scans available up to the end of
    // the buffer.

    // Case #0: Index == HeadScan, (no new scans available).
    if ( ScanIndex == HeadScan )
        AvailScans = 0;

    // Cases #1 and #2: TailScan <= Index < HeadScan < MaxScans,
    // (valid scans between Index and HeadScan).
    else if ( ScanIndex < HeadScan )
        AvailScans = HeadScan - ScanIndex;

    // Case #3: HeadScan < TailScan <= IndexScan < MaxScans,
    // (HeadScan wrapped, valid scans between Index and MaxScans).
    else if ( HeadScan < ScanIndex )
        AvailScans = MaxScans - ScanIndex;

    pScanInfo->NumValidScans = (pScanInfo->NumScans < AvailScans) ? pScanInfo->NumScans : AvailScans;

    // Update the next scan index, (this value is used the next time this function is called).
    pDaqBuf->ScanIndex = (ScanIndex + pScanInfo->NumValidScans) % MaxScans;

    //-----------------------------------------------------------------------
    // Check if we should recycle frames(s).
    if ( pDaqBuf->bWrap || pDaqBuf->bRecycle )
    {
        if(!pDaqBuf->bRecycle)
        {
           // Did the scan index cross one or more frame boundries?
           if ( (ScanIndex / FrameScans) > (TailScan / FrameScans) )
           {
               // Increment Tail to the last frame boundry preceeding the new
               // scan index.
               NumFramesToInc = (ScanIndex / FrameScans) - (TailScan / FrameScans);
               NewTail = ((TailScan / FrameScans) + NumFramesToInc) * pDaqBuf->FrameValues;
               pDaqBuf->Count -= NewTail - pDaqBuf->Tail;
   
               // Check if we wrapped and notify user.
               if ( NewTail >= pDaqBuf->MaxValues )
               {
                   pd_board[board].AoutSS.dwEventsNew |= eBufferWrapped;
               }
   
               pDaqBuf->Tail = NewTail % pDaqBuf->MaxValues;
           }
           else if ( (ScanIndex / FrameScans) < (TailScan / FrameScans) )
           {
               // Current scan index wrapped and is behind the tail.
               // Wrap tail and set it to 0.
               pDaqBuf->Count -= pDaqBuf->MaxValues - pDaqBuf->Tail;
               pDaqBuf->Tail = 0;
           }
        }
    }

    if (pd_board[board].AoutSS.SubsysState != ssRunning)
    {
        AvailScans = pDaqBuf->MaxValues;
        pScanInfo->NumValidScans = (pScanInfo->NumScans < AvailScans) ? pScanInfo->NumScans : AvailScans;
        pScanInfo->ScanIndex = pDaqBuf->ScanIndex;
    }

    /*
    dprintf(("+> Tail=%x, Head=%x, Count=%x, ScanIdx=%x, NxtScanIdx=%x, AvlScans=%x, NumScans=%x",
            pDaqBuf->Tail, pDaqBuf->Head, pDaqBuf->Count, ScanIndex, pDaqBuf->ScanIndex, AvailScans, pDaqBufScanInfo->NumScans));
    //-----------------------------------------------------------------------
    // Check if buffer is empty and notify user.

    if ( pDaqBuf->Count == 0 )
    {
        // Buffer is empty: notify user application.
        Status = STATUS_UNSUCCESSFUL;
    }
    */

    return 1;
}


//
// Function:    pd_get_buf_status
//
// Parameters:  int board
//              int subsystem -- IN:  subsystem type
//              PTBuf_Info pDaqBuf -- DaqBuf status struct:
//                  u32 dwSubsysState    -- OUT: current subsystem state
//                  u32 dwScanIndex      -- OUT: buffer index of first scan
//                  u32 dwNumValidValues -- OUT: number of valid values available
//                  u32 dwNumValidScans  -- OUT: number of valid scans available
//                  u32 dwNumValidFrames -- OUT: number of valid frames available
//                  u32 dwWrapCount      -- OUT: total num times buffer wrapped
//                  u32 dwFirstTimestamp -- OUT: first sample timestamp
//                  u32 dwLastTimestamp  -- OUT: last sample timestamp
//
// Returns:     1 = SUCCESS
//
// Description: The Get DaqBuf status function obtains the current asynchronous
//              acquisition DAQ buffer status.
//
// Notes:
//
int pd_get_buf_status(int board, int subsystem, PTBuf_Info pDaqBufStatus)
{
   PTBuf_Info pDaqBuf;
   u32 ValidValues = 0;
   u32 ValidScans = 0;
   u32 ValidFrames = 0;

   // AIn
   if ((subsystem == AnalogIn) ||
       (subsystem == DigitalIn) ||
       (subsystem == CounterTimer) ||
        (subsystem == DSPCounter))
   {
      pDaqBuf = &pd_board[board].AinSS.BufInfo;
   }
   else if ((subsystem == AnalogOut) ||
            (subsystem == DigitalOut))
   {
      pDaqBuf = &pd_board[board].AoutSS.BufInfo;
   }
   else
   {
      DPRINTK_T("pd_get_buf_status : Invalid subsystem specified");
      return 0;
   }

   pDaqBufStatus->SubsysState = pd_board[board].AinSS.SubsysState;
   pDaqBufStatus->ScanIndex = pDaqBuf->ScanIndex;
   pDaqBufStatus->WrapCount = pDaqBuf->WrapCount;
   pDaqBufStatus->FirstTimestamp = pDaqBuf->FirstTimestamp;
   pDaqBufStatus->LastTimestamp = pDaqBuf->LastTimestamp;

   //-----------------------------------------------------------------------
   // Get the number of valid scans available up to the end of the buffer.
   // Case #0: Index = Head, no new scans available.
   if ( (pDaqBuf->ScanIndex * pDaqBuf->ScanValues) == pDaqBuf->Head )
   {
      // Do nothing.
   }

   // Case #1 and #2: Tail <= Index < Head < MaxValues.
   else if ( (pDaqBuf->ScanIndex * pDaqBuf->ScanValues) < pDaqBuf->Head )
   {
      ValidValues = pDaqBuf->Head - (pDaqBuf->ScanIndex * pDaqBuf->ScanValues);
      ValidScans = (pDaqBuf->Head / pDaqBuf->ScanValues) - pDaqBuf->ScanIndex;
      ValidFrames = (pDaqBuf->Head / pDaqBuf->FrameValues)
                    - ((pDaqBuf->ScanIndex * pDaqBuf->ScanValues) / pDaqBuf->FrameValues);
   }

   // Case #3: Head <= Tail <= Index < MaxValues.
   else if ( pDaqBuf->Head <= pDaqBuf->Tail )
   {
      ValidValues = pDaqBuf->MaxValues - 1 - (pDaqBuf->ScanIndex * pDaqBuf->ScanValues);
      ValidScans = (pDaqBuf->MaxValues / pDaqBuf->ScanValues) - 1 - pDaqBuf->ScanIndex;
      ValidFrames = (pDaqBuf->MaxValues / pDaqBuf->FrameValues) - 1
                    - ((pDaqBuf->ScanIndex * pDaqBuf->ScanValues) / pDaqBuf->FrameValues);
   }
   else
      DPRINTK_T("pd_get_buf_status : Invalid condition here\n");

   pDaqBufStatus->NumValidValues = ValidValues;
   pDaqBufStatus->NumValidScans = ValidScans;
   pDaqBufStatus->NumValidFrames = ValidFrames;

   return 1;
}

//-----------------------------------------------------------------------
// Function:    BOOLEAN PdAODmaSet(PSYNCHRONIZED_CONTEXT pContext)
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//              ULONG dwCommand  -- command to the DIO256
//              ULONG dwValue   -- DOut value word to output
//
// Returns:     int status  -- FALSE: command failed
//                             TRUE:  command succeeded
//
// Description: The DIO 256 CMD performs specified in dwCommand command
//   and write optional data to register specified via the command
//
// Notes:
//
//-----------------------------------------------------------------------
int pd_aout_dmaSet(int board, u32 offset, u32 count, u32 source)
{
    u32 dwReply;

    PRINTK("pd_aout_dmaSet: offset=0x%x, count=0x%x, dest=0x%x\n", offset, count, source);
    
    // Send write command receive acknoledge
    dwReply = pd_dsp_cmd_ret_ack(board, PD_AODMASET);
    if ( dwReply != 1 )
        return 0;

    // Write command
    pd_dsp_write(board, offset);
    pd_dsp_write(board, count);

    // Write parameter and rcv ack
    dwReply = pd_dsp_write_ack(board, source);

    return (dwReply) ? 1 : 0;
}

//-----------------------------------------------------------------------
// Function:    void pd_aout_put_xbuf()
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//              ULONG NumToCopy -- how many samples (DWORD) to put to transfer
//                                 to the boards
//              ULONG UserBufCopied -- how many samples were output
//
// Returns:     none
//
// Description: This function puts samples from user buffer to XFer buffer
//              depends on mode used and transfers to the board
//
//              Six types of buffer types to handle
//              1. PD2-MF(S) with WORD buffer
//                 We need to combine two left-alignment values into one 24-bit
//                 word dwOut = ((DWORD)wVal1 << 8)|(wVal1 >> 4)
//              2. PD2-MF(S) with DWORD buffer - output directly
//              3. PD2-AO with WORD buffer
//                 Combine value with the channel list
//                 dwOut = (ChList[i%ChListSize]<<16)|wVal
//              4. PD2-AO with DWORD buffer
//                 Output values directly, channel list is incorporated if
//                 ChListSize == 0. If not:
//                 dwOut = (ChList[i%ChListSize]<<16)| (dwVal & 0xFFFF);
//              5. PD2-AO with DWORD buffer and BUF_FIXEDDMA
//                 DMA is programmed. Use direct output.
//              6. PD2-AO with WORD buffer and BUF_FIXEDDMA
//
// Notes:       NumToCopy is the number of values to copy to the XFer buffer.
//              We always put data from the beginning of XFer buffer
//              Number of samples in user buffer shall be greater or equal to
//              NumToCopy, except case 1, where we need to have twice as mach
//              WORDs.
//
//-----------------------------------------------------------------------

#define USEASMPUTBLOCKAO
int pd_aout_put_xbuf(int board, u32 NumToCopy, u32 *UserBufCopied)
{
   u32   Count;                  // num samples in buffer (queue)
   u32   Head;                   // queue head (wrapped buffer)
   u32   Tail;                   // queue tail (wrapped buffer)
   u32   FrameValues;            // num values in frame
   u32   MaxValues;              // max buffer samples
   u32   SamplesA, SamplesB;     // A: from head to the end, B: from the beginning
   u32   i, j, dwVal0, dwVal1, AvlSamples, id;
   u32   SamplesCopied, XFerSize, dwAdj, dwReply, dwWords, dwAllowed;
   u32*  pBuf;
   u32*  pdwUserBuf;
   u16*  pwUserBuf;
   
   // Set parameters.
   Count = pd_board[board].AoutSS.BufInfo.Count;
   Head  = pd_board[board].AoutSS.BufInfo.Head;
   Tail  = pd_board[board].AoutSS.BufInfo.Tail;
   FrameValues = pd_board[board].AoutSS.BufInfo.FrameValues;
   MaxValues = pd_board[board].AoutSS.BufInfo.MaxValues;
   dwAdj = 1;

   pBuf = pd_board[board].AoutSS.pXferBuf;  // buffer for XFer
   pdwUserBuf = (u32 *)pd_board[board].AoutSS.BufInfo.databuf; // buffer contains DWORD values
   pwUserBuf = (u16 *)pd_board[board].AoutSS.BufInfo.databuf; // buffer contains WORD values

   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;


   // You cannot put more data then we have space in DAC FIFO
   if (NumToCopy > pd_board[board].AoutSS.TranSize)
      NumToCopy = pd_board[board].AoutSS.TranSize;

   // make an adjustment in the case where one XFer word is made of
   // two user buffer words
   if ( ((PD_IS_MFX(id)) || PDL_IS_MFX(id)) &&
        (pd_board[board].AoutSS.BufInfo.DataWidth == sizeof(USHORT)) )
      NumToCopy = NumToCopy << 1;     // * 2

   // calculate, how many samples we can copy
   if (Head >= Tail)
   {
      AvlSamples = MaxValues - Head + Tail;
      if (NumToCopy > AvlSamples) 
          NumToCopy = AvlSamples;
      if ((MaxValues - Head) < NumToCopy)
      {
         SamplesA = MaxValues - Head;       // from Head to the end
         SamplesB = NumToCopy - SamplesA;   // from beginning
      }
      else
      {
         SamplesA = NumToCopy;
         SamplesB = 0;
      }

   }
   else
   {
      AvlSamples = Tail - Head;
      if (NumToCopy > AvlSamples) 
          NumToCopy = AvlSamples;
      SamplesA = NumToCopy;
      SamplesB = 0;
   }

   XFerSize = NumToCopy;

   // put data to the XFer buffer from the user buffer
   if (pd_board[board].AoutSS.BufInfo.DataWidth == sizeof(ULONG))
   {
      // DWORD user buffer
      if ( PD_IS_MFX(id) ||PDL_IS_MFX(id))
      {
         // (case 2, PD2-MF(S) with DWORD buffer - output directly)
         //RtlCopyBytes(pBuf, pdwUserBuf+Head, (SamplesA * sizeof(ULONG)) );
         //RtlCopyBytes(pBuf, pdwUserBuf, (SamplesB * sizeof(ULONG)) );
         for (i = 0; i < SamplesA; i++)
         {
            (*(pBuf + i)) = (*(pdwUserBuf + i + Head));
         }
         if (SamplesB)
         {
            j = i;
            for (i = 0; i < SamplesB; i++, j++)
            {
               *(pBuf + j) = *(pdwUserBuf + i);
            }
         }

      }
      else
      {
         if (pd_board[board].AoutSS.dwChListChan)
         {
            // (case 4, PD2-AO with DWORD buffer)
            // combine channels data with channel list entries
            for (i = 0; i < SamplesA; i++)
            {
               *(pBuf + i) = *(pdwUserBuf + i + Head) |
                        pd_board[board].AoutSS.ChList[(i+Head)%pd_board[board].AoutSS.dwChListChan]<<16;
            }
            if (SamplesB)
            {
               j = i; // j - pointer in the XFer buffer
               for (i = 0; i < SamplesB; i++, j++)
               {
                  *(pBuf + j) = *(pdwUserBuf + i) |
                        pd_board[board].AoutSS.ChList[i%pd_board[board].AoutSS.dwChListChan]<<16;
               }
            }
         }
         else
         {

            // (case 5, PD2-AO with DWORD buffer and BUF_FIXEDDMA)
            // do direct copy if buffer filled with DWORDs - if MFS or direct AO
            //RtlCopyBytes(pBuf, pdwUserBuf+Head, (SamplesA * sizeof(ULONG)) );
            memcpy(pBuf, pdwUserBuf+Head, (SamplesA * sizeof(u32)));
            if (SamplesB)
               memcpy(pBuf, pdwUserBuf, (SamplesB * sizeof(u32)) );
         }
      }

   }
   else
   {

      // WORD user buffer

      if (PD_IS_MFX(id) ||PDL_IS_MFX(id))
      {
         // (case 1, PD2-MF(S) with WORD buffer)
         // combine two WORDs from user buffer into DWORD in XFer buffer for MF(S)
         for (i = 0; i < SamplesA; i+=2)
         {
            dwVal0 = *(pwUserBuf + i + Head);
            dwVal1 = *(pwUserBuf + i + 1 + Head);
            *(pBuf + (i>>1)) = (dwVal0 >> 4) | (dwVal1 << 8);
         }
         if (SamplesB)
         {
            j = i;
            for (i = 0; i < SamplesB; i+=2, j+=2)
            {
               dwVal0 = *(pwUserBuf + i);
               dwVal1 = *(pwUserBuf + i + 1);
               *(pBuf + (j>>1)) = (dwVal0 >> 4) | (dwVal1 << 8);
            }
         }
         dwAdj = 2;
         XFerSize = NumToCopy/dwAdj;

      }
      else
      {
         if(pd_board[board].AoutSS.dwChListChan)
         //if (!((pd_board[board].AoutSS.BufInfo.bFixedDMA)||(pd_board[board].AoutSS.SubsysConfig & AOB_DMAEN)))
         {
            //(case 3, PD2-AO with WORD buffer)
            // complement WORD value with CL entry
            for (i = 0; i < SamplesA; i++)
            {
               dwVal0 = *(pwUserBuf + i + Head);
               *(pBuf + i) = dwVal0 |
                             pd_board[board].AoutSS.ChList[(i+Head)%pd_board[board].AoutSS.dwChListChan]<<16;
            }
            if (SamplesB)
            {
               j = i;
               for (i = 0; i < SamplesB; i++)
               {
                  dwVal0 = *(pwUserBuf + i);
                  *(pBuf + j) = dwVal0 |
                                pd_board[board].AoutSS.ChList[i%pd_board[board].AoutSS.dwChListChan]<<16;
               }
            }
         }
         else
         {
            // (case 6, PD2-AO with WORD buffer and BUF_FIXEDDMA)
            for (i = 0; i < SamplesA; i++)
            {
               dwVal0 = *(pwUserBuf + i + Head);
               *(pBuf + i) = (USHORT)dwVal0;
            }
            if (SamplesB)
            {
               j = i;
               for (i = 0; i < SamplesB; i++, j++)
               {
                  dwVal0 = *(pwUserBuf + i);
                  *(pBuf + j) = (USHORT)dwVal0;
               }
            }
         }
      }
   }

   // Copy data from the XFer buffer to the board
   // now we have samples in the transfer buffer and need to get them out
   // Issue PD_AOPUTBLOCK command and read remaining buffer size.

   dwAllowed = pd_dsp_cmd_ret_value(board, PD_AOPUTBLOCK);
   dwWords = (dwAllowed < XFerSize)? dwAllowed : XFerSize;

   // Write number of words in block to be written.
   pd_dsp_write(board, dwWords);

   // Write block of dwWords to transmit register.
   if (pd_board[board].dwXFerMode != XFERMODE_NORMAL)
   {
      u32*  pulSrc = (u32*)pBuf;
      u32*  pulDest = (u32*)(pd_board[board].address + PCI_HTXR);
      u32   ulCnt   = dwWords & 0xFFF;

      asm ("rep movsl;"
		:	/* no output */	
		:"S"(pulSrc), "D"(pulDest), "c"(ulCnt)		/* input */
		);	/* clobbered register */
      /*__asm   cld
      __asm   mov ecx,ulCnt
      __asm   mov esi,pulSrc
      __asm   mov edi,pulDest
      __asm   rep movsd */
   }
   else
   {
      for ( i = 0; i < dwWords; i++ )
      {
         //PdAdapterWrite(pAdapter, *(pBuf + i));
         //dwReply = *(pBuf + i); //0xFFFFFF * (i%2);
         pd_dsp_write(board, *(pBuf + i));
      }
   }

   SamplesCopied = dwWords;
   dwReply = pd_dsp_read_ack(board);

   // Adjust head and tail (if recycled)
   if (pd_board[board].AoutSS.BufInfo.bRecycle)
   {
      Tail = Head;
      Head += SamplesCopied * dwAdj;
      Count += SamplesCopied * dwAdj;
   }
   else
   {
      Head += SamplesCopied * dwAdj;
      Count += SamplesCopied * dwAdj;
   }
   if (Head >= MaxValues) Head -= MaxValues;  // wrap around

   // Store new head (and, possibly, tail)
   pd_board[board].AoutSS.BufInfo.Count = Count;
   pd_board[board].AoutSS.BufInfo.Head = Head;
   pd_board[board].AoutSS.BufInfo.Tail = Tail;

   //dprintf(("0x%x (0x%x+0x%x) samples gone Count=0x%x Head=0x%x Tail=0x%x",
   //         SamplesCopied, SamplesA, SamplesB, Count, Head, Tail));

   // return number of samples actually copied
   *UserBufCopied = SamplesCopied;

   return dwReply;
}



