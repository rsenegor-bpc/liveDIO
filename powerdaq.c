//===========================================================================
//
// NAME:    powerdaq.c
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver
//
// AUTHOR: Frederic Villeneuve 
//         Alex Ivchenko and Sebastian Kuzminski
//
//---------------------------------------------------------------------------
//      Copyright (C) 2000,2005 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------
// For more informations on using and distributing this software, please see
// the accompanying "LICENSE" file.
//
// "Include" map
//
//  powerdaq.c
//     |
//     + pdfw_def.h
//     + powerdaq-internal.h
//     |   |
//     |   + pdfw_def.h
//     |   + win_ddk_types.h
//     |
//     + powerdaq.h
//
#define PD_GLOBAL_PREFIX 
#include "include/powerdaq_kernel.h"

#include "kvmem.c" // include part of mbuff driver by Tomasz Motylewski

// parameters that can be passed wnen the module is loaded
// with the command "insmod pwrdaq.ko xferMode=1 pd_major=61 rqstirq=0"
int xferMode = 1;
int pd_major = PD_MAJOR;
int rqstirq = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   module_param(xferMode, int, 0);
   module_param(pd_major, int, 0);
   module_param(rqstirq, int, 0);
   MODULE_ALIAS_CHARDEV_MAJOR(PD_MAJOR);
   MODULE_LICENSE("GPL");
#else
   MODULE_PARM(xferMode,"i");
   MODULE_PARM(pd_major,"i");
   MODULE_PARM(rqstirq,"i");
#endif


// declare the class_simple object used to create an entry
// in /sys/class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
   #include <linux/seq_file.h>
   static struct class *pwrdaq_class;
   #define class_device device
   #define class_device_destroy device_destroy
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
   static struct class *pwrdaq_class;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
   static struct class_simple *pwrdaq_class;
   #define class_create class_simple_create
   #define class_device_create class_simple_device_add
   #define class_device_destroy(a,b) class_simple_device_remove(b)
   #define class_destroy class_simple_destroy
#endif

#ifndef SPIN_LOCK_UNLOCKED 
   #define SPIN_LOCK_UNLOCKED __SPIN_LOCK_UNLOCKED(old_style_spin_init)
#endif


