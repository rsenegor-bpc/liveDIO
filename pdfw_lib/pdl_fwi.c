//===========================================================================
//
// NAME:    pdl_fwi.c: 
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to basic r/w FW functions
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

//////////////////////////////////////////////////////////////////////
//
// FIRMWARE INTERFACE
//
//////////////////////////////////////////////////////////////////////

// 
//       name:  pd_dsp_get_status
//
//   function:  Reads the status of the PD's DSP.  Duh.
//
//  arguments:  The board (index) to read from.
//  
//    returns:  The status.
//
inline u32 pd_dsp_get_status(int board) 
{
   return pd_readl(pd_board[board].address + PCI_HSTR);
}

// 
//       name:  pd_dsp_int_status
//
//   function:  Reads the status of DSP HSTR. Returns true if
//              (1<<HSTR_HINT)
//
//
inline u32 pd_dsp_int_status(int board) 
{
   u32 val;

   // Get DSP PCI interrupt status and check if INTA was asserted.
   val = pd_dsp_get_status(board);
    
   return (val & (1<<HSTR_HINT));
}


// 
//       name:  pd_dsp_get_flags()
//
//   function:  Reads the flags of the PD's DSP.  Duh.
//
//  arguments:  The board (index) to read from.
//  
//    returns:  The flags.
//
inline u32 pd_dsp_get_flags(int board) {
   return (pd_dsp_get_status(board) >> HSTR_HF) & 0x7;
}

// 
//       name:  pd_dsp_set_flags()
//
//   function:  Sets the flags of the PD's DSP.
//
//  arguments:  The board (index) to set DSP flags on, and the new flags.
//  
//    returns:  Nothing.
//
inline void pd_dsp_set_flags(int board, u32 new_flags) {
   // clear flags
   pd_board[board].HI32CtrlCfg &= ~(u32)(0x07<<HCTR_HF);

   // set flags
   pd_board[board].HI32CtrlCfg |= (u32)((new_flags & 0x07)<<HCTR_HF);
   // DPRINTK_T("setting flags to 0x%08lX (reg=0x%08lX)\n", (unsigned long)new_flags, (unsigned long)pd_board[board].HI32CtrlCfg);

   pd_writel(pd_board[board].HI32CtrlCfg, (pd_board[board].address + PCI_HCTR));
}

// 
//       name:  pd_dsp_command(), pd_dsp_cmd_no_ret()
//
//   function:  Sends a command to the DSP.
//
//  arguments:  The board (index) of the DSP to send the command to.
//  
//    returns:  Nothing.
//
inline void pd_dsp_command(int board, int command) {
   // DPRINTK_T("sending command 0x%08X to board %d\n", command, board);
   pd_writel((1L | command), (pd_board[board].address + PCI_HCVR));
}

inline void pd_dsp_cmd_no_ret(int board, u16 command) 
{
   pd_writel((1L | command), (pd_board[board].address + PCI_HCVR));
}

// 
//       name:  pd_dsp_write()
//
//   function:  Write data to the PowerDAQ transmit data register.
//
//  arguments:  The board (index) to write to, and the data to write.
//  
//    returns:  Nothing.
//
inline void pd_dsp_write(int board, u32 data) 
{
   unsigned long i;

   // DPRINTK_T("want to send data, waiting for board to be ready\n");

   for (i = 0; (!(pd_dsp_get_status(board) & (1 << HSTR_HTRQ))) && (i < MAX_PCI_BUSY_WAIT); i ++) { }
   if (i == MAX_PCI_BUSY_WAIT) 
   {
      DPRINTK_F("ERROR! board not responding during PCI write\n");
      return;
   }

   // DPRINTK_T("board is ready, sending 0x%08lX\n", (unsigned long) data);

   pd_writel(data, (pd_board[board].address + PCI_HTXR));
}

// 
//       name:  pd_dsp_write_x()
//
//   function:  The same as pd_dsp_write_x() but doesn't check dsp status before write
//              Write data to the PowerDAQ transmit data register.
//
inline void pd_dsp_write_x(int board, u32 data) 
{
   pd_writel(data, (pd_board[board].address + PCI_HTXR));
}

