//===========================================================================
//
// NAME:    powerdaq32.c
//
// DESCRIPTION:
//
//          PowerDAQ Linux library implementation file
//
//
// AUTHOR:  Alex Ivchenko
//
// DATE:    12-APR-2000
//
// REV:     0.87
//
// R DATE:
//
// HISTORY:
//
//      Rev 0.8,     12-MAR-2000,     Initial version.
//      Rev 0.83     23-MAY-2000
//      Rev 0.87,    18-JUN-2000,     mmap() added, buffering changed, RT-comp.
//      Rev 0.9      26-JAN-2001,     DSP register access functions added
//
//---------------------------------------------------------------------------
//      Copyright (C) 2000 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------

#ifdef _PD_RTLPRO
#include "../include/powerdaq_kernel.h"
#else
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/win_sdk_types.h"

// handle to the shared mory used to store the adapter infos
static int shmid;

// key to reserve the shared memory
static const key_t shmkey = 0x44455246;
#endif

#include "../include/powerdaq.h"
#include "../include/powerdaq32.h"
#include "../include/pd_debug.h"

void __attribute__ ((constructor)) my_init(void);
void __attribute__ ((destructor)) my_fini(void);

#if defined _PD_XENOMAI
   #include <rtdm/rtdm.h>
   #define PD_IOCTL rt_dev_ioctl
   #define PD_OPEN rt_dev_open
   #define PD_CLOSE rt_dev_close
   #define PD_DEV_PREFIX "rt-pd"
   #define PD_OPEN_FLAGS O_RDWR
#elif defined _PD_RTLPRO
   #define PD_IOCTL rtl_ioctl
   #define PD_OPEN rtl_open
   #define PD_CLOSE rtl_close
   #define PD_DEV_PREFIX "/dev/rt-pd"
   #define PD_OPEN_FLAGS RTL_O_RDWR
#else
   #define PD_IOCTL ioctl
   #define PD_OPEN open
   #define PD_CLOSE close
   #define PD_DEV_PREFIX "/dev/pd"
   #define PD_OPEN_FLAGS O_RDWR
#endif

// Points to an array of infos for each adapter
// It is allocated by the first process that loads
// the library and freed by the last one
Adapter_Info *G_pAdapterInfo = NULL;
int G_NbBoards = 0;

#define DIO_REGS_NUM    8

//+
// int PdGetVersion(PPWRDAQ_VERSION pVersion)
// Returns version of the driver/library
//
// Arguments: None
// Parameters: PPWRDAQ_VERSION pVersion -- driver/dll version info
//
// Returns: Negative error code or 0
//
//-
int PdGetVersion(PPWRDAQ_VERSION pVersion)
{
    pVersion->SystemSize = 0; // fixme: physical RAM available
    pVersion->NtServerSystem = FALSE;
    pVersion->NumberProcessors = 1; // fixme: number of processors
    pVersion->MajorVersion = PD_VERSION_MAJOR;     // Win32 DLL compatibility version
    pVersion->MinorVersion = PD_VERSION_MINOR;    //
    return TRUE;
}

//+
// int PdGetNumberAdapters( void )
// Returns number of PowerDAQ adapters installed
//
// Arguments: None
//
// Returns: negative error code or number of adapters
//-
int PdGetNumberAdapters( void )
{
    int ret;
    int pd_ain_fd = 0;
    tCmd cmd;


    pd_ain_fd = PD_OPEN(PD_DEV_PREFIX "-c0-drv", PD_OPEN_FLAGS);
    if (pd_ain_fd < 0)
    {
       DPRINTK("PD_OPEN returned fd=%d (%s)\n", pd_ain_fd, strerror(errno));
       return -1;
    }

    ret = PD_IOCTL(pd_ain_fd, IOCTL_PWRDAQ_GET_NUMBER_ADAPTER, &cmd);
    
    DPRINTK("PdGetNumberAdapters: fd=%d, boards=%d, ret=%d\n", pd_ain_fd, cmd.dwParam[0], ret);

    PD_CLOSE(pd_ain_fd);

    if(ret < 0) 
    {
        return ret;
    }

    return cmd.dwParam[0];
}


//+
// int PdGetPciConfiguration(int handle, PPWRDAQ_PCI_CONFIG pPciConfig)
// Returns PCI configuration
//
// Arguments:  int board handle
// Parameters: PPWRDAQ_PCI_CONFIG pPciConfig - pointer to where to
//             copy PCI configuration
//
// Note: To get PCI configuration you must open BoardLevel ("Drv") subsystem
//
// Returns: Negative error code or 0
//-
int PdGetPciConfiguration(int handle, PPWRDAQ_PCI_CONFIG pPciConfig)
{
    int ret;
    tCmd cmd;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_PRIVATE_GETCFG, &cmd);
    memcpy(pPciConfig, &cmd.PciConfig, sizeof(PWRDAQ_PCI_CONFIG));
    return ret;
}


//+
// int PdAcquireSubsystem(int board, int dwSubsystem, int Action)
//
// Opens/closes and gets/releases subsystem into/from process control
//
// Arguments: int board
//            int subsystem
//            int action -- == 1 get ownership / == 0 release it
//
//     for open: int board is board ID [0..Max]
//     for release: int board is handle received upon opening
//
// Returns: handle to adapter/subsystem or negative error
//
// Note: there are 6 subsystems defined: AnalogIn, AnalogOut, DigitalIn,
//       DigitalOut, CounterTimer and BoardLevel
//       Subsystem must be opened before accessing it. Driver stores
//       handle to the subsystem and checks it before accessing.
//       Thus, there's no such thing as sharing of subsystem among
//       processes are available.
//-
int PdAcquireSubsystem(int board, int dwSubsystem, int Action)
{
   int  fd;
   int  ret=0;
   char device_file[64];
   char subsystem[10];
   tCmd   Cmd;

   fd = -1;
   switch (dwSubsystem)
   {
   case AnalogIn:
      sprintf(subsystem, "ain");
      break;

   case AnalogOut:
      sprintf(subsystem, "aout");
      break;

   case DigitalIn:
      sprintf(subsystem, "din");
      break;

   case DigitalOut:
      sprintf(subsystem, "dout");
      break;

   case CounterTimer:
      sprintf(subsystem, "uct");
      break;

   case DSPCounter:
      sprintf(subsystem, "uct");
      break;

   case BoardLevel:
      sprintf(subsystem, "drv");
      break;

   default:
      return -ENOSYS;
   }

   if (Action) // open ss
   {
      // Get subsystem in use
      sprintf(device_file, PD_DEV_PREFIX "-c%d-%s", board, subsystem);
      fd = PD_OPEN(device_file, PD_OPEN_FLAGS);
      
      DPRINTK("PD_OPEN returned fd=%d\n", fd);

      // Let driver store file handle
      if (fd >= 0)
      {
         Cmd.AcqSS.subSystem = dwSubsystem;
         Cmd.AcqSS.fileDescriptor = fd;
         ret = PD_IOCTL(fd, IOCTL_PWRDAQ_OPENSUBSYSTEM, &Cmd);
      }
      else
      {
         ret = fd;
         DPRINTK("PdAcquireSubsystem-open: error %d opening device %s\n", ret, device_file);
         return ret;
      }

      DPRINTK("PdAcquireSubsystem-open: handle %d: s/s %d ret=%d\n", fd, dwSubsystem, ret);

      return fd;
   }
   else
   { // close ss

      fd = board;  
      // Let driver release file handle
      if (fd >= 0)
      {
         Cmd.AcqSS.subSystem = dwSubsystem;
         Cmd.AcqSS.fileDescriptor = fd;
         ret = PD_IOCTL(fd, IOCTL_PWRDAQ_CLOSESUBSYSTEM, &Cmd);

         // Release subsystem
         PD_CLOSE(fd);
      }
      else
      {
         DPRINTK("PdAcquireSubsystem-close: invalid file handle %d\n", fd);
         ret = -2;
      }

      DPRINTK("PdAcquireSubsystem-close: handle %d: s/s %d ret=%d\n", fd, dwSubsystem, ret);

      return ret;
   }
}

//--- Event functions ----------------------------------------------------
//
//+
// Function:    _PdSetUserEvents
//
// Parameters:  int handle -- handle to adapter
//              PD_SUBSYSTEM Subsystem  -- subsystem type
//              DWORD dwEvents  -- IN: user events to set
//
// Returns: Negative error code or 0
//
// Description: The Set User Events function sets and enables event
//              notification of specified user defined DAQ events.
//
//              Event bit:  1 -- enable event notification upon assertion
//                               of this event and clear event status bit.
//                          0 -- no change to event configuration or status.
//
//              Setting an event for notification enables the hardware or
//              driver event for notification upon assertion and clears the
//              event status bit.
//
//              Once the event asserts and the status bit is set, the
//              DLL/User notification is triggered and the event is
//              automatically disabled from notification and must be set
//              again before DLL/User can be notified of its subsequent
//              assertion.
//
//              User events operate in latched mode and must be cleared
//              either by calling PdSetUserEvents or PdClearUserEvents
//              to clear the event status bits.
//
//
// Following events are defined for AnalogIn and AnalogOut subsystems:
//                AIn AOut
// eStartTrig      +  +   Start trigger received, operation started
// eStopTrig       +  +   Stop trigger received, operation stopped
// eInputTrig      -  -   Subsystem specific input trigger (if any)
// eDataAvailable  +  -   New data available
// eScanDone       -  -   Scan done (for future use)
// eFrameDone      +  +   One or more frames are done (or half of DAC FIFO is done)
// eFrameRecycled  +  -   Cyclic buffer frame recycled
//                        (i.e. an unread frame is over-written by new data)
// eBufferDone     +  +   Buffer done
// eBufferWrapped  +  -   Cyclic buffer wrapped
// eConvError      +  -   Conversion clock error - pulse came before board is ready
//                        to process it
// eScanError      +  -   Scan clock error
// eBufferError    +  +   Buffer over/under run error
// eStopped        +  +   Operation stopped (possibly because of error)
// eTimeout        +  -   Operation timed out
// eAllEvents      +  +   Set/clear all events
//
// Following events are defined for DIO and UC subsystems:
//              DIn UCT
// eDInEvent     +   -    Digital Input event
// eUct0Event    -   +    Uct0 countdown event
// eUct1Event    -   +    Uct1 countdown event
// eUct2Event    -   +    Uct2 countdown event
//
// Notes: Events are available for AIn, AOut, DIO and UCT should be set
//        separately
//-
int _PdSetUserEvents(int handle, PD_SUBSYSTEM Subsystem, DWORD dwEvents)
{
   tCmd   Cmd;
   int ret;

   Cmd.dwParam[0] = Subsystem;
   Cmd.dwParam[1] = dwEvents;
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_SET_USER_EVENTS, &Cmd);

   return ret;
}

//+
// Function:    _PdClearUserEvents
//
// Parameters:  int handle -- handle to adapter
//              PD_SUBSYSTEM Subsystem  -- subsystem type
//              DWORD dwEvents  -- IN: user events to clear
//
// Returns: Negative error code or 0
//
// Description: The Clear User Events function clears and disables event
//              notification of specified user defined DAQ events.
//
//              Event bit:  1 -- disable event notification of this event and
//                               clear the event status bit.
//                          0 -- no change to event configuration or status.
//
//              Clearing an event from notification disables the hardware
//              or driver event for notification upon assertion and clears
//              the event status bit.
//
//              All DLL calls waiting on the events that are cleared are
//              signalled.
//
//              This function can also be called to clear event status bits
//              on events that are checked by polling and were not enabled
//              for notification.
//
// Notes:       See _PdSetUserEvents for events definition
//-
int _PdClearUserEvents(int handle, PD_SUBSYSTEM Subsystem, DWORD dwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = Subsystem;
    Cmd.dwParam[1] = dwEvents;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_CLEAR_USER_EVENTS, &Cmd);

    return ret;
}

//+
// Function:    _PdGetUserEvents
//
// Parameters:  int handle -- handle to adapter
//              PD_SUBSYSTEM Subsystem  -- subsystem type
//              DWORD *pdwEvents -- OUT: ptr to user events
//
// Returns:     Negative error code or 0
//
// Description: The Get User Events function gets the current user event
//              status. The event configuration and status are not changed.
//
//              Event bit:  0 -- event had not asserted.
//                          1 -- event asserted.
//
//              User events are not automatically re-enabled. Clearing
//              and thus re-enabling of user events is initiated by DLL.
//
// Notes:       THIS FUNCTION GETS THE CURRENT EVENT STATUS, NOT THE QUEUED
//              EVENTS.
//              See _PdSetUserEvents for events definition
//-
int _PdGetUserEvents(int handle, PD_SUBSYSTEM Subsystem, DWORD *pdwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = Subsystem;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GET_USER_EVENTS, &Cmd);
    *pdwEvents = Cmd.dwParam[1];


    return ret;
}

//+
// Function:    _PdImmediateUpdate
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The Immediate Update function immediately updates the adapter
//              status, events, gets all samples acquired from adapter and
//              updates the latest sample counts.
//              Driver handles _PdImmediateUpdate like interrupt from the board,
//              so all event notification mechnisms will work properly
//
// Notes:       Use this function in following circumstances:
//
//              1. When acquisition rate is slow. Driver transfers data
//              when FIFO becomes half-full. In other words data will not
//              appear in the buffer until 512 samples (if default 1kS FIFO
//              is installed) are acquired. So, if you, say, select frame size
//              as big as 50 samples and your rate is 100Hz you'll get 11 frames
//              per event each 5.5 s. Thus, if you want to achieve better response
//              time, put _PdImmediateUpdate call in a timer loop.
//
//              2. When you want to clock acquisition externally and clock
//              frequency may vary it's a good idea to call _PdImmediateUpdate
//              periodically to see is there any scans available
//
//              3. _PdImmediateUpdate consumes some processor time. It's not
//              recommended to call this function more then 10 times a second
//              at the high acquisition rate (>100kS/s). With a low rate it
//              seems reasonable to call _PdImmediateUpdate with up to 1000Hz
//              rate.
//-
int _PdImmediateUpdate(int handle)
{
    return PD_IOCTL(handle, IOCTL_PWRDAQ_IMMEDIATE_UPDATE, NULL);
}

//+
// Function:    _PdAdapterGetBoardStatus
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD *pdwStatusBuf -- ptr to buffer to store event words
//
// Returns:     Negative error code or 0
//
// Description: The get board status command returns the status and
//              events of all subsystems, but does not disable or clear
//              any asserted board event bits.
//
//              All error conditions are included in the board events.
//
//              PARAM #1    <-- (5) fixed block size of 5 words
//              PARAM #2    <-- BrdStatus
//              PARAM #3    <-- combined registers PD_UDEIntr and PD_AUEStat
//              PARAM #4    <-- combined registers PD_AIOIntr1 and PD_AIOIntr2
//              PARAM #5    <-- AInIntrStat
//              PARAM #6    <-- AOutIntrStat
//
// Notes:       * The get board status command does not clear, enable, or
//                disable the PC Host interrupt or board events.
//
//              * The size of pdwStatusBuf should be 5 dwords.
//
//              * No ack is read after last data word is read.
//
//              * This routine is optimized for fastest execution possible.
//
//-
int _PdAdapterGetBoardStatus(int handle, tEvents* pEvents)
{
    tCmd   Cmd;
    int ret;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDSTATUS, &Cmd);

    *pEvents = Cmd.Event;

    return ret;
}

//+
// Function:    _PdAdapterSetBoardEvents1
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEvents  -- value of ADUEIntrStat Event configuration word
//
// Returns:     Negative error code or 0
//
// Description: The set board events 1 command sets selected UDEIntr register
//              event bits enabling/disabling and/or clearing individual
//              board level interrupt events, thereby re-enabling the event
//              interrupts.
//
//              Interrupt Mask (Im) bits:   0 = disable, 1 = enable interrupt
//              Status/Clear (SC) bits:     0 = clear interrupt, 1 = unchanged
//
// Notes:       1. This function is rarely used to call directly
//
//              2. The set board events 1 command does not clear, enable,
//                 or disable the PC Host interrupt.
//-
int _PdAdapterSetBoardEvents1(int handle, DWORD dwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = (DWORD)dwEvents;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDSETEVNTS1, &Cmd);

    return ret;
}

//+
// Function:    _PdAdapterSetBoardEvents2
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEvents  -- value of AIOIntr Event configuration word
//
// Returns:     Negative error code or 0
//
// Description: The set board events 2 command sets selected AIOIntr1 and
//              AIOIntr2 register event bits enabling/disabling and/or
//              clearing individual board level interrupt events, thereby
//              re-enabling the event interrupts.
//
//              Interrupt Mask (Im) bits:   0 = disable, 1 = enable interrupt
//              Status/Clear (SC) bits:     0 = clear interrupt, 1 = unchanged
//
//              In use: 1. Keep a copy of latest dwEvents word written.
//
//                      2. Boolean OR the dwEvents word to set all status
//                         (SC) bits to 1.
//
//                      3. To disable interrupts, change corresponding
//                         interrupt mask bits (Im) to 0, to enable, change
//                         mask bits to 1.
//
//                      4. To clear interrupt status bits (SC), re-enabling
//                         the interrupts, set the corresponding bits to 0.
//
//                      5. Save a copy of the new dwEvents word and issue
//                         command to set events.
//
// Notes:       1. This function is rarely used to call directly
//
//              2. Registers PD_AIOIntr1 and PD_AIOIntr2 are combined.
//
//              3. The set board events 2 command does not clear, enable,
//                or disable the PC Host interrupt.
//
//-
int _PdAdapterSetBoardEvents2(int handle, DWORD dwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = (DWORD)dwEvents;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDSETEVNTS2, &Cmd);

    return ret;
}

