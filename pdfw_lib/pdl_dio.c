//===========================================================================
//
// NAME:    pdl_dio.c: 
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to Digital I/O FW functions
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

#define DIO_REGS_NUM    8

////////////////////////////////////////////////////////////////////////
//   
// DIGITAL INPUT/OUTPUT SECTION
//
////////////////////////////////////////////////////////////////////////

//      
//       NAME:  pd_din_set_config
//
//   FUNCTION:  Sets the configuration of the digital input subsystem.  Duh.
//
//  ARGUMENTS:  The board to configure, and the config word.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_din_set_config(int board, u32 config) 
{
   pd_dsp_command(board, PD_DICFG);
   if (pd_dsp_read(board) != 1) 
   {
      return 0;
   }
   pd_dsp_write(board, config);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_din_set_event_config()
//
//   FUNCTION:  Set DIn Event Mask bits to enable/disable individual
//              digital input line change of state events.
//
//  ARGUMENTS:  The board to configure, and the din event configuration word.
//
//    RETURNS:  1 if it worked and 0 if it failed.
//
int pd_din_set_event(int board, u32 events) 
{
    return pd_dsp_cmd_write_ack(board, PD_BRDSETEVNTS1, events);
}

//
//       NAME:  pd_din_clear_events()
//
//   FUNCTION:  Clears the DIn Event bits.
//
//  ARGUMENTS:  The board to clear, and the "DInEvent Clear Word", whatever
//              that is.
//
//    RETURNS:  1 if it worked and 0 if it failed.
//
int pd_din_clear_events(int board, u32 events) 
{
    return pd_dsp_cmd_write_ack(board, PD_BRDSETEVNTS1, events);
}

//
//       NAME:  pd_din_read_inputs()
//
//   FUNCTION:  Reads the current values on the digital input lines.
//
//  ARGUMENTS:  The board to read from, and a pointer to a u32 to hold the
//              values (in the bottom 8 bits).
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_din_read_inputs(int board, u32 *pdwValue) 
{
   pd_dsp_command(board, PD_DIREAD);          
   *pdwValue = pd_dsp_read(board);
   if (*pdwValue == ERR_RET) 
   {
      return 0;
   }
   return 1;
}

//
//       NAME:  pd_din_clear_data()
//
//   FUNCTION:  Clears all stored DIn data.
//
//  ARGUMENTS:  The board to clear.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_din_clear_data(int board)
{
   pd_dsp_command(board, PD_DICLRDATA);       
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//
//       NAME:  pd_din_reset()
//
//   FUNCTION:  Resets the digital input subsystem to the default startup
//              configuration.
//
//  ARGUMENTS:  The board to reset.
//
//    RETURNS:  1 if it worked and 0 if it failed.
//
int pd_din_reset(int board) 
{
   pd_dsp_command(board, PD_DIRESET);   
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}

//      
//       NAME:  pd_din_status()
//
//   FUNCTION:  Gets the current digital input levels and the currently
//              latched input change events of all the digital input lines.
//
//  ARGUMENTS:  The board to read from, and a pointer to where to put the
//              digital input status word.
//
//    RETURNS:  1 if it worked and 0 if it failed.
//
int pd_din_status(int board, u32 *pdwValue) 
{
   pd_dsp_command(board, PD_DISTATUS); 
   *pdwValue = pd_dsp_read(board);
   if (*pdwValue == ERR_RET) 
   {
      return 0;
   }
   return 1;
}

//  
//       NAME:  pd_dout_write_outputs()
//
//   FUNCTION:  Sets the values of the digital output lines.
//
//  ARGUMENTS:  The value to write to the digital output lines.
//
//    RETURNS:  1 if it worked, 0 if it failed.
//
int pd_dout_write_outputs(int board, u32 val) 
{
   pd_dsp_command(board, PD_DOWRITE);         
   if (pd_dsp_read(board) != 1) 
   {
      return 0;
   }
   pd_dsp_write(board, val);
   return (pd_dsp_read(board) == 1) ? 1 : 0;
}


//  
//       NAME:  pd_dio256_write_output()
//
//   FUNCTION:  Sets the values of the digital output lines
//
//  ARGUMENTS:  Command word and value to write to the digital output lines
//
//    RETURNS:  1 if it worked, 0 if it failed
//
int pd_dio256_write_output(int board, u32 cmd, u32 val)
{
    int ret;

    // Send write command receive acknoledge
    ret = pd_dsp_cmd_ret_ack(board, PD_DI0256WR);
    if (ret != 1) 
       return 0;

    // Write command
    pd_dsp_write(board, cmd);

    // Write parameter and rcv ack
    ret = pd_dsp_write_ack(board, val);
    return (ret)?1:0;
}

//
// Function:    int pd_dio256_write_all(int board, u32* pdata)
//
// Parameters:  board - index of the board to read data from
//              pdata - pointer to a buffer of at least 8 u32 values
//
// Returns:     int status  -- 0: command failed
//                             1:  command succeeded
//
// Description: Writes all ports via single call for PDx-DIO-64 and only first 64 bits (4 ports) 
//              for PD2-DIO-128.
//
// Notes: 
//
int pd_dio256_write_all(int board, u32* pdata)
{
    int i;
    
    pd_dsp_cmd_no_ret(board, PD_DIO256WR_ALL);

    for (i = 0; i < DIO_REGS_NUM/2; i++)  
    {
        pd_dsp_write(board, *(pdata+i));
    }

    // Read ack
    return pd_dsp_read_ack(board);
}

//  
//       NAME:  pd_dio256_read_input()
//
//   FUNCTION:  Sets the values of the digital output lines
//
//  ARGUMENTS:  Command word and value read from the digital input lines
//
//    RETURNS:  1 if it worked, 0 if it failed
//
int pd_dio256_read_input(int board, u32 cmd, u32* val)
{
    int ret;

    // Send write command receive acknoledge
    ret = pd_dsp_cmd_ret_ack(board, PD_DI0256RD);
    if (ret != 1) 
       return 0;

    // Write command
    pd_dsp_write(board, cmd);

    // read value from the board
    *val = pd_dsp_read(board);

    return ret;
}

//
// Function:    int pd_dio256_read_all(int board, u32* pdata)
//
// Parameters:  board - index of the board to read data from
//              pdata - pointer to a buffer of at least 8 u32 values
//
// Returns:     int status  -- 0: command failed
//                             1:  command succeeded
//
// Description: Reads all ports via single call for PDx-DIO-64 and only first 64 bits (4 ports) 
//              for PD2-DIO-128
// Notes:
//
int pd_dio256_read_all(int board, u32* pdata)
{
    int i;
    
    pd_dsp_cmd_no_ret(board, PD_DIO256RD_ALL);

    // Read data
    for (i = 0; i < DIO_REGS_NUM/2; i++)
    { 
       *(pdata+i) = pd_dsp_read(board); 
    }

    // read ack
    return pd_dsp_read_ack(board);
}

//-----------------------------------------------------------------------
// Function:    int PdDIODMASet(int board, u32 offset, u32 count, u32 source)
//
// Description: The DIO 256 CMD performs specified in dwCommand command
//   and write optional data to register specified via the command
//
// Notes:
//
//-----------------------------------------------------------------------
int pd_dio_dmaSet(int board, u32 offset, u32 count, u32 source)
{
   int ret;

   // Send write command receive acknoledge
   ret = pd_dsp_cmd_ret_ack(board, PD_DIODMASET);
   if ( ret != 1 )
      return 0;

   // Write command
   pd_dsp_write(board, offset);
   pd_dsp_write(board, count);

   // Write parameter and rcv ack
   ret = pd_dsp_write_ack(board, source);

   return(ret) ? 1 : 0;
}

//-----------------------------------------------------------------------
// Function:    BOOLEAN pd_dio256_setIntrMask(int board)
//
// Description: Sets up interrupt mask
//
// Notes:
//
//-----------------------------------------------------------------------
int pd_dio256_setIntrMask(int board)
{
    int ret, i;
    u32 *pIntrMask = pd_board[board].DinSS.intrMask;

    // Send write command receive acknoledge
    pd_dsp_cmd_no_ret(board, PD_DINSETINTRMASK);

    ret = pd_dsp_read(board);

    for (i = 0; i < ret; i++)
    {
        if (i < DIO_REGS_NUM)
            pd_dsp_write(board, *(pIntrMask+i));
        else
            pd_dsp_write(board, 0);
    }

    // Read ack
    return pd_dsp_read_ack(board);
}

//-----------------------------------------------------------------------
// Function:    int pd_dio256_getIntrData(int board)
//
// Description: Gets interrupt data, like a latch
//
// Notes:
//
//-----------------------------------------------------------------------
#define DIO_REGS_NUM    8
int pd_dio256_getIntrData(int board)
{
    int ret, i;
    u32 *pIntData = pd_board[board].DinSS.intrData;
    u32 *pEdgeData = pd_board[board].DinSS.intrData + DIO_REGS_NUM;

    // Send write command receive acknoledge
    pd_dsp_cmd_no_ret(board, PD_DINGETINTRDATA);

    ret = pd_dsp_read(board);

    // Read interrupt data
    for (i = 0; i < ret; i++)
    {
        if (i < DIO_REGS_NUM)
            *(pIntData+i) = pd_dsp_read(board);
        else
            pd_dsp_read(board);
    }

    // Read edge data
    for (i = 0; i < ret; i++)
    {
        if (i < DIO_REGS_NUM)
            *(pEdgeData+i) = pd_dsp_read(board);
        else
            pd_dsp_read(board);
    }

    // read ack
    return pd_dsp_read_ack(board);
}

//-----------------------------------------------------------------------
// Function:    int pd_dio256_intrEnable(int board, u32 enable)
//
// Parameters:  PADAPTER_INFO pAdapter -- ptr to adapter info struct
//
// Returns:     BOOLEAN status  -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Enable/Re-enable DIn interrupt/mask for the DIO board
//
// Notes:
//
//-----------------------------------------------------------------------
int pd_dio256_intrEnable(int board, u32 enable)
{
   // Issue PD_DIRESET command and check ack.
   pd_dsp_cmd_ret_ack(board, PD_DININTRREENABLE);

   return pd_dsp_write_ack(board, enable);
}


// -------------------------------------------------------------------------- 
//       NAME:  pd_dsp_reg_write()
//
//   FUNCTION:  This function accesses DSP registers (write)
//
//   ARGUMENTS:  board number, register number, value to write
//
//    RETURNS:  0 if fails
//
int pd_dsp_reg_write(int board, u32 reg, u32 data) 
{
    int ret;
    
    // write command
    ret = pd_dsp_cmd_ret_ack(board, PD_BRDREGWR);
    if (ret != 1) 
       return 0;
    
    // write register and command
    pd_dsp_write(board, reg);
    ret = pd_dsp_write_ack(board, data);
    
    return ret;
}

// -------------------------------------------------------------------------- 
//       NAME:  pd_dsp_reg_read()
//
//   FUNCTION:  This function accesses DSP registers (read)
//
//   ARGUMENTS:  board number, register number, value read
//
//    RETURNS:  0 if fails
//
int pd_dsp_reg_read(int board, u32 reg, u32* data) 
{
    int ret;

    // write command
    ret = pd_dsp_cmd_ret_ack(board, PD_BRDREGRD);
    if (ret != 1) 
       return 0;
    
    // write register and command
    pd_dsp_write(board, reg);
    *data = pd_dsp_read(board);
    
    return TRUE;
}