//////////////////////////////////////////////////////////////////////////
//
//       name:  deal_with_device
//
//   function:  Checks if the passed-in device is one we know how to deal
//              with, and if so deals with it.
//
//  arguments:  The PCI device to deal with.  It is expected to have
//              Motorola's Vendor ID and the DSP56301's Device ID.
//
//    returns:  Nothing.
//
//
static void deal_with_device(struct pci_dev *dev) 
{
   u16 sub_vendor_id;
   u8  u8val;
   u16 u16val;
   u16 sub_device_id;
   int ret, t;
   char* model_name;


   if (dev == NULL)
   {
      DPRINTK_F("ERROR!  null PCI device passed to deal_with_device! skipping...\n");
      goto fail0;
   }

   if (dev->hdr_type != PCI_HEADER_TYPE_NORMAL)
   {
      DPRINTK_F("unexpected PCI header, skipping\n");
      goto fail0;
   }

   ret = pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor_id);
   if (sub_vendor_id != UEI_SUBVENID)
   {
      DPRINTK_F("not a UEI board, skipping\n");
      goto fail0;
   }

   pci_read_config_word(dev, PCI_SUBSYSTEM_ID, &sub_device_id);
   DPRINTK_N("SubSystemID of board %d is 0x%x\n", num_pd_boards, sub_device_id);
   if ( (sub_device_id < PD_SUBSYSTEMID_FIRST) || (sub_device_id > PD_SUBSYSTEMID_LAST) )
   {
      DPRINTK_F("subdeviceid=%08X: this model of PowerDAQ board is not supported, skipping\n", sub_device_id);
      goto fail0;
   }

   DPRINTK_N("found a PowerDAQ board (bus=%d device=%d func=%d)\n", dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
   DPRINTK_N("Subsystem Id is 0x%x\n", sub_device_id);
   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
   // enable device to ensure PCI routing of IRQs
   ret = pci_enable_device(dev);
   if(ret)
   {
      DPRINTK_F("pci_enable_device error, skipping\n");
      goto fail0;
   }
#endif

   // initialize data structures
   memset(&pd_board[num_pd_boards], 0, sizeof(pd_board_t));

   pd_board[num_pd_boards].dev = dev;
   pd_board[num_pd_boards].caps_idx = sub_device_id - PD_SUBSYSTEMID_FIRST;
   pd_board[num_pd_boards].index = num_pd_boards;
   pd_board[num_pd_boards].size = 65536;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
   pd_board[num_pd_boards].address = ioremap(PCI_BASE_ADDRESS_MEM_MASK & dev->base_address[0], pd_board[num_pd_boards].size);
   DPRINTK_N("\tbus address: 0x%08lX\n", (unsigned long)dev->base_address[0]);
#else
   pd_board[num_pd_boards].address = ioremap(PCI_BASE_ADDRESS_MEM_MASK & dev->resource[0].start, pd_board[num_pd_boards].size);
   DPRINTK_N("\tbus address: 0x%08lX\n", (unsigned long)dev->resource[0].start);
#endif
   DPRINTK_N("\tvirt address: 0x%08lX\n", (unsigned long)pd_board[num_pd_boards].address);

   // store PCI configuration
   pci_read_config_word(dev, PCI_VENDOR_ID, &u16val);
   pd_board[num_pd_boards].PCI_Config.VendorID = u16val;

   pci_read_config_word(dev, PCI_DEVICE_ID, &u16val);
   pd_board[num_pd_boards].PCI_Config.DeviceID = u16val;

   pci_read_config_word(dev, PCI_COMMAND, &u16val);
   pd_board[num_pd_boards].PCI_Config.Command = u16val;

   pci_read_config_word(dev, PCI_STATUS, &u16val);
   pd_board[num_pd_boards].PCI_Config.Status = u16val;

   pci_read_config_byte(dev, PCI_REVISION_ID, &u8val);
   pd_board[num_pd_boards].PCI_Config.RevisionID = u8val;

   pci_read_config_byte(dev, PCI_CACHE_LINE_SIZE, &u8val);
   pd_board[num_pd_boards].PCI_Config.CacheLineSize = u8val;

   pci_read_config_byte(dev, PCI_LATENCY_TIMER, &u8val);
   pd_board[num_pd_boards].PCI_Config.LatencyTimer = u8val;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
   pd_board[num_pd_boards].PCI_Config.BaseAddress0 = dev->base_address[0];
#else
   pd_board[num_pd_boards].PCI_Config.BaseAddress0 = dev->resource[0].start;
#endif
   pd_board[num_pd_boards].PCI_Config.SubsystemVendorID = sub_vendor_id;
   pd_board[num_pd_boards].PCI_Config.SubsystemID = sub_device_id;

   pd_board[num_pd_boards].PCI_Config.InterruptLine = dev->irq;

   pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &u8val);
   pd_board[num_pd_boards].PCI_Config.InterruptPin = u8val;

   pci_read_config_byte(dev, PCI_MIN_GNT, &u8val);
   pd_board[num_pd_boards].PCI_Config.MinimumGrant = u8val;

   pci_read_config_byte(dev, PCI_MAX_LAT, &u8val);
   pd_board[num_pd_boards].PCI_Config.MaximumLatency = u8val;

   // check FW state and download it if necessarily
   if (!pd_dsp_startup (num_pd_boards)) goto fail1;

   pd_board[num_pd_boards].irq = dev->irq;
   pd_board[num_pd_boards].open = 0;

   // get hold on interrupt line
   if (rqstirq)
   {
      if (pd_driver_request_irq(num_pd_boards, NULL))
      {
         DPRINTK_F("couldnt allocate ISR, skipping card\n");
         goto fail1;
      }
   } 
   else
   {
      DPRINTK("rqstirq=0 interrupts disabled\n");
   }

   pd_init_pd_board(num_pd_boards);

   if (pd_event_create(num_pd_boards, &pd_board[num_pd_boards].AinSS.synch) != 0)
   {
      DPRINTK_T("Could not create the Ain synch object.\n");
      goto fail1;
   }

   if (pd_event_create(num_pd_boards, &pd_board[num_pd_boards].AoutSS.synch) != 0)
   {
      DPRINTK_T("Could not create the Aout synch object.\n");
      goto fail1;
   }
   
   if (pd_event_create(num_pd_boards, &pd_board[num_pd_boards].DinSS.synch) != 0)
   {
      DPRINTK_T("Could not create the Din synch object.\n");
      goto fail1;
   }
   
   if (pd_event_create(num_pd_boards, &pd_board[num_pd_boards].DoutSS.synch) != 0)
   {
      DPRINTK_T("Could not create the Dout synch object.\n");
      goto fail1;
   }
   
   if (pd_event_create(num_pd_boards, &pd_board[num_pd_boards].UctSS.synch) != 0)
   {
      DPRINTK_T("Could not create the Uct synch object.\n");
      goto fail1;
   }

   pd_board[num_pd_boards].AinSS.synch->subsystem = AnalogIn;
   pd_board[num_pd_boards].AoutSS.synch->subsystem = AnalogOut;
   pd_board[num_pd_boards].DinSS.synch->subsystem = DigitalIn;
   pd_board[num_pd_boards].DoutSS.synch->subsystem = DigitalOut;
   pd_board[num_pd_boards].UctSS.synch->subsystem = CounterTimer;

   // Set the Xferm mode to whatever the user passed as a parameter
   pd_board[num_pd_boards].dwXFerMode = xferMode;
   DPRINTK_N("Xfer mode is %d\n", xferMode);

   // initialize calibration values
   pd_init_calibration(num_pd_boards);

   {
      u32 XBMPageSz, XAOPageSz;
      tAllocContigMem Mem;
      
      // Verify that we do have enough memory for our games
      // load ADC FIFO transfer size
      if (( pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize >= 1 ) &&
          ( pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize <= 0x40 ))
      {
          pd_board[num_pd_boards].AinSS.FifoValues = pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize << 10; // kbytes->bytes
          if (pd_board[num_pd_boards].AinSS.FifoValues <= PD_AIN_MAX_FIFO_VALUES)
              pd_board[num_pd_boards].AinSS.XferBufValues = pd_board[num_pd_boards].AinSS.FifoValues;
          else
              pd_board[num_pd_boards].AinSS.XferBufValues = PD_AIN_MAX_FIFO_VALUES;    // maximum limited by transfer buffer
      }
      else
      {
          pd_board[num_pd_boards].AinSS.FifoValues = PD_AIN_FIFO_VALUES;  // minimum size for 1kS FIFO
          pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize = 1;
      }


      // calculate how many times we have to make 512 (1/2K FIFO) or 1024 ((4/8/16/32/64K FIFO)) samples transfer. 
      // The idea is that we can make 1/2 ADC FIFO transfers
      pd_board[num_pd_boards].AinSS.FifoXFerCycles = (pd_board[num_pd_boards].AinSS.FifoValues / PD_AIN_FIFO_VALUES);

      // Set up bus-mastering pages parameters
      pd_board[num_pd_boards].AinSS.DoGetSamples = TRUE;

      // calculate this in pages: minimum - 1 page, maximum - x pages
      // please notice that XFer requres only one page where BM requires two
      XBMPageSz = 4;
      switch (pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize) 
      {
      case 1:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 1;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 8;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE512;
         break;
      case 2:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 1;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 8;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE512;
         break;
      case 4:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 2;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 4;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE1024;
         break;
      case 8:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 4;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 4;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE1024;
         break;
      case 16:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 8;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 4;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE1024;
         XBMPageSz = 8;
         break;
      case 32:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 8;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 4;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE1024;
         XBMPageSz = 8;
         break;
      case 64:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 8;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 4;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE1024;
         XBMPageSz = 16;
         break;
      default:
         pd_board[num_pd_boards].AinSS.BmFHFXFers = 1;
         pd_board[num_pd_boards].AinSS.BmPageXFers = 8;
         pd_board[num_pd_boards].AinSS.AIBMTXSize = AIBM_TXSIZE512;
         break;
      }

      if(pd_board[num_pd_boards].dwXFerMode == XFERMODE_RTBM)
      {
         pd_board[num_pd_boards].AinSS.BmFHFXFers = RTModeBmFHFXFers;
         pd_board[num_pd_boards].AinSS.BmPageXFers = RTModeBmPageXFers;
      }

      // Initialize memory chunks 0 and 1 for bus-mastering operations
      Mem.idx = AI_PAGE0;
      Mem.size = XBMPageSz;
      ret = pd_alloc_contig_memory(num_pd_boards, &Mem);
      if (ret != 0) 
      {
          pd_board[num_pd_boards].dwXFerMode = XFERMODE_NOAIMEM;
          pd_board[num_pd_boards].AinSS.XferBufValues = 0; 
          pd_board[num_pd_boards].AinSS.FifoValues = 0;
          pd_board[num_pd_boards].AinSS.pXferBuf = NULL;

          DPRINTK_F("pd_init ain error: couldn't allocate AIn buffer size 0x%x\n", XBMPageSz);

          goto fail1;
      } 
      else 
      {
          Mem.idx = AI_PAGE1;
          Mem.size = XBMPageSz;
          ret = pd_alloc_contig_memory(num_pd_boards, &Mem);
          if (ret != 0) 
          {
              pd_board[num_pd_boards].dwXFerMode = XFERMODE_NOAIMEM;
              pd_board[num_pd_boards].AinSS.XferBufValues = 0;
              pd_board[num_pd_boards].AinSS.FifoValues = 0;
              pd_board[num_pd_boards].AinSS.pXferBuf = NULL;
              goto fail1;
          }
          pd_board[num_pd_boards].AinSS.pXferBuf = pd_board[num_pd_boards].pSysBMB[AI_PAGE1];  // buffer address for XFer and BM
      }

      // load DAC FIFO transfer size
      if (pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize > 0x80) // incorrect DA88C FIFO size
      {
          pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize = 2;
      }

      if (( pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize >= 2 ) &&
          ( pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize <= 0x40 ))
      {
          pd_board[num_pd_boards].AoutSS.FifoValues = pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize << 10; // kbytes->bytes
          if (pd_board[num_pd_boards].AoutSS.FifoValues <= PD_AOUT_MAX_FIFO_VALUES)
          {
             pd_board[num_pd_boards].AoutSS.XferBufValues = pd_board[num_pd_boards].AoutSS.FifoValues;
          }
          else
          {
             pd_board[num_pd_boards].AoutSS.XferBufValues = ANALOG_XFERBUF_VALUES;    // maximum limited by transfer buffer
          }
      } 
      else 
      {
          pd_board[num_pd_boards].AoutSS.FifoValues = PD_AOUT_MAX_FIFO_VALUES;  // minimum size for 1kS FIFO
      }

      pd_board[num_pd_boards].AoutSS.TranSize = pd_board[num_pd_boards].AoutSS.FifoValues;

      // calculate how many pages need to be allocated
      XAOPageSz = (pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize << 12)/PAGE_SIZE;
      if ((pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize << 12)%PAGE_SIZE) 
      {
         XAOPageSz++;
      }

      Mem.idx = AO_PAGE0;
      Mem.size = XAOPageSz;
      ret = pd_alloc_contig_memory(num_pd_boards, &Mem);
      if (ret != 0) 
      {
         pd_board[num_pd_boards].AoutSS.TranSize = 0;
         pd_board[num_pd_boards].AoutSS.FifoValues = 0;
         pd_board[num_pd_boards].AoutSS.pXferBuf = NULL;
         pd_board[num_pd_boards].dwXFerMode = XFERMODE_NOAOMEM;

         DPRINTK_F("pd_init AO/AO32 error: couldn't allocate AOut buffer size 0x%x\n", XAOPageSz);
      }
      pd_board[num_pd_boards].AoutSS.pXferBuf = pd_board[num_pd_boards].pSysBMB[AO_PAGE0];
   }

   DPRINTK_N("AI: XferBufValues = %d, BlkXferValues = %d, FifoValues = %d, FifoXFerCycles = %d\n", 
                    pd_board[num_pd_boards].AinSS.XferBufValues,
                    pd_board[num_pd_boards].AinSS.BlkXferValues, 
                    pd_board[num_pd_boards].AinSS.FifoValues, 
                    pd_board[num_pd_boards].AinSS.FifoXFerCycles);


   DPRINTK_N("AI0:0x%lx AI1:0x%lx AO0:0x%lx\n", (unsigned long)pd_board[num_pd_boards].pSysBMB[AI_PAGE0], 
                                           (unsigned long)pd_board[num_pd_boards].pSysBMB[AI_PAGE1], 
                                           (unsigned long)pd_board[num_pd_boards].pSysBMB[AO_PAGE0]);

   DPRINTK_N("pd_board[%d] has been initialized\n", num_pd_boards);
   
   model_name = pd_get_board_name(num_pd_boards);

   PRINTK("Board %d:\n", num_pd_boards);
   PRINTK("\tName: %s\n", model_name);
   PRINTK("\tSerial Number: %8s\n", pd_board[num_pd_boards].Eeprom.u.Header.SerialNumber);
   PRINTK("\tInput FIFO size: %d samples\n", pd_board[num_pd_boards].Eeprom.u.Header.ADCFifoSize*1024);
   PRINTK("\tInput channel list FIFO size: %d entries\n", pd_board[num_pd_boards].Eeprom.u.Header.CLFifoSize*256);
   PRINTK("\tOutput FIFO size: %d samples\n", pd_board[num_pd_boards].Eeprom.u.Header.DACFifoSize*1024);
   PRINTK("\tManufacture date: %s\n", pd_board[num_pd_boards].Eeprom.u.Header.ManufactureDate);
   PRINTK("\tCalibration date: %s\n", pd_board[num_pd_boards].Eeprom.u.Header.CalibrationDate);
   PRINTK("\tBase address: 0x%x\n", pd_board[num_pd_boards].PCI_Config.BaseAddress0);
   PRINTK("\tIRQ line: 0x%x\n", pd_board[num_pd_boards].PCI_Config.InterruptLine); 
   PRINTK("\tDSP Rev: v%d\n", (pd_board[num_pd_boards].fwTimestamp[0]&0x0F0000)>>16);  
   t =  pd_board[num_pd_boards].fwTimestamp[0]>>20;
   PRINTK("\tFirmware type: %s, rev: %d.%d/%x\n",
          (t < 1)?"Generic": ((t<2)?"AO": ((t<3)?"DIO":((t<4)?"LMF":((t<5)?"MFx":"Unknown")))),
          (pd_board[num_pd_boards].fwTimestamp[0]&0xF00)>>8,
          pd_board[num_pd_boards].fwTimestamp[0]&0xFF,
          pd_board[num_pd_boards].fwTimestamp[1]); 

   num_pd_boards ++;
   return;

   fail1:  iounmap(pd_board[num_pd_boards].address);
   fail0:  return;
}