//+
// Function:    int _PdAdapterEnableInterrupt
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEnable  -- 0: disable, 1: enable Irq
//
// Returns:     Negative error code or 0
//
// Description: Enable or Disable board interrupt generation.
//              During interrupt generation, the PCI INTA line is
//              is asserted to request servicing of board events.
//
// Notes:       Interrupt generation is disabled following the assertion
//              of an interrupt and must be explicitly called to re-enable
//              assertion of subsequent interrupts.
//
//              This command does not service the interrupt, i.e., it
//              does not clear an asserted PCI INTA line.
//-
int _PdAdapterEnableInterrupt(int handle, DWORD dwEnable)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = (DWORD)dwEnable;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDENABLEINTERRUPT, &Cmd);

    return ret;
}


//+
// Function:    _PdAdapterEepromRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwMaxSize -- max buffer size in words
//              WORD *pwReadBuf -- ptr to buffer to save ID data
//              DWORD *pdwWords -- num words saved in buffer
//
// Returns:     WORDs read or negative value on error
//
// Description: Read board ID data block from onboard SPI EEPROM.
//
//              The data block returned contains all board identification,
//              configuration, calibration settings, PN, REV, SN, Firmware
//              Version, Calibration Date, and First Use Date in a 256
//              16-bit word block.
//
//-
int _PdAdapterEepromRead(int handle, DWORD dwMaxSize, WORD *pwReadBuf, DWORD *pdwWords)
{
    int ret, i;
    tCmd  cmd;

    cmd.EepromAcc.MaxSize = dwMaxSize;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDEEPROMREAD, &cmd);
    *pdwWords = cmd.EepromAcc.WordsRead;

    for (i = 0; (i < dwMaxSize)&&(i< cmd.EepromAcc.WordsRead)&&(i< PD_EEPROM_SIZE); i++)
        *(pwReadBuf+i) = cmd.EepromAcc.Buffer[i];
    return ret;
}

//+
// Function:    _PdAdapterEepromWrite
//
// Parameters:  int handle -- handle to adapter
//              WORD *wWriteBuf -- buffer containing data to write
//              DWORD dwSize    -- write buffer size in words
//
// Returns:     WORDs written or negative value on error
//
// Description: Write block to SPI EEPROM.
//              The EEPROM write command writes a data block containing
//              board ID and configuration information to the onboard
//              EEPROM.
//
// Notes:
//
//-
int _PdAdapterEepromWrite(int handle, WORD *pwWriteBuf, DWORD dwSize)
{
    int ret, i;
    tCmd  cmd;

    cmd.EepromAcc.MaxSize = dwSize;
    cmd.EepromAcc.WordsRead = dwSize;
    for (i = 0; (i < dwSize)&&(i< PD_EEPROM_SIZE); i++)
        cmd.EepromAcc.Buffer[i] = *(pwWriteBuf+i);

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDEEPROMWRITE, &cmd);
    return ret;
}

//+
// Function:    _PdAInAsyncInit
//
// Parameters:  int handle -- handle to adapter
//                  DWORD  dwAInCfg           -- IN: AIn configuration word
//                  DWORD  dwAInPreTrigCount  -- IN: pre-trigger scan count (always 0)
//                  DWORD  dwAInPostTrigCount -- IN: post-trigger scan count (always 0)
//                  DWORD  dwAInCvClkDiv      -- IN: conv. start clk div.
//                  DWORD  dwAInClClkDiv      -- IN: chan. list start clk div.
//                  DWORD  dwEventsNotify     -- IN: subsys user events notif.
//                  DWORD  dwChListChan       -- IN: number of channels in list
//                  DWORD* pdwChList          -- IN: channel list data buffer
//
// Returns:     Negative error code or 0
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
// Configuration:
// dwAInCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// AIn Subsystem Configuration (AInCfg) Bits:
//
// AIB_INPMODE     // AIn Input Mode (Single-Ended/Differential if set)
// AIB_INPTYPE     // AIn Input Type (Unipolar/Bipolar if set)
// AIB_INPRANGE    // AIn Input Range (5V/10V if set)
// AIB_CVSTART0    // AIn Conv Start Clk Source (2 bits)
// AIB_CVSTART1    // 00 - SW, 01 - internal, 10 - external, 11 - Continuous
// AIB_EXTCVS      // AIn External Conv Start (Pacer) Clk Edge (falling edge if set)
// AIB_CLSTART0    // AIn Ch List Start (Burst) Clk Source (2 bits)
// AIB_CLSTART1    // 00 - SW, 01 - internal, 10 - external, 11 - Continuous
// AIB_EXTCLS      // AIn External Ch List Start (Burst) Clk Edge (falling edge if set)
// AIB_INTCVSBASE  // AIn Internal Conv Start Clk Base (11MHz/33Mhz if set)
// AIB_INTCLSBASE  // AIn Internal Ch List Start Clk Base (11MHz/33Mhz if set)
// AIB_STARTTRIG0  // AIn Start Trigger Source (2 bits) (SW/External if set)
// AIB_STARTTRIG1  // rising edge / falling edge if set
// AIB_STOPTRIG0   // AIn Stop Trigger Source (2 bits) (SW/External if set)
// AIB_STOPTRIG1   // rising edge / falling edge if set
//
// All other bits are to be used internally
//
// dwAInCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from either an 11 MHz or 33 MHz base frequency.
//   The divider then reduces this frequency down to a specific sampling frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwAInCvClkDiv = (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwAInCvClkDiv
//   parameter to 22.)
//
// dwAInClClkDiv (DWORD) - sets the value for the channel list (CL) clock divider.
//   The CL clock can come from either an 11 MHz or 33 MHz base frequency.
//   The divider then reduces this frequency down to a specific scan frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count lower than the value you want to utilize
//
// If selected frequency is higher then possible conversion or scan rate,
// board ignores pulses come before it is ready to process next smaple/scan
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
// Event configuration:
//
// eStartTrig       Start trigger received, operation started
// eStopTrig        Stop trigger received, operation stopped
// eInputTrig       Subsystem specific input trigger (if any)
// eDataAvailable   New data available
// eScanDone        Scan done (for future use)
// eFrameDone       One or more frames are done
// eFrameRecycled   Cyclic buffer frame recycled
//                  (i.e. an unread frame is over-written by new data)
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eConvError       Conversion clock error - pulse came before board is ready to
//                  process it
// eScanError       Scan clock error
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eTimeout         Operation timed out
// eAllEvents       Set/clear all events
//
//
// dwAInScanSize (DWORD) - indicates the number of channels in each scan
// pdwChList (pointer to a DWORD) - specify the pointer to the channel list array.
//
//-
int _PdAInAsyncInit(int handle,
                    DWORD dwAInCfg,
                    DWORD dwAInPreTrigCount, DWORD dwAInPostTrigCount,
                    DWORD dwAInCvClkDiv, DWORD dwAInClClkDiv,
                    DWORD dwEventsNotify,
                    DWORD dwChListSize, PDWORD pdwChList)
{
    tCmd   Cmd;
    int ret;
    int i;

    Cmd.AsyncCfg.Subsystem = AnalogIn;
    Cmd.AsyncCfg.dwAInCfg = dwAInCfg;
    Cmd.AsyncCfg.dwAInPreTrigCount = dwAInPreTrigCount;
    Cmd.AsyncCfg.dwAInPostTrigCount = dwAInPostTrigCount;
    Cmd.AsyncCfg.dwCvRate = dwAInCvClkDiv;
    Cmd.AsyncCfg.dwClRate = dwAInClClkDiv;
    Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
    Cmd.AsyncCfg.dwChListSize = dwChListSize;
    for (i=0; (i < dwChListSize)&&(i < PD_MAX_CL_SIZE); i++)
        Cmd.AsyncCfg.dwChList[i] = *(pdwChList+i);

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_INIT, &Cmd);
    return ret;
}

//+
// -------------------------------------------------------------------------------
//
// Function:    _PdDIAsyncInit
//
// Parameters:  int handle -- handle to adapter
//              DWORD  dwDInCfg           -- IN: AIn configuration word
//              DWORD  dwDInCvClkDiv      -- IN: conv. start clk div.
//              DWORD  dwEventsNotify     -- IN: subsys user events notif.
//              DWORD  dwChListChan       -- IN: number of channels in list (1,2,4,8)
//              DWORD  dwFirstChannel     -- IN: channel number to start from
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Initialize Asynchronous Buffered Acquisition function
//              initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
// Configuration:
// dwDInCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// DIn Subsystem Configuration (AInCfg) Bits:
//
// AIB_CVSTART0    // DIn Conv Start Clk Source (2 bits)
// AIB_CVSTART1    // 00 - SW, 01 - internal, 10 - external
// AIB_EXTCVS      // DIn External Conv Start (Pacer) Clk Edge (falling edge if set)
// AIB_INTCVSBASE  // DIn Internal Conv Start Clk Base (11MHz/33Mhz if set)
// AIB_STARTTRIG0  // DIn Start Trigger Source (2 bits) (SW/External if set)
// AIB_STARTTRIG1  // rising edge / falling edge if set
// AIB_STOPTRIG0   // DIn Stop Trigger Source (2 bits) (SW/External if set)
// AIB_STOPTRIG1   // rising edge / falling edge if set
//
// All other bits are to be used internally
//
// dwDInCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from either an 11 MHz or 33 MHz base frequency.
//   The divider then reduces this frequency down to a specific sampling frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwDInCvClkDiv - (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwAInCvClkDiv
//   parameter to 22.)
//
// If selected frequency is higher then possible conversion or scan rate,
// board ignores pulses come before it is ready to process next smaple/scan
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
// Event configuration:
//
// eStartTrig       Start trigger received, operation started
// eStopTrig        Stop trigger received, operation stopped
// eDataAvailable   New data available
// eFrameDone       One or more frames are done
// eFrameRecycled   Cyclic buffer frame recycled
//                  (i.e. an unread frame is over-written by new data)
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eConvError       Conversion clock error - pulse came before board is ready to
//                  process it
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eTimeout         Operation timed out
// eAllEvents       Set/clear all events
//
// dwChListChan (DWORD) - indicates the number of channels in each scan
// --------------------------------------------------------------------------
//-
int _PdDIAsyncInit(int handle, 
                   DWORD dwDInCfg,
                   DWORD dwDInCvClkDiv,
                   DWORD dwEventsNotify,
                   DWORD dwChListSize,
                   DWORD dwFirstChannel) //r3
{
   tCmd   Cmd;
   int ret;
   int i;

   if ((dwChListSize != 1)&&(dwChListSize & 0x1)) 
      return -1;
   if (dwFirstChannel > 95) 
      return -1;

   Cmd.AsyncCfg.Subsystem = DigitalIn;
   Cmd.AsyncCfg.dwAInCfg = dwDInCfg & 0xFFFFFF;
   Cmd.AsyncCfg.dwAInCfg |= AIB_INPTYPE | AIB_CLSTART0 | AIB_CLSTART1 | AIB_SELMODE3 | AIB_SELMODE4;
   Cmd.AsyncCfg.dwAInPreTrigCount = 0;
   Cmd.AsyncCfg.dwAInPostTrigCount = 0;
   Cmd.AsyncCfg.dwCvRate = dwDInCvClkDiv;
   Cmd.AsyncCfg.dwClRate = dwDInCvClkDiv;
   Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
   Cmd.AsyncCfg.dwChListSize = dwChListSize;
   for ( i = 0; i < dwChListSize; i++ )
        Cmd.AsyncCfg.dwChList[i] = dwFirstChannel + i;
   
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_INIT, &Cmd);
   return ret;
}

//+
// -----------------------------------------------------------------------------
//
// Function:    _PdCTAsyncInit
//
// Parameters:  int handle -- handle to adapter
//              DWORD  dwCTCfg           -- IN: CT configuration word
//              DWORD  dwCTCvClkDiv      -- IN: conv. start clk div.
//              DWORD  dwEventsNotify     -- IN: subsys user events notif.
//              DWORD  dwChListChan       -- IN: number of channels in list
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Initialize Asynchronous Buffered Acquisition function for DSP CT
//              Initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
// Configuration:
// dwCTCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// CT Subsystem Configuration (CTCfg) Bits:
//
// AIB_CVSTART0    // CT Conv Start Clk Source (2 bits)
// AIB_CVSTART1    // 00 - SW, 01 - internal, 10 - external, 11 - Continuous
// AIB_EXTCVS      // CT External Conv Start (Pacer) Clk Edge (falling edge if set)
// AIB_INTCVSBASE  // CT Internal Conv Start Clk Base (11MHz/33Mhz if set)
// AIB_STARTTRIG0  // CT Start Trigger Source (2 bits) (SW/External if set)
// AIB_STARTTRIG1  // rising edge / falling edge if set
// AIB_STOPTRIG0   // CT Stop Trigger Source (2 bits) (SW/External if set)
// AIB_STOPTRIG1   // rising edge / falling edge if set
//
// All other bits are to be used internally
//
// dwCTCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from either an 11 MHz or 33 MHz base frequency.
//   The divider then reduces this frequency down to a specific sampling frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwCTCvClkDiv - (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwCTCvClkDiv
//   parameter to 22.)
//
// If selected frequency is higher then possible conversion rate,
// board ignores pulses come before it is ready to process next smaple/scan
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
// Event configuration:
//
// eFrameDone       One or more frames are done
// eFrameRecycled   Cyclic buffer frame recycled
//                  (i.e. an unread frame is over-written by new data)
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eAllEvents       Set/clear all events
//
// dwChListChan (DWORD) - indicates the number of channels in each scan.
// You can use one or two counters (CT1 and/or CT2)
//
//-
int _PdCTAsyncInit(int handle,
                   DWORD dwCTCfg,
                   DWORD dwCTCvClkDiv,
                   DWORD dwEventsNotify,
                   DWORD dwChListSize)  // r3
{
   tCmd Cmd;
   int ret;
   int i;

   // FIXME: how to select between one and two channels
   if (dwChListSize > 2) 
      return -1;

   Cmd.AsyncCfg.Subsystem = DSPCounter;
   Cmd.AsyncCfg.dwAInCfg = dwCTCfg & 0xFFFFFF;
   Cmd.AsyncCfg.dwAInCfg |= AIB_INPTYPE | AIB_CLSTART0 | AIB_CLSTART1 | AIB_SELMODE4; // switch to the CT (PDx-DIO)
   if (dwChListSize == 2) 
      Cmd.AsyncCfg.dwAInCfg |= AIB_SELMODE2;
   Cmd.AsyncCfg.dwAInPreTrigCount = 0;
   Cmd.AsyncCfg.dwAInPostTrigCount = 0;
   Cmd.AsyncCfg.dwCvRate = dwCTCvClkDiv;
   Cmd.AsyncCfg.dwClRate = dwCTCvClkDiv;
   Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
   Cmd.AsyncCfg.dwChListSize = dwChListSize;
   for ( i = 0; i < dwChListSize; i++ )
        Cmd.AsyncCfg.dwChList[i] = i;
   
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_INIT, &Cmd);
   return ret;
}

//+
// -----------------------------------------------------------------------------
//
// Function:    _PdAOutAsyncInit
//
// Parameters:      int handle   -- handle to adapter
//                  DWORD  dwAOutCfg  -- IN: AIn configuration word
//                  DWORD  dwAOutCvClkDiv   -- IN: conv. start clk div.
//                  DWORD  dwEventsNotify   -- IN: subsys user events notif.
//
// Returns:     TRUE if success
//
// Description: Initialize Asynchronous Buffered Acquisition function
//              initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
// Configuration:
// dwAOutCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// AOut Subsystem Configuration (AInCfg) Bits:
//
// AOB_CVSTART0      AOut Conv (Pacer) Start Clk Source (2 bits)
// AOB_CVSTART1      00 - SW, 01 - internal, 10 - external
// AOB_EXTCVS        AOut External Conv (Pacer) Clock Edge
//                   rising edge/falling edge if set
// AOB_STARTTRIG0    AOut Start Trigger Source (2 bits) (SW/external if set)
// AOB_STARTTRIG1    rising edge/falling edge if set
// AOB_STOPTRIG0     AOut Stop Trigger Source (2 bits) (SW/external if set)
// AOB_STOPTRIG1     rising edge/falling edge if set
// AOB_REGENERATE    Switch to regenerate mode - use DAC FIFO as circular buffer
//                   If waveform size is less then 2048 values uploads all values
//                   directly to the DAC FIFO using _PdAOutPutValues.
//                   If the size is bigger then 2048 values, driver will upload
//                   needed number of values when DAC FIFO becomes half-full.
//                   No events generated but eBufferError.
//
// All other bits are to be used internally
//
// dwAOutCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from 11 MHz base frequency.
//   The divider then reduces this frequency down to a specific conversion frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwAInCvClkDiv - (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwAOutCvClkDiv
//   parameter to 22.)
//
// If selected frequency is higher then possible conversion rate,
// board ignores pulses come before it is ready to process next samaple
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
//
// Event configuration:
//
// eStartTrig       Start trigger received, operation started
// eStopTrig        Stop trigger received, operation stopped
// eFrameDone       One or more frames are done
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eConvError       Conversion clock error - pulse came before board is ready to
//                  process it
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eAllEvents       Set/clear all events
//
// Notes: PDx-MFx analog output only
//
// -----------------------------------------------------------------------------
//-
int _PdAOutAsyncInit(int handle, DWORD dwAOutCfg, 
                     DWORD dwAOutCvClkDiv, DWORD dwEventsNotify) //r3
{
   tCmd   Cmd;
   int ret;
   int i;

   // Reset board
   if (_PdAOutReset(handle)<0)
       return -1;

   Cmd.AsyncCfg.Subsystem = AnalogOut;
   Cmd.AsyncCfg.dwAInCfg = dwAOutCfg;
   Cmd.AsyncCfg.dwAInPreTrigCount = 0;
   Cmd.AsyncCfg.dwAInPostTrigCount = 0;
   Cmd.AsyncCfg.dwCvRate = dwAOutCvClkDiv;
   Cmd.AsyncCfg.dwClRate = dwAOutCvClkDiv;
   Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
   Cmd.AsyncCfg.dwChListSize = 2;    // always for PD2-MFx AOut -- it shall 
                                // be set automatically in PdAcquireBuffer

   for ( i = 0; i < 2; i++ )
       Cmd.AsyncCfg.dwChList[i] = i;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_INIT, &Cmd);
   return ret;
}

