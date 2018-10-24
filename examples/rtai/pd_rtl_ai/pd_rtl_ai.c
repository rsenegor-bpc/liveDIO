//===========================================================================
//
// NAME:    pd_rtl_ai.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver asynchronous AIn test program
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    18-JAN-2003
//
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
// This implements a RTAI periodic task.
// This process writes a character each frame to an rtl_fifo, frequency = 10 Hz
//
// View the output with "cat /dev/rtf0"
//

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


#define PERIODIC_FREQ_HZ        10.0   // periodic rate
#define FRAME_PERIOD_NS ((RTIME)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define FIFO_ID       0                // rt-fifo on which to copy acquired data
#define FIFO_LENGTH   0x100            // enough space for 256 samples
#define MAX_RESULT_QTY          0x100  // maximum number of samples is limited by maximum CL size = 0x100
#define MAX_CL_SIZE             0x100  // maximum number of CL entries on MFx board
#define CL_SIZE                 8      // current CL size
#define PDAQ_ENABLE 1

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1

//==============================================================================
// Prototypes
void Periodic_entry_point(int framerate_fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
RT_TASK periodic_task;  // our periodic thread
int board_handle = 0;  // first board in the system. See /proc/pwrdaq for board ID
int board_id;

//==============================================================================
void Periodic_entry_point(int framerate_fifo_id) // the entry-point function of the rt task
{  
    int ret;
    static u32 adc_cl[MAX_RESULT_QTY];
    static u16 adc_data[MAX_RESULT_QTY];
    int i, offs;
    char buf[1024];
    u32 samples;

    // fill CL with "slow bit" flag
    for (i = 0; i < CL_SIZE; i++) 
    {
        adc_cl[i] = i | 0x100;
    }

    ret = pd_ain_set_config(board_handle, AIN_CFG, 0, 0);
    ret = pd_ain_set_channel_list(board_handle, CL_SIZE, adc_cl);
    ret = pd_ain_set_enable_conversion(board_handle, PDAQ_ENABLE);
    ret = pd_ain_sw_start_trigger(board_handle);
    ret = pd_ain_sw_cl_start(board_handle);  

    while (1)    // this loop wakes up once per period
    {
        // send previous powerdaq samples to user space via rtl_fifo
        //samples = 0;
        pd_ain_get_samples(board_handle, MAX_RESULT_QTY, adc_data, &samples);

        // convert first 8 samples to output thru the fifo
        offs = 0;
        for (i = 0; i < CL_SIZE; i++)
            offs += sprintf(buf+offs, "0x%4x  ", adc_data[i]^0x8000);
        offs += sprintf(buf+offs, "\n");

        // throw data into rtl fifo
        ret = rtf_put(framerate_fifo_id, buf, offs+1);  
        if(ret < 0)
	{
	   rt_printk("Error %d writing data to the FIFO\n", ret);
	}
	
        // program powerdaq for a new burst
        ret = pd_ain_sw_cl_start(board_handle);      // starts clock

        // end this thread until next periodic call
        rt_task_wait_period();
    }
}


//==============================================================================
int init_module(void)                   // called when module is loaded
{
    char board_name[256];
    char serial[32];

    int retVal = 0;
    
    rt_printk("init_module pd_rtai_ai\n");

    // get ai board informations
    if ((board_id = pd_enum_devices(board_handle, board_name, sizeof(board_name), serial, sizeof(serial)))<0)
    {  
         rt_printk("Error: Could not find board %d\n", board_handle);
         return -ENXIO;
    }
   
    rt_printk("Found AI board %d: %s, s/n %s, id 0x%x\n", board_handle, board_name, serial, board_id);
   
    if (!PD_IS_MFX(board_id) && !PDL_IS_MFX(board_id))
    {  
         rt_printk("Error: board %d can't do analog input\n", board_handle);
         return -ENXIO;
    }

    // create fifo used to communicate with user-space via /dev/rtf0
    rtf_destroy(FIFO_ID);     // free up the resource, just in case
    retVal = rtf_create(FIFO_ID, FIFO_LENGTH);
    if(retVal<0)
    {
       rt_printk("Error %d, could not create FIFO /de/rtf%d\n", retVal, FIFO_ID);
       return retVal;
    }  

    rt_set_periodic_mode();

    // create the task
    rt_task_init(&periodic_task, Periodic_entry_point, 0, 10000, FIFO_ID, 0, 0);
    // Make task periodic
    rt_task_make_periodic(&periodic_task, rt_get_time(), nano2count(FRAME_PERIOD_NS));
    //rt_task_resume(&periodic_task);

    start_rt_timer(nano2count(1000000));

    return 0;
}


//==============================================================================
void cleanup_module(void)
{
    rt_printk("cleanup_module pd_rtai_ai\n");

    // get rid of fifo and thread
    stop_rt_timer();
    rtf_destroy(FIFO_ID);  // free up the resource
    rt_task_delete(&periodic_task);  // delete the thread
}