// 
//       name:  pd_dsp_read()
//
//   function:  Read data from the PowerDAQ slave receive data register.
//
//  arguments:  The board (index) to read from.
//  
//    returns:  The 32-bit word read.
//
inline u32 pd_dsp_read(int board) 
{
   unsigned long i;

   // DPRINTK_T("want to read data, waiting for board to be ready\n");

   for (i = 0; !(pd_dsp_get_status(board) & (1 << HSTR_HRRQ)) && (i < MAX_PCI_BUSY_WAIT); i ++) { }
   if (i == MAX_PCI_BUSY_WAIT) {
      DPRINTK_F("ERROR! board not responding during PCI read\n");
      return -1;
   }

   // DPRINTK_T("board is ready, reading\n");
   return pd_readl(pd_board[board].address + PCI_HRXS);
}

// 
//       name:  pd_dsp_read_x()
//
//   function:  The same as pd_dsp_read() but doesn't check dsp status before read
//              Read data from the PowerDAQ slave receive data register.
//              
inline u32 pd_dsp_read_x(int board) 
{
   // DPRINTK_T("board is ready, reading\n");
   return pd_readl(pd_board[board].address + PCI_HRXS);
}


// Function:    u32 pd_dsp_read_ack()
//
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Read one word PowerDAQ command completion acknowledge and
//              check acknowledge value.
//
inline u32 pd_dsp_read_ack(int board)
{
    // Read command ack.
    if (pd_dsp_read(board) != 1)
    {
        return 0;
    }
    return 1;
}


//
//       name:  pd_dsp_cmd_ret_ack()
//
// description: Read one word PowerDAQ command completion acknowledge and
//              check acknowledge value.
//
//  arguments:  The board (index) to read from, command
//  
//    returns:  1 if works, 0 if fails
//
inline u32 pd_dsp_cmd_ret_ack(int board, u16 wCmd)
{
    // Issue command.
    pd_dsp_command(board, wCmd);

    // Read and check command ack.
    return pd_dsp_read_ack(board);
}

//
//       name:  pd_dsp_cmd_ret_value()
//
// description: Read one word PowerDAQ command completion acknowledge and
//              return value.
//
//  arguments:  The board (index) to read from, command
//  
//    returns:  value read from the DSP
//
inline u32 pd_dsp_cmd_ret_value(int board, u16 wCmd)
{
    // Issue command.
    pd_dsp_command(board, wCmd);

    // Read and check command ack.
    return pd_dsp_read(board);
}

//
// Function:    u32 pd_dsp_write_ack()
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//              ULONG dwValue   -- value to write to transmit data register
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Write one word to DSP transmit data register and read
//              PowerDAQ command completion acknowledge and check
//              acknowledge value.
//
inline u32 pd_dsp_write_ack(int board, u32 dwValue)
{
    pd_dsp_write(board, dwValue);

    return pd_dsp_read_ack(board);
}

//
// Function:    inline u32 pd_dsp_cmd_write_ack()
//
// Parameters:  USHORT  wCmd    -- PD command to issue
//              ULONG dwValue   -- value to write to transmit data register
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Issue PD command with one word parameter: issue command,
//              read and check acknowledge, write one word to DSP transmit
//              data register and read/check command completion acknowledge.
//
inline u32 pd_dsp_cmd_write_ack(int board, u16 wCmd, u32 dwValue)
{
    // Issue command and check ack.
    if ( !pd_dsp_cmd_ret_ack(board, wCmd))
        return FALSE;

    // Write 1 word parameter and check command completion acknowledge.
    return pd_dsp_write_ack(board, dwValue);
}

//  
//       NAME:  pd_dsp_acknowledge_interrupt()
//
//   FUNCTION:  The acknowledge interrupt command clears and disables the
//              Host PC interrupt.
//              Servicing an interrupt does not re-enable the interrupt.
//              After all events have been processed, the interrupt should
//              be re-enabled by calling the PDFWEnableIrq() function.
//
//  ARGUMENTS:  
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
inline u32 pd_dsp_acknowledge_interrupt( int board )
{
   u32 i, j, val; 

   pd_dsp_cmd_no_ret( board, PD_BRDINTACK );

   for (i = 0; i < MAX_PCI_BUSY_WAIT; i++) 
   {
       // something like KeStallExecutionProcessor(1); //pd_udelay(1);
       for (j = 100; j>0; j--) val = j;
       if (!pd_dsp_int_status(board)) 
           return TRUE;
   }

   return FALSE; 
}