//+
// -----------------------------------------------------------------------------
//
// Function:    _PdAOAsyncInit
//
// Parameters:      int handle   -- handle to adapter
//                  DWORD  dwAOutCfg  -- IN: AIn configuration word
//                  DWORD  dwAOutCvClkDiv   -- IN: conv. start clk div.
//                  DWORD  dwEventsNotify   -- IN: subsys user events notif.
//                  DWORD  dwChListSize -- size of the channel list
//                  PDWORD pdwChList -- channel list data array
//
// Returns:     TRUE if success
//
// Description: Initialize Asynchronous Buffered Acquisition function
//              initializes the configuration and allocates memory for buffered
//              acquisiton.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
// Configuration:
// dwAOutCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// AOut Subsystem Configuration (AInCfg) Bits:
//
// AOB_CVSTART0      AOut Conv (Pacer) Start Clk Source (2 bits)
// AOB_CVSTART1      00 - SW, 01 - internal, 10 - external
// AOB_EXTCVS        AOut External Conv (Pacer) Clock Edge
//                   rising edge/falling edge if set
// AOB_STARTTRIG0    AOut Start Trigger Source (2 bits) (SW/external rising edge if set)
// AOB_STOPTRIG0     AOut Stop Trigger Source (2 bits) (SW/external rising edge if set)
// AOB_REGENERATE    Switch to regenerate mode - use DAC FIFO as circular buffer
//                   If waveform size is less then 2048 values uploads all values
//                   directly to the DAC FIFO using _PdAOutPutValues.
//                   If the size is bigger then 2048 values, driver will upload
//                   needed number of values when DAC FIFO becomes half-full.
//                   No events generated but eBufferError.
//
// All other bits are to be used internally
//
// dwAOutCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from 11 MHz base frequency.
//   The divider then reduces this frequency down to a specific conversion frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwAInCvClkDiv - (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwAOutCvClkDiv
//   parameter to 22.)
//
// If selected frequency is higher then possible conversion rate,
// board ignores pulses come before it is ready to process next sample
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
//
// Event configuration:
//
// eStartTrig       Start trigger received, operation started
// eStopTrig        Stop trigger received, operation stopped
// eFrameDone       One or more frames are done
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eConvError       Conversion clock error - pulse came before board is ready to
//                  process it
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eAllEvents       Set/clear all events
//
// Channel list for the PD2-AO boards has following format
//
//        6   5  4           0
//      +---+---+-------------+
//      |UA |WH |  channel #  |
//      +---+---+-------------+
//
// Update All (UA) bit. If bit #6 is selected (set to 1) all analog output
// channels are updated when this channel is written.
// If no new data was written the previous data is used and output remain
// unchanged.
//
// Write And Hold (WH) bit. When bit #5 is selected (set to 1) data written to
// the DAC is stored in the input buffer. DAC output will be updated upon update
// command.
//
// If channel list size is equal to zero, data will be transferred into DAC FIFO
// unchanged
//
// Notes: PD2-AO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdAOAsyncInit(int handle, DWORD dwAOutCfg,
                   DWORD dwAOutCvClkDiv, DWORD dwEventsNotify,
                   DWORD dwChListSize, PDWORD pdwChList) //r3
{
   tCmd   Cmd;
   int ret;
   int i;

   // Reset board
   if (_PdAOutReset(handle)<0)
       return -1;

   Cmd.AsyncCfg.Subsystem = AnalogOut;
   Cmd.AsyncCfg.dwAInCfg = dwAOutCfg | AOB_AOUT32;
   Cmd.AsyncCfg.dwAInPreTrigCount = 0;
   Cmd.AsyncCfg.dwAInPostTrigCount = 0;
   Cmd.AsyncCfg.dwCvRate = dwAOutCvClkDiv;
   Cmd.AsyncCfg.dwClRate = dwAOutCvClkDiv;
   Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
   Cmd.AsyncCfg.dwChListSize = dwChListSize;    
                                
   if (dwChListSize < PD_MAX_CL_SIZE)
      for ( i = 0; i < dwChListSize; i++ )
         Cmd.AsyncCfg.dwChList[i] = *(pdwChList+i); // in DMA mode ChList[0] = first channel

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_INIT, &Cmd);
   return ret;
}

//+
// -----------------------------------------------------------------------------
//
// Function:    _PdDOAsyncInit
//
// Parameters:      int handle   -- handle to adapter
//                  DWORD  dwDOutCfg  -- IN: AIn configuration word
//                  DWORD  dwDOutCvClkDiv   -- IN: conv. start clk div.
//                  DWORD  dwEventsNotify   -- IN: subsys user events notif.
//                  DWORD  dwChListSize -- size of chennel list
//                  PDWORD pdwChList -- channel list data array
//
// Returns:     TRUE if success
//
// Description: Initialize Asynchronous Buffered Acquisition function
//              initializes the configuration and allocates memory for buffered
//              output.
//
// Notes:       This driver function does NO checking on the hardware
//              configuration parameters, it is the responsibility of the
//              DLL to verify that the parameters are valid for the device
//              type being configured.
//
// Configuration:
// dwDOutCfg (DWORD) - this represents a variety of configuration parameters.
// from pdfw_def.h:
//
// DOut Subsystem Configuration (AInCfg) Bits:
//
// AOB_CVSTART0      DOut Conv (Pacer) Start Clk Source (2 bits)
// AOB_CVSTART1      00 - SW, 01 - internal, 10 - external
// AOB_EXTCVS        DOut External Conv (Pacer) Clock Edge
//                   rising edge/falling edge if set
// AOB_STARTTRIG0    DOut Start Trigger Source (2 bits) (SW/external if set)
// AOB_STARTTRIG1    rising edge/falling edge if set
// AOB_STOPTRIG0     DOut Stop Trigger Source (2 bits) (SW/external if set)
// AOB_STOPTRIG1     rising edge/falling edge if set
// AOB_REGENERATE    Switch to regenerate mode - use DAC FIFO as circular buffer
//
// All other bits are to be used internally
//
// dwDOutCvClkDiv (DWORD) - sets the value for the conversion (CV) clock divider.
//   The CV clock can come from 11 MHz base frequency.
//   The divider then reduces this frequency down to a specific conversion frequency.
//   Due to a feature in the DSP counter operation, the divider value needs to be
//   one count less than the value you want to utilize.
//   dwAInCvClkDiv - (base frequency / acquisition rate) - 1
//   (I.e. If you want a divider value of 23, you should set the dwDOutCvClkDiv
//   parameter to 22.)
//
// If selected frequency is higher then possible conversion rate,
// board ignores pulses come before it is ready to process next sample
//
// dwEventsNotify (DWORD) - this flag tells the driver upon which events it
// should notify the application.  Each bit of the value references a specific
// event as listed in the table below
// Event configuration:
//
// eStartTrig       Start trigger received, operation started
// eStopTrig        Stop trigger received, operation stopped
// eFrameDone       One or more frames are done
// eBufferDone      Buffer done
// eBufferWrapped   Cyclic buffer wrapped
// eConvError       Conversion clock error - pulse came before board is ready to
//                  process it
// eBufferError     Buffer over/under run error
// eStopped         Operation stopped (possibly because of error)
// eAllEvents       Set/clear all events
//
// Channel list for the PD2-DIO boards has the following format
//
//               2           0
//              +-------------+
//              |  channel #  |
//              +-------------+
//
// Notes: PD2-DIO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdDOAsyncInit(int handle, DWORD dwDOutCfg,
                   DWORD dwDOutCvClkDiv, DWORD dwEventsNotify,
                   DWORD dwChListSize, PDWORD pdwChList) //r3
{
   tCmd   Cmd;
   int ret;
   int i;

   // Reset board
   if (_PdAOutReset(handle)<0)
       return -1;

   Cmd.AsyncCfg.Subsystem = DigitalOut;
   Cmd.AsyncCfg.dwAInCfg = dwDOutCfg; //FIXME
   Cmd.AsyncCfg.dwAInPreTrigCount = 0;
   Cmd.AsyncCfg.dwAInPostTrigCount = 0;
   Cmd.AsyncCfg.dwCvRate = dwDOutCvClkDiv;
   Cmd.AsyncCfg.dwClRate = dwDOutCvClkDiv;
   Cmd.AsyncCfg.dwEventsNotify = dwEventsNotify;
   Cmd.AsyncCfg.dwChListSize = dwChListSize;    
                                
   if (dwChListSize < PD_MAX_CL_SIZE)
      for ( i = 0; i < dwChListSize; i++ )
         Cmd.AsyncCfg.dwChList[i] = *(pdwChList+i); // in DMA mode ChList[0] = first channel

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_INIT, &Cmd);
   return ret;
}


//+
// Function:    PdAInAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The AIn Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered
//              acquisition.
//
// Notes:
//-
int _PdAInAsyncTerm(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_TERM, NULL);
    return ret;
}

// -------------------------------------------------------------------------------
//
// Function:    _PdDIAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered
//              acquisition.
//
// Notes: PD2-DIO boards only
//
// --------------------------------------------------------------------------
//-
int _PdDIAsyncTerm(int handle) // r3
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_TERM, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdAOutAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered output.
//
// Notes: PDx-MFx analog output only
//
// -----------------------------------------------------------------------------
//-
int _PdAOutAsyncTerm(int handle) // r3
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_TERM, NULL);
    return ret;
}

//+
// -----------------------------------------------------------------------------
// Function:    PdAOAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered output.
//
// Notes: PD2-AO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdAOAsyncTerm(int handle) // r3
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_TERM, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    PdDOAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered output.
//
// Notes: PD2-DIO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdDOAsyncTerm(int handle) // r3
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_TERM, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    PdCTAsyncTerm
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Terminate Asynchronous Buffered Acquisition function
//              terminates and releases memory allocated for buffered output.
//
// Notes: PD2-DIO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdCTAsyncTerm(int handle) // r3
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_TERM, NULL);
    return ret;
}


//+
// Function:    PdAInAsyncStart
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The AIn Start Asynchronous Buffered Acquisition function starts
//              buffered acquisition.
//
// Notes:
//-
int _PdAInAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_START, NULL);
    return ret;
}

// -------------------------------------------------------------------------------
//
// Function:    _PdDIAsyncStart
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Start Asynchronous Buffered Acquisition function starts
//              buffered acquisition.
//
// Notes: PD2-DIO boards only
//
// --------------------------------------------------------------------------
//-
int _PdDIAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_START, NULL);
    return ret;
}
//+
// -------------------------------------------------------------------------------
//
// Function:    _PdCTAsyncStart
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Start Asynchronous Buffered Acquisition function starts
//              buffered acquisition.
//
// Notes: PD2-DIO boards only
//
// --------------------------------------------------------------------------
//-
int _PdCTAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_START, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdAOutAsyncStart
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Start Asynchronous Buffered Output function starts
//              buffered analog output.
//
// Notes: PDx-MFx analog output only
//
// -----------------------------------------------------------------------------
//-
int _PdAOutAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_START, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdAOAsyncStart
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Start Asynchronous Buffered Acquisition function starts
//              buffered analog output.
//
// Notes: PD2-AO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdAOAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_START, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdDOAsyncStart
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Start Asynchronous Buffered Acquisition function starts
//              buffered analog output.
//
// Notes: PD2-DIO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdDOAsyncStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_START, NULL);
    return ret;
}


//+
// Function:    PdAInAsyncStop
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The AIn Stop Asynchronous Buffered Acquisition function stops
//              buffered acquisition.
//
// Notes:
//-
int _PdAInAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_STOP, NULL);
    return ret;
}

// -------------------------------------------------------------------------------
//
// Function:    _PdDIAsyncStop
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Stop Asynchronous Buffered Acquisition function stops
//              buffered acquisition.
//
// Notes: PD2-DIO boards only
//
//-
int _PdDIAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_STOP, NULL);
    return ret;
}
//+
// -------------------------------------------------------------------------------
//
// Function:    _PdCTAsyncStop
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Stop Asynchronous Buffered Acquisition function stops
//              buffered acquisition.
//
// Notes: PD2-DIO boards only
//
//-
int _PdCTAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIN_ASYNC_STOP, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdAOutAsyncStop
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Stop Asynchronous Buffered Acquisition function stops
//              perviously started buffered analog output.
//
// Notes: PDx-MFx analog output only
//
// -----------------------------------------------------------------------------
//-
int _PdAOutAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_STOP, NULL);
    return ret;
}
//+
// -----------------------------------------------------------------------------
// Function:    _PdAOAsyncStop
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Stop Asynchronous Buffered Acquisition function stops
//              perviously started buffered analog output.
//
// Notes: PD2-AO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdAOAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_STOP, NULL);
    return ret;
}

//+
// -----------------------------------------------------------------------------
// Function:    _PdDOAsyncStop
//
// Parameters:  int handle -- handle to adapter
//              
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Stop Asynchronous Buffered Acquisition function stops
//              perviously started buffered analog output.
//
// Notes: PD2-DIO boards only
//
// -----------------------------------------------------------------------------
//-
int _PdDOAsyncStop(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AO_ASYNC_STOP, NULL);
    return ret;
}


//+
// Function:    _PdAInGetScans
//
// Parameters:  int handle -- handle to adapter
//              DWORD NumScans        -- IN:  number of scans to get
//              DWORD ScanRetMode     -- IN:  mode to get scans
//                      AIN_SCANRETMODE_MMAP - use shared hernel.user space buffer
//                      AIN_SCANRETMODE_RAW  - convert and copy values into user
//                                             buffer in short format
//                      AIN_SCANRETMODE_VOLTS - convert and copy values into user
//                                              buffer in float format
//              See _PdRegisterBuffer for details
//
//              DWORD *pScanIndex     -- OUT: ptr to buffer index of first scan
//              DWORD *pNumValidScans -- OUT: ptr to number of valid scans available
//
// Returns:     Negative error code or 0
//
// Description: The AIn Get Scans function returns the oldest scan index
//              in the DAQ buffer and releases (recycles) frame(s) of scans
//              that had been obtained previously.
//
// Notes:
//
//-
int _PdAInGetScans(int handle, DWORD NumScans, DWORD ScanRetMode, DWORD *pScanIndex,
                   DWORD *pNumValidScans)
{
   int ret;
   tCmd cmd;
   cmd.ScanInfo.NumScans = NumScans;
   cmd.ScanInfo.ScanRetMode = ScanRetMode;
   cmd.ScanInfo.Subsystem = AnalogIn;
   cmd.ScanInfo.ScanIndex = 0;
   cmd.ScanInfo.NumValidScans = 0;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GET_DAQBUF_SCANS, &cmd);

   *pScanIndex = cmd.ScanInfo.ScanIndex;
   *pNumValidScans = cmd.ScanInfo.NumValidScans;

   return ret;
}