///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_fasync
//
//   Function:  This gets called when the user-space process changes the
//              FASYNC flag.
//
//  Arguments:  Hmmm...
//
//    Returns:  0 if everything went well, and "what errno should be" if
//              there's a problem.
//
int pd_fasync(int fd, struct file *file, int mode) 
{
   int real_minor, board, board_minor, ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   ret = fasync_helper(fd, file, mode, &pd_board[board].fasync);

   DPRINTK_I("board %d, %s (minor %u) got fasync mode %d... ret=%d fd=%d\n", board,
             pd_devices_by_minor[board_minor], real_minor, mode, ret, fd);

   return ret;
}



//////////////////////////////////////////////////////////////////////////
//
//       NAME: pd_open()
//
//   FUNCTION: Open a PowerDAQ device.
//
//  ARGUMENTS: Hmmm..
//
//    RETURNS: 0 if all went well, and some error code if there was a problem.
//
int pd_open(struct inode *inode, struct file *file)
{
   int real_minor, board, board_minor;
   int ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   DPRINTK_I("trying to open board %d, %s (minor %u)\n",
             board, pd_devices_by_minor[board_minor], real_minor);

   ret = pd_driver_open(board, board_minor);
   
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
   if(!ret)
   {
      MOD_INC_USE_COUNT;
   }
#endif

   return ret;
}

#if defined(_PD_XENOMAI)
int rt_pd_open(struct rtdm_dev_context *context,
               rtdm_user_info_t *user_info, int oflags)
{
   pd_rtdm_ctx_t           *ctx;
   int                     dev_id = context->device->device_id;
   int                     board = dev_id / PD_MINOR_RANGE;
   int                     minor = dev_id % PD_MINOR_RANGE;


   ctx = (pd_rtdm_ctx_t *)context->dev_private;

   return (pd_driver_open(board, minor));
}

int rt_pd_close(struct rtdm_dev_context *context,
               rtdm_user_info_t *user_info)
{
   pd_rtdm_ctx_t           *ctx;
   int                     dev_id = context->device->device_id;
   int                     board = dev_id / PD_MINOR_RANGE;
   int                     minor = dev_id % PD_MINOR_RANGE;


   ctx = (pd_rtdm_ctx_t *)context->dev_private;

   return(pd_driver_close(board, minor));
}

int rt_pd_ioctl_rt(struct rtdm_dev_context *context,
                    rtdm_user_info_t *user_info, unsigned int request, void *arg)
{
   int  dev_id = context->device->device_id;
   int  board = dev_id / PD_MINOR_RANGE;
   int  minor = dev_id % PD_MINOR_RANGE;
   int  ret = 0;
   tCmd argcmd;
      
   if(arg != NULL)
   {
      if (user_info)
      {
         if (!rtdm_rw_user_ok(user_info, arg, sizeof(tCmd)) ||
            rtdm_copy_from_user(user_info, &argcmd, arg, sizeof(tCmd)))
         return -EFAULT;
      } 
      else
      {
         memcpy(&argcmd, arg, sizeof(tCmd));
      }
   }

   ret = pd_driver_ioctl(board, minor, request, &argcmd);
   
   if(arg != NULL)
   {
      if (user_info)
      {
         if(rtdm_copy_to_user(user_info, arg, &argcmd, sizeof(tCmd)))
            return -EFAULT;
      } 
      else
      {
         memcpy(arg, &argcmd, sizeof(tCmd));
      }
   }

   return ret;
}

static const struct rtdm_device __initdata pd_rtdm_device_tmpl = 
{
    struct_version:     RTDM_DEVICE_STRUCT_VER,

    device_flags:       RTDM_NAMED_DEVICE | RTDM_EXCLUSIVE,
    context_size:       0,
    device_name:        "",

    // open_rt and close_rt are not real-time safe due to calls to kmalloc/kfree
    open_rt:            NULL,
    open_nrt:           rt_pd_open,

    ops: {
        close_rt:       NULL,
        close_nrt:      rt_pd_close,

        ioctl_rt:       rt_pd_ioctl_rt,
        ioctl_nrt:      rt_pd_ioctl_rt,

        read_rt:        NULL,
        read_nrt:       NULL,

        write_rt:       NULL,
        write_nrt:      NULL,

        recvmsg_rt:     NULL,
        recvmsg_nrt:    NULL,

        sendmsg_rt:     NULL,
        sendmsg_nrt:    NULL,
    },

    device_class:       RTDM_CLASS_EXPERIMENTAL,
    device_sub_class:   0,
    driver_name:        "pwrdaq",
    driver_version:     RTDM_DRIVER_VER(PD_VERSION_MAJOR, PD_VERSION_MINOR, PD_VERSION_EXTRA),
    peripheral_name:    "PowerDAQ adapter",
    provider_name:      "United Electronics Industries",
};

int pd_driver_register(int board, int minor)
{
   struct rtdm_device *dev = kmalloc(sizeof(struct rtdm_device), GFP_KERNEL);
   int ret = -ENOMEM;
   if (!dev)
   {
      DPRINTK("unable to allocate rtdm_device structure\n");
      return ret;
   }

   memcpy(dev, &pd_rtdm_device_tmpl, sizeof(struct rtdm_device));
   snprintf(dev->device_name, RTDM_MAX_DEVNAME_LEN, "rt-pd-c%d-%s", board, pd_devices_by_minor[minor]);

   dev->device_id = board*PD_MINOR_RANGE + minor;
   dev->proc_name = dev->device_name;

   ret = rtdm_dev_register(dev);

   pd_board_ext[board].rtdmdev[minor] = dev;

   return ret;
}

