//===========================================================================
//
// NAME:    rtl_dma_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ RTAI driver DMA-based AIn test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    18-JAN-2003
//
// HISTORY:
//
//      Rev 0.1,     18-JAN-2003,     Initial version.
//
//---------------------------------------------------------------------------
//  (C) 2003 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTAI hard interrupt handler to serve PowerDAQ board 
// DMA page done interrupts.
// This process writes a character each frame to an rt_fifo (0)
//
// modules pwrdaq.o, rtai_fifo.o, rtai_sched.o, rtai.o must be loaded before
// this one
// Insert the module with "insmod rtai_int_ai.o"
//
// View the output with "cat /dev/rtf0"
//
// PowerDAQ Linux driver pwdaq.o shall be compiled with
// make RTAI=1
//
//
// This was compiled using the RTAI 24.1.10 distribution on kernel 2.4.18,
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define data_fifo_id       0
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_CL_SIZE        0x100  // maximum number of CL entries on MFx board
#define CL_SIZE            2      // current CL size

#define CV_CLK 1099              // Conversion clock divider set for a
#define CL_CLK 1099              // sampling rate of 10000Hz

// Ain config 
#define AIN_CFG AIN_BIPOLAR |       \
                AIN_RANGE_10V |     \
                AIB_CVSTART0 |      \
                AIB_CLSTART0 |      \
                AIB_CLSTART1 |      \
                AIB_SELMODE1            

//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];
int hbrd = 1;  // first board in the system. See /proc/pwrdaq for board ID
u32 XBMPageSz; // size of a dma buffer, it is dependant on the FIFO size
u32 BmFHFXFers; // Size of the 1/2 FIFO measured in units of 512 DWORDS
u32 BmPageXFers; // Size of the chunk of sata written to the DMA buffer
                 // in units of 512 DWORDS
u32* BmBuffers[2] = {NULL, NULL};  // DMA buffers

u16* adc_data;

//==============================================================================
// ISR
int pd_isr(int trigger_irq) 
{
    int ret;
    int board = hbrd;
    int data_rtfifo = (int)data_fifo_id;
    int i, offs;
    char buf[1024];   // temp char buffer
    u32 page = AI_PAGE0;
    tEvents Events;
    u32 mask = 0;

    if ( pd_dsp_int_status(board) ) 
    { // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        
        // check for non-recoverable errors
        if (Events.AInIntr & AIB_BMErrSC) 
        {
           printk("rtai_dma_ai: BM error\n");
           pd_stop_and_disable_ain(board);
        } 
        else // is there new page of data available?
        if ((Events.AInIntr & AIB_BMPg0DoneSC)||(Events.AInIntr & AIB_BMPg1DoneSC)) 
        {
           mask |= Events.AInIntr & (AIB_BMPgDoneIm | 
                                     AIB_BMErrIm |
                                     AIB_StartIm | 
                                     AIB_StopIm | 
                                     AIB_BMEnabled | 
                                     AIB_BMActive);

           // reset event register
           if (Events.AInIntr & AIB_BMPg0DoneSC) 
           {
              page = AI_PAGE0;
           } 
           else if (Events.AInIntr & AIB_BMPg1DoneSC)
           {
              page = AI_PAGE1;
           }

           // Copy the page that was just done to temp buffer
           for(i=0; i<BmPageXFers * AIBM_TXSIZE; i++)
           {
              adc_data[i] = (u16)*((u32*)BmBuffers[page]+i);
           }

           pd_ain_set_events(board, mask);
	    
           // convert first 8 samples to output thru the fifo
           offs = 0;
           for (i = 0; i < CL_SIZE; i++)
              offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
           offs += sprintf(buf+offs, "\n");

           // throw data into rtl fifo
           ret = rtf_put(data_rtfifo, buf, offs+1);
        }
    }

    // reenable irq handling
    pd_adapter_enable_interrupt(board, 1);
    rt_enable_irq(pd_board[board].irq);

    return 0;
}