// ----------------------------------------------------------------------
// Function:    _PdAInGetBufState
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples are acquired and where to get
//              them
//
// --------------------------------------------------------------------------
//-
int _PdAInGetBufState(int handle, DWORD NumScans, 
                      DWORD ScanRetMode, DWORD *pScanIndex,
                      DWORD *pNumValidScans) //r3
{
    return _PdAInGetScans(handle, NumScans, ScanRetMode, pScanIndex, pNumValidScans);
}
//+
// ----------------------------------------------------------------------
// Function:    _PdDIGetBufState
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode --
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples are acquired and where to get
//              them
//
// --------------------------------------------------------------------------
//-
int _PdDIGetBufState(int handle, DWORD NumScans, 
                      DWORD ScanRetMode, DWORD *pScanIndex,
                      DWORD *pNumValidScans) //r3
{
    return _PdAInGetScans(handle, NumScans, ScanRetMode, pScanIndex, pNumValidScans);
}
//+
// ----------------------------------------------------------------------
// Function:    _PdCTGetBufState
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode --
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples are acquired and where to get
//              them
//
//-
int _PdCTGetBufState(int handle, DWORD NumScans, 
                      DWORD ScanRetMode, DWORD *pScanIndex,
                      DWORD *pNumValidScans) //r3
{
    return _PdAInGetScans(handle, NumScans, ScanRetMode, pScanIndex, pNumValidScans);
}
//+
// ----------------------------------------------------------------------
// Function:    _PdAOutGetBufState
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode --
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples can be accepted and where to put
//              them
//
// Notes: PDx-MFx analog output only. We can update both of AOut channels at
//        the same time only. So, channel list size is fixed and is always 2.
//        Thus, you have to allocate buffer using _PdAcquireBuffer with the
//        ScanSize always equal 2.
//
// Special mode:
//        If AIB_DWORDVALUES flag in _PdAcquireBuffer() dwMode parameter is
//        selected you have to pack both channels values into one DWORD.
//        Values for both channels are packed in one DWORD. Channel 0 occupies
//        bits from 0 to 11 and channel 1 from 12 to 23
//
// --------------------------------------------------------------------------
//-
int _PdAOutGetBufState(int handle, DWORD NumScans, DWORD ScanRetMode, 
                       DWORD* pScanIndex, DWORD* pNumValidScans) // r3
{
   int ret;
   tCmd cmd;
   cmd.ScanInfo.NumScans = NumScans;
   cmd.ScanInfo.ScanRetMode = ScanRetMode;
   cmd.ScanInfo.Subsystem = AnalogOut;
   cmd.ScanInfo.ScanIndex = 0;
   cmd.ScanInfo.NumValidScans = 0;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GET_DAQBUF_SCANS, &cmd);

   *pScanIndex = cmd.ScanInfo.ScanIndex;
   *pNumValidScans = cmd.ScanInfo.NumValidScans;

   return ret;
}
//+
// ----------------------------------------------------------------------
// Function:    _PdAOGetBufState
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode --
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples can be accepted and where to put
//              them
//
// Notes: PD2-AO boards only
//
// Special mode:
//        If AIB_DWORDVALUES flag in _PdAcquireBuffer() dwMode parameter is
//        selected you have to combine data and channel to write in one DWORD.
//        Channel list size should be equal to 0
//
// --------------------------------------------------------------------------
//-
int _PdAOGetBufState(int handle, DWORD NumScans, DWORD ScanRetMode, 
                       DWORD* pScanIndex, DWORD* pNumValidScans) // r3
{
   int ret;
   tCmd cmd;
   cmd.ScanInfo.NumScans = NumScans;
   cmd.ScanInfo.ScanRetMode = ScanRetMode;
   cmd.ScanInfo.Subsystem = AnalogOut;
   cmd.ScanInfo.ScanIndex = 0;
   cmd.ScanInfo.NumValidScans = 0;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GET_DAQBUF_SCANS, &cmd);

   *pScanIndex = cmd.ScanInfo.ScanIndex;
   *pNumValidScans = cmd.ScanInfo.NumValidScans;

   return ret;
}
//+
// ----------------------------------------------------------------------
// Function:    _PdDOGetBufState
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwMaxScans -- maximum number of scans requested to get
//              DWORD ScanRetMode --
//              DWORD* dwScanIndex -- offset in the buffer (scans)
//              DWORD* dwValuesCount -- OUT: how many scans are available
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Get Frame Info Asynchronous Buffered Output function informs
//              application how many samples can be accepted and where to put
//              them
//
// Notes: PD2-DO boards only
//
// Special mode:
//        If AIB_DWORDVALUES flag in _PdAcquireBuffer() dwMode parameter is
//        selected you have to combine data and channel to write in one DWORD.
//        Channel list size should be equal to 0
//
// --------------------------------------------------------------------------
//-
int _PdDOGetBufState(int handle, DWORD NumScans, DWORD ScanRetMode, 
                     DWORD* pScanIndex, DWORD* pNumValidScans) // r3
{
   int ret;
   tCmd cmd;
   cmd.ScanInfo.NumScans = NumScans;
   cmd.ScanInfo.ScanRetMode = ScanRetMode;
   cmd.ScanInfo.Subsystem = AnalogOut;
   cmd.ScanInfo.ScanIndex = 0;
   cmd.ScanInfo.NumValidScans = 0;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GET_DAQBUF_SCANS, &cmd);

   *pScanIndex = cmd.ScanInfo.ScanIndex;
   *pNumValidScans = cmd.ScanInfo.NumValidScans;

   return ret;
}


// Unused
int _PdAdapterBoardReset(int handle) { return -EIO; }

//+
// Function:    _PdAInSetCfg
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwAInCfg      -- AIn Configuration word
//              DWORD dwAInPreTrig  -- AIn Pre-trigger Scan Count (always 0)
//              DWORD dwAInPostTrig -- AIn Post-trigger Scan Count (always 0)
//
// Returns:     Negative error code or 0
//
// Description: The set AIn configuration command sets the operating
//              configuration for the AIn subsystem.
//
// Notes:       This command is valid only when the AIn subsystem is in
//              the configuration state (acquisition disabled).
//
// AIn Subsystem Configuration (AInCfg) Bits:
//
// AIB_INPMODE     // AIn Input Mode (Single-Ended/Differential if set)
// AIB_INPTYPE     // AIn Input Type (Unipolar/Bipolar if set)
// AIB_INPRANGE    // AIn Input Range (5V/10V if set)
// AIB_CVSTART0    // AIn Conv Start Clk Source (2 bits)
// AIB_CVSTART1    // 00 - SW, 01 - internal, 10 - external, 11 - Continuous
// AIB_EXTCVS      // AIn External Conv Start (Pacer) Clk Edge (falling edge if set)
// AIB_CLSTART0    // AIn Ch List Start (Burst) Clk Source (2 bits)
// AIB_CLSTART1    // 00 - SW, 01 - internal, 10 - external, 11 - Continuous
// AIB_EXTCLS      // AIn External Ch List Start (Burst) Clk Edge (falling edge if set)
// AIB_INTCVSBASE  // AIn Internal Conv Start Clk Base (11MHz/33Mhz if set)
// AIB_INTCLSBASE  // AIn Internal Ch List Start Clk Base (11MHz/33Mhz if set)
// AIB_STARTTRIG0  // AIn Start Trigger Source (2 bits) (SW/External if set)
// AIB_STARTTRIG1  // rising edge / falling edge if set
// AIB_STOPTRIG0   // AIn Stop Trigger Source (2 bits) (SW/External if set)
// AIB_STOPTRIG1   // rising edge / falling edge if set
//
//-
int _PdAInSetCfg(int handle, DWORD dwAInCfg, DWORD dwAInPreTrig, DWORD dwAInPostTrig)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwAInCfg;
    Cmd.dwParam[1] = dwAInPreTrig;
    Cmd.dwParam[2] = dwAInPostTrig;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETCFG, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSetCvClk
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwClkDiv  -- AIn conversion start clock divider
//
// Returns:     Negative error code or 0
//
// Description: The set internal AIn Conversion Start (pacer) clock
//              configures the DSP Timer to generate a clock signal
//              using the specified divider from either a 11.0 MHz or
//              33.0 MHz base clock frequency.
//
// Notes:   1. Configure AIn Conv Start clock Source to Internal in purpose to
//          utilize internal AIn Conversion Start (pacer) clock:
//          AIB_CVSTART0
//
//          2. Use AIB_INTCVSBASE to switch between Internal Conv Start Clk Base
//          (11MHz/33Mhz if the bit is set)
//
//          3. Divisor = (BaseFreq / DesiredFreq) - 1;
//
//-
int _PdAInSetCvClk(int handle, DWORD dwClkDiv)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwClkDiv;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETCVCLK, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSetClClk
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwClkDiv  -- AIn channel list start clock divider
//
// Returns:     Negative error code or 0
//
// Description: The set internal AIn Channel List Start (scan) clock
//              configures the DSP Timer to generate a clock signal
//              using the specified divider from either a 11.0 MHz or
//              33.0 MHz base clock frequency.
//
// Notes:   1. Configure AIn CL Start clock Source to Internal in purpose to
//          utilize internal AIn Channel List Start (scan) clock:  AIB_CLSTART0
//
//          2. Use AIB_INTCLSBASE to switch between Internal CL Start Clk Base
//          (11MHz/33Mhz if the bit is set)
//
//          3. Divisor = (BaseFreq / DesiredFreq) - 1;
//-
int _PdAInSetClClk(int handle, DWORD dwClkDiv)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwClkDiv;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETCLCLK, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSetChList
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwCh      -- number of channels in list
//              DWORD *pdwChList -- channel list data buffer
//
// Returns:     Negative error code or 0
//
// Description: The set channel list command programs the ADC Channel/Gain
//              List.  The ADC Channel List can contain from 1 to 256
//              channel entries.  Configuration data word for each channel
//              includes the channel mux selection, gain, and slow bit
//              setting.
//
// Notes:       1.Use following macros to program channel list (powerdaq.h)
//              Macros for constructing Channel List entries.
//
//              #define CHAN(c)     ((c) & 0x3f)
//              #define GAIN(g)     (((g) & 0x3) << 6)
//              #define SLOW        (1<<8)
//              #define CHLIST_ENT(c,g,s)   (CHAN(c) | GAIN(g) | ((s) ? SLOW : 0))
//
//              2. Writing a Channel List block clears and overwrites the
//              previous settings.
//
//              3. Writing a channel list with 0 channel entries clears the
//              channel list.
//
//              4. There is no limit to the number of entries that can be
//              written to the channel list FIFO. You need to check CL size
//              yourself (currently up to 256 entries)
//-
int _PdAInSetChList(int handle, DWORD dwCh, DWORD *pdwChList)
{
    tCmd   Cmd;
    int ret, i;

    Cmd.SyncCfg.dwChListSize = dwCh;
    for (i = 0; (i < dwCh)&&(i < PD_MAX_CL_SIZE); i++)
    {
        Cmd.SyncCfg.dwChList[i] = *(pdwChList + i);
    }

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETCHLIST, &Cmd);
    return ret;
}

//+
// Function:    _PdAInEnableConv
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEnable  -- 0: disable, 1: enable AIn conversions
//
// Returns:     Negative error code or 0
//
// Description: The enable AIn conversions command enables or disables
//              AIn conversions irrespective of the AIn Conversion Start
//              or AIn Channel List Start signals.
//
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
// Notes:
//-
int _PdAInEnableConv(int handle, DWORD dwEnable)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwEnable;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AICVEN, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSetEvents
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEvents  -- AInIntrStat Event configuration word
//
// Returns:     Negative error code or 0
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
// Notes:       1. Used automatically inside the driver, rarely used in user code
//              2. See pdfw_def.h for the AInIntrStat event word format.
//-
int _PdAInSetEvents(int handle, DWORD dwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwEvents;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETEVNT, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSwStartTrig
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AIn start trigger command triggers the AIn Start
//              event to start sample acquisition.
//
// Notes:       1. AIn Start trigger should be in software mode (bits are not set)
//              2. See _PdAInSetCfg how to set up start trigger
//-
int _PdAInSwStartTrig(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISTARTTRIG, NULL);
    return ret;

}

//+
// Function:    _PdAInSwStopTrig
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AIn stop trigger command triggers the AIn Stop
//              event to stop sample acquisition.
//
// Notes:       1. AIn Start trigger should be in software mode (bits are not set)
//              2. See _PdAInSetCfg how to set up start trigger
//              3. If clocks are not disabled, SW stop trigger allows board
//              to complete started channel list. It means that you can use
//              start/stop trigger to control acquisition without risk of losing
//              samples.
//-
int _PdAInSwStopTrig(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISTOPTRIG, NULL);
    return ret;
}

//+
// Function:    _PdAInSwCvStart
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AIn conversion start command triggers the ADC
//              Conversion Start signal.
//
// Notes:       AIn CV clock should be configured into software clock mode
//              See _PdAInSetCfg for details
//-
int _PdAInSwCvStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISWCVSTART, NULL);
    return ret;
}

//+
// Function:    _PdAInSwCvStart
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AIn conversion start command triggers the ADC
//              Channel List (scan) Start signal.
//
// Notes:       AIn CL clock should be configured into software clock mode
//              See _PdAInSetCfg for details
//-
int _PdAInSwClStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISWCLSTART, NULL);
    return ret;
}

//+
// Function:    _PdAInResetCl
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset AIn channel list command resets the ADC channel
//              list to the first channel in the list.  This command is
//              similar to the SW Channel List Start, but does not enable
//              the list for conversions.
//
// Notes:
//-
int _PdAInResetCl(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AICLRESET, NULL);
    return ret;
}

//+
// Function:    _PdAInClearData
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The clear all AIn data command clears the ADC FIFO and
//              all AIn data storage buffers.
//
// Notes:
//-
int _PdAInClearData(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AICLRDATA, NULL);
    return ret;
}

//+
// Function:    _PdAInReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset AIn command resets the AIn subsystem to the
//              default startup configuration.  All operations in progress
//              are stopped and all configurations and buffers are cleared.
//              Any transfers in progress are signaled as terminated.
//
// Notes:
//-
int _PdAInReset(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIRESET, NULL);
    return ret;
}

//+
// Function:    _PdAInGetValue
//
// Parameters:  int handle -- handle to adapter
//              WORD *pwSample  -- ptr to variable to store value
//
// Returns:     Negative error code or 0
//
// Description: The AIn Get Single Value command reads a single value
//              from the ADC FIFO.
//
// Notes:
//-
int _PdAInGetValue(int handle, WORD *pwSample)
{
    int ret;
    tCmd cmd;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIGETVALUE, &cmd); 
    *pwSample = cmd.dwParam[0];
    return ret;
}

//+
// Function:    _PdAInGetDataCount
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pdwSamples  -- ptr to variable to store value
//
// Returns:     Negative error code or 0
//
// Description: Returns number of samples in the user registered buffer.
//
// Notes:       AIn Async mode only
//-
int _PdAInGetDataCount(int handle, DWORD *pdwSamples)
{
    int ret;
    tCmd cmd;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIGETSAMPLECOUNT, &cmd);
    *pdwSamples = cmd.dwParam[0];
    return ret;
}

//+
// Function:    _PdAInGetSamples
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwMaxSamples  -- maximal number of samples to receive
//              WORD *pwBuf         -- buffer for storing sample data
//              DWORD *pdwSamples   -- number of valid samples read
//
// Returns:     Negative error code or number of samples
//
// Description: The AIn Get Samples command reads upto nMaxBufSize samples
//              from the ADC FIFO until empty.
//
// Notes:       Each sample is stored in 16 bits (signed short format)
//-
int _PdAInGetSamples(int handle, DWORD dwMaxBufSize, WORD *pwBuf, DWORD *pdwSamples)
{
    tCmd cmd;
    int ret;

    if((dwMaxBufSize*sizeof(WORD)) > PD_MAX_BUFFER_SIZE)
    {
       return -E2BIG;
    }
    cmd.bufParam.size = dwMaxBufSize*sizeof(WORD);
    
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIGETSAMPLES, &cmd);

    *pdwSamples = cmd.bufParam.size/sizeof(WORD);
    memcpy(pwBuf, cmd.bufParam.buffer, cmd.bufParam.size);
    return ret;
}

//+
// Function:    _PdAInGetXFerSamples
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwMaxSamples  -- maximal number of samples to receive (ignored)
//              WORD *pwBuf         -- buffer for storing sample data
//              DWORD *pdwSamples   -- number of valid samples read
//
// Returns:     Negative error code or number of samples
//
// Description: The AIn Get Samples command reads upto nMaxBufSize samples
//              from the ADC FIFO until empty.
//
// Notes:       The difference between _PdAInGetSamples and _PdAInGetXFerSamples
//              is the follows.
//              _PdAInGetSamples transfers sample-by-sample from ADC FIFO and
//              checks for FIFO empty flag each time.
//
//              _PdAInGetXFerSamples transfers data from the ADC FIFO using DMA
//              in bursts. The size of transfer shall be selected using
//              _PdAInSetXferSize function. Selected size shall be less or
//              equal to the number of samples stored in ADC FIFO at the moment
//              you call _PdAInGetXFerSamples. If FIFO doesn't contain enough
//              samples buffer is padded with last available sample
//
//              _PdAInGetXFerSamples is a good function to use from RT-Linux
//              kernel task, when number of samples to retrieve is predefined
//-
int _PdAInGetXFerSamples(int handle, DWORD dwMaxBufSize, WORD *pwBuf, DWORD *pdwSamples)
{
    tCmd  cmd;
    int ret;

    if((dwMaxBufSize*sizeof(WORD)) > PD_MAX_BUFFER_SIZE)
    {
       return -E2BIG;
    }

    cmd.bufParam.size = dwMaxBufSize*sizeof(WORD);

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AIGETXFERSAMPLES, &cmd);

    *pdwSamples = cmd.bufParam.size/sizeof(WORD);
    memcpy(pwBuf, cmd.bufParam.buffer, cmd.bufParam.size);
    return ret;
}

//+
// Function:    _PdAInSetSSHGain
//
// Description: function is obsolete. Functionality is moved into FW
//-
int _PdAInSetSSHGain(int handle, DWORD dwCfg)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwCfg;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETSSHGAIN, &Cmd);
    return ret;
}

//+
// Function:    _PdAInSetXferSize
//
// Parameters:  int handle -- handle to adapter
//              DWORD size  -- size of the burst transfer
//
// Returns:     Negative error code or 0
//
// Description: Set up DMA transfer burst size
//
// Notes:       Don't use this function with asynchronous (buffered) mode.
//              Use _PdAInReset to return to default settings
//-
int _PdAInSetXferSize(int handle, DWORD size)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = size;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AISETXFERSIZE, &Cmd);
    return ret;
}

