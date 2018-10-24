//===========================================================================
//
// NAME:    powerdaq_isr.c
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver
//          
//          This file implements the interrupt service routine
//          handling routines for various version of the Linux kernel.
//
// AUTHOR: Frederic Villeneuve 
//
//---------------------------------------------------------------------------
//      Copyright (C) 2005 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------
#include "include/powerdaq_kernel.h"

#ifdef PD_DEBUG
static unsigned long intcnt = 0;
#endif

////////////////////////////////////////////////////////////////////////
//
//       NAME:  pd_bottom_half (dpc)
//
//   FUNCTION:  Handles the bottom half of PowerDAQ interrupts.
//
//  ARGUMENTS:  A board that needs attention.
//
//    RETURNS:  Nothing.
//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
void pd_work_func(struct work_struct* work)
{
   pd_board_t *pBoard = container_of(work, pd_board_t, worker);
   pd_bottom_half((void*)(long)pBoard->index);
}
#endif
void pd_bottom_half(void *data)
{
   long board = (long)data;

   DPRINTK_T("i>bottom half got board %ld\n", board);

   _fw_spinlock    // set spin lock
   
   // check what happens and process events
   pd_process_events(board);
   
   // re-enable interrupts on this board
   pd_adapter_enable_interrupt(board, 1);
   
   _fw_spinunlock    // release spin lock
}

#if defined(_PD_RTL)
void* pd_rt_bottom_half(void *data)
{
   long board = (long)data;

   // Wait for interrupts in a loop and notify the main thread
   // of the cause of the interrupt.
   while (1)
   {
      pthread_suspend_np(pthread_self());
      
      DPRINTK_T("i>bottom half got board %d\n", board);
      
      // call bottom-half
      pd_bottom_half(data);

      // Call user routine if registered
      // The call is protected by a mutex to syncronize it with the
      // calls to pd_register_user_isr() and pd_unregister_user_isr().
      pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);

      if(pd_board_ext[board].user_isr != NULL)
      {
         pd_board_ext[board].user_isr(pd_board_ext[board].user_param);
      }

      pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
   }
}
#elif defined(_PD_RTLPRO)
void* pd_rt_bottom_half(void *data)
{
   long board = (long)data;

   // Wait for interrupts in a loop and notify the main thread
   // of the cause of the interrupt.
   while (1)
   {
      rtl_sem_wait (&rt_bh_sem[board]);

      DPRINTK_T("i>rtlpro bottom half thread got board %d\n", board);
      
      // call bottom-half
      pd_bottom_half(data);

      // Call user routine if registered
      // The call is protected by a mutex to syncronize it with the
      // calls to pd_register_user_isr() and pd_unregister_user_isr().
      rtl_pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);

      if(pd_board_ext[board].user_isr != NULL)
      {
         pd_board_ext[board].user_isr(pd_board_ext[board].user_param);
      }

      rtl_pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
   }
}

#elif defined(_PD_RTAI)
void pd_rt_bottom_half(int board)
{
   // Wait for interrupts in a loop and notify the main thread
   // of the cause of the interrupt.
   while (1)
   {
      rt_sem_wait (&rt_bh_sem[board]);

      DPRINTK_T("i>bottom half got board %d\n", board);
      
      // call bottom-half
      pd_bottom_half((void*)board);

      // Call user routine if registered
      // The call is protected by a mutex to syncronize it with the
      // calls to pd_register_user_isr() and pd_unregister_user_isr().
      rt_mutex_lock(&pd_board_ext[board].user_isr_lock);

      if(pd_board_ext[board].user_isr != NULL)
      {
         pd_board_ext[board].user_isr(pd_board_ext[board].user_param);
      }

      rt_mutex_unlock(&pd_board_ext[board].user_isr_lock);
   }
}
#endif