int pd_driver_unregister(int board, int minor)
{
   int ret = 0;

   if (pd_board_ext[board].rtdmdev[minor] != NULL) 
   {
      ret = rtdm_dev_unregister(pd_board_ext[board].rtdmdev[minor], 1000);
      kfree(pd_board_ext[board].rtdmdev[minor]);
   }

   return ret;
}

#elif defined(_PD_RTLPRO)

int rtl_pd_open(struct rtl_file *filp)
{
   int  board = filp->f_priv / PD_MINOR_RANGE;
   int  minor = filp->f_priv % PD_MINOR_RANGE;

   DPRINTK("Entering rtl_pd_open (%d,%d)\n", board, minor);

   return (pd_driver_open(board, minor));
}

int rtl_pd_release(struct rtl_file *filp)
{
   int  board = filp->f_priv / PD_MINOR_RANGE;
   int  minor = filp->f_priv % PD_MINOR_RANGE;

   DPRINTK("Entering rtl_pd_release (%d,%d)\n", board, minor);

   return (pd_driver_close(board, minor));
}

/*rtl_ssize_t rtl_pd_read(struct rtl_file *filp, char *buf, 
                        rtl_size_t count, rtl_off_t* ppos)
{
}

rtl_ssize_t rtl_pd_write(struct rtl_file *filp, const char *buf, 
                         rtl_size_t count, rtl_off_t* ppos)
{
}*/

int rtl_pd_ioctl(struct rtl_file *filp, unsigned int request, 
                 unsigned long l)
{
   int ret = 0;
   int  board = filp->f_priv / PD_MINOR_RANGE;
   int  minor = filp->f_priv % PD_MINOR_RANGE;
   //tCmd argcmd;
   tCmd* arg = (tCmd*)l;

   DPRINTK("Entering rtl_pd_ioctl (%d,%d), request=0x%x, arg=0x%lx\n", board, minor, request, l);
   
   /*if(arg != NULL)
   {
      rtl_memcpy(&argcmd, arg, sizeof(tCmd));
   } */

   ret = pd_driver_ioctl(board, minor, request, arg/*&argcmd*/);
   
   /*if(arg != NULL)
   {
      rtl_memcpy(arg, &argcmd, sizeof(tCmd));
   }*/ 
   
   return ret;
}

static struct rtl_file_operations rtl_pd_fops =
{
   read:   NULL,
   write:  NULL,
   ioctl:  rtl_pd_ioctl,
   open:   rtl_pd_open,
   release:rtl_pd_release,
};

int pd_driver_register(int board, int minor)
{
   int ret = 0;
   static char logical_device[64];
   snprintf(logical_device, 64, "/dev/rt-pd-c%d-%s", board, pd_devices_by_minor[minor]);

   DPRINTK("Registering RTL device %s (%d, %d)\n", logical_device, board, minor);
   ret = rtl_register_dev(logical_device, &rtl_pd_fops, board*PD_MINOR_RANGE + minor);

   return ret;
}

int pd_driver_unregister(int board, int minor)
{
   int ret = 0;
   static char logical_device[64];
   snprintf(logical_device, 64, "/dev/rt-pd-c%d-%s", board, pd_devices_by_minor[minor]);

   DPRINTK("Unregistering RTL device %s (%d, %d)\n", logical_device, board, minor);
   ret = rtl_unregister_dev(logical_device);
    
   return ret;
}

#endif


//////////////////////////////////////////////////////////////////////////
//
//       NAME:  pd_release()
//
//   FUNCTION:  Frees up a PowerDAQ device.
//
//  ARGUMENTS:  Hmmm..
//
//    RETURNS:  None.
//
int pd_release(
              struct inode *inode,
              struct file *file
              )
{
   int real_minor, board, board_minor, subsystem;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;
   subsystem = BoardLevel;

   DPRINTK_I("pd_release: board %d, (minor %u)\n",
             board, real_minor);

   // if release is called, library shall already inform driver about it
   // using IOCTL_PWRDAQ_CLOSESUBSYSTEM, so clearing of fd seems to be safe
   switch (board_minor)
   {
   case PD_MINOR_AIN:
      subsystem = AnalogIn;
      break;

   case PD_MINOR_AOUT:
      subsystem = AnalogOut;
      break;

   case PD_MINOR_DIN:
      subsystem = DigitalIn;
      break;

   case PD_MINOR_DOUT:
      subsystem = DigitalOut;
      break;

   case PD_MINOR_UCT:
      subsystem = CounterTimer;
      break;

   case PD_MINOR_DSPCT:
      subsystem = DSPCounter;
      break;

   }

   // possibly our app crashes. We need to deallocate buffers...
   pd_unregister_daq_buffer(board, subsystem);

   // release any asynchronous readers, fd not used since fasync stuct being released
   pd_fasync( -1, file, 0);  //FIXME: shouldn't I release it for each subsystem???

   pd_driver_close(board, board_minor);
   
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
   MOD_DEC_USE_COUNT;
#endif

   return 0;
}


////////////////////////////////////////////////////////////////////////
//
// FILE OPERATIONS SECTION
//
////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_read()
//
//   Function:  Reads data from the board.
//
//  Arguments:  The file being read from, the userspace buffer to read into,
//              the number of bytes to read, and the offset (what's this?)...
//
//    Returns:  Number of bytes actually read.
//
ssize_t pd_read(
               struct file *file,
               char *buffer,
               size_t count,
               loff_t *ppos )
{
   int real_minor, board, board_minor;
   int ret = -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   DPRINTK_I("trying to read from board %d, %s (minor %u)\n", board,
             pd_devices_by_minor[board_minor], real_minor);

   _fw_spinlock    // set spin lock

   switch (board_minor)
   {
   case PD_MINOR_AIN:
      if (!(pd_board[board].AinSS.SubsysState & ssActive))
      {
         //ret = pd_ain_read_scan(board, buffer, count);
      }
      break;

   case PD_MINOR_AOUT:
      if (!(pd_board[board].AoutSS.SubsysState & ssActive))
      {
         DPRINTK_I("not implemented yet...\n");
      }
      ret = -ENODEV;
      break;

   case PD_MINOR_DIN:
      if (!(pd_board[board].DinSS.SubsysState & ssActive))
      {
         //ret = pd_din_read(board, buffer, count);
      }
      break;

   case PD_MINOR_DOUT:
      if (!(pd_board[board].DoutSS.SubsysState & ssActive))
      {
         //ret = pd_din_read(board, buffer, count);
      }
      ret = -ENODEV;
      break;

   case PD_MINOR_UCT:
      if (!(pd_board[board].UctSS.SubsysState & ssActive))
      {
         DPRINTK_I("not implemented yet...\n");
      }
      ret = -ENODEV;
      break;
   }

   _fw_spinunlock    // release spin lock

   return ret;
}


///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_write()
//
//   Function:  Writes data to the board.
//
//  Arguments:  Hmmm...
//
//    Returns:  Number of bytes actually written.
//
ssize_t pd_write(
                struct file *file,
                const char *buffer,
                size_t count,
                loff_t *ppos )
{
   int real_minor, board, board_minor;
   int ret = -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   DPRINTK_I("trying to write to board %d, %s (minor %u)\n",
             board, pd_devices_by_minor[board_minor], real_minor);

   _fw_spinlock    // set spin lock

   switch (board_minor)
   {
   
   case PD_MINOR_AIN:
      if (!(pd_board[board].AinSS.SubsysState & ssActive))
      {
         //DPRINTK_I("not implemented yet...\n");
      }
      ret = -ENODEV;
      break;

   case PD_MINOR_AOUT:
      if (!(pd_board[board].AoutSS.SubsysState & ssActive))
      {
         //ret = pd_aout_write_scan(board, buffer, count);
      }
      break;

   case PD_MINOR_DIN:
      if (!(pd_board[board].DinSS.SubsysState & ssActive))
      {
         //ret = pd_dout_write(board, buffer, count);
      }
      ret = -ENODEV;
      break;

   case PD_MINOR_DOUT:
      if (!(pd_board[board].DoutSS.SubsysState & ssActive))
      {
         //ret = pd_dout_write(board, buffer, count);
      }
      break;

   case PD_MINOR_UCT:
      if (!(pd_board[board].UctSS.SubsysState & ssActive))
      {
         DPRINTK_I("not implemented yet...\n");
      }
      ret = -ENODEV;
      break;
   }

   _fw_spinunlock    // release spin lock

   return ret;
}

