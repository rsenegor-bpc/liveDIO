//===========================================================================
//
// NAME:    rtl_dma_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ RTLinux driver DMA-based AIn test program
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
// This implements a RTLinux hard interrupt handler to serve PowerDAQ board 
// DMA page done interrupts.
// This process writes a character each frame to an rt_fifo (0)
//
// Insert the module with "insmod rtl_dma_ai.o"
//
// View the output with "cat /dev/rtf0"
//
// PowerDAQ Linux driver pwdaq.o shall be compiled with
// make RTL=1
//
//
// This was compiled using RTLinux 3.2-pre2
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl.h>
#include <pthread.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#define data_fifo_id       0
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_CL_SIZE        0x100  // maximum number of CL entries on MFx board
#define CL_SIZE            2      // current CL size

#define CV_CLK 19              // Conversion clock divider set for a
#define CL_CLK 19              // sampling rate of 10000Hz

// Ain config
#define AIN_CFG AIN_BIPOLAR |       \
                AIN_RANGE_10V |     \
                AIB_CVSTART0 |      \
                AIB_CLSTART0 |      \
                AIB_CLSTART1 |      \
                AIB_SELMODE1


#define ADRAIBM_DEFDMASIZE	0x1D
#define ADRAIBM_DEFBURSTS	0x1E
#define ADRAIBM_DEFBURSTSIZE	0x1F

#define ADRREALTIMEBMMODE	0x1		// Enable/Disable Real Time Bus master mode

#define ADRRTModeAIBM_TRANSFERSIZEFIRST	0x23  	// size of first data block
#define ADRRTModeAIBM_TRANSFERSIZEALL	0x24  	// size of all data block
#define ADRRTModeAIBM_DEFDMASIZELAST	0x25  	// DMA burst size for last data block

//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];
int hbrd = 0;  // first board in the system. See /proc/pwrdaq for board ID
u32 XBMPageSz; // size of a dma buffer, it is dependant on the FIFO size
u32 BmFHFXFers; // Size of the 1/2 FIFO measured in units of 512 DWORDS
u32 BmPageXFers; // Size of the chunk of sata written to the DMA buffer
                 // in units of 512 DWORDS
u32* BmBuffers[2] = {NULL, NULL};  // DMA buffers

u16* adc_data;
pthread_t AiThread;
int bAbort = FALSE;
u32 page = AI_PAGE0;
TEvents Events;
u32 xferMode = XFERMODE_BM;

void *AiThreadProc(void *param);