////////////////////////////////////////////////////////////////////////
//
//
//       NAME:  pd_isr_serve_board
//
//   FUNCTION:  Handles the PowerDAQ interrupts.  Called directly by the
//              main ISR for each board that uses the interrupt that
//              triggered.  Raises SIGIO to notify the userspace process
//              of asynchronous events.  Copies the first 512 samples out
//              of the sample FIFO to the user buffer.  Schedules the
//              bottom half.
//
//  ARGUMENTS:  A board that might need attention.
//
//    RETURNS:  1 if the board needed service and we provided it, and 0 if
//              the board did not need service.
//
int pd_isr_serve_board(int board)
{
   tEvents Events;

#ifdef MEASURE_LATENCY
   asm volatile
   (
       "rdtsc" : "=a" (pd_board[board].tscLow), "=d" (pd_board[board].tscHigh)
   );
#endif

   // check if the interrupt came from our adapter
   if ( !pd_dsp_int_status(board) )
   {
      // this board does not have an interrupt pending
      DPRINTK_Q("ISR: bogus interrupt! board=%d isr#%ld\n", board, intcnt);
      return 0;
   }

   _fw_spinlock    // set spin lock
   // acknowledge the interrupt
   if (!pd_dsp_acknowledge_interrupt(board))
      DPRINTK_F("isr: board %d not responding\n", board);

   if (pd_board[board].bTestInt == 1)
   {
      DPRINTK_Q("i>: test interrupt\n");
      pd_board[board].bTestInt = 0;
      if (pd_board[board].fasync)
      {
         //void pd_kill_fasync(struct fasync_struct *fa, int sig);
         if (pd_board[board].fasync)
            pd_kill_fasync(pd_board[board].fasync, SIGIO);
         DPRINTK_N("i>: testing interrupts...\n");
      }
      // get out of here
      _fw_spinunlock    // release spin lock
      return 1;
   }

   if (pd_board[board].bUseHeavyIsr && (pd_board[board].dwXFerMode != XFERMODE_BM) && 
       (pd_board[board].dwXFerMode != XFERMODE_BM8WORD))
   {
      // here we should immediately read 512 samples from the FIFO
      pd_adapter_get_board_status(board, &Events);

      memcpy(&pd_board[board].FwEventsStatus, &Events, sizeof(tEvents));

      DPRINTK_T("i|AIB_FHF:%ld AIB_FHFSC:%ld\n",(Events.ADUIntr & AIB_FHF),(Events.AIOIntr & AIB_FHFSC));
      DPRINTK_T("i|AInIntr:0x%x\n", Events.AInIntr);

      if (pd_board[board].AinSS.bCheckHalfDone && 
          ((Events.ADUIntr & AIB_FHF) || (Events.AIOIntr & AIB_FHFSC)))
      { //AI:like my fix for WinNT
         DPRINTK_Q("i>isr-%ld: FHF flag is set, trying to flush FIFO\n", intcnt);

         if (!pd_ain_flush_fifo(board, 1))
         {
            DPRINTK_T("i>isr: No samples in the FIFO, sorry\n");
            _fw_spinunlock    // release spin lock
            return 1;
         } else
         {
            DPRINTK_T("i>isr: FIFO flashed with heavy interrupt\n");
         }
      }
   }

#if defined(_PD_RTL) 
   // wake-up thread
   pthread_wakeup_np (rt_bh_thread[board]);

   // re-enable interrupt
   rtl_hard_enable_irq(pd_board[board].irq);
#elif defined(_PD_RTLPRO)
   // wake-up thread
   rtl_sem_post (&rt_bh_sem[board]);

   // re-enable interrupt
   rtl_hard_enable_irq(pd_board[board].irq);
#elif defined(_PD_RTAI)
   rt_sem_signal (&rt_bh_sem[board]);

   // re-enable interrupt
   rt_enable_irq(pd_board[board].irq);
#else
   // schedule bottom half to run
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
   schedule_work(&pd_board[board].worker);
#else
   pd_board[board].worker.data = (void*)board;
   queue_task(&pd_board[board].worker, &tq_immediate);
   mark_bh(IMMEDIATE_BH);
#endif
#endif
   //DPRINTK_T("i>isr: DPC is scheduled!\n");

   _fw_spinunlock    // release spin lock

   return 1;
}