///////////////////////////////////////////////////////////////////////
//
// DISPATCH ROUTINE
//
///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_ioctl / pd_rt_ioctl
//
//   Function:  Special functions for configuring the driver subsystems.
//
//  Arguments:  Minor doesn't matter ... really
//
//    Returns:  0 if everything went well, and "what errno should be" if
//              there's a problem.
//
//    Note:     Majority of FW functions have another return code notation.
//              FW returns 1 if SUCCESS and 0 if FAILURE. Don't forget
//              to translate them
//
//
#if defined (_PD_RTL) || defined(_PD_RTAI) || defined(_PD_RTLPRO)
int pd_rt_ioctl(int board, unsigned int command, unsigned long arg)
{
   int retcode;
   tCmd argcmd;
      
   if((void*)arg != NULL)
   {
      pd_copy_from_user32((u32*)&argcmd, (u32*)arg, sizeof(tCmd));

   }

   retcode = pd_driver_ioctl(board, 0, command, &argcmd);

   if((void*)arg != NULL)
   {
      pd_copy_to_user32((u32*)arg, (u32*)&argcmd, sizeof(tCmd));
   }

   return retcode;
}

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
long pd_ioctl(
            struct file *file,
            unsigned int command,
            unsigned long arg)
#else
int pd_ioctl(
            struct inode *inode,
            struct file *file,
            unsigned int command,
            unsigned long arg)
#endif
{
   int real_minor, board, board_minor;
   tCmd argcmd;
   int ret = 0;
   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   if((void*)arg != NULL)
   {
      pd_copy_from_user32((u32*)&argcmd, (u32*)arg, sizeof(tCmd));
   }

   ret = pd_driver_ioctl(board, board_minor, command, &argcmd);

   if((void*)arg != NULL)
   {
      pd_copy_to_user32((u32*)arg, (u32*)&argcmd, sizeof(tCmd));
   }

   return ret;
}



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_proc_show/pd_proc_open
//
//   Function:  This gets called when the user-space process accesses
//              /proc/powerdaq file
//
//  Arguments:  Defined in proc_fs.h
//


static int pd_proc_show (struct seq_file *sfp, void *vp)
{
   int i;
   char modelname[64];
   u32 t;
   int id;

   // Let's print information about PD boards installed
   seq_printf(sfp, "PowerDAQ Driver, version %d.%d.%d\n\n", PD_VERSION_MAJOR, PD_VERSION_MINOR, PD_VERSION_EXTRA);
   seq_printf(sfp, "PowerDAQ Info: %d board(s) installed\n\n", num_pd_boards);
   for (i = 0; i < num_pd_boards; i++)
   {     
      if((id = pd_enum_devices(i, modelname, 64, NULL, 0))<0)
         return 0;
      
      // print board name and s/n
      seq_printf(sfp, "PowerDAQ board #%d type:\t%18s\n\ts/n:\t%8s\n",
                    i+1,
                    modelname,
                    pd_board[i].Eeprom.u.Header.SerialNumber);

      // print ADC FIFO size
      if(PD_IS_DIO(id) || PD_IS_MFX(id) || PDL_IS_DIO(id) || PDL_IS_MFX(id))
      {
          seq_printf(sfp, "\tInput Fifo size:\t%d samples\n\tCL FIFO Size:\t%d entries\n",
                        pd_board[i].Eeprom.u.Header.ADCFifoSize*1024,
                        pd_board[i].Eeprom.u.Header.CLFifoSize*256);
      }

      // print DAC FIFO size
      seq_printf(sfp, "\tOutput Fifo size:\t%d samples\n",
                    pd_board[i].Eeprom.u.Header.DACFifoSize*1024);


      // print mfg and cal dates
      seq_printf(sfp, "\tMfg. date:\t%12s\n\tCal. date:\t%12s\n",
                    pd_board[i].Eeprom.u.Header.ManufactureDate,
                    pd_board[i].Eeprom.u.Header.CalibrationDate);

      // print base address and IRQ
      seq_printf(sfp, "\tBase address:\t0x%x\n\tIRQ line:\t0x%x\n",
                    pd_board[i].PCI_Config.BaseAddress0,
                    pd_board[i].PCI_Config.InterruptLine);

      // print DSP revision
      seq_printf(sfp, "\tDSP: v%d\n", (pd_board[i].fwTimestamp[0]&0x0F0000)>>16);

      // print firmware revision
      t = pd_board[i].fwTimestamp[0] >> 20;
      seq_printf(sfp, "\tFirmware type:\t%s rev: %d.%d/%x\n",
                    (t<1)?"Generic":((t<2)?"AO":((t<3)?"DIO":((t<4)?"LMF":((t<5)?"MFx":"Unknown")))),
                    (pd_board[i].fwTimestamp[0]&0xF00)>>8, (pd_board[i].fwTimestamp[0]&0xFF), 
                    pd_board[i].fwTimestamp[1]);

      // print Logic Revision
      seq_printf(sfp, "\tLogic rev:\t0x%x\n",
                    pd_board[i].logicRev);

      // print xfer mode
      seq_printf(sfp, "\tTransfer mode:\t%d\n\n",
                    pd_board[i].dwXFerMode);
   }
   return 0;
}

static int pd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pd_proc_show, NULL);
}

static const struct file_operations pd_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= pd_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#else

///////////////////////////////////////////////////////////////////////
//
//       Name:  pd_proc_read/pd_get_info
//
//   Function:  This gets called when the user-space process accesses
//              /proc/powerdaq file
//
//  Arguments:  Defined in proc_fs.h
//
#define LIMITPAGE (PAGE_SIZE-80)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
static int pd_proc_read (char *buf, char **start, off_t offset, int len, int unused)
#else
static int pd_get_info (char *buf, char **start, off_t offset, int len)
#endif
{
   int i;
   char modelname[64];
   u32 t;
   int id;

   // Let's print information about PD boards installed
   i = 0; len = 0;
   len += sprintf(buf+len, "PowerDAQ Driver, version %d.%d.%d\n\n", PD_VERSION_MAJOR, PD_VERSION_MINOR, PD_VERSION_EXTRA);
   len += sprintf(buf+len, "PowerDAQ Info: %d board(s) installed\n\n", num_pd_boards);
   for (i = 0; i < num_pd_boards; i++)
   {     
      if((id = pd_enum_devices(i, modelname, 64, NULL, 0))<0)
         return 0;
      
      // print board name and s/n
      len+= sprintf(buf+len, "PowerDAQ board #%d type:\t%18s\n\ts/n:\t%8s\n",
                    i+1,
                    modelname,
                    pd_board[i].Eeprom.u.Header.SerialNumber);
      if (len > LIMITPAGE) return len;

      // print ADC FIFO size
      if(PD_IS_DIO(id) || PD_IS_MFX(id) || PDL_IS_DIO(id) || PDL_IS_MFX(id))
      {
          len+= sprintf(buf+len, "\tInput Fifo size:\t%d samples\n\tCL FIFO Size:\t%d entries\n",
                        pd_board[i].Eeprom.u.Header.ADCFifoSize*1024,
                        pd_board[i].Eeprom.u.Header.CLFifoSize*256);
          if (len > LIMITPAGE) return len;
      }

      // print DAC FIFO size
      len+= sprintf(buf+len, "\tOutput Fifo size:\t%d samples\n",
                    pd_board[i].Eeprom.u.Header.DACFifoSize*1024);
      if (len > LIMITPAGE) return len;


      // print mfg and cal dates
      len+= sprintf(buf+len, "\tMfg. date:\t%12s\n\tCal. date:\t%12s\n",
                    pd_board[i].Eeprom.u.Header.ManufactureDate,
                    pd_board[i].Eeprom.u.Header.CalibrationDate);
      if (len > LIMITPAGE) return len;

      // print base address and IRQ
      len+= sprintf(buf+len, "\tBase address:\t0x%x\n\tIRQ line:\t0x%x\n",
                    pd_board[i].PCI_Config.BaseAddress0,
                    pd_board[i].PCI_Config.InterruptLine);
      if (len > LIMITPAGE) return len;

      // print DSP revision
      len+= sprintf(buf+len, "\tDSP: v%d\n", (pd_board[i].fwTimestamp[0]&0x0F0000)>>16);
      if (len > LIMITPAGE) return len;

      // print firmware revision
      t = pd_board[i].fwTimestamp[0] >> 20;
      len+= sprintf(buf+len, "\tFirmware type:\t%s rev: %d.%d/%x\n",
                    (t<1)?"Generic":((t<2)?"AO":((t<3)?"DIO":((t<4)?"LMF":((t<5)?"MFx":"Unknown")))),
                    (pd_board[i].fwTimestamp[0]&0xF00)>>8, (pd_board[i].fwTimestamp[0]&0xFF), 
                    pd_board[i].fwTimestamp[1]);
      if (len > LIMITPAGE) return len;

      // print Logic Revision
      len+= sprintf(buf+len, "\tLogic rev:\t0x%x\n",
                    pd_board[i].logicRev);
      if (len > LIMITPAGE) return len;

      // print xfer mode
      len+= sprintf(buf+len, "\tTransfer mode:\t%d\n\n",
                    pd_board[i].dwXFerMode);
      if (len > LIMITPAGE) return len;
   }
   return len;
}

