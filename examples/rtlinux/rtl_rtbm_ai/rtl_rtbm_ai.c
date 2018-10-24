//===========================================================================
//
// NAME:    rtl_rtbm_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ RTLinux driver DMA-based AIn test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    18-JUN-2004
//
// HISTORY:
//
//      Rev 0.1,     18-JUN-2004,     Initial version.
//
//---------------------------------------------------------------------------
//  (C) 2004 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux hard interrupt handler to serve PowerDAQ board 
// DMA page done interrupts.
// This is the fastest way to do single scan acquisition. The example is configured
// to acquire 1000 scans and print the time it took to do the acquisition.
// This example also writes each scan to an rt_fifo (0)
//
// Insert the module with "insmod rtl_rtbm_ai.o"
//
// View the output with "cat /dev/rtf0"
//
// PowerDAQ Linux driver pwdaq.o shall be compiled with
// make RTL=1
//
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
#include <time.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"
#include "uct.h"

#define data_fifo_id       0
#define DATA_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_CL_SIZE        0x100  // maximum number of CL entries on MFx board

#define CV_CLK 333000              // Conversion clock divider set for a
#define CL_CLK 1000              // sampling rate of 10000Hz

#define ADRAIBM_DEFDMASIZE	0x1D
#define ADRAIBM_DEFBURSTS	0x1E
#define ADRAIBM_DEFBURSTSIZE	0x1F

#define ADRREALTIMEBMMODE	0x20 // Enable/Disable Real Time Bus master mode

#define ADRRTModeAIBM_TRANSFERSIZE 0x23 // size of first data block

#define ADRRRTModeAIBM_SAMPLENUM 0x24 

//==============================================================================
// global variables
extern pd_board_t pd_board[PD_MAX_BOARDS];

typedef struct _RTBMAiData
{
   int brd;  // first board in the system. See /proc/pwrdaq for board ID
   u32* BmBuffers[2];  // DMA buffers
   u32 adc_cl[64];
   u32 cl_size;
   u32 ain_cfg;
   u16* adc_data;
   pthread_t aiThread;
   int bAbort;
   u32 page;
   u32 xferMode;
   TEvents events;
} RTBMAiData;

RTBMAiData G_AiData;
tUctData G_UctData;

void *AiThreadProc(void *param);

//==============================================================================
// ISR
unsigned int pd_isr(unsigned int trigger_irq,struct pt_regs *regs) 
{
    int ret;
    
    if (pd_dsp_int_status(G_AiData.brd)) 
    { 
        // acknowledge the interrupt
        pd_dsp_acknowledge_interrupt(G_AiData.brd); 
     
        // check what happens and process events
        ret = pd_adapter_get_board_status(G_AiData.brd, &G_AiData.events);
       
        // check for non-recoverable errors
        if (G_AiData.events.AInIntr & AIB_BMErrSC) 
        {
           pd_stop_and_disable_ain(G_AiData.brd);
           rtl_printf("BM Error\n");
        } 
        else // is there new page of data available?
        if ((G_AiData.events.AInIntr & AIB_BMPg0DoneSC)||(G_AiData.events.AInIntr & AIB_BMPg1DoneSC)) 
        {
           if (G_AiData.events.AInIntr & AIB_BMPg0DoneSC) 
           {
              G_AiData.page = AI_PAGE0;
           } 
           else if (G_AiData.events.AInIntr & AIB_BMPg1DoneSC)
           {
              G_AiData.page = AI_PAGE1;
           }
           // Notify thread that some new data is available
           pthread_wakeup_np (G_AiData.aiThread);

           rtl_printf("t\n");
        }
        else if (G_AiData.events.ADUIntr & UTB_Uct2IntrSC) 
        {
        }
    }

    // reenable irq handling
    rtl_hard_enable_irq(pd_board[G_AiData.brd].irq);

    return 0;
}

