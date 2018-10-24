//===========================================================================
//
// NAME:    pdl_init.c: 
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver structures initialization functions
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

void  pd_init_pd_board(int board)
{
    PPD_EEPROM eeprom = &pd_board[board].Eeprom;
    int	n;
    
    // clean up subsystems
    memset(&pd_board[board].AinSS, 0x00, sizeof(TAinSS));
    memset(&pd_board[board].AoutSS, 0x00, sizeof(TAoutSS));
    memset(&pd_board[board].DinSS, 0x00, sizeof(TDioSS));
    memset(&pd_board[board].DoutSS, 0x00, sizeof(TDioSS));
    memset(&pd_board[board].UctSS, 0x00, sizeof(TUctSS));

    memset(&pd_board[board].FwEventsConfig, 0x00, sizeof(tEvents));
    memset(&pd_board[board].FwEventsNotify, 0x00, sizeof(tEvents));
    memset(&pd_board[board].FwEventsStatus, 0x00, sizeof(tEvents));
    
    // read eeprom to get board fifo size
    DPRINTK_N("Size of EEPROM = %ld\n", (long)sizeof(eeprom->u.Header));

    n = pd_adapter_eeprom_read(board, sizeof(eeprom->u.Header), eeprom->u.WordValues);
    pd_udelay(500);
    n = pd_adapter_eeprom_read(board, sizeof(eeprom->u.Header), eeprom->u.WordValues);
    if ( n < sizeof(eeprom->u.Header))
    {
        DPRINTK_F("pd_init_pd_board: eeprom read Read %d bytes\n", n);
    }
    
    DPRINTK_N( "pd_init_pd_board: eeprom read (%d) -->\n", n);
    DPRINTK_N( "\tADC Fifo size: %d CL FIFO Size: %d entries\n", 
    	        eeprom->u.Header.ADCFifoSize*1024,
                eeprom->u.Header.CLFifoSize*256);
    DPRINTK_N( "\tDAC Fifo size: %d\n", 
    	        eeprom->u.Header.DACFifoSize*1024);
    DPRINTK_N( "\tSerial Number: %s\n", eeprom->u.Header.SerialNumber);

    // Old boards may return 0 as DAC Fifo size, changes it to
    // 2 which is the default value
    if(eeprom->u.Header.DACFifoSize == 0 || eeprom->u.Header.DACFifoSize == 0xFF)
        eeprom->u.Header.DACFifoSize = 2;

    // Read firmware timestamp
    pd_dsp_reg_read(board, 0, &pd_board[board].fwTimestamp[0]);
    pd_dsp_reg_read(board, 1, &pd_board[board].fwTimestamp[1]);

    // Read revision of the FPGA logic
    pd_dio256_read_input(board, 0xFFFFB0, &pd_board[board].logicRev);
    pd_board[board].logicRev = pd_board[board].logicRev & 0xFFF;
    
    // Initialize analog input subsystem
    //

    // Variables for pd_ain_flush_fifo
    pd_board[board].AinSS.BufInfo.DataWidth = sizeof(WORD);
    pd_board[board].AinSS.BufInfo.databuf = NULL;
    pd_board[board].AinSS.BufInfo.BufSizeInBytes = 0;
    pd_board[board].AinSS.FifoXFerCycles = 1;
    pd_board[board].AinSS.XferBufValues = 0;    	// init later
    pd_board[board].AinSS.pXferBuf = NULL;    		// dynamic alloc
    pd_board[board].AinSS.BlkXferValues = AIN_BLKSIZE;              //0x200
    pd_board[board].AinSS.FifoValues = PD_AIN_MAX_FIFO_VALUES;
    pd_board[board].AinSS.SubsysState = ssConfig;
    pd_board[board].AinSS.SubsysConfig = 0xFFFFFFFF;
    pd_board[board].AinSS.synch = NULL;
    pd_board[board].AinSS.bCheckFifoError = FALSE;
    pd_board[board].AinSS.bCheckHalfDone = FALSE;
    
    // Initialize analog output subsystem
    pd_board[board].AoutSS.BufInfo.DataWidth = sizeof(WORD);
    pd_board[board].AoutSS.BufInfo.databuf = NULL;
    pd_board[board].AoutSS.BufInfo.BufSizeInBytes = 0;
    pd_board[board].AoutSS.XferBufValues = 0;    	// init later
    pd_board[board].AoutSS.pXferBuf = NULL;    		// dynamic alloc
    pd_board[board].AoutSS.BlkXferValues = AOUT_BLKSIZE;              //0x200
    pd_board[board].AoutSS.FifoValues = PD_AOUT_MAX_FIFO_VALUES;
    pd_board[board].AoutSS.SubsysState = ssConfig;
    pd_board[board].AoutSS.synch = NULL;
    pd_board[board].AoutSS.bCheckFifoError = FALSE;
    pd_board[board].AoutSS.bCheckHalfDone = FALSE;
    pd_board[board].AoutSS.bRev3Mode = FALSE;
    
    // Initialize digital input/output subsystem
    pd_board[board].DinSS.SubsysState = ssConfig;
    pd_board[board].DinSS.bAsyncMode = FALSE;
    pd_board[board].DinSS.synch = NULL;

    pd_board[board].DoutSS.SubsysState = ssConfig;
    pd_board[board].DoutSS.bAsyncMode = FALSE;
    pd_board[board].DoutSS.synch = NULL;

    
    // Initialize user counter timer subsystem
    pd_board[board].UctSS.SubsysState = ssConfig;
    pd_board[board].UctSS.bAsyncMode = FALSE;
    pd_board[board].UctSS.synch = NULL;

    pd_board[board].intMutex = 0;
    pd_board[board].dwXFerMode = XFERMODE_NORMAL;
}