#endif

///////////////////////////////////////////////////////////////////////////

int pd_mmap(struct file *file, struct vm_area_struct *vma)
{
   int real_minor, board, board_minor;
   int ret;
   void* buf;
   TBuf_Info  *pDaqBuf; 

   // find out what are we talking about
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
   real_minor = iminor(file->f_path.dentry->d_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
   real_minor = iminor(file->f_dentry->d_inode);
#else
   real_minor = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
   board = real_minor / PD_MINOR_RANGE;
   board_minor = real_minor % PD_MINOR_RANGE;

   DPRINTK_I("trying to mmap board %d, %s (minor %u)\n",
             board, pd_devices_by_minor[board_minor], real_minor);

   // now let's check - was ain or aout buffer allocated
   switch (board_minor)
   {
   case PD_MINOR_AIN:
   case PD_MINOR_DIN:
   case PD_MINOR_UCT:
      if (!pd_board[board].AinSS.BufInfo.databuf)
         return -EIO;

      pDaqBuf = &pd_board[board].AinSS.BufInfo;
      break;

   case PD_MINOR_AOUT:
   case PD_MINOR_DOUT:
      if (!pd_board[board].AoutSS.BufInfo.databuf)
         return -EIO;

      pDaqBuf = &pd_board[board].AoutSS.BufInfo;
      break;
   default:
      return -ENOSYS;
   }

   if (!pDaqBuf)
      return -EFAULT;

   buf = pDaqBuf->databuf;

   // map it!
   if ((ret = rvmmap(buf, pDaqBuf->BufSizeInBytes, vma)) < 0)
   {
      DPRINTK_F("rvmmap fails with %d\n", ret);
      return ret;
   } else
   {
      DPRINTK_F("rvmmap succeed with %d\n", ret);
      return 0;
   }
}

///////////////////////////////////////////////////////////////////////////
struct file_operations pd_fops = 
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

//   owner:	THIS_MODULE,
#endif
   .llseek =  NULL,     
   .read =    pd_read,  
   .write =   pd_write, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
   .unlocked_ioctl = pd_ioctl,
#else
   .ioctl =   pd_ioctl, 
#endif
   .mmap =    pd_mmap,  
   .open =    pd_open,  
   .flush =   NULL, 
   .release = pd_release, 
   .fsync =   NULL,
   .fasync =  pd_fasync,  
};


//////////////////////////////////////////////////////////////////////////
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
static struct proc_dir_entry pd_proc_entry = 
{
   0,                            /* low_ino    */
   sizeof("pwrdaq")-1,           /* namelen    */
   "pwrdaq",                     /* name       */
   S_IFREG | S_IRUGO,            /* mode       */
   1,                            /* nlink      */
   0,                            /* uid        */
   0,                            /* gid        */
   0,                            /* size       */
   NULL,                         /* ops        */
   pd_proc_read,                 /* get_info   */
   NULL,                         /* fill_inode */
   NULL,                         /* next       */
   NULL,                         /* parent     */
   NULL,                         /* subdir     */
   NULL                          /* data       */
};
#endif

//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
//       name:  init_module
//
//   function:  Initialize the hardware and the device driver.
//
//  arguments:  None.
//
//    returns:  0 if the module loaded successfully, and 1 if not.
//
static int powerdaq_init(void) 
{
   struct pci_dev *dev = NULL;
   int i, j, ret;
   
   PRINTK("PowerDAQ Driver %d.%d.%d, Copyright (C) 2000,2009 United Electronic Industries, Inc.\n",
          PD_VERSION_MAJOR, PD_VERSION_MINOR, PD_VERSION_EXTRA);

   num_pd_boards = 0;
#if defined(_PD_RTL)
   pthread_spin_init(&pd_fw_lock, 0);
#elif defined(_PD_RTLPRO)
   rtl_pthread_spin_init(&pd_fw_lock, 0);
#elif defined(_PD_RTAI)
   pd_fw_lock = SPIN_LOCK_UNLOCKED;
#elif defined(_PD_XENOMAI)
   pd_fw_lock = RTDM_LOCK_UNLOCKED;
#else
   pd_fw_lock = (spinlock_t)SPIN_LOCK_UNLOCKED;
#endif

   // Enumerate all PCI devices equipped with a Motorola DSP56301 chip.
   // We use the sub-vendor id to determine if it is a UEI device.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
   while ((dev = pci_get_device(MOTOROLA_VENDORID, DSP56301_DEVICEID, dev)))
#else
   while ((dev = pci_find_device(MOTOROLA_VENDORID, DSP56301_DEVICEID, dev)))
#endif   
   {
      deal_with_device(dev);
   }

   if (num_pd_boards == 0)
   {
      PRINTK("No PowerDAQ board was found\n");
      return -ENODEV;
   }

//#if !defined(_PD_RTLPRO)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
   // initializes the pwrdaq class and add an entry for each
   // device's subsystem in /sys/class/pwrdaq
   // This will allow udev to automatically create device nodes
   // in /dev when the pwrdaq driver is loaded
   pwrdaq_class = class_create(THIS_MODULE, "pwrdaq");
   if(IS_ERR(pwrdaq_class))
   {
      PRINTK("Error Creating pwrdaq class.\n");
      return PTR_ERR(pwrdaq_class); 
   }

   for (i = 0; i < num_pd_boards; i++)
   {
      // Create a device class for each sub-system
      for(j=0; j<PD_MINOR_RANGE; j++)
      {
         int pd_minor = i*PD_MINOR_RANGE + j;
         struct class_device* cldev; 
         
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
         cldev = device_create(pwrdaq_class,
                                     NULL,
                                     MKDEV(pd_major, pd_minor),
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26)
                                     /*pd_board[i].dev->dev*/NULL, 
#endif
                                     "pd-c%d-%s", 
                                     i, 
                                     pd_devices_by_minor[j]);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
         cldev = class_device_create(pwrdaq_class,
                                     NULL,
                                     MKDEV(pd_major, pd_minor),
                                     /*pd_board[i].dev->dev*/NULL, 
                                     "pd-c%d-%s", 
                                     i, 
                                     pd_devices_by_minor[j]);
#else
         cldev = class_device_create(pwrdaq_class, 
                                    MKDEV(pd_major, pd_minor),
                                    /*pd_board[i].dev->dev*/NULL, 
                                    "pd-c%d-%s", 
                                    i, 
                                    pd_devices_by_minor[j]);
#endif
         if(IS_ERR(cldev))
         {
            PRINTK("Can't add device class, major=%d, minor=%d\n", pd_major, pd_minor);
            class_destroy(pwrdaq_class);
            return PTR_ERR(cldev);
         }
      }
   }
#endif