////////////////////////////////////////////////////////////////////////
//
//       NAME:  pd_isr
//
//   FUNCTION:  Handles the PowerDAQ interrupts.
//
//  ARGUMENTS:  The irq that triggered, and some other mumbo-jumbo.
//
//    RETURNS:  Nothing.
//
#if defined(_PD_RTL)
unsigned int pd_rt_isr(unsigned int irq, struct pt_regs *regs)
{
   int served;
   int board;

   //DPRINTK_Q("i>got interrupt! irq=%d i#=%ld\n", irq, intcnt++);

   // see if any of our boards use this irq
   // if so, service all that do
   served = 0;
   for (board = 0; board < num_pd_boards; board ++)
   {
      if (pd_board[board].irq == irq)
      {
         if (pd_isr_serve_board(board))
         {
            served ++;
         }
      }
   }

   if(served == 0)
   {
      // This interrupt was not for us pass it on to the Linux kernel
      rtl_global_pend_irq(irq);
      rtl_hard_enable_irq(irq);
   }

   return 0;
}
#elif defined(_PD_RTLPRO)
unsigned int pd_rt_isr(unsigned int irq, struct rtl_frame *regs)
{
   int served;
   int board;

   DPRINTK_Q("i>got interrupt! irq=%d i#=%ld\n", irq, intcnt++);

   // see if any of our boards use this irq
   // if so, service all that do
   served = 0;
   for (board = 0; board < num_pd_boards; board ++)
   {
      if (pd_board[board].irq == irq)
      {
         if (pd_isr_serve_board(board))
         {
            served ++;
         }
      }
   }

   if(served == 0)
   {
      // This interrupt was not for us pass it on to the Linux kernel
      rtl_global_pend_irq(irq);
      rtl_hard_enable_irq(irq);
   }

   return 0;
}
#elif defined(_PD_RTAI)
void pd_rt_isr(int irq)
{
   int served;
   int board;
   
   DPRINTK_Q("i>got interrupt! irq=%d i#=%ld\n", irq, intcnt++);

   // see if any of our boards use this irq
   // if so, service all that do
   served = 0;
   for (board = 0; board < num_pd_boards; board ++)
   {
      if (pd_board[board].irq == irq)
      {
         if (pd_isr_serve_board(board))
         {
            served ++;
         }
      }
   }

   if(served == 0)
   {
      // This interrupt was not for us pass it on to the Linux kernel
      rt_pend_linux_irq(irq);
      rt_enable_irq(irq);
   }
}
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
irqreturn_t pd_isr(int trigger_irq, void *dev_id)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
irqreturn_t pd_isr(int trigger_irq, void *dev_id, struct pt_regs * regs)
#else
void pd_isr(int trigger_irq, void *dev_id, struct pt_regs * regs)
#endif
{
   int served;
   int board;

   //DPRINTK_Q("i>got interrupt! irq=%d i#=%ld\n", trigger_irq, intcnt++);

   // see if any of our boards use this irq
   // if so, service all that do
   served = 0;
   for (board = 0; board < num_pd_boards; board ++)
   {
      if (&pd_board[board] == dev_id)
      {
         if (pd_isr_serve_board(board))
         {
            served ++;
         }
      }
   }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
   return ((served != 0) ? IRQ_HANDLED : IRQ_NONE);
#endif
}
#endif