//--- AOut Subsystem Commands: ----------------------------------------
//
//+
// Function:    _PdAOutSetCfg
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwAOutCfg -- AOut configuration word
//              DWORD dwAOutPostTrig -- AOut Post-trigger Scan Count
//
// Returns:     Negative error code or 0
//
// Description: The set AOut configuration command sets the operating
//              configuration of the AOut subsystem.
//
// Notes:       This command is valid only when the AOut subsystem is in
//              the configuration state (acquisition disabled).
//
// AOut Subsystem Configuration (AInCfg) Bits:
//
// AOB_CVSTART0      AOut Conv (Pacer) Start Clk Source (2 bits)
// AOB_CVSTART1      00 - SW, 01 - internal, 10 - external
// AOB_EXTCVS        AOut External Conv (Pacer) Clock Edge
//                   rising edge/falling edge if set
// AOB_STARTTRIG0    AOut Start Trigger Source (2 bits) (SW/external if set)
// AOB_STARTTRIG1    rising edge/falling edge if set
// AOB_STOPTRIG0     AOut Stop Trigger Source (2 bits) (SW/external if set)
// AOB_STOPTRIG1     rising edge/falling edge if set
// AOB_REGENERATE    Switch to regenerate mode - use DAC FIFO as circular buffer
// AOB_AOUT32        switch to PD2-AO board (format: (channel<<16)|(value & 0xFFFF))
//
//-
int _PdAOutSetCfg(int handle, DWORD dwAOutCfg, DWORD dwAOutPostTrig)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwAOutCfg;
    Cmd.dwParam[1] = dwAOutPostTrig;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSETCFG, &Cmd);
    return ret;
}

//+
// Function:    _PdAOutSetCvClk
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwClkDiv  -- AOut conversion start clock divider
//
// Returns:     Negative error code or 0
//
// Description: The set internal AOut Conversion Start (pacer) clock
//              configures the DSP Timer to generate a clock signal
//              using the specified divider from 11.0 MHz
//
// Notes:   1. Divisor = (BaseFreq / DesiredFreq) - 1;
//-
int _PdAOutSetCvClk(int handle, DWORD dwClkDiv)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwClkDiv;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSETCVCLK, &Cmd);
    return ret;
}

//+
// Function:    _PdAOutSetEvents
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEvents  -- AOutIntrStat Event configuration word
//
// Returns:     Negative error code or 0
//
// Description: Set selected AOut AOutIntrStat event bits enabling/disabling
//              and/or clearing individual firmware level events, thereby
//              re-enabling the event interrupts.
//
//              AOutIntrStat Bit Settings:
//
//                  AOB_xxxxIm bits:    0 = disable, 1 = enable interrupt
//                  AOB_xxxxSC bits:    0 = clear interrupt, 1 = no change
//
// Notes:       1. Used automatically inside the driver, rarely used in user code
//              2. See pdfw_def.h for the AOutIntrStat event word format.
//-
int _PdAOutSetEvents(int handle, DWORD dwEvents)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwEvents;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSETEVNT, &Cmd);
    return ret;
}

//+
// Function:    _PdAOutGetStatus
//
// Parameters:  int handle -- handle to adapter
//              ULONG *pdwStatus    -- AOut Event/Status word
//
// Returns:     Negative error code or 0
//
// Description: The get AOut status command obtains the current status
//              and events, including error events, of the AOut subsystem.
//
// Notes:       See pdfw_def.h for the AOutIntrStat event word format.
//-
int _PdAOutGetStatus(int handle, DWORD *pdwStatus)
{
    tCmd cmd;
    int ret;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSTATUS, &cmd);
    *pdwStatus = cmd.dwParam[0];
    return ret;
}

//+
// Function:    _PdAOutEnableConv
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwEnable  -- 0: disable, 1: enable AOut conversions
//
// Returns:     Negative error code or 0
//
// Description: The enable AOut conversions command enables or disables
//              AOut conversions irrespective of the AOut Conversion Clock
//              signal or Start Trigger. During configuration and following
//              an error condition the AOut conversion process is disabled
//              and must be re-enabled to perform subsequent conversions.
//
//              This command permits completing AOut configuration before
//              the subsystem responds to the Start trigger.
//
//              PD_AONCVEN <== 0:   AOut subsystem Start Trigger is disabled
//                                  and ignored. Conversion in progress
//                                  will not be interrupted but the start
//                                  trigger is disabled from retriggering
//                                  the subsystem again.
//
//              PD_AONCVEN <== 1:   AOut subsystem Start Trigger is enabled
//                                  and data acquisition will start on the
//                                  first valid AOut start trigger.
//
// Notes:
//-
int _PdAOutEnableConv(int handle, DWORD dwEnable)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwEnable;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOCVEN, &Cmd);
    return ret;
}

//+
// Function:    _PdAOutSwStartTrig
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AOut start trigger command triggers the AOut Start
//              event to start value output.
//
// Notes:       Software trigger should be selected in _PdAOutSetCfg
//              Block mode only
//-
int _PdAOutSwStartTrig(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSTARTTRIG, NULL);
    return ret;
}

//+
// Function:    _PdAOutSwStopTrig
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AOut stop trigger command triggers the AOut Stop
//              event to stop value output.
//
// Notes:       Software trigger should be selected in _PdAOutSetCfg
//              Block mode only
//-
int _PdAOutSwStopTrig(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSTOPTRIG, NULL);
    return ret;
}

//+
// Function:    _PdAOutSwCvStart
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW AOut conversion start command triggers the DAC
//              Conversion Start signal.
//
// Notes:       1. To use this function you should select SW clock in _PdAOutSetCfg,
//              load buffer using _PdAOutPutValues with appropriate number of
//              values and then clock them out (convert to analog) one by one
//
//-
int _PdAOutSwCvStart(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOSWCVSTART, NULL);
    return ret;
}

//+
// Function:    _PdAOutClearData
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The clear all AOut data command clears the DAC latch and
//              all AOut data storage buffers.
//
// Notes:
//-
int _PdAOutClearData(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOCLRDATA, NULL);
    return ret;
}

//+
// Function:    _PdAOutReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset AOut command resets the AOut subsystem to the
//              default startup configuration.  All operations in progress
//              are stopped and all configurations and buffers are cleared.
//              Any transfers in progress are signaled as terminated.
//
// Notes:
//-
int _PdAOutReset(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AORESET, NULL);
    return ret;
}

//+
// Function:    _PdAOutPutValue
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwValue -- value(s) to output
//
// Returns:     Negative error code or 0
//
// Description: The AOut put single value command writes a single value
//              to be converted and output by the specified DAC.
//
// Notes:       This function works only with MF(S) boards. To set up output
//              value for PD2-AO series use _PdAO32Write call
//-
int _PdAOutPutValue(int handle, DWORD dwValue)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwValue;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOPUTVALUE, &Cmd);
    return ret;
}

//+
// Function:    _PdAOutPutBlock
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwNumValues   -- number of values in buf to output
//              DWORD *pdwBuf       -- buffer containing values to output
//              DWORD *pdwCount     -- number of values successfully written
//
// Returns:     Negative error code or 0
//
// Description: The AOut put block command writes a block of values
//              to the DAC FIFO
//
// Notes:       This function can be used either with PDx-MFx or PD2-AO
//              board.
//              To use it with PD2-AO boards AOB_AOUT32 bit should be set
//              in AOut configuration word.
//              DAC FIFO size is 2048 samples. If FIFO is not empty, function
//              returns the number of values it was able to write until FIFO
//              became full
//-
int _PdAOutPutBlock(int handle, DWORD dwValues, DWORD *pdwBuf, DWORD *pdwCount)
{
    tCmd   Cmd;
    int ret;

    if(dwValues*sizeof(DWORD)>PD_MAX_BUFFER_SIZE)
    {
       return -E2BIG;
    }

    Cmd.bufParam.size = dwValues * sizeof(DWORD);
    memcpy(Cmd.bufParam.buffer, pdwBuf, dwValues*sizeof(DWORD));
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AOPUTBLOCK, &Cmd);
    *pdwCount = Cmd.bufParam.size;

    return ret;
}

//--- DIn/DOut Subsystem Commands: -----------------------------------------
//
//+
// Function:    _PdDInSetCfg
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD dwDInCfg  -- DIn configuration word
//
// Returns:     Negative error code or 0
//
// Description: The set DIn configuration command sets the operating
//              configuration for the DIn subsystem.
//
// DIn configuration.
// Eight lower DIn lines on PD2-MFx boards are edge-sensitive
// You can program DIn to fire an interrupt when particular line changes state
// to desired one. File pdfw_def.h contains following definition, where x = [0..7]
//
// DIB_xCFG0      DIn Bit x set rising edge to fire interrupt
// DIB_xCFG1      DIn Bit x set falling edge to fire interrupt
// ...
//
// Notes:   PD2-DIO boards have their own set of functions. See _PdDIOxxx
//-
int _PdDInSetCfg(int handle, DWORD dwDInCfg)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwDInCfg;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DISETCFG, &Cmd);
    return ret;
}

//+
// Function:    _PdDInGetStatus
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD *pdwEvents -- word for storing input status word
//                    Bits 0 - 7:  Digital Input Bit Level (i.e. current level)
//                    Bits 8 - 15: Digital Input Bit Trigger Status (latched data)
//
// Returns:     Negative error code or 0
//
// Description: The get DIn Status command obtains the current input
//              levels and the currently latched input change events
//              of all digital input signals.
//
// Notes:       1. See pdfw_def.h for details (DIB_LEVELx and DIB_INTRx bits)
//              2. Only one bit of status per line available. Thus, if you
//              programmed board to generate interrupt, say, on any edge
//              of line 0, bit 8 will not tell you which edge caused event.
//              To find this out you need to analyze bit 0 for current line state
//              and bit 8 for latched event.
//-
int _PdDInGetStatus(int handle, DWORD *pdwEvents)
{
    tCmd   Cmd;
    int ret;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DISTATUS, &Cmd);
    *pdwEvents = Cmd.dwParam[0];

    return ret;
}

//+
// Function:    _PdDInRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD *pdwValue -- DIn Data Value word
//
// Returns:     Negative error code or 0
//
// Description: The DIn Read Data command obtains the current input
//              levels of all 16 digital input lines for PD2 series or
//              8 digital input lines for PD series
//
// Notes: PDx-MFx boards only. See _PdDIO... functions for PD2-DIO board family
//
//-
int _PdDInRead(int handle, DWORD *pdwValue)
{
    tCmd   Cmd;
    int ret;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIREAD, &Cmd);
    *pdwValue = Cmd.dwParam[0];

    return ret;
}

//+
// Function:    _PdDInClearData
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The clear all DIn latched data command clears all stored
//              DIn data.
//
// Notes:       Use this function after your program was informed that
//              changes occured on digital line and status was read using
//              _PdDInGtStatus. Calling this funstion you clear bits 8-15
//              of the status word
//-
int _PdDInClearData(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DICLRDATA, NULL);
    return ret;
}

//+
// Function:    _PdDInReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset DIn command resets the DIn subsystem to the
//              default startup configuration.
// Notes:
//-
int _PdDInReset(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIRESET, NULL);
    return ret;
}

//+
// Function:    _PdDOutWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwValue   -- DOut value word to output
//
// Returns:     Negative error code or 0
//
// Description: The DOut Write Value command updates the levels of the
//              DOut output signals.
//
// Notes:       PD2-MFx boards have 16 DOut lines, PD-MFx boards - eight
//
//-
int _PdDOutWrite(int handle, DWORD dwValue)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = (DWORD)dwValue;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DOWRITE, &Cmd);
    return ret;
}

//+
// Function:    _PdDOutReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset DIn command resets the DIn subsystem to the
//              default startup configuration - logical 0
//
// Notes: PDx-MFx boards have no predefined DOut states
//-
int _PdDOutReset(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DORESET, NULL);
    return ret;
}


//--- DIO256 Subsystem Commands: ----------------------------------------
//
//+
// Function:    _PdDIO256CmdWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwCmd     -- command to DIO
//              DWORD dwValue   -- value to write
//
// Returns:     Negative error code or 0
//
// Description: write the command and parameter into DIO256/AO32 board
//
// Notes: This function can write to any address of DSP bus. So, you might
//        want to be cautious when call this function
//-
int _PdDIO256CmdWrite(int handle, DWORD dwCmd, DWORD dwValue)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = dwCmd;
    Cmd.dwParam[1] = dwValue;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256CMDWR, &Cmd);

    return ret;
}

//+
// Function:    _PdDIO256CmdRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwCmd     -- command to DIO
//              DWORD* dwValue   -- read value
//
// Returns:     Negative error code or 0
//
// Description: write the command and read value from DIO256/AO32 board
//
// Notes: This function can read from any address of DSP bus. So, you might
//        want to be cautious when call this function
//-
int _PdDIO256CmdRead(int handle, DWORD dwCmd, DWORD *pdwValue)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = dwCmd;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256CMDRD, &Cmd);
    *pdwValue = Cmd.dwParam[1];

    return ret;
}

//+
// Function:    _PdDIO256CmdRead
//
// Description: Internal functions help to creates a command
//_
DWORD __PdDIO256MakeCmd(DWORD dwRegister)
{
    DWORD dwCmd = 0;
    switch (dwRegister)
    {
        case 0: dwCmd |= DIO_REG0; break;
        case 1: dwCmd |= DIO_REG1; break;
        case 2: dwCmd |= DIO_REG2; break;
        case 3: dwCmd |= DIO_REG3; break;
        case 4: dwCmd |= DIO_REG4; break;
        case 5: dwCmd |= DIO_REG5; break;
        case 6: dwCmd |= DIO_REG6; break;
        case 7: dwCmd |= DIO_REG7; break;
        default: dwCmd |= DIO_REG0; break;
    }
    return dwCmd;
}

//+
// Function:    _PdDIO256CmdRead
//
// Parameters:  DWORD dwBank -- number of I/O bank (0 or 1)
//              DWORD dwRegMask -- register mask
//
// Description: function forms bits 0-4 to write to DIO board
//
// Notes:       1. dwBank: 0-3 = bank 0; 4-7 = bank 1 (bXX)
//              2. DIO control command when register mask is used
//                 cmd1 cmd0 r3 r2 bank r1 r0
//-
DWORD __PdDIO256MakeRegMask(DWORD dwBank, DWORD dwRegMask)
{
    DWORD   dwReg = dwBank;
    dwReg = (dwReg & 0x4);
    dwReg |= (dwRegMask & 0x3);
    dwReg |= ((dwRegMask << 1) & 0x18);
    return dwReg;
}

//+
// Function:    _PdDIO256CmdWriteAll
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwValue   -- ptr on values to write
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: write the command and parameter into DIO256
// 
// Notes:
//
//-
int _PdDIO256CmdWriteAll(int handle, DWORD *pdwValue)
{
    tCmd   Cmd;
    int    ret;

    memcpy(Cmd.dwParam, pdwValue, DIO_REGS_NUM/2*sizeof(DWORD));
    
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256CMDWR_ALL, &Cmd);

    return ret;
}

//+
// Function:    _PdDIO256CmdReadAll
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwValue   -- ptr on values to write
//
// Returns:     BOOL status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Read the command and parameter from DIO256
// 
// Notes:
//
//-
int _PdDIO256CmdReadAll(int handle, DWORD *pdwValue)
{
    tCmd   Cmd;
    int    ret;
    
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256CMDRD_ALL, &Cmd);

    memcpy(pdwValue, Cmd.dwParam, DIO_REGS_NUM/2*sizeof(DWORD));

    return ret;
}

//+
// Function:    _PdDIOReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: Resets PD2-DIO board, disables (switches to DIn)
//              all DIO lines
// Notes:
//
//-
int _PdDIOReset(int handle)
{
    BOOL    bRes = TRUE;
    bRes &= _PdDIO256CmdWrite(handle, DIO_DIS_0_3,
                              0);
    bRes &= _PdDIO256CmdWrite(handle, DIO_DIS_4_7,
                              0);
    return bRes;
}

//+
// Function:    _PdDIOEnableOutput
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegMask -- mask of the registers to enable output
//
// Returns:     Negative error code or 0
//
// Description: Set output enable
//
// Notes: 1. dwRegMask selects which of 16-bit register set as DIn and which
//          as DOut. If register is in DIn it's tristated.
//          PD2-DIO64 uses only four lower bits of dwRegMask and PD2-DIO128 - eight
//          of them. Rest of the bits should be zero.
//
//        2. dwRegMask format: r7 r6 r5 r4 r3 r2 r1 r0
//          1 in the dwRegMask means that register is selected for output
//          0 means that register is selected for input
//          Example: To select registers 0,1 and 4,5 for output
//          dwRegMask = 0x33 ( 00110011 )
//-
int _PdDIOEnableOutput(int handle, DWORD dwRegMask)
{
     DWORD dwRegM = ~dwRegMask;
     BOOL  bRes = TRUE;

     bRes &= _PdDIO256CmdWrite(handle,
                               __PdDIO256MakeRegMask(DIO_REG0 & 0xF, (dwRegM & 0xF)) | DIO_WOE,
                               0);
     bRes &= _PdDIO256CmdWrite(handle,
                               __PdDIO256MakeRegMask(DIO_REG4 & 0xF, (dwRegM & 0xF0)>>4 ) | DIO_WOE,
                               0);
     return bRes;
}

//+
// Function:    _PdDIOLatchAll
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegister -- register
//
// Returns:     Negative error code or 0
//
// Description: Latch the state of all inputs in a bank
//              This function strobe latch signal and data presents
//              on the input lines is clocked into registers.
//              Use this function to latch all inputs are the same time
//              (simultaneously) and function _PdDIOSimpleRead to read latched
//              registers one by one (withuot re-latching them).
//
// Notes:       Function latches data for only one bank (16 x 4 lines)
//              If you use PD2-DIO128 board with two banks you might
//              need to call this function two times - first time for the bank 0
//              (dwRegister = 0) and second time for the bank 1 (dwRegister = 4)
//-
int _PdDIOLatchAll(int handle, DWORD dwRegister)
{
    DWORD   dwResult;
    return _PdDIO256CmdRead (handle, DIO_LAL | (dwRegister & 0x4),
                            &dwResult);
}