//#endif

   // register the device with the kernel
   DPRINTK_N("registering driver, major number %d\n>\n>\n", pd_major);
   if ((ret = register_chrdev(pd_major, "powerdaq", &pd_fops)))
   {
      DPRINTK_F("unable to get major number %d\n", pd_major);
      return -EBUSY;
   }

// Register the driver with realtime kernel
#if defined(_PD_XENOMAI) || defined(_PD_RTLPRO)
   for (i = 0; i < num_pd_boards; i++)
   {
      int minor, ret;
      for(minor = 0; minor<PD_MINOR_RANGE; minor++)
      {
         ret = pd_driver_register(i, minor);
         if(ret)
         {
            DPRINTK("pd_driver_register failed for board%d, subsystem %s\n",
                    i, pd_devices_by_minor[minor]);
            return ret;
         }
      }
   }
#endif

   // initialize /proc/powerdaq file
//#if !defined(_PD_RTLPRO)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
   proc_register(&proc_root, &pd_proc_entry);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
   create_proc_info_entry("pwrdaq", 0, NULL, pd_get_info);
#else
   proc_create("pwrdaq", 0, NULL, &pd_proc_fops);
#endif
//#endif

   // prepare the bottom half task
#if defined(_PD_RTL)
   if(rqstirq)
   {
      rtl_allow_interrupts();
   }
#elif  defined(_PD_RTLPRO)
   if(rqstirq)
   {
      rtl_allow_interrupts();
   }
#endif

   return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//       name:  cleanup_module
//
//   function:  Shut down the hardware and the device driver.
//
//  arguments:  None.
//
//    returns:  Nothing.
//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void __exit powerdaq_cleanup(void) 
#else
static void powerdaq_cleanup(void) 
#endif
{
   int i, j;
   tAllocContigMem Mem;
   
   DPRINTK_N("unloading PowerDAQ driver...\n");

   for (i = 0; i < num_pd_boards; i ++)
   {
      DPRINTK_N("shutting down card %d\n", i);

      pd_reset_dsp(i);
            
      //-------------------------------------------------------------------
      // Release memory chunks for bus-mastering operations
      Mem.idx = AI_PAGE0;
      pd_dealloc_contig_memory(i, &Mem);

      Mem.idx = AI_PAGE1;
      pd_dealloc_contig_memory(i, &Mem);

      Mem.idx = AO_PAGE0;
      pd_dealloc_contig_memory(i, &Mem);

      // free the IRQ
      if(rqstirq)
      {
         pd_driver_release_irq(i);
      }

      // free up the memory allocated for the synch objects
      pd_event_destroy(i, pd_board[i].AinSS.synch);
      pd_event_destroy(i, pd_board[i].AoutSS.synch);
      pd_event_destroy(i, pd_board[i].DinSS.synch);
      pd_event_destroy(i, pd_board[i].DoutSS.synch);
      pd_event_destroy(i, pd_board[i].UctSS.synch);

      // free up the remapped pages
      iounmap(pd_board[i].address);
   }

   // cleanup /sys/class directory
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
   for (i = 0; i < num_pd_boards; i ++)
   {
      for(j=0; j<PD_MINOR_RANGE; j++)
      {
         class_device_destroy(pwrdaq_class, MKDEV(pd_major, i*PD_MINOR_RANGE+j));
      }
   }
   class_destroy(pwrdaq_class);
#endif

#if defined(_PD_XENOMAI) || defined(_PD_RTLPRO)
   for (i = 0; i < num_pd_boards; i ++)
   {
      int minor;
      for (minor = 0; minor < PD_MINOR_RANGE; minor++)
      {
         pd_driver_unregister(i, minor);
      }
   }
#endif

   // stop /proc/powerdaq file operations
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
   proc_unregister(&proc_root, pd_proc_entry.low_ino);
#else
   remove_proc_entry("pwrdaq", NULL);
#endif

   unregister_chrdev(pd_major, "powerdaq");
   DPRINTK_N("PowerDAQ driver unloaded\n>\n>\n>\n");
}

void pd_clean_device(int board)
{
#if defined(_PD_RTL)
    pthread_mutex_unlock(&pd_board[board].AinSS.synch->event_lock);
    pthread_mutex_unlock(&pd_board[board].AoutSS.synch->event_lock);
    pthread_mutex_unlock(&pd_board[board].DinSS.synch->event_lock);
    pthread_mutex_unlock(&pd_board[board].DoutSS.synch->event_lock);
    pthread_mutex_unlock(&pd_board[board].UctSS.synch->event_lock);
#endif
}

#if defined(_PD_RTLPRO)
// Under RTLinux Pro the PowerDAQ driver is compiled as 
// a regular .rtl application.
// You can launch the driver with the command ./pwrdaq.rtl &
int main(int argc, char* argv[])
{
   int i=0;

   for(i=1; i<argc; i++)
   {
      char* equal = rtl_strchr(argv[i], '=');
      if(equal != NULL)
      {
	 char* paramValue = equal+1;
	 if(!rtl_strncmp(argv[i], "xferMode", strlen("xferMode")))
	 {
	    xferMode = rtl_atoi(paramValue);
	    DPRINTK("xferMode set to %d\n", xferMode);
	 }

	 if(!rtl_strncmp(argv[i], "rqstirq", strlen("rqstirq")))
	 {
	    rqstirq = rtl_atoi(paramValue);
	    DPRINTK("rqstirq set to %d\n", rqstirq);
	 }
      }
   }
   powerdaq_init();

   // wait for module to be unloaded 
   rtl_main_wait();

   powerdaq_cleanup();

   return 0;
}
#else
// Under Linux, RTLinux free and RTAI the PowerDAQ driver is a
// regular kernel module.
// You can load and unload the module with the commands:
// insmod pwrdaq.ko/ rmmod pwrdaq
module_init(powerdaq_init);
module_exit(powerdaq_cleanup);
#endif

// EXPORT_SYMBOL_NOVERS was removed from kernel 2.6.11
// We still need it for kernels 2.4.x
#ifndef EXPORT_SYMBOL_NOVERS
   #define EXPORT_SYMBOL_NOVERS EXPORT_SYMBOL
#endif

