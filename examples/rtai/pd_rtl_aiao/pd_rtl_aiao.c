//===========================================================================
//
// NAME:    pd_rtl_aiao.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux Simultaneous AI and AO 
//
// AUTHOR:  Frederic Villeneuve
//
// DATE:    12-AUG-2005
//
//
// HISTORY:
//
//      Rev 0.1,     12-AUG-2005,     Initial version.
//
//---------------------------------------------------------------------------
//  (C) 2005 United Electronic Industries, Inc.
//
//  Feel free to distribute and modify this example
//---------------------------------------------------------------------------
//
// Note:
//
// This implements a RTAI periodic task that reads data from the analog inputs
// of a PDx-MF board and plays it on the same board's analog outputs..
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


#define PERIODIC_FREQ_HZ        1000.0   // periodic rate
#define DURATION_S		10        // Duration of the task
#define TICK_PERIOD_NS ((RTIME)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define MAX_CL_SIZE             0x100  // maximum number of CL entries on MFx board
#define CL_SIZE                 1      // current CL size

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1
#define AOUT_CFG 0

//==============================================================================
// Prototypes
void Periodic_entry_point(int framerate_fifo_id);   // the entry-point function of the rt task

//==============================================================================
// global variables
RT_TASK periodic_task;  // our periodic thread
int ai_board_handle = 0;  // first board in the system. See /proc/pwrdaq for board ID
int ai_board_id;                // model id of the board
int ao_board_handle = 0;
int ao_board_id;
RTIME tick_period;

//==============================================================================
void Periodic_entry_point(int framerate_fifo_id) // the entry-point function of the rt task
{  // id of fifo to which to send flag is sent in as arg from scheduler
   int ret;
   static u32 adc_cl[CL_SIZE];
   static u16 adc_data[CL_SIZE];
   int i;
   u32 samples;
   u32 count = 0;
   static u32 dac_data[CL_SIZE];
   u32 ai_resolution = 16;
   u32 ao_resolution = 12;
   u32 ai_ANDMask = 0xFFFF;
   u32 ai_XORMask = 0x8000;
   u32 max_count = DURATION_S * PERIODIC_FREQ_HZ;


   // Get more inormations about each board 

   // fill CL with "slow bit" flag
   for (i = 0; i < CL_SIZE; i++)
   {
      adc_cl[i] = i | 0x100;
   }

   ret = pd_ain_set_config(ai_board_handle, AIN_CFG, 0, 0);
   ret = pd_ain_set_channel_list(ai_board_handle, CL_SIZE, adc_cl);
   ret = pd_ain_set_enable_conversion(ai_board_handle, 1);
   ret = pd_ain_sw_start_trigger(ai_board_handle);
   ret = pd_ain_sw_cl_start(ai_board_handle); 

   ret = pd_aout_set_config(ao_board_handle, AOUT_CFG, 0);

   // loop until the specified number of samples is processed
   while (count < max_count)    
   {
      // send previous powerdaq samples to user space via rtl_fifo
      //samples = 0;
      ret = pd_ain_get_samples(ai_board_handle, CL_SIZE, adc_data, &samples);

      for (i=0; i<CL_SIZE; i++)
      {
         // Convert data between analog input raw binary format and analog output raw binary format         
         dac_data[i]= ((adc_data[i] & ai_ANDMask) ^ ai_XORMask) >> (16 - ai_resolution);

         // Scale data to match input and output channel resolutions (AI 12,14 or 16 bits -> AO 12 or 16 bits)
         //value = (unsigned long)floor(((double)value / (double)G_AiData.maxCode) * (double)G_AoData.maxCode);
	 dac_data[i] = dac_data[i] >> 4;
      }

      if(PD_IS_MFX(ao_board_id) || PDL_IS_MFX(ao_board_id))
      {
         if(CL_SIZE > 1)
         {
            dac_data[0] = (dac_data[0]&0xFFF) | ((dac_data[1]&0xFFF) << 12);
         }
         ret = pd_aout_put_value(ao_board_handle, dac_data[0]);
      }
      else
      {
         for(i=0; i<CL_SIZE; i++)
         {
            int updateAll;

            // update all channels simultaneously when the last one is written
            if(i==(CL_SIZE-1))
            {
               updateAll = 1;
            }
            else
            {
               updateAll = 0;
            }
            ret = pd_ao32_writex(ao_board_handle, i, dac_data[i], 1, updateAll);
         }
      }

      // program powerdaq for a new burst
      ret = pd_ain_sw_cl_start(ai_board_handle);      // starts clock

      // end this thread until next periodic call
      rt_task_wait_period();

      count++;
   }

   rt_printk("pd_rtai_aiao task terminated\n");
}


//==============================================================================
int init_module(void)                   // called when module is loaded
{
   char board_name[256];
   char serial[32];

   rt_printk("init_module pd_rtai_aiao\n");

   // get ai board informations
   if ((ai_board_id = pd_enum_devices(ai_board_handle, board_name, sizeof(board_name), 
                                      serial, sizeof(serial)))<0)
   {
      rt_printk("Error: Could not find board %d\n", ai_board_handle);
      return -ENXIO;
   }

   rt_printk("Found AI board %d: %s, s/n %s, id 0x%x\n", ai_board_handle, board_name, serial, ai_board_id);

   if (!PD_IS_MFX(ai_board_id) && !PDL_IS_MFX(ai_board_id))
   {
      rt_printk("Error: board %d can't do analog input\n", ai_board_handle);
      return -ENXIO;
   }

   // get ai board informations
   if ((ao_board_id = pd_enum_devices(ao_board_handle, board_name, sizeof(board_name), 
                                      serial, sizeof(serial)))<0)
   {
      rt_printk("Error: Could not find board %d\n", ao_board_handle);
      return -ENXIO;
   }

   rt_printk("Found AO board %d: %s, s/n %s, id 0x%x\n", ao_board_handle, board_name, serial, ao_board_id);

   if (!PD_IS_MFX(ao_board_id) && !PDL_IS_MFX(ao_board_id) && !PD_IS_AO(ao_board_id))
   {
      rt_printk("Error: board %d can't do analog output\n", ao_board_handle);
      return -ENXIO;
   }

   rt_set_periodic_mode();

   // create the task
   rt_task_init(&periodic_task, Periodic_entry_point, 0, 10000, 0, 0, 0);
   
   tick_period = start_rt_timer(nano2count(TICK_PERIOD_NS));
   rt_printk("Tick period is %d, %ldns\n", tick_period, count2nano(tick_period));
   
   // Make task periodic
   rt_task_make_periodic(&periodic_task, rt_get_time()+tick_period, tick_period);

   return 0;
}


//==============================================================================
void cleanup_module(void)
{
   rt_printk("cleanup_module pd_rtai_aiao\n");

   // get rid of fifo and thread
   stop_rt_timer();
   rt_task_delete(&periodic_task);  // delete the thread
}