//==============================================================================
// ISR
unsigned int pd_isr(unsigned int trigger_irq,struct pt_regs *regs) 
{
    int ret;
    int board = hbrd;
    u32 mask = 0;

    if ( pd_dsp_int_status(board) ) 
    { // interrupt pending
        pd_dsp_acknowledge_interrupt(board); // acknowledge the interrupt

        if(bAbort)
            return 0;

        // check what happens and process events
        ret = pd_adapter_get_board_status(board, &Events);
        
        // check for non-recoverable errors
        if (Events.AInIntr & AIB_BMErrSC) 
        {
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
	    
           // Notify thread that some new data is available
           pthread_wakeup_np (AiThread);
        }
    }

    // reenable irq handling
    rtl_hard_enable_irq(pd_board[board].irq);

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

    u32 BmDefDMASize;  // Default DMA burst size - 1
    u32 BmDefBursts;   // Default # of BM bursts
    u32 BmDefBurstSize; // Default BM burst size
    u32 StatMemAddr;    // Address of status memory in the DSP address space
    u32 FlagMemAddr;    // Address of flags memory in the DSP address space


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
       XBMPageSz = 16;
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

    if(xferMode == XFERMODE_BM)
    {
       BmDefDMASize = 15;   // 16 words bursts
       BmDefBursts = 64;
    }
    else
    {
       BmDefDMASize = 7;    // 8 words bursts
       BmDefBursts = 32;
    }

    BmDefBurstSize = BmDefDMASize << 16;

    // Read addresses of status and flag memory
    pd_dsp_reg_read(brd, 0x2, &StatMemAddr);
    pd_dsp_reg_read(brd, 0x2, &FlagMemAddr);

    // Clear flag of RTBM mode
    pd_dsp_reg_write(brd, ADRREALTIMEBMMODE+FlagMemAddr, 0);

    pd_dsp_reg_write(brd, ADRAIBM_DEFDMASIZE+StatMemAddr, BmDefDMASize);
    pd_dsp_reg_write(brd, ADRAIBM_DEFBURSTS+StatMemAddr, BmDefBursts);
    pd_dsp_reg_write(brd, ADRAIBM_DEFBURSTSIZE+StatMemAddr, BmDefBurstSize);

    BMList[0] = Buffer0PhysAddr >> 8;
    BMList[1] = Buffer1PhysAddr >> 8;
    BMList[2] = BmFHFXFers;  // size of 1/2 FIFO measured in transfer sizes AIBM_TXSIZE = 512
    BMList[3] = BmPageXFers; // size of each contigous physical pages in AIBM_TXSIZE

    // allocate temp buffer where we copy data after each page
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

    bAbort = TRUE;

    ret = pd_adapter_enable_interrupt(brd, 0);
    ret &= pd_ain_set_enable_conversion(brd, 0);
    ret &= pd_ain_sw_stop_trigger(brd);
    ret &= pd_ain_reset(brd); // this cleans all AIn configuration and triggers

    pthread_cancel(AiThread);
    pthread_join(AiThread, NULL);
    
    kfree(BmBuffers[AI_PAGE0]);
    kfree(BmBuffers[AI_PAGE1]);

    kfree(adc_data);

    return ret;
}


void *AiThreadProc(void *param) 
{
    int i, offs;
    char buf[1024];   // temp char buffer
    int ret;
    u32 mask;

    while(!bAbort)
    {
        pthread_suspend_np(pthread_self());
	if(bAbort)
            break;

        // Copy the page that was just done to temp buffer
        for(i=0; i<BmPageXFers * AIBM_TXSIZE; i++)
        {
           adc_data[i] = (u16)*((u32*)BmBuffers[page]+i);
        }

        mask = Events.AInIntr & (AIB_BMPgDoneIm | 
                                     AIB_BMErrIm |
                                     AIB_StartIm | 
                                     AIB_StopIm | 
                                     AIB_BMEnabled | 
                                     AIB_BMActive);

        pd_ain_set_events(hbrd,mask);
        pd_adapter_enable_interrupt(hbrd, 1);

        // convert first 8 samples to output thru the fifo
        offs = 0;
        for (i = 0; i < CL_SIZE; i++)
            offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
        offs += sprintf(buf+offs, "\n");
 
        // throw data into rtl fifo
        ret = rtf_put(data_fifo_id, buf, offs+1);

    }

    return NULL;
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
    printk("init_module rtai_dma_ai\n");

    // create fifo we can talk via /dev/rtf0
    rtf_destroy(data_fifo_id);     // free up the resource, just in case
    rtf_create(data_fifo_id, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device

    // get hold on interrupt vector
    if(rtl_request_irq(pd_board[hbrd].irq, pd_isr))
    {
        printk("rt_request_global_irq 0x%x failed, try to load the pwrdaq driver with the option rqstirq=0\n", pd_board[hbrd].irq);
        return -EIO;
    }

    // create the thread
    pthread_create(&AiThread, NULL, AiThreadProc, NULL);

    // kick analog input
    AInStart(hbrd);

    // enable it
    rtl_hard_enable_irq(pd_board[hbrd].irq);

    return 0;
}


//==============================================================================
void cleanup_module(void)
{
    printk("cleanup_module rtai_dma_ai\n");

    rtl_hard_disable_irq(pd_board[hbrd].irq);

    bAbort = TRUE;
    
    // stop analog input
    AInStop(hbrd);

    // get rid of fifo and thread
    rtf_destroy(data_fifo_id);      // free up the resource
    rtl_free_irq(pd_board[hbrd].irq);  // release IRQ 
}