//+
// Function:    _PdDIOSimpleRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegister -- register
//              DWORD *pdwValue  -- ptr to variable to store value
//
// Returns:     Negative error code or 0
//
// Description: Returns value stored in the latch without strobing
//              of latch signal
//
// Notes:       Function doesn't return tha actual state of DIn lines
//              but rather data stored when latch was strobed last time
//-
int _PdDIOSimpleRead(int handle, DWORD dwRegister, DWORD *pdwValue)
{
    return _PdDIO256CmdRead(handle,
                            (__PdDIO256MakeCmd(dwRegister) | DIO_SRD),
                            pdwValue);
}

//+
// Function:    _PdDIORead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegister -- register
//              DWORD *pdwValue  -- ptr to variable to store value
//
// Returns:     Negative error code or 0
//
// Description: Strobe latch line for the register specified and
//              returns value stored in the latch
//
// Notes:       Use this function to retrieve state of the inputs
//              immediately
//-
int _PdDIORead(int handle, DWORD dwRegister, DWORD *pdwValue)
{
    return _PdDIO256CmdRead(handle,
                            (__PdDIO256MakeCmd(dwRegister) | DIO_LAR),
                            pdwValue);
}

//+
// Function:    _PdDIOReadAll
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pdwValue  -- ptr to variable to store value
//
// Returns:     Negative error code or 0
//
// Description: Read all ports at once
//
// Notes:       Use this function to retrieve state of the inputs
//              immediately
//-
int _PdDIOReadAll(int handle, DWORD *pdwValue)
{
    return _PdDIO256CmdReadAll(handle, pdwValue);
}


//+
// Function:    _PdDIOWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegister -- register
//              DWORD pdwValue  -- variable to write to DOut register
//
// Returns:     Negative error code or 0
//
// Description: Write values to digital output register
//
// Notes:       Upon this function call value is written to the output
//              register. To see actual voltages on the ouputs, specified
//              register shall be configured as output using _PdDIOEnableOutput
//-
int _PdDIOWrite(int handle, DWORD dwRegister, DWORD dwValue)
{
    return _PdDIO256CmdWrite(handle,
                            (__PdDIO256MakeCmd(dwRegister) | DIO_SWR),
                            (dwValue & 0xFFFF));
}

//+
// Function:    _PdDIOWriteAll
//
// Parameters:  int handle -- handle to adapter
//              DWORD pdwValue  -- values to write to all ports
//
// Returns:     Negative error code or 0
//
// Description: Write values to all digital output ports
//
// Notes:       Upon this function call value is written to the output
//              register. To see actual voltages on the ouputs, specified
//              register shall be configured as output using _PdDIOEnableOutput
//-
int _PdDIOWriteAll(int handle, DWORD *pdwValue)
{
    return _PdDIO256CmdWriteAll(handle, pdwValue);
}

//+
// Function:    _PdDIOPropEnable
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD dwRegMask -- register mask
//
// Returns:     Negative error code or 0
//
// Description: Enable or disable propagate signal generation
//
// Notes:       PD2-DIO boards have special line "Propagate" to inform external
//              device about data has been writen to the output.
//              You can select write to which register causes "propagete" pulse.
//              dwRegMask format is <r7 r6 r5 r4 r3 r2 r1 r0> where rx are
//              16-bit registers. PD2-DIO64 has only fours registers, PD2-DIO128
//              eight of them.
//              1 in the dwRegMask means that write to this register will cause
//                pulse on "porpagate" line
//              0 in the dwRegMask means that write to this register will not
//                affect this line
//              Example: dwRegMask = 0xF0. It means that any write to the bank 1
//              will cause pulse on "propagate" line and writes to the bank 0
//              will not.
//-
int _PdDIOPropEnable(int handle, DWORD dwRegMask)
{
     DWORD dwRegM = ~dwRegMask;
     BOOL  bRes = TRUE;

     bRes &= _PdDIO256CmdWrite(handle,
                               __PdDIO256MakeRegMask(DIO_REG0 & 0xF, (dwRegM & 0xF)) | DIO_PWR,
                               0);

     bRes &= _PdDIO256CmdWrite(handle,
                               __PdDIO256MakeRegMask(DIO_REG4 & 0xF, (dwRegM & 0xF0)>>4 ) | DIO_PWR,
                               0);
     return bRes;
}

//+
// Function:    _PdDIOExtLatchEnable
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwBank -- bank
//              BOOL bEnable -- enable (1) or disable (0)
//
// Returns:     Negative error code or 0
//
// Description: Set or clear external latch enable bit for specified bank
//              bEnable = 0 - disable external latch line (default)
//              bEnable = 1 - enable external latch line
//
// Notes:       You can enable or disable external latch line for each
//              register bank separately. If the "latch" line is enabled
//              pulse on this line will cause input registers to store
//              input signal levels.
//              You can use _PdDIOExtLatchRead function to find out was data
//              latched or not. Use _PdDIOSimpleRead function to read latched
//              data
//-
int _PdDIOExtLatchEnable(int handle, DWORD dwRegister, BOOL bEnable)
{
    return _PdDIO256CmdWrite(handle,
                            (__PdDIO256MakeCmd(dwRegister & 0x4) | DIO_LWR | ((bEnable)?1:0)<<1),
                            0);
}

//+
// Function:    _PdDIOExtLatchRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwRegister -- bank
//              BOOL *bLatch  -- ptr to variable to store value
//
// Returns:     Negative error code; 0 if data wasn't latched and 1 if it was
//
// Description: Returns status of the external latch line
//
// Notes:       External latch pulse set external latch status bit to "1"
//              This function clears external latch status bit.
//-
int _PdDIOExtLatchRead(int handle, DWORD dwRegister, BOOL *bLatch)
{
    return _PdDIO256CmdRead(handle,
                            (__PdDIO256MakeCmd(dwRegister & 0x4) | DIO_LRD),
                            (DWORD*)bLatch);
}

//+
// Function:    _PdDIOIntrEnable
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD dwEnable  -- 0: disable, 1: enable DIO interrupts
//
// Returns:     Negative error code or 0
//
// Description: This function enables or disables host interrupt generation
//              for PD2-DIO board. Use _PdDIOSetIntrMask to set up DIO
//              interrupt mask
//
// Notes:
//-
int _PdDIOIntrEnable(int handle, DWORD dwEnable)
{
   tCmd   Cmd;
   int ret;

   Cmd.dwParam[0] = (DWORD)dwEnable;
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256INTRREENABLE, &Cmd);
   return ret;
}

//+
// Function:    _PdDIOSetIntrMask
//
// Parameters:  int handle -- handle to adapter
//              DWORD* dwIntMask -- interrupt mask (array of 8 DWORDs)
//
// Returns:     Negative error code or 0
//
// Description: This function sets up interrupt mask. PD2-DIO is capable to
//              generate host interrupt when selected bit changes its state.
//              dwIntMask is array of 8 DWORDs each of them correspondes to
//              one register of PD2-DIO board.
//              Only lower 16 bits are valid
//
// Notes:
//-
int _PdDIOSetIntrMask(int handle, DWORD* dwIntMask) 
{
   tCmd   cmd;
   int ret, i;
   

   for (i = 0; i < 8; i++)
   {
      ((DWORD*)cmd.bufParam.buffer)[i] = *(dwIntMask + i);
   }

   cmd.bufParam.size = 8*sizeof(DWORD);
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256SETINTRMASK, &cmd);
   return ret;
}

//+
// Function:    _PdDIOGetIntrData
//
// Parameters:  int handle -- handle to adapter
//              DWORD* dwIntData    -- array to store int data (8 DWORDs)
//              DWORD* dwEdgeData   -- array to store edge data (8 DWORDs)
//
// Returns:     Negative error code or 0
//
// Description: Function returns cause of interrupt
//              dwIntData contains "1" in position where bits have changed
//              their states. Only LSW is valid
//              dwEdgeData bits are valid only in the positions where dwIntData
//              contains "1"s. If a bit is "1" - rising edge caused the
//              interrupt, if a bit is "0" - falling edge occurs.
//
// Notes:       AND dwEdgeData and dwIntData with dwIntMask to mask "not care"
//              bits
//
//-
int _PdDIOGetIntrData(int handle, DWORD* dwIntData, DWORD* dwEdgeData)
{ 
   tCmd cmd;
   int ret, i;
      
   cmd.bufParam.size = 16*sizeof(DWORD);
      
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIO256GETINTRDATA, &cmd);

   for (i = 0; i < 8; i++) 
   {
      *(dwIntData+i) = ((DWORD*)cmd.bufParam.buffer)[i];
      *(dwEdgeData+i) = ((DWORD*)cmd.bufParam.buffer)[i+8];
   }

   return ret;
}

//+
// ---------------------------------------------------------------------------
// Function:    _PdDIOSetIntCh
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              DWORD dwChannels -- channels in sequence
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Sets number of active channels for DIO interrupt on line state
//              changes. 0 - disables line state checking
//
// Notes: See _PdDIOSetIntrMask and _PdDIOGetIntrData to program DIO interrupts
//        When interrupt happens eDInEvent is issued
//
// ----------------------------------------------------------------------
//-
int _PdDIOSetIntCh(int handle, DWORD dwChannels)
{
   return -EIO;
}


//+
// Function:    _PdDIODMASet
//
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwOffset -- DSP DMA channel 0 offset register
//              DWORD dwCount -- DSP DMA channel 0 count register
//              DWORD dwSource -- DSP DMA channel 0 source register
//
// Returns:     Negative error code or 0
//
// Notes: Use constants frm pdfw_def.h for proper operations
//
//-
int _PdDIODMASet(int handle, DWORD dwOffset, DWORD dwCount, DWORD dwSource)
{
   tCmd Cmd;
   int ret;
   
   Cmd.dwParam[0] = dwOffset;
   Cmd.dwParam[1] = dwCount;
   Cmd.dwParam[2] = dwSource;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_DIODMASET, &Cmd);
   return ret;
}


//--- AO32 Subsystem Commands: ------------------------------------------
//+
// Function:    _PdAO32Reset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: Reset PD2-AO32 subsystem to 0V state
//
// Notes:
//-
int _PdAO32Reset(int handle)
{
    BOOL    bRes = TRUE;
    DWORD   i;

    // remove any update channels
    bRes &= _PdDIO256CmdRead(handle, 0, &i);

    // write zeroes to all DACs
    for (i = 0; i < 32; i++)
        bRes &= _PdDIO256CmdWrite(handle, i|AO32_WRPR|AO32_BASE, 0x7fff);

    return bRes;
}

int _PdAO96Reset(int handle)
{
    BOOL    bRes = TRUE;
    DWORD   i;

    // remove any update channels
    bRes &= _PdDIO256CmdRead(handle, 0, &i);

    // write zeroes to all DACs
    for (i = 0; i < 96; i++)
        bRes &= _PdDIO256CmdWrite(handle, AOB_DACBASE | i, 0x7fff);

    return bRes;
}

//+
// Function:    _PdAO32Write
//
// Parameters:  int handle -- handle to adapter
//              WORD   wChannel -- number of channel to write
//              WORD   wValue -- value to write
//
// Returns:     Negative error code or 0
//
// Description: Write and update analog output immediately
//
// Notes:
//-
int _PdAO32Write(int handle, WORD wChannel, WORD wValue)
{
    return _PdDIO256CmdWrite(handle,
                             (wChannel & 0x7F)|AO32_WRPR|AO32_BASE, wValue);
}

int _PdAO96Write(int handle, WORD wChannel, WORD wValue)
{
    return _PdDIO256CmdWrite(handle, AOB_DACBASE |(wChannel&0x7f), wValue);
}


//+
// Function:    _PdAO32WriteHold
//
// Parameters:  int handle -- handle to adapter
//              WORD   wChannel -- number of channel to write
//              WORD   wValue -- value to write
//
// Returns:     Negative error code or 0
//
// Description: Write and update analog output upon PdAO32Update command
//
// Notes:
//-
int _PdAO32WriteHold(int handle, WORD wChannel, WORD wValue)
{
    return _PdDIO256CmdWrite(handle,
                             (wChannel & 0x7F)|AO32_WRH|AO32_BASE, wValue);
}

int _PdAO96WriteHold(int handle, WORD wChannel, WORD wValue)
{
    return _PdDIO256CmdWrite(handle,
                             AOB_DACBASE |(wChannel&0x7f)| AOB_AO96WRITEHOLD, wValue);
}

//+
// Function:    _PdAO32Update
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: Update all outputs with previously written values
//
// Notes:       Use this function in pair with _PdAO32Write.
//              Write values to the DACs you want to update. Values will
//              be stored in registers. Then _PdAO32Update outputs stored
//              values to DACs
//-
int _PdAO32Update(int handle)
{
    DWORD   dwValue;
    return _PdDIO256CmdRead(handle, AO32_UPDALL|AO32_BASE, &dwValue);
}

int _PdAO96Update(int handle)
{
    DWORD   dwValue;
    return _PdDIO256CmdRead(handle, AOB_DACBASE, &dwValue);
}

//+
// Function:    _PdAO32SetUpdateChannel
//
// Parameters:  int handle -- handle to adapter
//              WORD   wChannel -- number of channel to write
//              BOOL   bEnable -- TRUE to enable, FALSE to release
//
// Returns:     Negative error code or 0
//
// Description: Set channel number writing to which updates all values
//
// Notes:       You can set channel which will trigger DACs update line
//              You might want to write data to all needed registers and
//              update them on the last write to selected register
//
//-
int _PdAO32SetUpdateChannel(int handle, WORD wChannel, BOOL bEnable)
{
    DWORD   dwValue;
    return _PdDIO256CmdRead(handle,
                            wChannel | AO32_SETUPDMD | AO32_BASE | ((bEnable)?AO32_SETUPDEN:0),
                            &dwValue);
}

int _PdAO96SetUpdateChannel(int handle, WORD wChannel, DWORD Mode)
{
    return _PdDIO256CmdWrite(handle,
                            AOB_CTRBASE | wChannel | (Mode << 7), 0);
}

//+
// Function:    _PdAOODMASet
//
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwOffset -- DSP DMA channel 0 offset register
//              DWORD dwCount -- DSP DMA channel 0 count register
//              DWORD dwSource -- DSP DMA channel 0 source register
//
// Returns:     Negative error code or 0
//
// Notes: Use constants frm pdfw_def.h for proper operations
//
//-
int _PdAODMASet(int handle, DWORD dwOffset, DWORD dwCount, DWORD dwSource)
{
   tCmd Cmd;
   int ret;
   
   Cmd.dwParam[0] = dwOffset;
   Cmd.dwParam[1] = dwCount;
   Cmd.dwParam[2] = dwSource;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_AODMASET, &Cmd);
   return ret;
}

//--- UCT Subsystem Commands: -------------------------------------------
//
//+
// Function:    _PdUctSetCfg
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwUctCfg  -- UCT configuration word
//
// Returns:     Negative error code or 0
//
// Description: The set User Counter/Timer configuration command sets the
//              clock and gate for the specified user counter/timer.
//
// Notes:       1. UCT configuration bits are defined in pdfw_def.h
//
// UTB_CLK0       UCT 0 Clock Source (2 bits)
// UTB_CLK0_1     00 - SW clock; 01 - Internal 1MHz clock
//                11 - external clock
// UTB_CLK1       UCT 1 Clock Source (2 bits)
// UTB_CLK1_1     00 - SW clock; 01 - Internal 1MHz clock
//                10 - UCT0 output; 11 - external clock
// UTB_CLK2       UCT 2 Clock Source (2 bits)
// UTB_CLK2_1     00 - SW clock; 01 - Internal 1MHz clock
//                10 - UCT0 output; 11 - external clock
// UTB_GATE0      UCT 0 Gate Source bit: 0 - SW, 1 - external gate
// UTB_GATE1      UCT 1 Gate Source bit: 0 - SW, 1 - external gate
// UTB_GATE2      UCT 2 Gate Source bit: 0 - SW, 1 - external gate
// UTB_SWGATE0    UCT 0 SW Gate Setting bit: 0 - UCT disable, 1 - UCT enable (gate high)
// UTB_SWGATE1    UCT 1 SW Gate Setting bit: 0 - UCT disable, 1 - UCT enable (gate high)
// UTB_SWGATE2    UCT 2 SW Gate Setting bit: 0 - UCT disable, 1 - UCT enable (gate high)
//
//              2. To write value to UCT (82C54) you need to have some clock on
//                its input. The best way to do is to enable internal 1Mhz clock
//                and disable counting (put gate low)
//
//              3. Check datasheet of Intel 82C54 for further details
//-
int _PdUctSetCfg(int handle, DWORD dwUctCfg)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = (DWORD)dwUctCfg;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTSETCFG, &Cmd);
    return ret;
}

//+
// Function:    _PdUctGetStatus
//
// Parameters:  int handle -- handle to adapter
//              DWORD* pdwStatus -- UCT subsystem status
//
// Returns:     Negative error code or 0
//
// Description: The UCT status command obtains the output levels and
//              latched event bits that signaled an event of the three
//              user counter/timers (0, 1, 2).
//
//              UctStatus format:
//
//                  $000 00xx
//
//                      bbb bbb
//                       |   |__UCTxOut
//                       |_____UCTxIntrSC
//
// UCT status word format is defined in pdfw_def.h:
//
// UTB_LEVEL0   UCT 0 Output Level
// UTB_LEVEL1   UCT 1 Output Level
// UTB_LEVEL2   UCT 2 Output Level
// UTB_INTR0    UCT 0 Latched Interrupt
// UTB_INTR1    UCT 1 Latched Interrupt
// UTB_INTR2    UCT 2 Latched Interrupt
//
// Notes:
//-
int _PdUctGetStatus(int handle, DWORD *pdwStatus)
{
    tCmd   Cmd;
    int    ret;

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTSTATUS, &Cmd);
    *pdwStatus = Cmd.dwParam[0];

    return ret;
}