//==============================================================================
// AIn start
int AInStart (RTBMAiData* pAiData) 
{
    int i, ret;
    u32 mask;
    
    u32 BMList[4];
    u32 Buffer0PhysAddr;
    u32 Buffer1PhysAddr;

    u32 XBMPageSz;      // size of a dma buffer, it is dependant on the FIFO size
    u32 BmFHFXFers;     // Size of the 1/2 FIFO measured in units of 512 DWORDS
    u32 BmPageXFers;    // Size of the chunk of sata written to the DMA buffer
                        // in units of 512 DWORDS
    u32 BmNumOfChList;
    u32 BmDefDMASize;   // Default DMA burst size - 1
    u32 BmDefBursts;    // Default # of BM bursts
    u32 BmDefBurstSize; // Default BM burst size
    u32 BmTransferSize;
    u32 StatMemAddr;    // Address of status memory in the DSP address space
        
    // configure uct2
    ret = InitUct(&G_UctData);
    ret = StartUct(&G_UctData, pAiData->cl_size); 

    ret = pd_ain_reset(pAiData->brd);

    // fill CL, the number of channels in the channel list must be 4, 8, 16, 32 or 64
    for (i = 0; i < pAiData->cl_size; i++) 
       pAiData->adc_cl[i] = i;

    ret = pd_ain_set_config(pAiData->brd, pAiData->ain_cfg, 0, 0);
    ret &= pd_ain_set_cv_clock(pAiData->brd, CV_CLK);
    ret &= pd_ain_set_cl_clock(pAiData->brd, CL_CLK);

    ret &= pd_ain_set_channel_list(pAiData->brd, pAiData->cl_size, pAiData->adc_cl);

    // Set up bus mastering parameters
    XBMPageSz = 1;      // Number of pages to allocate
    BmFHFXFers = 1;     // Number of block of 512 words per half-FIFO
    BmPageXFers = 1;    // Number of block of 512 words to transfer
    
    pAiData->BmBuffers[AI_PAGE0] = (u32*)kmalloc(XBMPageSz * PAGE_SIZE, GFP_KERNEL|GFP_ATOMIC);
    pAiData->BmBuffers[AI_PAGE1] = (u32*)kmalloc(XBMPageSz * PAGE_SIZE, GFP_KERNEL|GFP_ATOMIC);

    Buffer0PhysAddr = (u32)virt_to_phys(pAiData->BmBuffers[AI_PAGE0]);
    Buffer1PhysAddr = (u32)virt_to_phys(pAiData->BmBuffers[AI_PAGE1]);

    BmNumOfChList = 1;   // Number of scans to acquire for each burst

    switch(pAiData->cl_size)
    {
    case 4:
       BmDefDMASize = 3;   // number of words sent for each burst 
       BmDefBursts = 1; // number of bursts for each scan
       break;
    case 8:
       BmDefDMASize = 7;   // number of words sent for each burst 
       BmDefBursts = 1; // number of bursts for each scan
       break;
    case 16:
       BmDefDMASize = 15;   // number of words sent for each burst 
       BmDefBursts = 1; // number of bursts for each scan
       break;
    case 32:
       BmDefDMASize = 15;   // number of words sent for each burst 
       BmDefBursts = 2; // number of bursts for each scan
       break;
    case 64:
       BmDefDMASize = 15;   // number of words sent for each burst 
       BmDefBursts = 4; // number of bursts for each scan
       break;
    default:
       return -3;
       break;
    }
        
    BmDefBurstSize = BmDefDMASize << 16;
    BmTransferSize = (BmDefDMASize+1)*(BmDefBursts-1);

    // Read addresses of status and flag memory
    pd_dsp_reg_read(pAiData->brd, 0x2, &StatMemAddr);

    // Clear flag of RTBM mode
    pd_dsp_reg_write(pAiData->brd, ADRREALTIMEBMMODE+StatMemAddr, 1);

    pd_dsp_reg_write(pAiData->brd, ADRAIBM_DEFDMASIZE+StatMemAddr, BmDefDMASize);
    pd_dsp_reg_write(pAiData->brd, ADRAIBM_DEFBURSTS+StatMemAddr, BmDefBursts);
    pd_dsp_reg_write(pAiData->brd, ADRAIBM_DEFBURSTSIZE+StatMemAddr, BmDefBurstSize);

    pd_dsp_reg_write(pAiData->brd, ADRRTModeAIBM_TRANSFERSIZE+StatMemAddr, BmTransferSize);

    BMList[0] = Buffer0PhysAddr >> 8;
    BMList[1] = Buffer1PhysAddr >> 8;
    BMList[2] = BmFHFXFers;  // size of 1/2 FIFO measured in transfer sizes AIBM_TXSIZE = 512
    BMList[3] = BmPageXFers; // size of each contigous physical pages in AIBM_TXSIZE

    // allocate temp buffer where we copy data after each page
    // has been filled-up
    pAiData->adc_data = (u16*)kmalloc(BmPageXFers * AIBM_TXSIZE, GFP_KERNEL|GFP_ATOMIC);

    ret &= pd_ain_set_BM_ctr(pAiData->brd, AIBM_SET, BMList);
    
    // enable bus master events
    mask = AIB_BMPgDoneIm | AIB_BMErrIm;
    ret &= pd_ain_set_events(pAiData->brd, mask);

    // enable UCT2 events 
    ret &= pd_adapter_set_board_event1(G_UctData.board, UTB_Uct2Im);
    if(!ret)
    {
        rtl_printf("scan_int: pd_adapter_set_board_event1 failed with code %x\n", ret);
        return ret;
    }

    ret &= pd_adapter_enable_interrupt(pAiData->brd, 1);
    ret &= pd_ain_set_enable_conversion(pAiData->brd, 1);
    ret &= pd_ain_sw_start_trigger(pAiData->brd);
    //ret &= pd_ain_sw_cl_start(pAiData->brd);
 
    return ret;
}

