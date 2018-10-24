//===========================================================================
//
// NAME:    pdl_ain.c: 
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to Analog Input FW functions
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



////////////////////////////////////////////////////////////////////////
//   
// ANALOG INPUT SECTION
//
////////////////////////////////////////////////////////////////////////
//
//       NAME:  pd_ain_set_config
//
//   FUNCTION:  Sets the configuration of the analog input subsystem.
//
//  ARGUMENTS:  int board                  -- The board to configure.
//              u32 pd_ain_config             -- the ain configuration word
//              u32 pd_ain_BlkXfer            -- AIn FHF DMA Block Xfer Size-1
//              u32 pd_ain_pre_trigger_count  -- AIn Pre-trigger Scan Count
//              u32 pd_ain_post_trigger_count -- AIn Post-trigger Scan Count
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_set_config(int board, u32 pd_ain_config, u32 pd_ain_pre_trigger_count,
                      u32 pd_ain_post_trigger_count) 
{
   u32 dwDataOffset, i, dwCalDACValue;
   u16  t0, t1;
   u32 ModeNum;


   DPRINTK_P("setting ain config to 0x%08X on board %d\n", pd_ain_config, board);

   pd_board[board].AinSS.bAsyncMode = FALSE;

   // Does this adapter support autocalibration?
   if ( pd_board[board].PCI_Config.SubsystemID & ADAPTER_AUTOCAL )
   {
       // Compare modes in configuration -- if mode changed --> load calibration:
       if ((pd_board[board].AinSS.SubsysConfig & (AIB_MODEBITS)) != 
           (pd_ain_config & (AIB_MODEBITS)))
       {
          ModeNum = pd_ain_config & (AIB_MODEBITS);
          ModeNum = ModeNum >> 1;

          // Find offset for current mode in bytes.
          dwDataOffset = ModeNum * 4;

          // Program CALDACs.
          for (i = 0; i < 4; i++)
          {
             dwCalDACValue = pd_board[board].Eeprom.u.Header.CalibrArea[dwDataOffset + i];
             t0 = (u16)(((i*2)<<8)|(dwCalDACValue & 0xFF));
             pd_dsp_cmd_write_ack(board, PD_CALDACWRITE, t0);
             t1 = (u16)(((i*2+1)<<8)|((dwCalDACValue & 0xFF00)>>8));
             pd_dsp_cmd_write_ack(board, PD_CALDACWRITE, t1);
          }
       }
   }

   // check for FW mode (depends of board type and logic used)
   if ((pd_board[board].Eeprom.u.Header.FWModeSelect & 0xFFFF) & PD_AIB_SELMODE0)
   {
      DPRINTK("pd_ain_set_config: FWModeSelect=0x%x, setting AIB_SELMODE0\n", pd_board[board].Eeprom.u.Header.FWModeSelect);
      pd_ain_config |= AIB_SELMODE0;
   }

   // Store current configuration.
   pd_board[board].AinSS.SubsysConfig = pd_ain_config;
   DPRINTK_P("pd_ain_config = 0x%x\n", pd_ain_config);      

   pd_dsp_command(board, PD_AICFG);     
   if (pd_dsp_read(board) != 1) 
   {
      DPRINTK_F("set ain config FAILS on board %d\n", board);
      return 0;
   }

   pd_dsp_write(board, pd_ain_config);
   pd_dsp_write(board, pd_ain_pre_trigger_count);
   pd_dsp_write(board, pd_ain_post_trigger_count);

   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_ain_set_cv_clock
//
//   FUNCTION:  Sets the analog input conversion start (pacer) clock.
//              Configures the DSP Timer to generate a clock signal using
//              the specified divider from either a 11.0 MHz or 33.0 MHz
//              base clock frequency.
//
//  ARGUMENTS:  int board          -- The board in question.
//              u32 clock_divisor  -- conversion start clock divider
//
//    RETURNS:  1 if it worked, 0 if it didn't.
//
int pd_ain_set_cv_clock(int board, u32 clock_divisor) 
{
   pd_dsp_command(board, PD_AICVCLK);  
   if (pd_dsp_read(board) != 1) 
   {
      return 0;
   }
   pd_dsp_write(board, clock_divisor);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_ain_set_cl_clock
//
//   FUNCTION:  Sets the analog input channel list start (burst) clock.
//              Configures the DSP Timer to generate a clock signal using
//              the specified divider from either an 11.0 MHz or a 33 MHz
//              base clock frequency.
//
//  ARGUMENTS:  int board          -- The board in question.
//              u32 clock_divisor  -- The new clock divisor.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_set_cl_clock(int board, u32 clock_divisor) 
{
   pd_dsp_command(board, PD_AICLCLK); 
   if (pd_dsp_read(board) != 1) 
   {
      return 0;
   }
   pd_dsp_write(board, clock_divisor);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}




//
//       NAME:  pd_ain_set_channel_list
//
//   FUNCTION:  Sets the ADC Channel/Gain List.  This overwrites and
//              completely replaces any pre-existing channel list.
//              The channel list can contain from 1 to 256 channel entries.
//              The configuration data word for each entry includes the
//              channel mux selection, gain, and slow bit setting.
//
//  ARGUMENTS:  int board        -- The board in question.
//              u32 num_entried  -- Number of channels in list.
//              u32 list[]       -- The list.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_set_channel_list(int board, u32 num_entries, u32 list[]) 
{
   int i;
   u32 dwGainCfg;
   unsigned long id;

   // channel list is located in the input buffer of ioctl()
   // that's why
   DPRINTK_P("setting ain channel list (%d entries) on board %d\n", num_entries, board);

   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;


   // Check is board MFS - set up gains
   if ( PD_IS_MFS(id) )
   {
       dwGainCfg = 0;
       for (i = 0; i < num_entries; i++)
       {
           if (((*list) & 0x3F) <= 8)
           {
               dwGainCfg |= ((*(list+i) >> 6) & 3) << ((*(list+i) & 0x7) << 1);
           }
       }
       pd_dsp_cmd_write_ack(board, PD_AISETSSHGAIN, dwGainCfg);
   }

   pd_dsp_command(board, PD_AICHLIST); 
   if (pd_dsp_read(board) != 1) 
   { 
      DPRINTK_F("setting ain channel list FAILS on board %d\n", board);
      return 0;
   }

   pd_dsp_write(board, num_entries);
   for (i = 0; i < num_entries; i++) 
   {
      pd_dsp_write(board, list[i]);
   }

   return (pd_dsp_read(board) == 1) ? 1 : 0;
}


//-----------------------------------------------------------------------
// Function:    BOOLEAN PdAInSetBMCtr(PSYNCHRONIZED_CONTEXT pContext)
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//              DWORD dwCh          -- number of channels in list
//              DWORD *pdwChList    -- bus master addresses and parameters
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Sets up parameters for bus mastering operation.
//              Parameters are:
//              Context+1 = page 0 physical address shifted 8 bits to the right
//              Context+2 = page 1 physical address shifted 8 bits to the right
//              Context+3 = size of 1/2 FIFO measured in transfer sizes AIBM_TXSIZE = 512
//              Context+4 = size of each contigous physical pages in AIBM_TXSIZE
//
//-----------------------------------------------------------------------
int pd_ain_set_BM_ctr(int board, u32 dwCh, u32* pdwChList)
{
    u32   i;
    
    // Issue PD_AICHLIST command and check ack.
    if ( !pd_dsp_cmd_ret_ack(board, PD_AICHLIST) )
        return 0;

    // Write number of Channel List entries.
    pd_dsp_write(board, AIBM_SET);

    // Write block of dwWords to transmit register.
    for ( i = 0; i < 4; i++ )
        pd_dsp_write(board, *(pdwChList + i));

    return pd_dsp_read_ack(board);
}


//-----------------------------------------------------------------------
// Function:    BOOLEAN PdAInGetBMCtr(PSYNCHRONIZED_CONTEXT pContext)
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//              DWORD dwCh          -- number of channels in list
//              DWORD *pdwChList    -- channel list data buffer
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Retrieve bus master counters (for the current page)
//              In Context+0 is burst conter and Context+1 is frame counter
//              Both of them are DWORDs
//
//-----------------------------------------------------------------------
int pd_ain_get_BM_ctr(int board, u32 dwCh, u32* pBurstCtr, u32* pFrameCtr)
{
    // Issue PD_AICHLIST command and check ack.
    if (!pd_dsp_cmd_ret_ack(board, PD_AICHLIST))
        return 0;

    // Write number of Channel List entries.
    pd_dsp_write(board, AIBM_GET);

    // Read burst and frame counters
    *pBurstCtr = pd_dsp_read(board); // read burst counter
    *pFrameCtr = pd_dsp_read(board); // read frame counter

    return pd_dsp_read_ack(board);
}


// Function:    pd_ain_set_events()
//
// Parameters:  DWORD dwEvents  -- AInIntrStat Event configuration word
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Set selected AIn AInIntrStat event bits enabling/disabling
//              and/or clearing individual firmware level events, thereby
//              re-enabling the event interrupts.
//
//              AInIntrStat Bit Settings:
//
//                  AIB_xxxxIm bits:    0 = disable, 1 = enable interrupt
//                  AIB_xxxxSC bits:    0 = clear interrupt, 1 = no change
//
// Notes:       See pdfw_def.h for the AInIntrStat event word format.
//
int pd_ain_set_events (int board, u32 dwEvents)
{
    return pd_dsp_cmd_write_ack(board, PD_AISETEVNT, dwEvents);
}


//
//       NAME:  pd_ain_get_status
//
//   FUNCTION:  Reads the analog in Interrupt/Status registers.
//
//  ARGUMENTS:  The board to read from.
//
//    RETURNS:  The contents of the registers.
//
int pd_ain_get_status(int board, u32* status) 
{
   pd_dsp_command(board, PD_AISTATUS);
   *status = pd_dsp_read(board);
   return TRUE;
}

//
//       NAME:  pd_ain_sw_start_trigger()
//
//   FUNCTION:  Triggers the AIn Start event to start sample aquisition.
//
//  ARGUMENTS:  The board to start.
//    
//    RETURNS:  1 if it worked and 0 if it failed
//
int pd_ain_sw_start_trigger(int board) 
{
   DPRINTK_P("ain sw start trigger on board %d\n", board);
   pd_dsp_command(board, PD_AISTARTTRIG); 
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_ain_sw_stop_trigger
//
//   FUNCTION:  Triggers the AIn Stop event to stop sample aquisition.
//
//  ARGUMENTS:  The board in question.
//
//    RETURNS:  1 if it worked, and 0 if not.
//
int pd_ain_sw_stop_trigger(int board) 
{
   DPRINTK_P("ain sw stop trigger on board %d\n", board);
   pd_dsp_command(board, PD_AISTOPTRIG);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//    
//       NAME:  pd_ain_set_enable_conversion
//
//   FUNCTION:  Enables or disables ADC conversions.  This function is
//              irrespective of the AIn Conversion Start and AIn Channel
//              List Start signals.  During configuration and following 
//              an error condition, the analog input process is disabled
//              and must be re-enabled to perform subsequent conversions.
//              This command permits completing AIn configuration before
//              the subsystem responds to the Start trigger.
//
//              PD_AINCVEN <== 0:   AIn subsystem Start Trigger is disabled
//                                  and ignored. Conversion in progress
//                                  will not be interrupted but the start
//                                  trigger is disabled from retriggering
//                                  the subsystem again.
//
//              PD_AINCVEN <== 1:   AIn subsystem Start Trigger is enabled
//                                  and data acquisition will start on the
//                                  first valid AIn start trigger.
//
//  ARGUMENTS:  The board in questions, and a boolean - 0 to disable ADC
//              conversions, 1 to enable.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_set_enable_conversion(int board, int enable) 
{
   DPRINTK_P("setting ain conversion to %d on board %d\n", enable, board);
   pd_dsp_command(board, PD_AICVEN);
   if (pd_dsp_read(board) != 1) 
   {
      return 0;
   }
   pd_dsp_write(board, (u32)enable);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

// Function:    pd_ain_get_value()
//
// Parameters:  board
//              USHORT *pwSample
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: The AIn Get Single Value command reads a single value
//              from the ADC FIFO.
//
// Notes:
//
int pd_ain_get_value(int board, u16* value)
{
    u32 dwReply;

    // Issue PD_AIGETVALUE command and get return value.
    dwReply = pd_dsp_cmd_ret_value(board, PD_AIGETVALUE);
    if ( dwReply == ERR_RET )
    {
        *value = 0;
        return 0;
    }
    *value = dwReply;
    return 1;
}

//
// Function:    pd_ain_set_ssh_gain()
//
// Parameters:  board
//              ULONG dwCfg -- SSH PGA configuration
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Sets SSH PGA configuration
//
// Notes: PD2-MFS-xDG boards only
//
int pd_ain_set_ssh_gain(int board, u32 dwCfg)
{
    // Issue PD_AISETSSHGAIN with param and check acknowledge.
    return pd_dsp_cmd_write_ack(board, PD_AISETSSHGAIN, dwCfg);
}


//
//       NAME:  pd_ain_get_samples
//
//   FUNCTION:  Get any available samples from the ADC FIFO.
//
//  ARGUMENTS:  int board        -- The board in question.
//              int max_samples  -- Read at most this many samples.
//              u16 buffer[]     -- Put the samples here.
//
//    RETURNS:  The number of samples actually read.
//
//       NOTE:  Last value read must always by ERR_RET.
//
int pd_ain_get_samples(int board, int max_samples, u16* buffer, u32* samples) 
{
   int i;
   int nSamples;
   u32 dwSample = 0;
   unsigned long id;

   DPRINTK_P("getting ain %d samples from board %d\n", max_samples, board);

   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;

   if (pd_board[board].AinSS.bAsyncMode && PD_IS_LABMF(id))
      return TRUE;


   //pd_dsp_command(board, PD_AIGETSAMPLES);
   if (pd_dsp_cmd_ret_ack(board, PD_AIGETSAMPLES) != 1) 
   {
      DPRINTK_F("getting ain samples FAILS from board %d\n", board);
      return FALSE;
   }

   pd_dsp_write(board, (u32)max_samples);
   nSamples = 0;
   for (i = 0; i < max_samples + 1; i++) 
   {
      if(pd_board[board].dwXFerMode == XFERMODE_NORMAL) 
         dwSample = pd_dsp_read(board);
      else
         dwSample = pd_readl(pd_board[board].address + PCI_HRXS);

      if (dwSample == ERR_RET) 
      {
         DPRINTK_F("end of samples (%d)\n", nSamples);
         break;
      }

      if (i < max_samples) 
      {
        *(buffer+nSamples++) = (u16)dwSample;
      }
   }

   if (dwSample != ERR_RET) {
      DPRINTK_F("ERROR! no end of buffer in pd_ain_get_samples!\n");
      return FALSE;
   }
   *samples = nSamples;
   return TRUE;
}


// special version of get samples for ISR
int pd_ain_get_samples_ulong(int board, u32 dwMaxSamples, u32* pdwBuf, u32* pdwNumSamples)
{
   u32   dwCount;
   u32   dwSample = 0;
   u32   id;

   DPRINTK_N("GetSamplesULONG - internal\n");

   *pdwNumSamples = 0;

   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;

   if (pd_board[board].AinSS.bAsyncMode && PD_IS_LABMF(id))
       return 1;

    // Issue PD_AIGETSAMPLES command and check ack.
    if ( !pd_dsp_cmd_ret_ack(board, PD_AIGETSAMPLES))
        return FALSE;

    // Write max number of sample words that can be read.
    pd_dsp_write(board, dwMaxSamples);

    // Read block of dwMaxBufSize+1 from receive register until ERR_RET.
    for (dwCount = 0; dwCount < dwMaxSamples+1; dwCount++, pdwBuf++)
    {
        dwSample = pd_dsp_read(board);
        if (dwSample == ERR_RET)
            break;

         if (dwCount < dwMaxSamples)
            *pdwBuf = dwSample;
    }

    // Fatal error condition here!  FIX THIS HERE BORIS --> NEED TO RESET DRIVER???
    // (This condition should never occur unless there is a bug in the firmware/driver.)
    if (dwSample != ERR_RET)
    {
        DPRINTK_N("Error: No end of buffer in PdAInGetSamples.\n");
        return 0;
    }

    // Return # of valid samples read.
    *pdwNumSamples = dwCount;

    return 1;
}


//
//       NAME:  pd_ain_reset
//
//   FUNCTION:  Resets the ADC subsystem to the default startup
//              configuration.  All operations in progress are stopped and
//              all configurations and buffers are cleared.  All transfers
//              in  progress are interrupted and masked.
//   
//  ARGUMENTS:  The board on which to reset the ADC logic.
//
//    RETURNS:  1 if it worked and 0 if it failed.
//
int pd_ain_reset(int board)
{
   DPRINTK_P("ain reset on board %d\n", board);

   pd_board[board].AinSS.bAsyncMode = FALSE;

   pd_dsp_command(board, PD_AIRESET); 
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_ain_sw_cl_start()
//
//   FUNCTION:  Triggers the ADC Channel List Start signal.  This starts a
//              scan.
//
//  ARGUMENTS:  The board to trigger.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_sw_cl_start(int board) 
{
   DPRINTK_P("sending sw cl start signal to board %d\n", board);

   pd_dsp_command(board, PD_AISWCLSTART); 
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_ain_sw_cv_start()
//
//   FUNCTION:  Triggers the ADC Conversion Start signal.  This starts a
//              scan.
//
//  ARGUMENTS:  The board to trigger.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_sw_cv_start(int board) 
{
   DPRINTK_P("sending sw cl start signal to board %d\n", board);

   pd_dsp_command(board, PD_AISWCVSTART); 
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}


//
//       NAME:  pd_ain_reset_cl()
//
//   FUNCTION:  Resets the ADC Channel List to the first channel in the list.
//              This command is similar to the SW Channel List Start, but
//              does not enable the list for conversions.
//
//  ARGUMENTS:  The board to reset.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_ain_reset_cl(int board) 
{
   pd_dsp_command(board, PD_AICLRESET); 
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

// Function:    pd_ain_clear_data()
//
// Parameters:  board
//
// RETURNS:     1 if it worked, 0 if it failed.
//
// Description: The clear all AIn data command clears the ADC FIFO and
//              all AIn data storage buffers.
//
// Notes:
//
int pd_ain_clear_data(int board)
{
    // Issue PD_AICLRDATA command and check ack.
    return pd_dsp_cmd_ret_ack(board, PD_AICLRDATA);
}


//
// Function:    PdAInFlushFifo
//
// Parameters:  PADAPTER_INFO pAdapter -- pointer to device extension
//
//
// Description: The Flush AIn Sample FIFO function gets all samples from the
//              AIn sample FIFO as quickly as possible and stores them in
//              a temporary buffer stored in device extention.
//
//              This function is called from the ISR or a DPC to process the
//              AIn FHF hardware interrupt event by transferring data from
//              board to the temporary buffer.
//
//              Two types of reads from the board are performed, first
//              a Block Transfer is used to read a fixed size block from
//              the DSP / AIn FIFO using the fastest PCI Slave mode
//              possible via DSP DMA. Following the block transfer, a
//              PD_AIGETSAMPLES is performed to read all remaining samples.
//
// Notes:       The board status must indicate FHF before this function is
//              called otherwise data read could be invalid.
//
//              Each sample is stored in 16 bits.
//
//
//

int pd_ain_flush_fifo(int board, int from_isr)
{
    u32   dwMaxSamples;
    u16*  pwBuf = (u16*)pd_board[board].AinSS.pXferBuf;
    u32   dwCount;
    u32   dwSample = ERR_RET;
    u32   i;
    u32   total, dwCnt, dwXfr;
    u32   id;
    
    dwMaxSamples = pd_board[board].AinSS.XferBufValues;             //0x400

    DPRINTK_P("pd_ain_flush_fifo: XferBufValues(1): 0x%x BlkXferValues: 0x%x XferBufValueCount: 0x%x FifoXFerCycles: 0x%x\n", 
            pd_board[board].AinSS.XferBufValues,
            pd_board[board].AinSS.BlkXferValues,
            pd_board[board].AinSS.XferBufValueCount,
            pd_board[board].AinSS.FifoXFerCycles);

    if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
       id = pd_board[board].PCI_Config.SubsystemID - 0x100;
    else
       id = pd_board[board].PCI_Config.SubsystemID;

    dwCnt = pd_board[board].AinSS.BlkXferValues;
    pd_board[board].AinSS.XferBufValueCount = 0;
    dwCount = total = 0;
    dwXfr = pd_board[board].AinSS.FifoXFerCycles;
   
    for ( i = 0; i < dwXfr; i++)
    {
      // Issue PD_AIN_BLK_XFER command w/o return value.
      pd_dsp_cmd_no_ret(board, PD_AIN_BLK_XFER);

      if (pd_board[board].dwXFerMode == XFERMODE_NORMAL) 
      {
  
         // Read fixed size block of samples from receive register.
         for (dwCount = 0; dwCount < dwCnt; dwCount++, total++)
         {
            *(pwBuf++) = (u16)pd_dsp_read_x(board);
         }
      }
      else
      {
         // Read fixed size block of samples from receive register.
         for (dwCount = 0; dwCount < dwCnt; dwCount++, total++ ) 
         {
            *(pwBuf++) = (u16)pd_readl(pd_board[board].address + PCI_HRXS);
         }
      }
    }
    dwCount = total;
    
    DPRINTK_T("pd_ain_flush_fifo: xfer 0x%x values\n", dwCount);
    pd_board[board].AinSS.XferBufValueCount = dwCount;

    if (!from_isr && !PD_IS_LABMF(id) && (pd_board[board].AinSS.FifoXFerCycles < 8)
        && (pd_board[board].AinSS.DoGetSamples)) 
    {
       // Issue PD_AIGETSAMPLES command and check ack.
       if ( !pd_dsp_cmd_ret_ack(board, PD_AIGETSAMPLES) )
       {
           DPRINTK_F("Error: cannot execute PD_AIGETSAMPLES in pd_ain_flush_fifo\n");
           return FALSE;
       }
   
       // Write max number of sample words that can be read.
       pd_dsp_write(board, (dwMaxSamples-dwCount)); 
   
       // Read block of dwMaxBufSize+1 from receive register until ERR_RET.
       for ( ; dwCount < (dwMaxSamples+1); dwCount++ )
       {
          if(pd_board[board].dwXFerMode == XFERMODE_NORMAL) 
             dwSample = pd_dsp_read_x(board);
          else
             dwSample = pd_readl(pd_board[board].address + PCI_HRXS);
           
          if (dwSample == ERR_RET)
             break;
   
          if (dwCount < dwMaxSamples)
          {
             *(pwBuf++) = (USHORT)dwSample;
          }          
       }

       if (dwSample != ERR_RET)
       {
          if ((pd_board[board].dwXFerMode == XFERMODE_NORMAL)||
              (pd_board[board].dwXFerMode == XFERMODE_FAST))
          {
             DPRINTK_N("Error: No end of buffer in PdFlushAInFifo.\n");
             return 0;
          }
       }
    }

    // set # of valid samples read.

    DPRINTK_T("pd_ain_flush_fifo: x-count 0x%x values\n", 
            dwCount - pd_board[board].AinSS.XferBufValueCount);

    pd_board[board].AinSS.XferBufValueCount = dwCount;

    DPRINTK_T("pd_ain_flush_fifo: BlkXferValues(2): 0x%x BlkXferValueCount: 0x%x\n", 
            pd_board[board].AinSS.BlkXferValues,
            pd_board[board].AinSS.XferBufValueCount);

    return TRUE;
}

//
//       NAME:  pd_ain_get_xfer_samples
//
//   FUNCTION:  Get samples from the ADC FIFO using DMA
//
//  ARGUMENTS:  int board        -- The board in question.
//              u16 buffer[]     -- Put the samples here.
//
//    RETURNS:  The number of samples actually read.
//
//       NOTE:  FW 00218 supports this function!!!
//
int pd_ain_get_xfer_samples(int board, uint16_t* buffer) 
{
    int nSamples = pd_board[board].AinSS.BlkXferValues;
    int i;

    // Read fixed size block of samples from receive register.
    pd_dsp_cmd_no_ret(board, PD_AIN_BLK_XFER);
   
    DPRINTK_P("AIN_BLKXFER_16: nSamples = %d\n", nSamples);    
    for (i = 0; i < nSamples; i++ )  // 16-bit XFer
    {                                            
       *((uint16_t*)buffer++) = (u16)pd_dsp_read_x(board);
    }

    return nSamples;
}

//
//       NAME:  pd_ain_set_xfer_size
//
//   FUNCTION:  Set samples size to transfer from ADC FIFO using DMA
//
//  ARGUMENTS:  int board        -- The board in question.
//              u32              -- Size
//
//    RETURNS:  The number of samples actually read.
//
//       NOTE:  FW 00222 supports this function!!!
//
int pd_ain_set_xfer_size(int board, u32 size) 
{
    // store new DSP DMA transfer size - should be even
    pd_board[board].AinSS.BlkXferValues = size;
    
    // Issue PD_AIXFERSIZE command and check ack.
    return pd_dsp_cmd_write_ack(board, PD_AIXFERSIZE, size-1);
}