// 
//       name:  pd_reset_dsp()
//
//   function:  Resets the DSP and reloads the bootstrapper.
//
//  arguments:  The board (index) of the DSP to reset.
//  
//    returns:  1 if all went well, and 0 if there was a problem.
//
int pd_reset_dsp(int board) 
{
   int i;
   int HF;
   u32 HI32Cfg;

   pd_dsp_command(board, PD_BRDHRDRST);
   pd_mdelay(1);
   
   HF = pd_dsp_get_flags(board);
   for (i = 0; (HF != 1) && (HF != 3) && (i < MAX_PCI_WAIT); i ++) 
   {
      pd_mdelay(1);
      HF = pd_dsp_get_flags(board);
   }
   if (i == MAX_PCI_WAIT) {
      DPRINTK_F("ERROR! board not responding following DSP reset\n");
      return 0;
   }

   pd_board[board].HI32CtrlCfg = (u32)(1L << HCTR_HTF0 | 1L << HCTR_HRF0);
   pd_writel(pd_board[board].HI32CtrlCfg, (pd_board[board].address + PCI_HCTR));

   HI32Cfg = pd_readl(pd_board[board].address + PCI_HCTR);
   if (HI32Cfg != pd_board[board].HI32CtrlCfg) 
   {
      DPRINTK_F(KERN_ERR PD_ID "ERROR! HI32CtrlCfg didn't take\n");
      return 0;
   }

   return 1;
}

// 
//       name:  pd_download_firmware_bootstrap
//
//   function:  Downloads the bootstrapper.
//
//  arguments:  The board (index) to download the bootstrapper to.
//  
//    returns:  1 if all went well, and 0 if there was a problem.
//
int pd_download_firmware_bootstrap(int board) 
{
   int i;


   if(!pd_dsp_cmd_ret_ack(board, PD_BRDFWDNLD))
   {
      DPRINTK_F("pd_download_firmware_bootstrap: ERROR! unexpected response to BRDFWDNLD command\n");
      return 0;
   }
   DPRINTK_F("pd_download_firmware_bootstrap: PD_BRDFWDNLD command sent and acknowledged\n");

   pd_dsp_write(board, FWDnldLoader.dwMemSize);
   pd_dsp_write(board, FWDnldLoader.dwMemAdrs);
   DPRINTK_T("sent mem size and address\n");

   for (i = 0; i < FWDnldLoader.dwMemSize; i ++)
   {
      pd_dsp_write(board, FWDnldLoader.dwMemData[i]);
   }
   DPRINTK_T("sent bootloader data\n");

   for (i = 0; (pd_dsp_get_flags(board) != 1) && (i < MAX_PCI_WAIT); i ++) 
   {
      pd_mdelay(1);
   }
   if (i == MAX_PCI_WAIT) 
   {
      DPRINTK_F("pd_download_firmware_bootstrap: ERROR! board not responding following FW Loader download\n");
      return 0;
   }

   DPRINTK_T("bootloader acknowledged\n");

   return 1;
}

// 
//       name:  pd_reset_board
//
//   function:  Performs a complete reset.  All onboard operations are
//              stopped, all configurations and buffers are cleared, and
//              any transfers in progress are signaled as terminated.
//              Downloads the firmware loader to the DSP in preparation
//              of downloading the firmware itself (you should probably
//              follow a call to this function with a call to
//              pd_download_firmware).
//
//  arguments:  The board (index) to reset.
//  
//    returns:  1 if all went well, and 0 if there was a problem.
//
int pd_reset_board(int board) 
{
   if (! pd_reset_dsp(board)) 
   {
      return 0;
   }

   if (! pd_download_firmware_bootstrap(board)) 
   {
      return 0;
   }

   return 1;
}