//==============================================================================
// AIn start
int AInStart (int brd) 
{
    int i, ret;
    u32 mask;
    u32 adc_cl[64] = {0};
    u32 BMList[4];
    u32 Buffer0PhysAddr;
    u32 Buffer1PhysAddr;
    
    ret = pd_ain_reset(brd);

    // fill CL with "slow bit" flag
    for (i = 0; i < CL_SIZE; i++) adc_cl[i] = i;

    ret = pd_ain_set_config(brd, AIN_CFG, 0, 0);
    ret &= pd_ain_set_cv_clock(brd, CV_CLK);
    ret &= pd_ain_set_cl_clock(brd, CL_CLK);

    ret &= pd_ain_set_channel_list(brd, CL_SIZE, adc_cl);

    // Set up bus mastering parameters
    XBMPageSz = 4;
    switch (pd_board[brd].Eeprom.u.Header.ADCFifoSize) 
    {
    case 1:
       BmFHFXFers = 1;
       BmPageXFers = 8;
       break;
    case 2:
       BmFHFXFers = 2;
       BmPageXFers = 8;
       break;
    case 4:
       BmFHFXFers = 4;
       BmPageXFers = 8;
       break;
    case 8:
       BmFHFXFers = 8;
       BmPageXFers = 8;
       break;
    case 16:
       BmFHFXFers = 16;
       BmPageXFers = 16;
       XBMPageSz = 8;
       break;
    case 32:
       BmFHFXFers = 16;
       BmPageXFers = 16;
       XBMPageSz = 8;
       break;
    case 64:
       BmFHFXFers = 16;
       BmPageXFers = 32;
       XBMPageSz = 16;
       break;
    default:
       BmFHFXFers = 1;
       BmPageXFers = 8;
       break;
    }

    BmBuffers[AI_PAGE0] = (u32*)kmalloc(XBMPageSz * PAGE_SIZE, GFP_KERNEL);
    BmBuffers[AI_PAGE1] = (u32*)kmalloc(XBMPageSz * PAGE_SIZE, GFP_KERNEL);

    Buffer0PhysAddr = (u32)virt_to_phys(BmBuffers[AI_PAGE0]);
    Buffer1PhysAddr = (u32)virt_to_phys(BmBuffers[AI_PAGE1]);

    BMList[0] = Buffer0PhysAddr >> 8;
    BMList[1] = Buffer1PhysAddr >> 8;
    BMList[2] = BmFHFXFers;  // size of 1/2 FIFO measured in transfer sizes AIBM_TXSIZE = 512
    BMList[3] = BmPageXFers; // size of each contigous physical pages in AIBM_TXSIZE

    // allocate temp buffer where we copy data after each page has
    // has been filled-up
    adc_data = (u16*)kmalloc(BmPageXFers * AIBM_TXSIZE, GFP_KERNEL);

    ret &= pd_ain_set_BM_ctr(brd, AIBM_SET, BMList);
    
    // enable events
    //mask2 = AIB_FFIm | AIB_FHFIm;  // FIFO full and half-full masks 
    mask = AIB_BMPgDoneIm | AIB_BMErrIm;
    ret &= pd_ain_set_events(brd, mask);
    ret &= pd_adapter_enable_interrupt(brd, 1);

    ret &= pd_ain_set_enable_conversion(brd, 1);
    ret &= pd_ain_sw_start_trigger(brd);

    return ret;
}

//==============================================================================
// AIn stop
int AInStop (int brd) 
{
    int ret;

    ret = pd_adapter_enable_interrupt(brd, 0);
    ret &= pd_ain_reset(brd); // this cleans all AIn configuration and triggers

    kfree(BmBuffers[AI_PAGE0]);
    kfree(BmBuffers[AI_PAGE1]);

    kfree(adc_data);

    return ret;
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
    printk("init_module rtai_int_ai\n");

    // create fifo we can talk via /dev/rtf0
    rtf_destroy(data_fifo_id);     // free up the resource, just in case
    rtf_create(data_fifo_id, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device

    // get hold on interrupt vector
    if(rt_request_global_irq(pd_board[hbrd].irq, (void(*)(void))pd_isr))
    {
        printk("rt_request_global_irq failed, try to load the pwrdaq driver with the option rqstirq=0\n");
        return -EIO;
    }

    // kick analog input
    AInStart(hbrd);

    // enable it
    rt_enable_irq(pd_board[hbrd].irq);

    return 0;
}


//==============================================================================
void cleanup_module(void)
{
    printk("cleanup_module rtai_int_ai\n");

    // stop analog input
    AInStop(hbrd);

    // get rid of fifo and thread
    rtf_destroy(data_fifo_id);      // free up the resource
    rt_free_global_irq(pd_board[hbrd].irq);  // release IRQ 
}