void  pd_init_calibration(int board)
{
   u32 i, j, dwCalDACValue, size;
   u32 id;

   DPRINTK_N("board %d: pd_init_calibration callde\n", board);
   
   // Load calibration for PD_MF series with CALDACs.
   // Does this adapter support autocalibration?
   if (pd_board[board].PCI_Config.SubsystemID & ADAPTER_AUTOCAL)
   {
      DPRINTK_N("Board %d is autocalibrated\n", board);

      if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
         id = pd_board[board].PCI_Config.SubsystemID - 0x100;
      else
         id = pd_board[board].PCI_Config.SubsystemID;

      // If installed board is PD(2) - MF(S) program caldacs
      if (PD_IS_MFX(id))
      {
         // Set default configuration.
         pd_board[board].AinSS.SubsysConfig = 0;
    
         // Program AIn CALDACs.
         for ( i = 0; i < 4; i++ )
         {
             dwCalDACValue = pd_board[board].Eeprom.u.Header.CalibrArea[i];
             if (!pd_dsp_cmd_write_ack(board, PD_CALDACWRITE,
                               (((i*2)<<8)|(dwCalDACValue & 0xFF))))
             DPRINTK_F("PD_CALDACWRITE (1) failed");

             if (!pd_dsp_cmd_write_ack(board, PD_CALDACWRITE,
                               (((i*2+1)<<8)|((dwCalDACValue & 0xFF00)>>8))))
             DPRINTK_F("PD_CALDACWRITE (2) failed");
         }
    
         // Program AOut CALDACs.
         for ( i = 0; i < 4; i++ )
         {
             dwCalDACValue = pd_board[board].Eeprom.u.Header.CalibrArea[16 + i];
             if (!pd_dsp_cmd_write_ack(board, PD_CALDACWRITE,
                               ((1<<11)|((i*2)<<8)|(dwCalDACValue & 0xFF))))
             DPRINTK_F("PD_CALDACWRITE (3) failed");

             if (!pd_dsp_cmd_write_ack(board, PD_CALDACWRITE,
                               ((1<<11)|((i*2+1)<<8)|((dwCalDACValue & 0xFF00)>>8))))
             DPRINTK_F("PD_CALDACWRITE (4) failed");                               
         }
  
         // if reasonable ADC FIFO sizes has found - initializte FlashFifo()
         size = pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize;
         if (( size < 1) && (size > 64)) 
            pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize = 1;
            
            // we decided to use several 512 transfers instead of one big.
            // it should help with multiboard support
            //   pd_ain_set_xfer_size(board, size*512);  // half-FIFO size
      }
  
      // If installed board is PD2 - AO program caldacs
      if (PD_IS_AO(id))
      {
         DPRINTK_N("Initializing AO board %d CALDAC\n", board);

         for ( i = 0; i < 8; i++ )
           for ( j = 0; j < 8; j++ )
           {
               dwCalDACValue = *((BYTE*)pd_board[board].Eeprom.u.Header.CalibrArea + i*8 + j);
               if (!pd_dsp_cmd_write_ack(board, PD_CALDACWRITE,
                                 ((i<<13)|(1<<12)|(j<<8)|(dwCalDACValue & 0xFF)))
                  )
               DPRINTK_F("PD_CALDACWRITE (5) failed");                                                               
           }
      }
   }
}

// This function enumerates devices and return -1 when no device is found
int pd_enum_devices(int board, char* board_name, int board_name_size, 
                    char *serial_number, int serial_number_size)
{
   char *modelname;
   char *serial;
   int id;

   if(board >= num_pd_boards)
   {
      return -1;
   }

   if(PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
   {
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   }
   else
   {
      id = pd_board[board].PCI_Config.SubsystemID;
   }
     
   modelname = pd_get_board_name(board);
   serial = (char*)pd_board[board].Eeprom.u.Header.SerialNumber;

   if(board_name != NULL)
   {
      if(strlen(modelname) >= board_name_size)
         strncpy(board_name, modelname, board_name_size - 1);
      else
         strcpy(board_name, modelname);
   }

   if(serial_number != NULL)
   {
      if(strlen(serial) >= serial_number_size)
         strncpy(serial_number, serial, serial_number_size - 1);
      else
         strcpy(serial_number, serial);
   }

   DPRINTK("pd_enum_devices: model %s, serial %s, id 0x%x\n", board_name, serial_number, id);

   return id;
}



pd_board_t* pd_get_board_object(int board)
{
   return &pd_board[board];
}

char* pd_get_board_name(int board)
{
   char *boardname;

   if(PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
   {
      boardname = pdxi_models[pd_board[board].PCI_Config.SubsystemID - 0x211].modelname;
   }
   else
   {
      boardname = pd_models[pd_board[board].PCI_Config.SubsystemID - 0x101].modelname;
   }

   return boardname;
}