// 
//       name:  pd_download_firmware
//
//   function:  Download the firmware to the DSP on the specified board.
//
//  arguments:  The board (index) to download firmware to.
//  
//    returns:  1 if all went well, and 0 if there was a problem.
//
int pd_download_firmware(int board) 
{
   int i, j;
   u32 dwReply, id;
   int* pnFWMemSegments = NULL;
   FW_MEMSEGMENT* pFWDownloadData = NULL;
   u32* pdwFWExecAdrs = NULL;


   if (pd_dsp_get_flags(board) != 1) 
   {
      DPRINTK_F("ERROR! dsp not ready for firmware download\n");
      return 0;
   }
   DPRINTK_T("DSP is ready, downloading firmware\n");

// Find out what kind of board are we dealing with...
#ifdef USECOMMONFW
    pnFWMemSegments = (int*)&nFWMemSegments;
    pdwFWExecAdrs = (DWORD*)&dwFWExecAdrs;
    pFWDownloadData = FWDownloadData;

#else
   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;

   if (PD_IS_MFX(id))
   {
      DPRINTK_T("Downloading MFx firmware\n");
      pnFWMemSegments = (int*)&nFWMemSegmentsM;
      pdwFWExecAdrs = (u32*)&dwFWExecAdrsM;
      pFWDownloadData = FWDownloadDataM;
   }
   if (PD_IS_AO(id))
   {
      DPRINTK_T("Downloading AO firmware\n");
      pnFWMemSegments = (int*)&nFWMemSegmentsA;
      pdwFWExecAdrs = (u32*)&dwFWExecAdrsA;
      pFWDownloadData = FWDownloadDataA;
   }
   if (PD_IS_DIO(id))
   {
      DPRINTK_T("Downloading DIO firmware\n");
      pnFWMemSegments = (int*)&nFWMemSegmentsD;
      pdwFWExecAdrs = (u32*)&dwFWExecAdrsD;
      pFWDownloadData = FWDownloadDataD;
   }

   if (PD_IS_LABMF(id))
   {
      DPRINTK_T("Downloading Labmf firmware\n");
      pnFWMemSegments = (int*)&nFWMemSegmentsL;
      pdwFWExecAdrs = (u32*)&dwFWExecAdrsL;
      pFWDownloadData = FWDownloadDataL;
   }

#endif

   for (i = 0; i < *pnFWMemSegments; i ++) 
   {
      // DPRINTK_T("memory segment %d type is %ld\n", i, FWDownloadData[i].dwMemType);

      switch (pFWDownloadData[i].dwMemType) 
      {
      case 1:
         pd_dsp_set_flags(board, 1);
         break;

      case 2:
         pd_dsp_set_flags(board, 3);
         break;

      case 3:
         pd_dsp_set_flags(board, 7);
         break;

      default:
         return 0;
      }

      // Write number of words to load.
      pd_dsp_write(board, pFWDownloadData[i].dwMemSize);

      // Write starting address to load.
      pd_dsp_write(board, pFWDownloadData[i].dwMemAdrs);
      
      // Issue PCI_LOAD command to initiate loading and read return value.
      pd_dsp_command(board, PCI_LOAD);

      // Check if word read equals block size.
      if (pd_dsp_read(board) != pFWDownloadData[i].dwMemSize) 
      {
	     return 0;
      }

      // Write code or data words to transmit register.
      for (j = 0; (u32)j < pFWDownloadData[i].dwMemSize; j ++) 
      {
	     pd_dsp_write(board, pFWDownloadData[i].dwMemData[j]);
      }

      // Read 1 word return ack and check if word read equals
      //      (starting address + block size).
      dwReply = pd_dsp_read(board);
      if (dwReply != (pFWDownloadData[i].dwMemAdrs + pFWDownloadData[i].dwMemSize) ) 
      {
	     return 0;
      }
   }

   pd_dsp_set_flags(board, 1);

   // Execute downloaded code.
   // Check if HF = 1.
   if (pd_dsp_get_flags(board) != 1) 
   {
      return 0;
   }

   // Write address to start execution.
   pd_dsp_write(board, *pdwFWExecAdrs);
   
   // Issue PCI_EXEC command to execute code and read return value
   pd_dsp_command(board, PCI_EXEC);
   dwReply = pd_dsp_read(board);
   if (dwReply != *pdwFWExecAdrs) 
   {
      return 0;
   }

   // Wait for firmware to start up (check if HF = 1).
   for (i = 0; (pd_dsp_get_flags(board) != 1) && (i < MAX_PCI_WAIT); i ++) 
   {
      pd_mdelay(1);
   }

   if (i == MAX_PCI_WAIT) 
   {
      DPRINTK_F("ERROR! board not responding following Firmware Download\n");
      return 0;
   }

   return 1;
}

