//===========================================================================
//
// NAME:    pd_rtl_ao.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver AOut test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    06-MAY-2003
//
//
// HISTORY:
//
//
//---------------------------------------------------------------------------
//  (C) 2003 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTLinux periodic task using the "pthreads" API.
// This process writes analog output with 1000 Hz
//
// modules pd.o, rtl_fifo, rtl_posixio, rtl_sched, rtl_time must be loaded before
// this one
// Insert the module with "insmod pd_rtl_ao.o"
//
// View the output with "cat /dev/rtf0"/ each cycle if you would like
// Attach a scope to analog outputs to see a ramp
//
// PowerDAQ Linux driver pd.o shall be compiled with
// #define _NO_USERSPACE
// and
// #deifne _PD_RT
//
// Uncomment those #defines in powerdaq-internal.h (lines 54 and 58)
// Then, "make clean; make; make install" in the PowerDAQ driver directory
// New driver will be located in ./drv/pd.o
//
// This was compiled using the RTLinux 2.2 distribution on kernel 2.2.14,
// it should work on RTLinux 2.3.and kernel 2.2.17
//

//#define PD2_AO_96 // undefine this line for PD2-AO-8/16/32

#include <pthread.h>
#include <rtl.h>
#include <rtl_sched.h>  // for threading funcs
#include <rtl_fifo.h>
#include <linux/cons.h>         // for DEBUGging functions: conprn()

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

#ifdef HERCMONDBG
    #include "PrintH.h"
#endif

#define PERIODIC_FREQ_HZ        1000.0   // periodic rate
#define FRAME_PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define FRAMERATE_FIFO_ID       0      // rt-fifo on which to put newframe flag
#define FRAMERATE_FIFO_LENGTH   0x100  // enough space for new-frame flags
#define MAX_RESULT_QTY          0x100  // maximum number of samples is limited by maximum CL size = 0x100
#define PDAQ_ENABLE 1

#define AO_CFG 0
//==============================================================================
// Prototypes
void *Periodic_entry_point(void *framerate_fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
pthread_t periodic_thread;  // our periodic thread
int board_handle    =   0;  // first board in the system. See /proc/pwrdaq for board ID

//==============================================================================
void *Periodic_entry_point(void *framerate_fifo_id) // the entry-point function of the rt task
{  // id of fifo to which to send flag is sent in as arg from scheduler
    int ret;
    int framerate_rtfifo = (int)framerate_fifo_id;
    int i, offs;
    static u16 val = 0x4000;
    char buf[256];

#ifdef HERCMONDBG
    static int count = 0;
#endif

    // make this thread RT periodic. This happens only the first time we're called
    // schedules task to run once per period
    if ( pthread_make_periodic_np(pthread_self(),gethrtime(),FRAME_PERIOD_NS) )
        {  printk("ERROR: Unable to register periodic thread !\n");  }

#ifdef HERCMONDBG
    hercmon_printf("configure powerdaq ao\n");
#endif

    ret = pd_aout_set_config(board_handle, AO_CFG, 0);

    while (1)    // this loop wakes up once per period
    {
        // send data to the ao card
#ifdef PD2_AO_96
        for (i = 0; i < 95; i++)
            ret = pd_ao96_writex(board_handle, i, val, 0, 0);
#else
        for (i = 0; i < 31; i++)
            ret = pd_ao32_writex(board_handle, i, val, 0, 0);
#endif
        val += 0x400; // choose whatever increment you want
        if (val > 0xC000) {
            val = 0x4000; // +/-5V ramp
            // write informtation about ongoing activity
            offs = 0;
            offs += sprintf(buf+offs,".");
            ret = rtf_put(framerate_rtfifo, buf, offs+1);
        }

        // end this thread until next periodic call
        pthread_wait_np();
    }
}

//==============================================================================
int init_module(void)                   // called when module is loaded
{
    pthread_attr_t attrib;              // thread attribute object. defined in pthreads
    struct sched_param sched_param;     // struct defined in pthreads

#ifdef HERCMONDBG
    hercmon_printf("init_module pd_rtl_ao\n");
#else
    printk("init_module pd_rtl_ao\n");
#endif

    // create fifo we can talk via /dev/rtf0
    rtf_destroy(FRAMERATE_FIFO_ID);     // free up the resource, just in case
    rtf_create(FRAMERATE_FIFO_ID, FRAMERATE_FIFO_LENGTH);  // fifo from which rt task talks to device

    // prepare periodic thread for creation
    pthread_attr_init(&attrib);         // initialize thread attributes
    sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);  // obtain highest priority
    pthread_attr_setschedparam(&attrib, &sched_param);    // set our priority

    // create the thread
    if ( pthread_create(&periodic_thread, &attrib, Periodic_entry_point,
                        (void *)FRAMERATE_FIFO_ID) );

    // wake up our thread
    pthread_wakeup_np(periodic_thread);

    return 0;
}


//==============================================================================
void cleanup_module(void)
{
#ifdef HERCMONDBG
    hercmon_printf("cleanup_module pd_rtl_ao\n");
    hercmon_scroll(0);  // clean up herc screen
#else
    printk("cleanup_module pd_rtl_ao\n");
#endif

    // get rid of fifo and thread
    rtf_destroy(FRAMERATE_FIFO_ID);      // free up the resource
    pthread_delete_np(periodic_thread);  // delete the thread
}