//+
// Function:    _PdUctWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwUctWord -- data and command combined together
//
// Returns:     Negative error code or 0
//
// Description: The UCT Write command writes two or three bytes to the
//              specified user counter/timer registers.
//
//              dwUctWord format:
//              31     24       16        8        0
//              |xxxxxxxx|___MSB__|___LSB__|_cntrl__|
//
//              cntrl - UCT control word. See below
//
// Notes:
// 1. Following definitions are useful to work with UCT subsystem
//
// 82C54 Control Word Format (CWF):
//
// bit definitions
// #define UCT_BCD     (1<<0)   // BCD mode (0 - binary 16-bit cntr, 1 - BCD cntr)
// #define UCT_M0      (1<<1)   // mode bit
// #define UCT_M1      (1<<2)   // mode bit
// #define UCT_M2      (1<<3)   // mode bit
// #define UCT_RW0     (1<<4)   // read/write mode
// #define UCT_RW1     (1<<5)   // read/write mode
// #define UCT_SC0     (1<<6)   // counter select
// #define UCT_SC1     (1<<7)   // counter select
//
// a. counter select
// #define UCT_SelCtr0     (0)                // select counter 0
// #define UCT_SelCtr1     (UCT_SC0)          // select counter 1
// #define UCT_SelCtr2     (UCT_SC1)          // select counter 2
// #define UCT_ReadBack    (UCT_SC0|UCT_SC1)  // Read-Back Command
//
// b. mode select
// #define UCT_Mode0       (0)              // output high on terminal count
// #define UCT_Mode1       (UCT_M0)         // retriggerable one-shot (use gate to retrigger)
// #define UCT_Mode2       (UCT_M1)         // rate generator
// #define UCT_Mode3       (UCT_M0|UCT_M1)  // square wave generator
// #define UCT_Mode4       (UCT_M2)         // software triggered strobe
// #define UCT_Mode5       (UCT_M0|UCT_M2)  // hardware triggered strobe
//
// c. read/write mode
// #define UCT_RWlsb       (UCT_RW0)            // r/w LSB only
// #define UCT_RWmsb       (UCT_RW1)            // r/w MSB only
// #define UCT_RW16bit     (UCT_RW0|UCT_RW1)    // r/w LSB first then MSB
// #define UCT_CtrLatch    (0)                  // Counter Latch Command (read)
//
// You need to combine a+b+c to write command to the counter
//
// 2. To write value to UCT (82C54) you need to have some clock on
//   its input. The best way to do is to enable internal 1Mhz clock
//   and disable counting (put gate low)
//
//-
int _PdUctWrite(int handle, DWORD dwUctWord)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = (DWORD)dwUctWord;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTWRITE, &Cmd);
    return ret;
}

//+
// Function:    _PdUctRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwUctReadCfg  -- UCT Read format word
//              DWORD *pdwUctWord   -- UCT Word to store word read
//
// Returns:     Negative error code or 0
//
// Description: The UCT Read command reads 0, 1, 2, or 3 bytes from the
//              specified user counter/timer registers.
//
// Notes:
//
// For read operation PDx-MFx UCT subsystem has a control word format:
//          15      8         0
//          |_FWCW___|_CWF____|  , FWCW is a firmware control word
//
// Use following definitions to make dwUctReadCfg
//
// a. operation parameters - firmware control word (bits 8-15)
//
// #define UCTREAD_CFW     (1<<8)   // use command-word format (CWF)(see _PdUctWrite)
//                                  // if this bit is 0 function ignores CWF bits
// #define UCTREAD_UCT0    (0)      // counter 0 (lines A1, A0)
// #define UCTREAD_UCT1    (1<<9)   // counter 1 (lines A1, A0)
// #define UCTREAD_UCT2    (2<<9)   // counter 2 (lines A1, A0)
// #define UCTREAD_0BYTES  (0)      // read 0 bytes
// #define UCTREAD_1BYTE   (1<<11)  // read 1 byte.  data: [LSB]
// #define UCTREAD_2BYTES  (2<<11)  // read 2 bytes. data: [MSB, LSB]
// #define UCTREAD_3BYTES  (3<<11)  // read 3 bytes. data: [MSB, LSB, StatusByte]
//
// b. There's rare need to program CWF yourself. The simplest way to read from
//  UCT is to combine flags UCTREAD_UCTx + UCTREAD_yBYTES
//
//  if UCTREAD_CFW is "1" you have to supply CWF (it it's "0" FW forms CWF
//  automatically).
//  If you've selected "1" to provide read configuration command yourself you
//  can chose one of the three read methods:
//  1. read  2. counter latch  3. read-back
//
//  See Intel 82C54 datasheet for details
//-
int _PdUctRead(int handle, DWORD dwUctReadCfg, DWORD *pdwUctWORD)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = (DWORD)dwUctReadCfg;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTREAD, &Cmd);
    *pdwUctWORD = Cmd.dwParam[1];

    return ret;
}

//+
// Function:    _PdUctSwSetGate
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwGateLevels -- gate levels to set for each
//                                    counter/timer
//
// Returns:     Negative error code or 0
//
// Description: The SW UCT gate setting command sets the UCT gate input
//              levels of the specified User Counter/Timers, thus enabling
//              or disabling counting by software command.
//
//                        2  1  0
//              Format: [g2 g1 g0] - set 0 to put gate low, 1 to put gate high
//
// Notes:
//
//-
int _PdUctSwSetGate(int handle, DWORD dwGateLevels)
{
    tCmd   Cmd;
    int    ret;

    Cmd.dwParam[0] = (DWORD)dwGateLevels;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTSWGATE, &Cmd);
    return ret;
}

//+
// Function:    _PdUctSwClkStrobe
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The SW UCT clock strobe command strobes the UCT clock
//              input of all User Counter/Timers that are configured for
//              the SW Command Clock Strobe.
//
// Notes:
//
//-
int _PdUctSwClkStrobe(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTSWCLK, NULL);
    return ret;
}

//+
// Function:    _PdUctReset
//
// Parameters:  int handle -- handle to adapter
//
// Returns:     Negative error code or 0
//
// Description: The reset UCT command resets the UCT subsystem to the
//              default startup configuration.  All operations in progress
//              are stopped and all configurations are cleared.
//
// Notes:
//-
int _PdUctReset(int handle)
{
    int ret;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UCTRESET, NULL);
    return ret;
}

// ----------------------------------------------------------------------
// Function:    _PdUctSetMode
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwCounter   -- counter to use
//              DWORD dwSource -- clock and gate sources
//              DWORD dwMode -- mode to use
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Preconfigures UCT to use in particular mode.
//
//   dwMode -- see 82C54 datasheet (modes 0-5)
//            UCT_MDEVENTCNT - event counting
//            UCT_MDPULSEGEN - pulse generation
//            UCT_MDTRAINGEN - pulse train generation
//            UCT_MDSQWAVEGEN - square wave generation
//            UCT_MDEVENTGEN - driver event generation
//
//   dwCounter -- 0,1 or 2
//   dwSource -- UCT_INT1MHZCLK - 1MHz internal timebase
//               UCT_EXTERNALCLK - external clock
//               UCT_SWCLK - sw strobes
//               UCT_UCT0OUTCLK - output of UCT0 as a clock
//               combine clock source with gate source
//               UCT_SWGATE - software controls gates
//               UCT_HWGATE - external gating
//
// Notes: 1. PDx-MFx boards only
//        2. If SW gate is selected it's set to "low"
//           Use _PdUctSwSetGate to control SW gates
// ----------------------------------------------------------------------
//-
int _PdUctSetMode(int handle, DWORD dwCounter, 
                  DWORD dwSource, DWORD dwMode)
{
    return -EIO;
}
//+
// ----------------------------------------------------------------------
// Function:    _PdUctWriteValue
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwCounter -- counter to write value to
//              WORD wValue   -- value to write
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Write 16-bit value
//
//
// ----------------------------------------------------------------------
//-
int _PdUctWriteValue(int handle, DWORD dwCounter, WORD wValue)
{
   return -EIO;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdUctReadValue
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwCounter -- counter to write value from
//              WORD* wValue   -- ptr to store value
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Read 16-bit value
//
// ----------------------------------------------------------------------
//-
int _PdUctReadValue(int handle, DWORD dwCounter, WORD* wValue)
{
    return -EIO;
}


//+
// ----------------------------------------------------------------------
// Function:    _PdUctFrqCounter
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwCounter -- counter to write value from
//              float fTime -- set up counter timebase (10us - 1s)
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Call this function to initiate frequency counting
//              It takes time (fTime) to complete the operation.
//              Call _PdUctFrqCounterStatus to get status of operation
//
//
// ----------------------------------------------------------------------
//-
int _PdUctFrqCounter(int handle, DWORD dwCounter, float fTime)
{
   return -EIO;
}

//+
// ----------------------------------------------------------------------
// Function:    _PdUctFrqGetValue
//
// Parameters:  HANDLE hAdapter -- handle to adapter
//              DWORD dwCounter -- counter to write value from
//              int* nFrequency   -- frequency
//
// Returns:     BOOL Status     -- TRUE:  command succeeded
//                                 FALSE: command failed
//
// Description: Returns status of frequency counting operation.
//              if nFrequency == -1 then operation is not completed.
//              otherwise nFrequency returns counts.
//
// ----------------------------------------------------------------------
//-
int _PdUctFrqGetValue(int handle, DWORD dwCounter, int* nFrequency)
{
   return -EIO;
}



//--- Calibration Commands: ---------------------------------------------
int _PdCalSet(int handle, DWORD dwCalCfg)
{
    return -ENOSYS;
}

//+
// Function:    _PdCalDACWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwCalDACValue -- Cal DAC adrs and value to output
//                                     bits 0-7:  8bit value to output
//                                     bits 8-10: 3bit DAC output select
//                                     bit  11:   Cal DAC 0/1 select
//
// Returns:     Negative error code or 0
//
// Description: The Cal DAC Write command writes the DAC select adrs and
//              value to the specified calibration DAC.
//
// Notes:       This function updates the driver's AIn configuration and
//              calibration table.
//
//-
int _PdCalDACWrite(int handle, DWORD dwCalDACValue)
{
    tCmd   Cmd;
    int ret;
    Cmd.dwParam[0] = (DWORD)dwCalDACValue;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_CALDACWRITE, &Cmd);
    return ret;
}


//+
// Function:    _CalDACSet
//
// Parameters:  int handle -- handle to adapter
//              DWORD *pError   -- ptr to last error status
//              int nDAC        -- Cal DAC selection (0/1)
//              int nOut        -- DAC Output selection (0-7)
//              int nValue      -- Value to output (0-255)
//
// Returns:     Negative error code or 0
//
// Description: Calls function PDFWCalDACWrite() to set calibration DAC.
//
// Notes:       * THIS FUNCTION IS NOT PART OF THE INTERFACE *
//
//-
int _PdCalDACSet(int handle, int nDAC, int nOut, int nValue)
{
    DWORD dwCalValue;
    dwCalValue = (DWORD)(((nDAC & 1) << 12) | ((nOut & 0x7) << 8) | (nValue & 0xff));
    return _PdCalDACWrite(handle, dwCalValue);
}
//+
// Function:    _PdDspRegWrite
//
// Parameters:  int handle -- handle to adapter
//              DWORD reg   -- DSP register to write to
//              DWORD data -- data to write
//
// Returns:     Negative error code or 0
//
// Description: This functions allows to write to specified DSP register
//
//-
int _PdDspRegWrite(int handle, DWORD reg, DWORD data)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = reg;
    Cmd.dwParam[1] = data;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDREGWR, &Cmd);
    return ret;
}

//+
// Function:    _PdDspRegRead
//
// Parameters:  int handle -- handle to adapter
//              DWORD reg   -- DSP register to read from
//              DWORD *data -- data read
//
// Returns:     Negative error code or 0
//
// Description: This functions allows to read specified DSP register
//
//-
int _PdDspRegRead(int handle, DWORD reg, DWORD *data)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = reg;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_BRDREGRD, &Cmd);
    *data = Cmd.dwParam[1];

    return ret;
}

//+
// Function: _PdWaitForEvent
//
// Parameters: int handle -- handle to subsystem
//             int event -- event flags (see _PdSetUserEvents)
//             int timeoutms -- ms to wait event
//
// Returns: Event flags associated with adapter subsystem (lower 24 bits)
//          Negative on error
//
// Description: This function issues blocking I/O request to
//              the adapter subsystem associated with the handle
//              If event happened before timeout (in ms) expires
//              function return event flags (24 bits)
//              Function returns -ETIMEDOUT if no event happens in timeoutms 
//              nilliseconds.
//              Function returns -EIO on other errors
//              If timeoutms = 0x7fffffff function waits indefinetly (MAX_SCHEDULE_TIMEOUT = (~0UL>>1)
//              If timeoutms = 0 function returns pending events (no wait)
//              
// Note: this is a blocking call to the driver. It means that your process
//       will not be executed until function returns. Thus, a good idea is
//       to call this function from the separate thread responsible of
//       handling events for particular subsystem or child process created
//       using fork()
//-
int _PdWaitForEvent(int handle, int events, int timeoutms) {

    // case  IOCTL_PWRDAQ_PRIVATE_SET_EVENT:  // put to sleep...
            // ((PtCmd)argument)->dwParam[0], // Event
            // ((PtCmd)argument)->dwParam[1], // Timeout
	    
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = (DWORD)events;
    Cmd.dwParam[1] = (DWORD)timeoutms;
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_PRIVATE_SET_EVENT, &Cmd);
    return ret;
}

int _PdSleepOn(int handle, int timeoutms) {
    tCmd Cmd;
    Cmd.dwParam[0] = (DWORD)timeoutms;
    return PD_IOCTL(handle, IOCTL_PWRDAQ_PRIVATE_SLEEP_ON, &Cmd);
}

int _PdWakeUp(int handle) {
    tCmd Cmd;
    return PD_IOCTL(handle, IOCTL_PWRDAQ_PRIVATE_WAKE_UP, &Cmd);
}

//-----------------------------------------------------------------------
int _PdTestEvent(int handle)
{
    tCmd Cmd;
    memset(&Cmd,0,sizeof(tCmd));
    return PD_IOCTL(handle, IOCTL_PWRDAQ_TESTEVENTS, &Cmd);
}

//----------------------------------------------------------------------------
int _PdGetADCFifoSize(int handle, DWORD *fifoSize)
{
   tCmd   Cmd;
   int ret;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GETADCFIFOSIZE, &Cmd);
   *fifoSize = Cmd.dwParam[0];

   return ret;
}


int _PdGetSerialNumber(int handle, char serialNumber[PD_SERIALNUMBER_SIZE])
{
   tCmd   Cmd;
   int ret;

   Cmd.bufParam.size = PD_SERIALNUMBER_SIZE;
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GETSERIALNUMBER, &Cmd);
   memcpy(serialNumber, Cmd.bufParam.buffer, PD_SERIALNUMBER_SIZE);
   
   return ret;
}

int _PdGetManufactureDate(int handle, char manfctrDate[PD_DATE_SIZE])
{
   tCmd  Cmd;
   int   ret;

   Cmd.bufParam.size = PD_DATE_SIZE;
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GETMANFCTRDATE, &Cmd);
   memcpy(manfctrDate, Cmd.bufParam.buffer, PD_DATE_SIZE);
   
   return ret;
}


int _PdGetCalibrationDate(int handle, char calDate[PD_DATE_SIZE])
{
   tCmd  Cmd;
   int   ret;

   Cmd.bufParam.size = PD_DATE_SIZE;
   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GETCALIBRATIONDATE, &Cmd);
   memcpy(calDate, Cmd.bufParam.buffer, PD_DATE_SIZE);

   return ret;
}


#ifdef MEASURE_LATENCY
int _PdGetLastIntTsc(int handle, unsigned int tsc[2])
{
   tCmd   Cmd;
   int ret;

   ret = PD_IOCTL(handle, IOCTL_PWRDAQ_MEASURE_LATENCY, &Cmd);
   tsc[0] = Cmd.dwParam[0];
   tsc[1] = Cmd.dwParam[1];
   
   return ret;
}
#endif


//////////////////////////////////////////////////////////////////////////
// Auxilary function to show analog input events
void showEvents(DWORD events)
{
    char str[256];

    DPRINTK( "events:   0x%x\n", events);
    str[0] = '\0';
    if ( events & eTimeout)		strcat( str, "Timeout ");
    if ( events & eStopped)		strcat( str, "Stopped ");

    if ( events & eTrigError)		strcat( str, "TrigError ");
    if ( events & eBufferError)		strcat( str, "BufferError ");
    if ( events & eDataError)		strcat( str, "DataError ");
    if ( events & eScanError)		strcat( str, "ScanError ");

    if ( events & eConvError)		strcat( str, "ConvError ");
    if ( events & eBufferWrapped)	strcat( str, "BufferWrapped ");
    if ( events & eBufListDone)		strcat( str, "BufListDone ");
    if ( events & eBufferDone)		strcat( str, "BufferDone ");

    if ( events & eBlockDone)		strcat( str, "BlockDone ");
    if ( events & eFrameRecycled)	strcat( str, "FrameRecycled ");
    if ( events & eFrameDone)		strcat( str, "FrameDone ");
    if ( events & eScanDone)		strcat( str, "ScanDone ");

    if ( events & eDataAvailable)	strcat( str, "DataAvailable ");
    if ( events & eInputTrig)		strcat( str, "InputTrig ");
    if ( events & eStopTrig)		strcat( str, "StopTrig ");
    if ( events & eStartTrig)		strcat( str, "StartTrig ");
    DPRINTK ( "%s\n", str);
}