int pd_driver_request_irq(int board, tPdIrqHandler handler)
{
   int ret;
   
#if defined(_PD_RTL) 
   // Grab the interrupt
   if((ret = rtl_request_irq(pd_board[board].irq, pd_rt_isr)) != 0)
   {
      return ret;
   }
   
   rtl_hard_enable_irq(pd_board[board].irq);
   
   // setup interrupt handler thread
   ret = pthread_create(&rt_bh_thread[board], NULL, pd_rt_bottom_half, (void*)(long)board);
   if (ret != 0)
   {
      DPRINTK("pd_driver_request_irq: can't create interrupt thread\n");
      return ret;
   }

   // initializes user ISR
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   // Initializes the mutex that protects access to the condition
   if(pthread_mutex_init(&pd_board_ext[board].user_isr_lock, NULL) != 0)
   {
      DPRINTK("pd_driver_request_irq: can't create mutex\n");
   }
#elif defined(_PD_RTLPRO)
   // Grab the interrupt
   if((ret = rtl_request_irq(pd_board[board].irq, pd_rt_isr)!= 0))
   {
      return ret;
   }
   
   rtl_hard_enable_irq(pd_board[board].irq);
   
   rtl_sem_init (&rt_bh_sem[board], 1, 0);

   // setup interrupt handler thread
   ret = rtl_pthread_create(&rt_bh_thread[board], NULL, pd_rt_bottom_half, (void*)(long)board);
   if (ret != 0)
   {
      DPRINTK("pd_driver_request_irq: can't create interrupt thread\n");
      return ret;
   }
   
   // initializes user ISR
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   // Initializes the mutex that protects access to the condition
   if(rtl_pthread_mutex_init(&pd_board_ext[board].user_isr_lock, NULL) != 0)
   {
      DPRINTK("pd_driver_request_irq: can't create mutex\n");
   }
#elif defined(_PD_RTAI)
   // Grab the interrupt
   if((ret = rt_request_global_irq(pd_board[board].irq, (void(*)(void))pd_rt_isr))!= 0)
   {
      return ret;
   }
   
   rt_startup_irq(pd_board[board].irq);
   rt_enable_irq(pd_board[board].irq);
   
   rt_task_init (&rt_bh_task[board], pd_rt_bottom_half, board, 10000, 2, 1, 0);
   rt_sem_init (&rt_bh_sem[board], 0);
   rt_task_resume (&rt_bh_task[board]);

   // initializes user ISR
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   // Initializes the mutex that protects access to the condition
   rt_mutex_init(&pd_board_ext[board].user_isr_lock);
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
    unsigned long flags = IRQF_SHARED;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
    unsigned long flags =  IRQF_SHARED | IRQF_DISABLED;
#else
    unsigned long flags =  SA_SHIRQ | SA_INTERRUPT;
#endif
   if ((ret = request_irq(pd_board[board].irq, pd_isr,
                   flags, "PowerDAQ",
                   (void *)&pd_board[board])))
   {
      return ret;
   }
   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
   INIT_WORK(&pd_board[board].worker, pd_work_func);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
   INIT_WORK(&pd_board[board].worker, pd_bottom_half, (void*)(unsigned long)board);
#else
   pd_board[board].worker.routine = pd_bottom_half;
#endif

#endif

   return 0;
}

int pd_driver_release_irq(int board)
{
#if defined(_PD_RTL)
   // Kill the isr thread
   pthread_cancel(rt_bh_thread[board]);
   pthread_join (rt_bh_thread[board], NULL);

   // Destroy synchronization objects
   pthread_mutex_destroy(&pd_board_ext[board].user_isr_lock);

   // free irq
   rtl_hard_disable_irq(pd_board[board].irq);
   rtl_free_irq(pd_board[board].irq);
#elif defined(_PD_RTLPRO)
   // Kill the isr thread
   rtl_pthread_cancel(rt_bh_thread[board]);
   rtl_pthread_join (rt_bh_thread[board], NULL);

   // Destroy synchronization objects
   rtl_pthread_mutex_destroy(&pd_board_ext[board].user_isr_lock);

   // free irq
   rtl_hard_disable_irq(pd_board[board].irq);
   rtl_free_irq(pd_board[board].irq);
   rtl_sem_destroy (&rt_bh_sem[board]);
#elif defined(_PD_RTAI)        
   rt_shutdown_irq(pd_board[board].irq);
   rt_disable_irq(pd_board[board].irq);
   rt_free_global_irq(pd_board[board].irq);

   rt_mutex_destroy(&pd_board_ext[board].user_isr_lock);
   rt_task_delete (&rt_bh_task[board]);
   rt_sem_delete (&rt_bh_sem[board]);
#else
   free_irq(pd_board[board].irq, (void *)&pd_board[board]);
#endif
  
   return 0;
}