EXPORT_SYMBOL_NOVERS(pd_board);
EXPORT_SYMBOL_NOVERS(pd_ain_set_config);
EXPORT_SYMBOL_NOVERS(pd_ain_set_cv_clock);
EXPORT_SYMBOL_NOVERS(pd_ain_set_cl_clock);
EXPORT_SYMBOL_NOVERS(pd_ain_set_channel_list);
EXPORT_SYMBOL_NOVERS(pd_ain_set_BM_ctr);
EXPORT_SYMBOL_NOVERS(pd_ain_get_BM_ctr);
EXPORT_SYMBOL_NOVERS(pd_ain_set_events);
EXPORT_SYMBOL_NOVERS(pd_ain_get_status);
EXPORT_SYMBOL_NOVERS(pd_ain_sw_start_trigger);
EXPORT_SYMBOL_NOVERS(pd_ain_sw_stop_trigger);
EXPORT_SYMBOL_NOVERS(pd_ain_set_enable_conversion);
EXPORT_SYMBOL_NOVERS(pd_ain_get_value);
EXPORT_SYMBOL_NOVERS(pd_ain_set_ssh_gain);
EXPORT_SYMBOL_NOVERS(pd_ain_get_samples);
EXPORT_SYMBOL_NOVERS(pd_ain_reset);
EXPORT_SYMBOL_NOVERS(pd_ain_sw_cl_start);
EXPORT_SYMBOL_NOVERS(pd_ain_sw_cv_start);
EXPORT_SYMBOL_NOVERS(pd_ain_reset_cl);
EXPORT_SYMBOL_NOVERS(pd_ain_clear_data);
EXPORT_SYMBOL_NOVERS(pd_ain_flush_fifo);
EXPORT_SYMBOL_NOVERS(pd_ain_get_xfer_samples);
EXPORT_SYMBOL_NOVERS(pd_ain_set_xfer_size);
EXPORT_SYMBOL_NOVERS(pd_aout_set_config);
EXPORT_SYMBOL_NOVERS(pd_aout_set_cv_clk);
EXPORT_SYMBOL_NOVERS(pd_aout_set_events);
EXPORT_SYMBOL_NOVERS(pd_aout_get_status);
EXPORT_SYMBOL_NOVERS(pd_aout_set_enable_conversion);
EXPORT_SYMBOL_NOVERS(pd_aout_sw_start_trigger);
EXPORT_SYMBOL_NOVERS(pd_aout_sw_stop_trigger);
EXPORT_SYMBOL_NOVERS(pd_aout_sw_cv_start);
EXPORT_SYMBOL_NOVERS(pd_aout_clear_data);
EXPORT_SYMBOL_NOVERS(pd_aout_reset);
EXPORT_SYMBOL_NOVERS(pd_aout_put_value);
EXPORT_SYMBOL_NOVERS(pd_aout_put_block);
EXPORT_SYMBOL_NOVERS(pd_adapter_enable_interrupt);
EXPORT_SYMBOL_NOVERS(pd_adapter_acknowledge_interrupt);
EXPORT_SYMBOL_NOVERS(pd_adapter_get_board_status);
EXPORT_SYMBOL_NOVERS(pd_adapter_set_board_event1);
EXPORT_SYMBOL_NOVERS(pd_adapter_set_board_event2);
EXPORT_SYMBOL_NOVERS(pd_adapter_eeprom_read);
EXPORT_SYMBOL_NOVERS(pd_adapter_eeprom_write);
EXPORT_SYMBOL_NOVERS(pd_cal_dac_write);
EXPORT_SYMBOL_NOVERS(pd_dsp_get_status);
EXPORT_SYMBOL_NOVERS(pd_dsp_get_flags);
EXPORT_SYMBOL_NOVERS(pd_dsp_set_flags);
EXPORT_SYMBOL_NOVERS(pd_dsp_command);
EXPORT_SYMBOL_NOVERS(pd_dsp_cmd_no_ret);
EXPORT_SYMBOL_NOVERS(pd_dsp_write);
EXPORT_SYMBOL_NOVERS(pd_dsp_read);
EXPORT_SYMBOL_NOVERS(pd_dsp_cmd_ret_ack);
EXPORT_SYMBOL_NOVERS(pd_dsp_cmd_ret_value);
EXPORT_SYMBOL_NOVERS(pd_dsp_read_ack);
EXPORT_SYMBOL_NOVERS(pd_dsp_write_ack);
EXPORT_SYMBOL_NOVERS(pd_dsp_cmd_write_ack);
EXPORT_SYMBOL_NOVERS(pd_dsp_int_status);
EXPORT_SYMBOL_NOVERS(pd_dsp_acknowledge_interrupt);
EXPORT_SYMBOL_NOVERS(pd_dsp_startup);
EXPORT_SYMBOL_NOVERS(pd_init_calibration);
EXPORT_SYMBOL_NOVERS(pd_reset_dsp);
EXPORT_SYMBOL_NOVERS(pd_download_firmware_bootstrap);
EXPORT_SYMBOL_NOVERS(pd_reset_board);
EXPORT_SYMBOL_NOVERS(pd_download_firmware);
EXPORT_SYMBOL_NOVERS(pd_echo_test);
EXPORT_SYMBOL_NOVERS(pd_dsp_reg_write);
EXPORT_SYMBOL_NOVERS(pd_dsp_reg_read);
EXPORT_SYMBOL_NOVERS(pd_din_set_config);
EXPORT_SYMBOL_NOVERS(pd_din_set_event);
EXPORT_SYMBOL_NOVERS(pd_din_clear_events);
EXPORT_SYMBOL_NOVERS(pd_din_read_inputs);
EXPORT_SYMBOL_NOVERS(pd_din_clear_data);
EXPORT_SYMBOL_NOVERS(pd_din_reset);
EXPORT_SYMBOL_NOVERS(pd_din_status);
EXPORT_SYMBOL_NOVERS(pd_dout_write_outputs);
EXPORT_SYMBOL_NOVERS(pd_dio256_write_output);
EXPORT_SYMBOL_NOVERS(pd_dio256_read_input);
EXPORT_SYMBOL_NOVERS(pd_dio_dmaSet);
EXPORT_SYMBOL_NOVERS(pd_dio256_setIntrMask);
EXPORT_SYMBOL_NOVERS(pd_dio256_getIntrData);
EXPORT_SYMBOL_NOVERS(pd_dio256_intrEnable);
EXPORT_SYMBOL_NOVERS(pd_dio256_make_reg_mask);
EXPORT_SYMBOL_NOVERS(pd_ao96_writex);
EXPORT_SYMBOL_NOVERS(pd_ao32_writex);
EXPORT_SYMBOL_NOVERS(pd_ao96_reset);
EXPORT_SYMBOL_NOVERS(pd_ao32_reset);
EXPORT_SYMBOL_NOVERS(pd_ao96_write);
EXPORT_SYMBOL_NOVERS(pd_ao32_write);
EXPORT_SYMBOL_NOVERS(pd_ao96_write_hold);
EXPORT_SYMBOL_NOVERS(pd_ao32_write_hold);
EXPORT_SYMBOL_NOVERS(pd_ao96_update);
EXPORT_SYMBOL_NOVERS(pd_ao32_update);
EXPORT_SYMBOL_NOVERS(pd_ao96_set_update_channel);
EXPORT_SYMBOL_NOVERS(pd_ao32_set_update_channel);
EXPORT_SYMBOL_NOVERS(pd_dio_reset);
EXPORT_SYMBOL_NOVERS(pd_dio_enable_output);
EXPORT_SYMBOL_NOVERS(pd_dio_latch_all);
EXPORT_SYMBOL_NOVERS(pd_dio_simple_read);
EXPORT_SYMBOL_NOVERS(pd_dio_read);
EXPORT_SYMBOL_NOVERS(pd_dio_write);
EXPORT_SYMBOL_NOVERS(pd_dio_prop_enable);
EXPORT_SYMBOL_NOVERS(pd_dio_latch_enable);
EXPORT_SYMBOL_NOVERS(pd_init_pd_board);
EXPORT_SYMBOL_NOVERS(pd_uct_set_config);
EXPORT_SYMBOL_NOVERS(pd_uct_set_event);
EXPORT_SYMBOL_NOVERS(pd_uct_clear_event);
EXPORT_SYMBOL_NOVERS(pd_uct_get_status);
EXPORT_SYMBOL_NOVERS(pd_uct_write);
EXPORT_SYMBOL_NOVERS(pd_uct_read);
EXPORT_SYMBOL_NOVERS(pd_uct_set_sw_gate);
EXPORT_SYMBOL_NOVERS(pd_uct_sw_strobe);
EXPORT_SYMBOL_NOVERS(pd_uct_reset);
EXPORT_SYMBOL_NOVERS(pd_stop_and_disable_ain);
EXPORT_SYMBOL_NOVERS(pd_dspct_load);
EXPORT_SYMBOL_NOVERS(pd_dspct_enable_counter);
EXPORT_SYMBOL_NOVERS(pd_dspct_enable_interrupts);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_count);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_compare);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_status);
EXPORT_SYMBOL_NOVERS(pd_dspct_set_compare);
EXPORT_SYMBOL_NOVERS(pd_dspct_set_load);
EXPORT_SYMBOL_NOVERS(pd_dspct_set_status);
EXPORT_SYMBOL_NOVERS(pd_dsp_PS_load);
EXPORT_SYMBOL_NOVERS(pd_dsp_PS_get_count);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_count_addr);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_load_addr);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_status_addr);
EXPORT_SYMBOL_NOVERS(pd_dspct_get_compare_addr);
EXPORT_SYMBOL_NOVERS(pd_enable_events);
EXPORT_SYMBOL_NOVERS(pd_disable_events);
EXPORT_SYMBOL_NOVERS(pd_set_user_events);
EXPORT_SYMBOL_NOVERS(pd_clear_user_events);
EXPORT_SYMBOL_NOVERS(pd_get_user_events);
EXPORT_SYMBOL_NOVERS(pd_notify_event);
EXPORT_SYMBOL_NOVERS(pd_sleep_on_event);
EXPORT_SYMBOL_NOVERS(pd_enum_devices);
EXPORT_SYMBOL_NOVERS(pd_clean_device);
EXPORT_SYMBOL_NOVERS(pd_get_board_object);



