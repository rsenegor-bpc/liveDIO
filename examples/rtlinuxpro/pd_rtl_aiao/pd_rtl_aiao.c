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
// This was compiled using RTLinuxPro 2.2 
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "win_ddk_types.h"
#include "pdfw_def.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"


#define PERIODIC_FREQ_HZ        30000.0   // periodic rate
#define DURATION_S		100        // Duration of the task
#define TICK_PERIOD_NS ((hrtime_t)((1.0/PERIODIC_FREQ_HZ) * 1000000000.0))
#define CL_SIZE                 1      // current CL size

// Ain config: bipolar, +/-10V range, CL = SW, CV = cont, Single Ended
#define AIN_CFG AIN_BIPOLAR | AIN_RANGE_10V | AIB_CVSTART0 | AIB_CVSTART1
#define AOUT_CFG 0

//==============================================================================
// Prototypes
void *Periodic_entry_point(void *arg);   // the entry-point function of the rt task

//==============================================================================
// global variables
pthread_t periodic_thread;  // our periodic thread
int ai_board_handle = 1;  // first board in the system. See /proc/pwrdaq for board ID
int ai_board_id;                // model id of the board
int ao_board_handle = 1;
int ao_board_id;
int bStop = 0;

//==============================================================================
void *Periodic_entry_point(void *arg) // the entry-point function of the rt task
{  
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
   struct timespec abstime;


   // fill CL with "slow bit" flag
   for (i = 0; i < CL_SIZE; i++)
   {
      adc_cl[i] = i | 0x100;
   }

   // get the current time and setup for the first tick 
   clock_gettime(CLOCK_REALTIME, &abstime);

   // Configure Analog input board
   ret = pd_ain_set_config(ai_board_handle, AIN_CFG, 0, 0);
   ret = pd_ain_set_channel_list(ai_board_handle, CL_SIZE, adc_cl);
   ret = pd_ain_set_enable_conversion(ai_board_handle, 1);
   ret = pd_ain_sw_start_trigger(ai_board_handle);
   ret = pd_ain_sw_cl_start(ai_board_handle); 

   // configure analog outout board
   ret = pd_aout_set_config(ao_board_handle, AOUT_CFG, 0);

   // loop until the specified number of samples is processed
   while (count < max_count && !bStop)
   {
      // Sleep until the next tick 
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                      &abstime, NULL);

      // Setup absolute time for next loop
      timespec_add_ns(&abstime, TICK_PERIOD_NS);

      ret = pd_ain_get_samples(ai_board_handle, CL_SIZE, adc_data, &samples);

      for (i=0; i<CL_SIZE; i++)
      {
         // Convert data between analog input raw binary format and analog output raw binary format         
         dac_data[i]= ((adc_data[i] & ai_ANDMask) ^ ai_XORMask) >> (16 - ai_resolution);

         // Scale data to match input and output channel resolutions (AI 12,14 or 16 bits -> AO 12 or 16 bits)
         dac_data[i] = dac_data[i] >> (16 - ao_resolution);
      }

      if (PD_IS_MFX(ao_board_id) || PDL_IS_MFX(ao_board_id))
      {
         if (CL_SIZE > 1)
         {
            dac_data[0] = (dac_data[0]&0xFFF) | ((dac_data[1]&0xFFF) << 12);
         }
         ret = pd_aout_put_value(ao_board_handle, dac_data[0]);
      } 
      else
      {
         for (i=0; i<CL_SIZE; i++)
         {
            int updateAll;

            // update all channels simultaneously when the last one is written
            if (i==(CL_SIZE-1))
            {
               updateAll = 1;
            } else
            {
               updateAll = 0;
            }
            ret = pd_ao32_writex(ao_board_handle, i, dac_data[i], 1, updateAll);
         }
      }

      // program powerdaq for a new burst
      ret = pd_ain_sw_cl_start(ai_board_handle); 

      count++;
   }

   printf("pd_rtl_aiao task terminated\n");

   return NULL;
}


//==============================================================================
int main(void)                   // called when module is loaded
{
   pthread_attr_t attrib;              // thread attribute object. defined in pthreads
   char board_name[256];
   char serial[32];

   printf("init_module pd_rtl_ai\n");

   // get ai board informations
   if ((ai_board_id = pd_enum_devices(ai_board_handle, board_name, sizeof(board_name), 
                                      serial, sizeof(serial)))<0)
   {
      printf("Error: Could not find board %d\n", ai_board_handle);
      return -ENXIO;
   }

   printf("Found AI board %d: %s, s/n %s, id 0x%x\n", ai_board_handle, board_name, serial, ai_board_id);

   if (!PD_IS_MFX(ai_board_id) && !PDL_IS_MFX(ai_board_id))
   {
      printf("Error: board %d can't do analog input\n", ai_board_handle);
      return -ENXIO;
   }

   // get ai board informations
   if ((ao_board_id = pd_enum_devices(ao_board_handle, board_name, sizeof(board_name), 
                                      serial, sizeof(serial)))<0)
   {
      printf("Error: Could not find board %d\n", ao_board_handle);
      return -ENXIO;
   }

   printf("Found AO board %d: %s, s/n %s, id 0x%x\n", ao_board_handle, board_name, serial, ao_board_id);

   if (!PD_IS_MFX(ao_board_id) && !PDL_IS_MFX(ao_board_id) && !PD_IS_AO(ao_board_id))
   {
      printf("Error: board %d can't do analog output\n", ao_board_handle);
      return -ENXIO;
   }

   pthread_attr_init(&attrib);         // Initialize thread attributes
   pthread_attr_setfp_np(&attrib, 1);  // Allow use of floating-point operations in thread

   // create the thread
   pthread_create(&periodic_thread, &attrib, Periodic_entry_point, NULL);

   // wait for module to be unloaded 
   rtl_main_wait();

   // Stop thread
   bStop = 1;
   pthread_join(periodic_thread, NULL);

   return 0;
}