//==============================================================================
// AIn stop
int AInStop (RTBMAiData *pAiData) 
{
    int ret;

    
    ret = StopUct(&G_UctData);
    ret &= CleanUpUct(&G_UctData);

    ret &= pd_adapter_enable_interrupt(pAiData->brd, 0);
    ret &= pd_ain_set_enable_conversion(pAiData->brd, 0);
    ret &= pd_ain_sw_stop_trigger(pAiData->brd);
    ret &= pd_ain_reset(pAiData->brd); // this cleans all AIn configuration and triggers
    
    kfree(pAiData->BmBuffers[AI_PAGE0]);
    kfree(pAiData->BmBuffers[AI_PAGE1]);

    kfree(pAiData->adc_data);

    return ret;
}


void *AiThreadProc(void *param) 
{
    RTBMAiData* pAiData = (RTBMAiData*)param;
    int i, offs;
    char buf[1024];   // temp char buffer
    int ret, n;
    u32 mask;
    struct timespec ts1, ts2;

    rtl_printf("x\n");

    n = 0;
    clock_gettime(CLOCK_REALTIME, &ts1);

    while(!pAiData->bAbort && (n < 1000))
    {
        rtl_printf("a\n");

        // Acquire next scan
        mask = pAiData->events.AInIntr & (AIB_BMPgDoneIm | 
                                     AIB_BMErrIm |
                                     AIB_StartIm | 
                                     AIB_StopIm | 
                                     AIB_BMEnabled | 
                                     AIB_BMActive);
        pd_ain_set_events(pAiData->brd, mask);
        pd_adapter_enable_interrupt(pAiData->brd, 1);
        pd_ain_sw_cl_start(pAiData->brd);

        // Wait until the scan is acquired
        pthread_suspend_np(pthread_self());
        
        // Copy the scan to temp buffer
        for(i=0; i<pAiData->cl_size; i++)
        {
           pAiData->adc_data[i] = (u16)*((u32*)pAiData->BmBuffers[pAiData->page]+i);
        }

        // convert samples to output thru the fifo
        offs = 0;
        for (i = 0; i < pAiData->cl_size; i++)
            offs += sprintf(buf+offs, "0x%4x  ", pAiData->adc_data[i]^0x8000);
        offs += sprintf(buf+offs, "\n");
 
        // throw data into rtl fifo
        ret = rtf_put(data_fifo_id, buf, offs+1);

        n++;
    }

    clock_gettime(CLOCK_REALTIME, &ts2);

    timespec_sub(&ts2, &ts1);

    rtl_printf("Acquired %d scans in %ds and %dns, one scan in %dus\n", n, ts2.tv_sec, ts2.tv_nsec,
               (ts2.tv_sec * 1000000 + ts2.tv_nsec/1000)/n);

    // Stop the acquisition
    AInStop(pAiData);

    pthread_exit(0);

    return NULL;
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
    printk("init_module rtai_dma_ai\n");

    // create fifo we can talk via /dev/rtf0
    rtf_destroy(data_fifo_id);     // free up the resource, just in case
    rtf_create(data_fifo_id, DATA_FIFO_LENGTH);  // fifo from which rt task talks to device

    // use first board
    G_AiData.brd = 1;
    // Acquire 4 channels
    G_AiData.cl_size = 8;
    // Configure ai to use software CL clock and continuous CV clock
    G_AiData.ain_cfg = AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1 | AIB_CLSTART0 | AIB_SELMODE1;
    G_AiData.page = AI_PAGE0;
    G_AiData.xferMode = XFERMODE_BM;
    G_AiData.bAbort = FALSE;

    G_UctData.board = 1;
    G_UctData.counter = 2;
    G_UctData.clockSource = clkExternal;
    G_UctData.gateSource = gateSoftware;
    G_UctData.mode = PulseTrain;
 
    // get hold on interrupt vector
    if(rtl_request_irq(pd_board[G_AiData.brd].irq, pd_isr))
    {
        printk("rt_request_global_irq 0x%x failed, try to load the pwrdaq driver with the option rqstirq=0\n", pd_board[G_AiData.brd].irq);
        return -EIO;
    }

    // create the thread
    pthread_create(&G_AiData.aiThread, NULL, AiThreadProc, (void*)&G_AiData);

    // kick analog input
    AInStart(&G_AiData);

    // enable it
    rtl_hard_enable_irq(pd_board[G_AiData.brd].irq);

    return 0;
}


//==============================================================================
void cleanup_module(void)
{
    printk("cleanup_module rtai_dma_ai\n");
    
    // stop analog input
    G_AiData.bAbort = TRUE;
    pthread_join(G_AiData.aiThread, NULL);

    rtl_hard_disable_irq(pd_board[G_AiData.brd].irq);

    // get rid of fifo and thread
    rtf_destroy(data_fifo_id);      // free up the resource
    rtl_free_irq(pd_board[G_AiData.brd].irq);  // release IRQ 
}