// 
//       name:  pd_echo_test
//
//   function:  A diagnostic test.  Sends a 24 bit data lump to the DSP and
//              tries to read the same lump back.
//
//  arguments:  The board to ping.
//
//    returns:  1 if it worked, and 0 if it didn't.
//
int pd_echo_test(int board) 
{
   u32 lump;


   DPRINTK_T("doing echo test...\n");

   pd_dsp_command(board, PD_DIAGPCIECHO);
   lump = pd_dsp_read(board);
   if (lump != 1) 
   {
      return 0;
   }

   pd_dsp_write(board, 0xBAD);
   lump = pd_dsp_read(board);
   if (lump == 0xBAD) {
      DPRINTK_T("echo test PASSED\n");
      return 1;
   }

   DPRINTK_F("echo test FAILED!\n");
   return 0;
}

// 
//       name:  pd_dsp_startup
//
//   function:  check the status for the first time and decides what to do
//
//  arguments:  The board
//
//    returns:  1 if it worked, and 0 if it didn't.
//
int pd_dsp_startup (int board)
{
   u32 pd_dsp_flags;

   // update HI32CtrlCfg
   pd_board[board].HI32CtrlCfg = (1L << HCTR_HTF0) | (1L << HCTR_HRF0);
   pd_writel(pd_board[board].HI32CtrlCfg, (pd_board[board].address + PCI_HCTR));
   DPRINTK_T("writing HI32CtrlCfg: 0x%08lX\n", (unsigned long)pd_board[board].HI32CtrlCfg);

   // initialize the board, make sure the firmware is loaded
   pd_dsp_flags = pd_dsp_get_flags(board);
   DPRINTK_N("current DSP flags: 0x%08lX\n", (unsigned long)pd_dsp_flags);

   switch (pd_dsp_flags)
   {
   case 0:
      DPRINTK_F(KERN_ERR PD_ID "ERROR! DSP not ready for firmware download\n");
      return 0;

   case 1:
      DPRINTK_N("downloading firmware to DSP...\n");
      if (! pd_download_firmware(board))
      {
         DPRINTK_F(KERN_ERR PD_ID "ERROR! cannot download firmware to DSP\n");
         return 0;
      }
      DPRINTK_N("firmware download complete\n");
      break;

   case 3:
   case 7:
      DPRINTK_N("resetting board...\n");
      if (! pd_reset_board(board))
      {
         DPRINTK_F(KERN_ERR PD_ID "ERROR! cannot reset board\n");
         return 0;
      }
      DPRINTK_N("board reset complete\n");

      DPRINTK_N("downloading firmware to DSP...\n");
      if (! pd_download_firmware(board))
      {
         DPRINTK_F(KERN_ERR PD_ID "ERROR! cannot download firmware to DSP\n");
         return 0;
      }
      DPRINTK_N("firmware download complete\n");
      break;

   default:
      DPRINTK_F(KERN_ERR PD_ID "ERROR! DSP is in an unknown state\n");
      return 0;
   }

   pd_mdelay(1);

   pd_dsp_flags = pd_dsp_get_flags(board);
   DPRINTK_N("current DSP flags: 0x%08lX\n", (unsigned long)pd_dsp_flags);
   if (pd_dsp_flags != 3)
   {
      DPRINTK_F(KERN_ERR PD_ID "ERROR! DSP is in an unknown state after firmware download\n");
      return 0;
   }

   DPRINTK_N("board %d initialized\n", board);

   if (!pd_echo_test(board))
   {
      DPRINTK_F(KERN_ERR PD_ID "ERROR! board detected an initialized, but echo test fails!\n");
      return 0;
   }

   return 1;   
}