// The following functions can't be used from a real-time context
#ifndef _PD_RTLPRO

//--- Buffering functions ------------------------------------------------
//
//+
// Function:    int _PdRegisterBuffer
//
// Parameters:  int handle -- handle to adapter
//              PWORD* pBuffer -- pointer to store buffer address
//              DWORD dwSubsystem -- subsystem (AnalogIn or AnalogOut)
//              DWORD dwFramesBfr -- number of frames in buffer
//              DWORD dwScansFrm -- number of scans in the frame
//              DWORD dwScanSize -- channel list (scan) size
//              DWORD  bWrapAround -- buffering mode
//                  buffering modes:
//                  0 - single run (acquisition stops after buffer becomes full)
//                  AIB_BUFFERWRAPPED - circular buffer
//                  AIB_BUFFERRECYCLED - circular buffer with frame recycling
//
// Returns:     actual number of bytes allocated or negative value on error
//
// Description: function allocates data buffer
//
// Memory is allocated at the kernel space and then mmaped to the user space
// Driver doesn't touch data after DMA it from the board
//
// Function returns actual number of bytes allocated or negative value
// on error
//-
int _PdRegisterBuffer(int handle,PWORD* pBuffer,
                                 DWORD dwSubsystem,
                                 DWORD dwFramesBfr,
                                 DWORD dwScansFrm,
                                 DWORD dwScanSize,
                                 DWORD  bWrapAround)
{

    tCmd   Cmd;
    int ret;
    void* buf;
    int sizebytes;
    *pBuffer = NULL;

    Cmd.dwParam[0] = dwScanSize;
    Cmd.dwParam[1] = dwScansFrm;
    Cmd.dwParam[2] = dwFramesBfr;
    Cmd.dwParam[3] = bWrapAround;
    Cmd.dwParam[4] = dwSubsystem;

    DPRINTK("PdRegistereBuffer: %d samples, %d scans, %d frames\n", Cmd.dwParam[0], Cmd.dwParam[1], Cmd.dwParam[2]);

    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_REGISTER_BUFFER, &Cmd);

    // This ioctl returns the true buffer size allocated by the driver
    sizebytes = Cmd.dwParam[0];
    

    DPRINTK("Mapping buffer, size %d\n", sizebytes);

    // mmap buffer allocated in kernel
    if (ret < 0) return ret;
   buf = mmap(NULL, sizebytes,
              PROT_WRITE|PROT_READ,MAP_SHARED|MAP_FILE,
              handle, 0);
   if( buf == (void *) -1 ) return -EINVAL;
    *pBuffer = buf;

    return ret;
}


//+
// Function:    int _PdUnregisterBuffer
//
// Parameters:  int handle -- handle to adapter
//              PWORD* pBuffer -- pointer to store buffer address
//              DWORD dwSubsystem -- subsystem (AnalogIn or AnalogOut)
//
// Returns:     negative value on error
//
// Description: function deallocates data buffer
//
//-
int _PdUnregisterBuffer(int handle, PWORD pBuf, DWORD dwSubSystem)
{
    tCmd   Cmd;
    int ret;

    Cmd.dwParam[0] = dwSubSystem;

    // ioctl returns size of allocated buffer in Cmd.dwParam[1]
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_GETKERNELBUFSIZE, &Cmd);

    DPRINTK("Unmapping buffer, size %d\n", Cmd.dwParam[1]);

    // unmap buffer allocated in kernel
    if (!Cmd.dwParam[1]) 
        return -EIO;
    munmap(pBuf, Cmd.dwParam[1]);

    // free buffer
    Cmd.dwParam[0] = dwSubSystem;

    // ioctl returns size of allocated buffer in Cmd.dwParam[1]
    ret = PD_IOCTL(handle, IOCTL_PWRDAQ_UNREGISTER_BUFFER, &Cmd);

    return ret;
}

//
//+
// Function:    int _PdAcquireBuffer
//
// Parameters:  int handle -- handle to adapter
//              PWORD* pBuffer -- pointer to store buffer address
//              DWORD dwFramesBfr -- number of frames in buffer
//              DWORD dwScansFrm -- number of scans in the frame
//              DWORD dwScanSize -- channel list (scan) size
//              DWORD dwSubsystem -- subsystem
//              DWORD dwMode -- buffering mode
//                  buffering modes:
//                  0 - single run (acquisition stops after buffer becomes full)
//                  BUF_BUFFERWRAPPED - circular buffer
//                  BUF_BUFFERRECYCLED - circular buffer with frame recycling
//                  BUF_DWORDVALUES - Select the size of each sample for analog output
//
// Returns:     actual number of bytes allocated or negative value on error
//
// Description: function allocates data buffer
//
// Memory is allocated at the kernel space and then mmaped to the user space
// Driver doesn't touch data after DMA it from the board
//
// Function returns actual number of bytes allocated or negative value
// on error
//-
int _PdAcquireBuffer(int handle,void** pBuffer,
                                 DWORD dwFramesBfr,
                                 DWORD dwScansFrm,
                                 DWORD dwScanSize,
                                 DWORD dwSubsystem,
                                 DWORD dwMode)
{
   return _PdRegisterBuffer(handle, (WORD**)pBuffer, dwSubsystem, dwFramesBfr, dwScansFrm, dwScanSize, dwMode);
}

//+
// Function:    int _PdReleaseBuffer
//
// Parameters:  int handle -- handle to adapter
//              DWORD dwSubsystem -- subsystem
//              PWORD* pBuffer -- pointer to store buffer address 
//
// Returns:     negative value on error
//
// Description: function deallocates data buffer
//
//-
int _PdReleaseBuffer(int handle, DWORD dwSubSystem, void* pBuf)
{
   return _PdUnregisterBuffer(handle, pBuf, dwSubSystem);
}

//+
// Function:    _PdSetAsyncNotify
//
// Parameters:  int handle -- handle to adapter
//              struct sigaction *io_act -- structure to store sigaction
//              void (*sig_proc)(int) -- function to call upon signal
//
// Returns:     Negative error code or 0
//
// Description: Sets up event notification handler for user application
//
// Notes:
//
//-
int _PdSetAsyncNotify(int handle, struct sigaction *io_act, void (*sig_proc)(int))
{
    int flags;

    // register SIGIO handler
    io_act->sa_handler = sig_proc;
    sigemptyset(&io_act->sa_mask);
    io_act->sa_flags = 0;
    if (sigaction(SIGIO, io_act, NULL) != 0) {
        return -EIO;
    }

    // register for asynchronous notification of driver events
    if (fcntl(handle, F_SETOWN, getpid()) != 0) {
      return -EIO;
    }

    flags = fcntl(handle, F_GETFL);
    if (flags == -1) {
      return -EIO;
    }

    if (fcntl(handle, F_SETFL, flags | FASYNC) == -1) {
      return -EIO;
    }
   return 0;
}

// Those two functions are called when the shared library is loaded
// by a process
void my_init()
{
   int ret;
   int i;
   int bFirst = FALSE;

   DPRINTK("Loading powerdaq32 library\n");

   G_NbBoards = PdGetNumberAdapters();
   if(G_NbBoards <= 0)
   {
      fprintf(stderr, "No PowerDAQ board detected!\n");
      return;
   }

   DPRINTK("Found %d board(s)\n", G_NbBoards);

   // Try to grab the shared memory segment. If this is the first instance 
   // of the DLL to load, this will fail and we'll create it instead
   shmid = shmget(shmkey, G_NbBoards * sizeof(Adapter_Info), 0777);
   if(shmid == -1)
   {
      DPRINTK("shmget failed errno=%d, this is the first instance of the DLL.\n", errno);

      bFirst = TRUE;
      
      // Allocate the segment of shared memory
      shmid = shmget(shmkey, G_NbBoards * sizeof(Adapter_Info), 0777 | IPC_CREAT);
      if(shmid == -1)
      {
         fprintf(stderr ,"libpowerdaq32: shmget failed (%s)\n", strerror(errno));
         return;
      }
   }

   G_pAdapterInfo = (Adapter_Info *)shmat(shmid, (void *)0, 0);
   if(G_pAdapterInfo == (void *)-1)
   {
      fprintf(stderr ,"libpowerdaq32: shmat failed\n");
      return;
   }

   if(bFirst)
   {
      for(i= 0; i< G_NbBoards; i++)
      {
         ret = __PdGetAdapterInfo(i, &G_pAdapterInfo[i]);
         if(ret < 0)
         {
            fprintf(stderr, "libpowerdaq32: error retrieving adapter infos for board %d\n", i);
            return;
         }   
      }
   }

#ifdef PD_DEBUG
   for(i= 0; i< G_NbBoards; i++)
   {
      DPRINTK("Board %d is a %s\n", i, G_pAdapterInfo[i].lpBoardName);
      DPRINTK("\tBoard ID = %d\n", G_pAdapterInfo[i].dwBoardID);
      DPRINTK("\tBoard S/N = %s\n", G_pAdapterInfo[i].lpSerialNum);
   }
#endif
} 

void my_fini()
{
   struct shmid_ds shmds;
   
   DPRINTK("Unloading powerdaq32 library\n"); 

   if(shmdt(G_pAdapterInfo) == -1)
   {
      fprintf(stderr, "libpowerdaq32: shmdt failed (%s)\n", strerror(errno));
      return;
   }

   // destroy the shared memory segment if nobody else is using it
   if(shmctl(shmid, IPC_STAT, &shmds) == -1)
   {
      fprintf(stderr, "libpowerdaq32: shmctl(IPC_STAT) failed\n");
      return;
   } 

   if(shmds.shm_nattch == 0)
   {
      DPRINTK("No process is using the shared memory segment.\n");
      if(shmctl(shmid, IPC_RMID, 0) == -1)
      {
         fprintf(stderr, "libpowerdaq32: shmctl(IPC_RMID) failed\n");
         return;
      }
   }
}
#else
EXPORT_SYMBOL(PdAcquireSubsystem);
EXPORT_SYMBOL(PdGetNumberAdapters);
EXPORT_SYMBOL(PdGetPciConfiguration);
EXPORT_SYMBOL(_PdSetUserEvents);
EXPORT_SYMBOL(_PdClearUserEvents);
EXPORT_SYMBOL(_PdGetUserEvents);
EXPORT_SYMBOL(_PdImmediateUpdate);
EXPORT_SYMBOL(_PdWaitForEvent);
EXPORT_SYMBOL(_PdAdapterGetBoardStatus);
EXPORT_SYMBOL(_PdAdapterSetBoardEvents1);
EXPORT_SYMBOL(_PdAdapterSetBoardEvents2);
EXPORT_SYMBOL(_PdAdapterEnableInterrupt);
EXPORT_SYMBOL(_PdAdapterEepromRead);
EXPORT_SYMBOL(_PdAdapterEepromWrite); 
EXPORT_SYMBOL(_PdAInSetCfg);
EXPORT_SYMBOL(_PdAInSetCvClk);
EXPORT_SYMBOL(_PdAInSetClClk);
EXPORT_SYMBOL(_PdAInSetChList);
EXPORT_SYMBOL(_PdAInEnableConv);
EXPORT_SYMBOL(_PdAInSetEvents);
EXPORT_SYMBOL(_PdAInSwStartTrig);
EXPORT_SYMBOL(_PdAInSwStopTrig);
EXPORT_SYMBOL(_PdAInSwCvStart);
EXPORT_SYMBOL(_PdAInSwClStart);
EXPORT_SYMBOL(_PdAInResetCl);
EXPORT_SYMBOL(_PdAInClearData);
EXPORT_SYMBOL(_PdAInReset);
EXPORT_SYMBOL(_PdAInGetValue);
EXPORT_SYMBOL(_PdAInGetDataCount);
EXPORT_SYMBOL(_PdAInGetSamples);
EXPORT_SYMBOL(_PdAInGetXFerSamples);
EXPORT_SYMBOL(_PdAInSetXferSize);
EXPORT_SYMBOL(_PdAInSetSSHGain);
EXPORT_SYMBOL(_PdAOutSetCfg);
EXPORT_SYMBOL(_PdAOutSetCvClk);
EXPORT_SYMBOL(_PdAOutSetEvents);
EXPORT_SYMBOL(_PdAOutGetStatus);
EXPORT_SYMBOL(_PdAOutEnableConv);
EXPORT_SYMBOL(_PdAOutSwStartTrig);
EXPORT_SYMBOL(_PdAOutSwStopTrig);
EXPORT_SYMBOL(_PdAOutSwCvStart);
EXPORT_SYMBOL(_PdAOutClearData);
EXPORT_SYMBOL(_PdAOutReset);
EXPORT_SYMBOL(_PdAOutPutValue);
EXPORT_SYMBOL(_PdAOutPutBlock);
EXPORT_SYMBOL(_PdDInSetCfg);
EXPORT_SYMBOL(_PdDInGetStatus);
EXPORT_SYMBOL(_PdDInRead);
EXPORT_SYMBOL(_PdDInClearData);
EXPORT_SYMBOL(_PdDInReset);
EXPORT_SYMBOL(_PdDOutWrite);
EXPORT_SYMBOL(_PdDOutReset);
EXPORT_SYMBOL(_PdDIOReset);
EXPORT_SYMBOL(_PdDIOEnableOutput);
EXPORT_SYMBOL(_PdDIOLatchAll);
EXPORT_SYMBOL(_PdDIOSimpleRead);
EXPORT_SYMBOL(_PdDIORead);
EXPORT_SYMBOL(_PdDIOWrite);
EXPORT_SYMBOL(_PdDIOReadAll);
EXPORT_SYMBOL(_PdDIOWriteAll);
EXPORT_SYMBOL(_PdDIOPropEnable);
EXPORT_SYMBOL(_PdDIOExtLatchEnable);
EXPORT_SYMBOL(_PdDIOExtLatchRead);
EXPORT_SYMBOL(_PdDIOSetIntrMask);
EXPORT_SYMBOL(_PdDIOGetIntrData);
EXPORT_SYMBOL(_PdDIOIntrEnable);
EXPORT_SYMBOL(_PdDIOSetIntCh);
EXPORT_SYMBOL(_PdDIO256CmdWrite);
EXPORT_SYMBOL(_PdDIO256CmdRead);
EXPORT_SYMBOL(_PdDIO256CmdWriteAll);
EXPORT_SYMBOL(_PdDIO256CmdReadAll);
EXPORT_SYMBOL(_PdAO32Reset);
EXPORT_SYMBOL(_PdAO96Reset);
EXPORT_SYMBOL(_PdAO32Write);
EXPORT_SYMBOL(_PdAO96Write);
EXPORT_SYMBOL(_PdAO32WriteHold);
EXPORT_SYMBOL(_PdAO96WriteHold);
EXPORT_SYMBOL(_PdAO32Update);
EXPORT_SYMBOL(_PdAO96Update);
EXPORT_SYMBOL(_PdAO32SetUpdateChannel);
EXPORT_SYMBOL(_PdAO96SetUpdateChannel);
EXPORT_SYMBOL(_PdUctSetMode);
EXPORT_SYMBOL(_PdUctWriteValue);
EXPORT_SYMBOL(_PdUctReadValue);
EXPORT_SYMBOL(_PdUctFrqCounter);
EXPORT_SYMBOL(_PdUctFrqGetValue);
EXPORT_SYMBOL(_PdUctSetCfg);
EXPORT_SYMBOL(_PdUctGetStatus);
EXPORT_SYMBOL(_PdUctWrite);
EXPORT_SYMBOL(_PdUctRead);
EXPORT_SYMBOL(_PdUctSwSetGate);
EXPORT_SYMBOL(_PdUctSwClkStrobe);
EXPORT_SYMBOL(_PdUctReset);
/*EXPORT_SYMBOL(_PdDspCtLoad);
EXPORT_SYMBOL(_PdDspCtEnableCounter);
EXPORT_SYMBOL(_PdDspCtEnableInterrupts);
EXPORT_SYMBOL(_PdDspCtGetCount);
EXPORT_SYMBOL(_PdDspCtGetCompare);
EXPORT_SYMBOL(_PdDspCtGetStatus);
EXPORT_SYMBOL(_PdDspCtSetCompare);
EXPORT_SYMBOL(_PdDspCtSetLoad);
EXPORT_SYMBOL(_PdDspCtSetStatus);
EXPORT_SYMBOL(_PdDspPSLoad);
EXPORT_SYMBOL(_PdDspPSGetCount);
EXPORT_SYMBOL(_PdDspCtGetCountAddr);
EXPORT_SYMBOL(_PdDspCtGetLoadAddr);
EXPORT_SYMBOL(_PdDspCtGetStatusAddr);
EXPORT_SYMBOL(_PdDspCtGetCompareAddr);*/
EXPORT_SYMBOL(_PdCalSet);
EXPORT_SYMBOL(_PdCalDACWrite);
EXPORT_SYMBOL(_PdCalDACSet);
EXPORT_SYMBOL(_PdDspRegWrite);
EXPORT_SYMBOL(_PdDspRegRead);
EXPORT_SYMBOL(_PdGetAdapterInfo);
EXPORT_SYMBOL(_PdTestEvent);
#endif



//--- End of powerdaq32.c ------------------------------------------------
