//===========================================================================
//
// NAME:    powerdaq_osal.c
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver
//
//          This file implements an operatins system abstraction
//          layer to simplify porting the driver to various versions
//          Linux and other real-time operating systems.
//
// AUTHOR: Frederic Villeneuve
//
//---------------------------------------------------------------------------
//      Copyright (C) 2005 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------


#include "include/powerdaq_kernel.h"

void pd_kill_fasync(struct fasync_struct *fa, int sig)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
   kill_fasync(&fa, sig, 0);
#elif defined(PD_USING_REDHAT_62)
   kill_fasync(fa, sig, 0);
#else
   kill_fasync(fa, sig);
#endif
}
//--------------------------------------------------------------------
unsigned long pd_copy_from_user32(u32* to, u32* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_from_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}

unsigned long pd_copy_to_user32(u32* to, u32* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_to_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}

//--------------------------------------------------------------------
unsigned long pd_copy_from_user16(u16* to, u16* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_from_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}

unsigned long pd_copy_to_user16(u16* to, u16* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_to_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}


//--------------------------------------------------------------------
unsigned long pd_copy_from_user8(u8* to, u8* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_from_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}

unsigned long pd_copy_to_user8(u8* to, u8* from, u32 len)
{
#ifndef _NO_USERSPACE
   return copy_to_user(to, from, len);
#else
   return(unsigned long)memcpy(to, from, len);
#endif
}

//--------------------------------------------------------------------
void pd_udelay(u32 usecs)
{
#if defined(_PD_RTL) || defined(_PD_RTLPRO)
   rtl_delay(usecs*1000);
#else
   udelay(usecs);
#endif
}

void pd_mdelay(u32 msecs)
{
#if defined(_PD_RTL) || defined(_PD_RTLPRO)
   u32 i;
   for(i=0; i<msecs; i++)
   {
      rtl_delay(1000*1000);
   }
#else
   mdelay(msecs);
#endif
}

//-------------------------------------------------------------------
void* pd_alloc_bigbuf(u32 size)
{
   return rvmalloc(size);
}

void pd_free_bigbuf(void* mem, u32 size)
{
   rvfree(mem, size);
}

//--------------------------------------------------------------------
void* pd_kmalloc(u32 size, u32 priority)
{
   return kmalloc(size, priority);
}

void pd_kfree(void* adr)
{
   kfree(adr);
}

//--------------------------------------------------------------------
unsigned int pd_readl(void *address)
{
   return readl(address);
}


void pd_writel(unsigned int value, void *address)
{
   writel(value, address);
}


//--------------------------------------------------------------------
int pd_event_create(int board, TSynchSS **synch)
{
   int ret = 0;
   TSynchSS* pSynch;

   *synch = NULL;

   // allocates memory for the synchronization data structure
   pSynch = (TSynchSS *) pd_kmalloc(sizeof(TSynchSS), GFP_KERNEL);
   if (pSynch == NULL)
   {
      DPRINTK_T("Could not allocate memory for the synch object.\n");
      return -ENOMEM;
   }

   memset(pSynch, 0, sizeof(TSynchSS));

#if defined (_PD_RTL)
   // Initializes the conditional variable used by the subsystem to notify
   // events
   ret = pthread_cond_init(&(pSynch->event), NULL);
   if (ret != 0)
   {
      DPRINTK("pwrdaq RT: Error %d, can't create conditional variable\n", ret);
      return ret;
   }

   // Initializes the mutex that protects access to the condition
   ret = pthread_mutex_init(&(pSynch->event_lock), NULL);
   if (ret != 0)
   {
      DPRINTK("pwrdaq RT: Error %d, can't create mutex\n", ret);
      return ret;
   }
#elif  defined(_PD_RTLPRO)
   // Initializes the conditional variable used by the subsystem to notify
   // events
   ret = rtl_sem_init(&(pSynch->event), 1, 0);
   if (ret != 0)
   {
      DPRINTK("pwrdaq RT: Error %d, can't create semaphore\n", ret);
      return ret;
   }
#elif defined(_PD_RTAI)
   // Initializes the conditional variable used by the subsystem to notify
   // events
   rt_cond_init(&(pSynch->event));

   // Initializes the mutex that protects access to the condition
   rt_mutex_init(&(pSynch->event_lock));
   ret = 0;
#else
   ret = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
   pSynch->wait_q = NULL;
   pSynch->sem = MUTEX;
#else
   init_waitqueue_head(&(pSynch->wait_q));
#endif
#endif // _PD_RTL

   *synch = pSynch;

   return ret;
}

int pd_event_wait(int board, TSynchSS *synch, int timeoutms)
{
   int tret;

#if defined(_PD_RTL)
   struct timespec timeout;

   // Get the lock, pthread_cond_wait will unlock it while waiting and
   // relock it when it gets signaled
   tret = pthread_mutex_lock(&synch->event_lock);
   if (tret != 0)
   {
       DPRINTK_N("pd_event_wait: Could not acquire event lock\n");
       return tret;
   }

   DPRINTK_N("mutex locked\n");

   // Set a timeout from now for waiting for the answer of a module
   timeout.tv_sec =  time(0) + timeoutms/1000;
   timeout.tv_nsec = (timeoutms % 1000) + 1000000;

   tret = pthread_cond_timedwait(&synch->event, &synch->event_lock, &timeout);
   if (tret != 0)
   {
      if (tret == ETIMEDOUT)
      {
         DPRINTK_N("Timeout\n");
         tret = -1;
      }
      else
      {
         DPRINTK_N("Unexpected error %d\n", tret);
      }
   }
   else
      tret = 1;

   DPRINTK("cond Wait returned %d\n", tret);

   pthread_mutex_unlock(&synch->event_lock);

   DPRINTK("Mutex unlocked\n");
#elif defined(_PD_RTLPRO)
   struct rtl_timespec timeout;
   long timeoutns = timeoutms * 1000 * 1000;

   // Set a timeout from now for waiting for the answer of a module
   rtl_clock_gettime(RTL_CLOCK_REALTIME, &timeout);
   rtl_timespec_add_ns(&timeout, timeoutns);

   tret = rtl_sem_timedwait(&synch->event, &timeout);
   if (tret != 0)
   {
      if (tret == RTL_ETIMEDOUT)
      {
         DPRINTK_N("Timeout\n");
         tret = -1;
      }
      else
      {
         DPRINTK_N("Unexpected error %d\n", tret);
      }
   }
   else
      tret = 1;

#elif defined (_PD_RTAI)
   RTIME rt_timeout;
   RTIME timeout_ns;

   // Get the lock, pthrea_cond_wait will unlock it while waiting and
   // relock it when it gets signaled
   tret = rt_mutex_lock(&synch->event_lock);
   if (tret != 0)
   {
       DPRINTK_N("pd_event_wait: Could not acquire event lock\n");
       return tret;
   }

   // Set a timeout from now for waiting for the answer of a module
   timeout_ns = timeoutms * 1000 * 1000;
   rt_timeout = nano2count(timeout_ns);

   tret = rt_cond_wait_timed(&synch->event, &synch->event_lock, rt_timeout);
   if (tret != 0)
   {
      if (tret == SEM_TIMOUT)
      {
         DPRINTK_N("Timeout\n");
         tret = -1;
      }
      else
      {
         DPRINTK_N("Unexpected error %d\n", tret);
      }
   }
   else
      tret = 1;

   rt_mutex_unlock(&synch->event_lock);
#else
   int toutjiffies;
   wait_queue_t wait;
   toutjiffies = (timeoutms * HZ +999)/1000 + 1;
   tret = toutjiffies;
   init_waitqueue_entry(&wait, current);

   DPRINTK("pd_event_wait: about to go to sleep\n");

   // Handle the sleep on process ourself to avoid
   // race conditions that may occurr with interruptible_sleep_on_timeout
   add_wait_queue(&synch->wait_q, &wait);
   while (tret > 0 && !signal_pending(current))
   {
       set_current_state(TASK_INTERRUPTIBLE);
       if (synch->notifiedEvents != 0)
       {
          DPRINTK("pd_event_wait: no need to wait, event is already there\n");
          break;
       }
       tret = schedule_timeout(tret);

       DPRINTK("schedule_timeout ret %d\n", tret);
   }
   set_current_state(TASK_RUNNING);
   remove_wait_queue(&synch->wait_q, &wait);
#endif // _PD_RTL

   return tret;
}

int pd_event_signal(int board, TSynchSS *synch)
{
#if defined(_PD_RTL)
   // Signal the event
   pthread_cond_signal(&synch->event);
#elif defined(_PD_RTLPRO)
   // Signal the event
   rtl_sem_post(&synch->event);
#elif defined(_PD_RTAI)
   rt_cond_signal(&synch->event);
#else
   wake_up_interruptible(&synch->wait_q);
#endif // _PD_RTL

   return 0;
}

int pd_event_destroy(int board, TSynchSS *synch)
{
 #if defined(_PD_RTL)
   pthread_cond_destroy(&synch->event);
   pthread_mutex_destroy(&synch->event_lock);
#elif defined(_PD_RTLPRO)
   rtl_sem_destroy(&synch->event);
#elif defined(_PD_RTAI)
   rt_cond_destroy(&synch->event);
   rt_mutex_destroy(&synch->event_lock);
#endif

   // free up the memory allocated for the synch object
   pd_kfree(synch);

   return 0;
}

int pd_register_user_isr(int board, TUser_isr user_isr, void* user_param)
{
#if defined(_PD_RTL)
   pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = user_isr;
   pd_board_ext[board].user_param = user_param;
   pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#elif defined(_PD_RTLPRO)
   rtl_pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = user_isr;
   pd_board_ext[board].user_param = user_param;
   rtl_pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#elif defined (_PD_RTAI)
   rt_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = user_isr;
   pd_board_ext[board].user_param = user_param;
   rt_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#endif

   return 0;
}


int pd_unregister_user_isr(int board)
{
#if defined(_PD_RTL)
   pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#elif defined(_PD_RTLPRO)
   rtl_pthread_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   rtl_pthread_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#elif defined (_PD_RTAI)
   rt_mutex_lock(&pd_board_ext[board].user_isr_lock);
   pd_board_ext[board].user_isr = NULL;
   pd_board_ext[board].user_param = NULL;
   rt_mutex_unlock(&pd_board_ext[board].user_isr_lock);
#endif

   return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Allocates non-paged pool memory for use by the Pd device.  Actual Input
//    and Output comes from the IRP system buffer, and are AllocMemoryIn and
//    AllocMemoryOut structures (see Pddrv.h).
//    This routine keeps track of memory allocations by storing allocation
//    information in the device extension.  If memory has alrady been allocated
//    for the Pd device, this routine will return an error.
//    If the entire allocation request can not be satisfied, then no allocation is
//    performed (actually, the part of the allocaion that succeeds is undone).
//    Callers to this funcion should also call PdUpdatePagetable().
//
//////////////////////////////////////////////////////////////////////////
int pd_alloc_contig_memory(int board, tAllocContigMem *allocMemory)
{
   int idx = allocMemory->idx;
   ULONG errors = FALSE;
   int status = 0;

   DPRINTK("Allocating %d pages for index %d\n", allocMemory->size, allocMemory->idx);

   allocMemory->status = 0;

   if (idx & AIBM_GETPAGE)
   {
       // return existing data
       idx &= 0x7;
       allocMemory->linearAddr = (unsigned long) pd_board[board].pLinBMB[idx];
       allocMemory->physAddr = (unsigned long) pd_board[board].DMAHandleBMB[idx];
       //allocMemory->physAddr = (unsigned long) pd_board[board].pPhysBMB[idx];
       return status;
   }

   //
   // Check the device extension to see if memory has already
   // been allocated for this Pd device.  If so, return an
   // error
   //
   idx = allocMemory->idx & 0x7;
   if (pd_board[board].pSysBMB[idx])
   {
      allocMemory->status = 1;
      return 1;
   }

   pd_board[board].SizeBMB[idx] = allocMemory->size * PAGE_SIZE;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
   pd_board[board].pSysBMB[idx] = pci_alloc_consistent(pd_board[board].dev, pd_board[board].SizeBMB[idx], &(pd_board[board].DMAHandleBMB[idx]));
#else
   pd_board[board].pSysBMB[idx] = kmalloc(pd_board[board].SizeBMB[idx], GFP_KERNEL);
#endif

   if (!pd_board[board].pSysBMB[idx])
   {
      pd_board[board].SizeBMB[idx] = 0;
      allocMemory->status = 1;
      status = ENOMEM;
      errors = TRUE;
      goto CLEANUP;
   }

   //
   // now get a physical address
   pd_board[board].pLinBMB[idx] = pd_board[board].pSysBMB[idx];
   allocMemory->linearAddr = (unsigned long) pd_board[board].pLinBMB[idx];


CLEANUP:

   return status;
}

//////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Deallocates all memory that has been allocated for the Pd device.
//////////////////////////////////////////////////////////////////////////
void pd_dealloc_contig_memory(int board, tAllocContigMem * pDeallocMemory)
{
   int idx = pDeallocMemory->idx;

   DPRINTK("Freeing memory for index %d\n", pDeallocMemory->idx);

   //
   // free the buffer
   //
   if (pd_board[board].pSysBMB[idx])
   {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
      pci_free_consistent(pd_board[board].dev, pd_board[board].SizeBMB[idx],
                          pd_board[board].pSysBMB[idx],
                          pd_board[board].DMAHandleBMB[idx]);
#else
      kfree(pd_board[board].pSysBMB[idx]);
#endif

      pd_board[board].pSysBMB[idx] = NULL;
      pd_board[board].DMAHandleBMB[idx] = 0;
      pd_board[board].pLinBMB[idx] = 0;
      pd_board[board].SizeBMB[idx] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////
// Event notification functions
//
// This function puts process asleep
//
TSynchSS* pd_get_synch(int board, PD_SUBSYSTEM ss, int event)
{
   TSynchSS *synch;

   switch (ss)
   {
   case AnalogIn:
      synch = pd_board[board].AinSS.synch;
      break;

   case AnalogOut:
      synch = pd_board[board].AoutSS.synch;
      break;

   case DigitalIn:
      if (event & eDInEvent)
      {
         synch = pd_board[board].DinSS.synch;
      }
      else
      {
         synch = pd_board[board].AinSS.synch;
      }
      break;

   case DigitalOut:
      synch = pd_board[board].AoutSS.synch;
      break;

   case CounterTimer:
   case DSPCounter:
      if (event & (eUct0Event | eUct1Event | eUct2Event))
      {
         synch = pd_board[board].UctSS.synch;
      }
      else
      {
         synch = pd_board[board].AinSS.synch;
      }
      break;
   case CalDiag:
   case BoardLevel:
   default:
      synch = NULL;
      break;
   }

   return synch;
}


int pd_sleep_on_event(int board, PD_SUBSYSTEM ss, int event, int timeoutms)
{
   int tret, everet;
   TSynchSS *synch;

   tret = everet = 0;

   DPRINTK_N("pd_sleep_on_event: brd:%d ss:%d evt:%x\n",
             board, ss, event);

   synch = pd_get_synch(board, ss, event);
   if(NULL != synch)
   {
      synch->wakeupEvents = event;

      // unlock the spin lock so that we don't deadlock when the event occurs
      _fw_spinunlock
      tret = pd_event_wait(board, synch, timeoutms);
      _fw_spinlock

      everet = synch->notifiedEvents;
      synch->notifiedEvents = 0;
   }
   else
   {
      everet = 0;
      tret = -1;
   }

   DPRINTK_N("tret=%d %s\n", tret, ((tret <= 0)?"TIMEOUT":""));
   if (tret <= 0)
       everet |= eTimeout;
   return everet;
}

//
// This function awakes a process
//
int pd_notify_event(int board, PD_SUBSYSTEM ss, int event)
{
   DPRINTK_N("wake up!: brd:%d ss:%d evt:%x\n",
             board, ss, event);

   switch (ss)
   {
   case AnalogIn:
      pd_board[board].AinSS.synch->notifiedEvents = event;
      pd_event_signal(board, pd_board[board].AinSS.synch);
      break;

   case AnalogOut:
      pd_board[board].AoutSS.synch->notifiedEvents = event;
      pd_event_signal(board, pd_board[board].AoutSS.synch);
      break;

   case DigitalIn:
      pd_board[board].DinSS.synch->notifiedEvents = event;
      pd_event_signal(board, pd_board[board].DinSS.synch);
      break;

   case DigitalOut:
      pd_board[board].DoutSS.synch->notifiedEvents = event;
      pd_event_signal(board, pd_board[board].DoutSS.synch);
      break;

   case CounterTimer:
   case DSPCounter:
      pd_board[board].UctSS.synch->notifiedEvents = event;
      pd_event_signal(board, pd_board[board].UctSS.synch);
      break;

   case CalDiag:
   case BoardLevel:
   default:
      break;
   }

   return event;
}

int pd_driver_open(int board, int minor)
{
   switch (minor)
   {
   case PD_MINOR_AIN:
      if (!pd_board[board].AinSS.bInUse)
      {
         pd_board[board].AinSS.bInUse = TRUE;
         pd_board[board].fd[AnalogIn] = 0;
         pd_board[board].AinSS.synch->notifiedEvents = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: AinSS is in use\n");
         return -EBUSY;
      }
      break;

   case PD_MINOR_AOUT:
      if (!pd_board[board].AoutSS.bInUse)
      {
         pd_board[board].AoutSS.dwAoutValue = 0x800800;
         pd_board[board].AoutSS.bInUse = TRUE;
         pd_board[board].AoutSS.synch->notifiedEvents = 0;
         pd_board[board].fd[AnalogOut] = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: AoutSS is in use\n");
         return -EBUSY;
      }
      break;

   case PD_MINOR_DIN:
      if (!pd_board[board].DinSS.bInUse)
      {
         pd_board[board].DinSS.bInUse = TRUE;
         pd_board[board].fd[DigitalIn] = 0;
         pd_board[board].DinSS.synch->notifiedEvents = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: DinSS is in use\n");
         return -EBUSY;
      }
      break;

   case PD_MINOR_DOUT:
      if (!pd_board[board].DoutSS.bInUse)
      {
         pd_board[board].DoutSS.bInUse = TRUE;
         pd_board[board].fd[DigitalOut] = 0;
         pd_board[board].DoutSS.synch->notifiedEvents = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: DoutSS is in use\n");
         return -EBUSY;
      }
      break;

   case PD_MINOR_UCT:
      if (!pd_board[board].UctSS.bInUse)
      {
         pd_board[board].UctSS.bInUse = TRUE;
         pd_board[board].fd[CounterTimer] = 0;
         pd_board[board].UctSS.synch->notifiedEvents = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: UctSS is in use\n");
         return -EBUSY;
      }
      break;

   case PD_MINOR_DSPCT:
      if (!pd_board[board].UctSS.bInUse)
      {
         pd_board[board].UctSS.bInUse = TRUE;
         pd_board[board].fd[DSPCounter] = 0;
         pd_board[board].UctSS.synch->notifiedEvents = 0;
      }
      else
      {
         DPRINTK_I("pd_driver_open error: UctSS is in use\n");
         return -EBUSY;
      }
      break;

   }

   pd_board[board].open++;
   DPRINTK_I("pd_driver_open: open count = %d\n", pd_board[board].open);

   return 0;
}

int pd_driver_close(int board, int minor)
{
   // if release is called, library shall already inform driver about it
   // using IOCTL_PWRDAQ_CLOSESUBSYSTEM, so clearing of fd seems to be safe
   switch (minor)
   {
   case PD_MINOR_AIN:
      if (pd_board[board].AinSS.bInUse)
      {
         pd_board[board].AinSS.bInUse = FALSE;
         pd_board[board].fd[AnalogIn] = 0;
      }
      break;

   case PD_MINOR_AOUT:
      if (pd_board[board].AoutSS.bInUse)
      {
         pd_board[board].AoutSS.bInUse = FALSE;
         pd_board[board].fd[AnalogOut] = 0;
      }
      break;

   case PD_MINOR_DIN:
      if (pd_board[board].DinSS.bInUse)
      {
         pd_board[board].DinSS.bInUse = FALSE;
         pd_board[board].fd[DigitalIn] = 0;
      }
      break;

   case PD_MINOR_DOUT:
      if (pd_board[board].DoutSS.bInUse)
      {
         pd_board[board].DoutSS.bInUse = FALSE;
         pd_board[board].fd[DigitalOut] = 0;
      }
      break;

   case PD_MINOR_UCT:
      if (pd_board[board].UctSS.bInUse)
      {
         pd_board[board].UctSS.bInUse = FALSE;
         pd_board[board].fd[CounterTimer] = 0;
      }
      break;

   case PD_MINOR_DSPCT:
      if (pd_board[board].UctSS.bInUse)
      {
         pd_board[board].UctSS.bInUse = FALSE;
         pd_board[board].fd[DSPCounter] = 0;
      }
      break;

   }

   // if last board closed
   pd_board[board].open--;
   DPRINTK_I("pd_driver_close: open count = %d\n", pd_board[board].open);
   if (!pd_board[board].open)
   {
      pd_board[board].AinSS.bInUse = FALSE;
      pd_board[board].AoutSS.bInUse = FALSE;
      pd_board[board].DinSS.bInUse = FALSE;
      pd_board[board].DoutSS.bInUse = FALSE;
      pd_board[board].UctSS.bInUse = FALSE;
   }

   return 0;
}


int pd_driver_ioctl(int board, int board_minor, int command, tCmd* argcmd)
{
   int ret;
   PD_SUBSYSTEM ss;
   int retf = -ENODEV;
   int i;
   unsigned long id;

   if (board_minor == PD_MINOR_AIN)
      ss = AnalogIn;
   else if (board_minor == PD_MINOR_AOUT)
      ss = AnalogOut;
   else if (board_minor == PD_MINOR_DIN)
      ss = DigitalIn;
   else if (board_minor == PD_MINOR_DOUT)
      ss = DigitalOut;
   else if (board_minor == PD_MINOR_UCT)
      ss = CounterTimer;
   else if (board_minor == PD_MINOR_DSPCT)
      ss = DSPCounter;
   else if (board_minor == PD_MINOR_DRV)
      ss = BoardLevel;
   else
      ss = BoardLevel;

   DPRINTK_I("board %d, %s (minor %u) got ioctl 0x%X\n",
             board, pd_devices_by_minor[board_minor], board_minor, command);

   if (PD_IS_PDXI(pd_board[board].PCI_Config.SubsystemID))
      id = pd_board[board].PCI_Config.SubsystemID - 0x100;
   else
      id = pd_board[board].PCI_Config.SubsystemID;

   // check for board model before executing IOCTL
   if (PD_IS_DIO(id))
   {
      switch (command)
      {
      case IOCTL_PWRDAQ_DOWRITE:
      case IOCTL_PWRDAQ_DORESET:

      case IOCTL_PWRDAQ_UCTSETCFG:
      case IOCTL_PWRDAQ_UCTSTATUS:
      case IOCTL_PWRDAQ_UCTWRITE:
      case IOCTL_PWRDAQ_UCTREAD:
      case IOCTL_PWRDAQ_UCTSWGATE:

      case IOCTL_PWRDAQ_DICLRDATA:
      case IOCTL_PWRDAQ_DIREAD:
      case IOCTL_PWRDAQ_DIRESET:

      case IOCTL_PWRDAQ_CALSETCFG:
      case IOCTL_PWRDAQ_CALDACWRITE:
         retf = -ENOSYS;
         return retf;
         break;
      }
   }
   else if (PD_IS_AO(id))
   {
      switch (command)
      {
      case IOCTL_PWRDAQ_UCTSETCFG:
      case IOCTL_PWRDAQ_UCTSTATUS:
      case IOCTL_PWRDAQ_UCTWRITE:
      case IOCTL_PWRDAQ_UCTREAD:
      case IOCTL_PWRDAQ_UCTSWGATE:
      case IOCTL_PWRDAQ_UCTSWCLK:
      case IOCTL_PWRDAQ_UCTRESET:

      case IOCTL_PWRDAQ_AISETCFG:
      case IOCTL_PWRDAQ_AISETCVCLK:
      case IOCTL_PWRDAQ_AISETCLCLK:
      case IOCTL_PWRDAQ_AISETCHLIST:
      case IOCTL_PWRDAQ_AISETEVNT:
      case IOCTL_PWRDAQ_AISTATUS:
      case IOCTL_PWRDAQ_AICVEN:
      case IOCTL_PWRDAQ_AISTARTTRIG:
      case IOCTL_PWRDAQ_AISTOPTRIG:
      case IOCTL_PWRDAQ_AISWCLSTART:
      case IOCTL_PWRDAQ_AICLRESET:
      case IOCTL_PWRDAQ_AICLRDATA:
      case IOCTL_PWRDAQ_AIRESET:
      case IOCTL_PWRDAQ_AIGETVALUE:
      case IOCTL_PWRDAQ_AIGETSAMPLES:
      case IOCTL_PWRDAQ_AISETSSHGAIN:
      case IOCTL_PWRDAQ_AISETXFERSIZE:
      //case IOCTL_PWRDAQ_AIN_BLK_XFER:
         retf = -ENOSYS;
         return retf;
         break;
      }
   }
   else if (PD_IS_LABMF(id))
   {
      switch (command)
      {
      case IOCTL_PWRDAQ_UCTSETCFG:
      case IOCTL_PWRDAQ_UCTSTATUS:
      case IOCTL_PWRDAQ_UCTWRITE:
      case IOCTL_PWRDAQ_UCTREAD:
      case IOCTL_PWRDAQ_UCTSWGATE:
      case IOCTL_PWRDAQ_UCTSWCLK:
      case IOCTL_PWRDAQ_UCTRESET:
         retf = -ENOSYS;
         return retf;
         break;
      }
   }

   _fw_spinlock

   switch (command)
   {
   case  IOCTL_PWRDAQ_OPENSUBSYSTEM:
      if (pd_board[board].fd[argcmd->AcqSS.subSystem] == 0)
      {
         pd_board[board].fd[argcmd->AcqSS.subSystem] = argcmd->AcqSS.fileDescriptor;
         retf = 0;
         DPRINTK_I("IOCTL_PWRDAQ_OPENSUBSYSTEM with %d,%d (%s)\n",
                   argcmd->AcqSS.subSystem,argcmd->AcqSS.fileDescriptor,
                   pd_devices_by_minor[board_minor]);
      }
      else
      {
         DPRINTK_I("IOCTL_PWRDAQ_OPENSUBSYSTEM: board %d susbsystem %s is busy\n", board, pd_devices_by_minor[board_minor]);
         retf = -EBUSY;
      }
      break;

   case  IOCTL_PWRDAQ_CLOSESUBSYSTEM:
      if (pd_board[board].fd[argcmd->AcqSS.subSystem] ==
          argcmd->AcqSS.fileDescriptor)
      {
         pd_board[board].fd[argcmd->AcqSS.subSystem] = 0;
         retf = 0;
         DPRINTK_I("IOCTL_PWRDAQ_CLOSESUBSYSTEM with %d,%d (%s)\n",
                   argcmd->AcqSS.subSystem, argcmd->AcqSS.fileDescriptor,
                   pd_devices_by_minor[board_minor]);

      }
      else
      {
         DPRINTK_I("IOCTL_PWRDAQ_CLOSESUBSYSTEM: board %d susbsystem %s file descriptor doesn'tmatch\n",board,pd_devices_by_minor[board_minor]);
         retf = -EBUSY;
      }
      break;

   case  IOCTL_PWRDAQ_RELEASEALL:
      retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_PRIVATE_GETCFG:
      memcpy(&argcmd->PciConfig, (u8*)&pd_board[board].PCI_Config, sizeof(PD_PCI_CONFIG));
      retf = 0;
      break;

   case  IOCTL_PWRDAQ_PRIVATE_SET_EVENT:  // put to sleep...
      // store timeout for subsystem
      retf = pd_sleep_on_event(board, ss,
                               argcmd->dwParam[0],  //event
                               argcmd->dwParam[1]); //timeout
      break;

   case IOCTL_PWRDAQ_PRIVATE_CLR_EVENT:
      // , // Event
      // for now - wake up from event
      retf = pd_notify_event(board, ss, argcmd->dwParam[0]);

      break;

   case  IOCTL_PWRDAQ_GET_NUMBER_ADAPTER:
      argcmd->dwParam[0] = num_pd_boards;
      retf = 0;
      break;

   case  IOCTL_PWRDAQ_REGISTER_BUFFER:
      // Release spinlock, the code in pd_register_daq_buffer
      // is not safe to run with spinlock held
      _fw_spinunlock

      retf = argcmd->dwParam[0] = pd_register_daq_buffer(board,
                                    argcmd->dwParam[4], // subsystem
                                    argcmd->dwParam[0], // dwScanSize
                                    argcmd->dwParam[1], // dwScansFrm
                                    argcmd->dwParam[2], // dwFramesBfr
                                    NULL, // ptr to user buffer
                                    argcmd->dwParam[3]);

      // Reaquire the lock
      _fw_spinlock

      retf = (retf) ? retf : -EIO;
      break;

#if defined(_PD_RTL) || defined(_PD_RTAI) || defined(_PD_RTLPRO)
   case  IOCTLRT_PWRDAQ_GETKERNELBUFPTR:
      if (board_minor == PD_MINOR_AIN)   // Note: kernel-to-kernel space request
         argcmd->dwParam[0] = (u32)pd_board[board].AinSS.BufInfo.databuf;
      else if (board_minor == PD_MINOR_AOUT)
         argcmd->dwParam[0] = (u32)pd_board[board].AoutSS.BufInfo.databuf;
      else argcmd->dwParam[0] = -EINVAL;
      break;
#endif

   case  IOCTL_PWRDAQ_UNREGISTER_BUFFER:
      // Release spinlock, the code in pd_unregister_daq_buffer
      // is not safe to run with spinlock held
      _fw_spinunlock

      retf = argcmd->dwParam[1] =
             pd_unregister_daq_buffer(board, argcmd->dwParam[0]);

      // Reaquire the lock
      _fw_spinlock

      retf = (retf) ? 0 : -EIO;
      break;

   case IOCTL_PWRDAQ_GETKERNELBUFSIZE:
      retf = argcmd->dwParam[1] =
              pd_get_daq_buffer_size(board, argcmd->dwParam[0]);
      retf = (retf) ? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_SET_USER_EVENTS:
      if (PD_IS_DIO(id) && !(argcmd->dwParam[0] & EdgeDetect))
      {
         if (argcmd->dwParam[0] == DigitalIn)
            argcmd->dwParam[0] = AnalogIn;
         if (argcmd->dwParam[0] == DigitalOut)
            argcmd->dwParam[0] = AnalogOut;
         if (argcmd->dwParam[0] == DSPCounter)
            argcmd->dwParam[0] = AnalogIn;
      }
      else
      {
         argcmd->dwParam[0] = argcmd->dwParam[0] & 0xF;
      }

      // reset synch data structure for the specified subsystem
      {
         TSynchSS *synch;
         synch = pd_get_synch(board, argcmd->dwParam[0], argcmd->dwParam[1]);
         if(synch != NULL)
            synch->notifiedEvents = 0;
      }

      retf = (pd_set_user_events(board,
                                 argcmd->dwParam[0],
                                 argcmd->dwParam[1]) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_CLEAR_USER_EVENTS:
      if (PD_IS_DIO(id) && !(argcmd->dwParam[0] & EdgeDetect))
      {
         if (argcmd->dwParam[0] == DigitalIn)
            argcmd->dwParam[0] = AnalogIn;
         if (argcmd->dwParam[0] == DigitalOut)
            argcmd->dwParam[0] = AnalogOut;
         if (argcmd->dwParam[0] == DSPCounter)
            argcmd->dwParam[0] = AnalogIn;
      }
      else
      {
         argcmd->dwParam[0] = argcmd->dwParam[0] & 0xF;
      }

      retf = (pd_clear_user_events(board,
                                   argcmd->dwParam[0],
                                   argcmd->dwParam[1])? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_GET_USER_EVENTS:
      if (PD_IS_DIO(id) && !(argcmd->dwParam[0] & EdgeDetect))
      {
         if (argcmd->dwParam[0] == DigitalIn)
            argcmd->dwParam[0] = AnalogIn;
         if (argcmd->dwParam[0] == DigitalOut)
            argcmd->dwParam[0] = AnalogOut;
         if (argcmd->dwParam[0] == DSPCounter)
            argcmd->dwParam[0] = AnalogIn;
      }
      else
      {
         argcmd->dwParam[0] = argcmd->dwParam[0] & 0xF;
      }

      retf = (pd_get_user_events(board,
                                 argcmd->dwParam[0],
                                 (u32*)&argcmd->dwParam[1]) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_IMMEDIATE_UPDATE:
      retf = (pd_immediate_update(board))? 0 : -EIO;
      break;


   case  IOCTL_PWRDAQ_SET_TIMED_UPDATE: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_AIN_ASYNC_INIT:
      retf = (pd_ain_async_init(board, &(argcmd->AsyncCfg)) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_AIN_ASYNC_TERM:
      retf = (pd_ain_async_term(board) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_AIN_ASYNC_START:
      if (pd_ain_async_start(board))
         retf = 0;
      else retf = -EIO;
      break;

   case  IOCTL_PWRDAQ_AIN_ASYNC_STOP:
      if (pd_ain_async_stop(board))
         retf = 0;
      else retf = -EIO;
      break;

   case  IOCTL_PWRDAQ_AO_ASYNC_INIT:
      retf = (pd_aout_async_init(board, &(argcmd->AsyncCfg)) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_AO_ASYNC_TERM:
      retf = (pd_aout_async_term(board) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_AO_ASYNC_START:
      if (pd_aout_async_start(board))
         retf = 0;
      else retf = -EIO;
      break;

   case  IOCTL_PWRDAQ_AO_ASYNC_STOP:
      if (pd_aout_async_stop(board))
         retf = 0;
      else retf = -EIO;
      break;

   case  IOCTL_PWRDAQ_GET_DAQBUF_STATUS: retf = -ENOSYS;
      //pd_ain_async_get_status(board); // not for now, OK?
      break;

   case  IOCTL_PWRDAQ_GET_DAQBUF_SCANS:
      if ((argcmd->ScanInfo.Subsystem == AnalogIn)||
          (argcmd->ScanInfo.Subsystem == DigitalIn)||
          (argcmd->ScanInfo.Subsystem == CounterTimer)||
          (argcmd->ScanInfo.Subsystem == DSPCounter))
         retf = pd_ain_get_scans( board, &argcmd->ScanInfo) ? 0 : -EIO;   // input buffer
      else if ((argcmd->ScanInfo.Subsystem == AnalogOut)||
               (argcmd->ScanInfo.Subsystem == DigitalOut))
         retf = pd_aout_get_scans( board, &argcmd->ScanInfo) ? 0 : -EIO; // output buffer

      break;

   case  IOCTL_PWRDAQ_CLEAR_DAQBUF: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_BRDRESET: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_BRDEEPROMREAD:
      if (argcmd->EepromAcc.MaxSize > PD_EEPROM_SIZE)
         argcmd->EepromAcc.MaxSize = PD_EEPROM_SIZE;

      memcpy(argcmd->EepromAcc.Buffer, pd_board[board].Eeprom.u.WordValues,
             argcmd->EepromAcc.MaxSize*sizeof(u16));

      argcmd->EepromAcc.WordsRead = argcmd->EepromAcc.MaxSize;
      retf = argcmd->EepromAcc.WordsRead;
      break;

   case  IOCTL_PWRDAQ_BRDEEPROMWRITE:
      retf = (pd_adapter_eeprom_write(board,
                                      argcmd->EepromAcc.MaxSize,
                                      argcmd->EepromAcc.Buffer))? 0 : -EIO;

      // update chached EEPROM
      if (argcmd->EepromAcc.MaxSize > PD_EEPROM_SIZE)
         argcmd->EepromAcc.MaxSize = PD_EEPROM_SIZE;

      memcpy(pd_board[board].Eeprom.u.WordValues,
             argcmd->EepromAcc.Buffer,
             argcmd->EepromAcc.MaxSize*sizeof(u16));

      break;

   case  IOCTL_PWRDAQ_BRDENABLEINTERRUPT:
      retf = (pd_adapter_enable_interrupt(board, argcmd->dwParam[0]) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_BRDTESTINTERRUPT:
      retf = (pd_adapter_test_interrupt(board) ? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_BRDSTATUS:
      retf = (pd_adapter_get_board_status(board, &argcmd->Event)?0:-EIO);
      break;

   case  IOCTL_PWRDAQ_BRDSETEVNTS1:
      retf = (pd_adapter_set_board_event1(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_BRDSETEVNTS2:
      retf = (pd_adapter_set_board_event2(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_BRDFWLOAD: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_BRDFWEXEC: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_BRDREGWR:
      retf = (pd_dsp_reg_write(board, argcmd->dwParam[0],
                               argcmd->dwParam[1]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_BRDREGRD:
      retf = (pd_dsp_reg_read(board, argcmd->dwParam[0],
                              (u32*)&argcmd->dwParam[1]))? 0 : -EIO;
      break;

      // AI
   case  IOCTL_PWRDAQ_AISETCFG:
      retf = (pd_ain_set_config(board, argcmd->dwParam[0],
                                argcmd->dwParam[1],argcmd->dwParam[2]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISETCVCLK:
      retf = (pd_ain_set_cv_clock(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISETCLCLK:
      retf = (pd_ain_set_cl_clock(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISETCHLIST:
      retf = (pd_ain_set_channel_list(board, argcmd->SyncCfg.dwChListSize,
                                      argcmd->SyncCfg.dwChList))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISETEVNT:
      retf = (pd_ain_set_events(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISTATUS:
      retf = (pd_ain_get_status(board, (u32*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AICVEN:
      retf = (pd_ain_set_enable_conversion(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISTARTTRIG:
      retf = (pd_ain_sw_start_trigger(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISTOPTRIG:
      retf = (pd_ain_sw_stop_trigger(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISWCVSTART:
      retf = (pd_ain_sw_cv_start(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AISWCLSTART:
      retf = (pd_ain_sw_cl_start(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AICLRESET:
      retf = (pd_ain_reset_cl(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AICLRDATA:
      retf = (pd_ain_clear_data(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AIRESET:
      retf = (pd_ain_reset(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AIGETVALUE:
      retf = (pd_ain_get_value(board, (u16*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AIGETSAMPLES:
      if (argcmd->bufParam.size > pd_board[board].AinSS.XferBufValues)
      {
         retf = -EINVAL;
         break;
      }
      pd_ain_get_samples(board,
                         argcmd->bufParam.size/sizeof(u16),
                         (u16*)argcmd->bufParam.buffer,
                         (u32*)&ret);

      argcmd->bufParam.size = ret*sizeof(u16);

     /* pd_copy_to_user16((uint16_t*)argcmd->bufParam.buffer,
                        (u16*)pd_board[board].AinSS.pXferBuf,
                        ret*sizeof(u16));*/
      retf = (ret < 0) ? -EIO : ret;
      break;

   case  IOCTL_PWRDAQ_AIGETXFERSAMPLES:
      ret = pd_ain_get_xfer_samples(board, (uint16_t *)pd_board[board].AinSS.pXferBuf);
      memcpy((uint16_t*)argcmd->bufParam.buffer,
		                (uint16_t*)pd_board[board].AinSS.pXferBuf,
			            argcmd->bufParam.size);
      ret = argcmd->bufParam.size/sizeof(u16);
      retf = (ret < 0) ? -EIO : ret;
      break;

   case  IOCTL_PWRDAQ_AISETXFERSIZE:
      retf = (pd_ain_set_xfer_size(board,
                                   argcmd->dwParam[0]
                                  ))? 0 : -EIO;
      break;


   case  IOCTL_PWRDAQ_AISETSSHGAIN: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_AIENABLECLCOUNT: retf = -ENOSYS;
      break;
   case  IOCTL_PWRDAQ_AIENABLETIMER: retf = -ENOSYS;
      break;
   case  IOCTL_PWRDAQ_AIFLUSHFIFO: retf = -ENOSYS;
      break;
   case  IOCTL_PWRDAQ_AIGETSAMPLECOUNT:
      //retf = -ENOSYS;
      argcmd->dwParam[0] = pd_board[board].AinSS.BufInfo.Count;
      break;
      // AO
   case  IOCTL_PWRDAQ_AOSETCFG:
      retf = (pd_aout_set_config(board,
                                 argcmd->dwParam[0],
                                 argcmd->dwParam[1]
                                ))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSETCVCLK:
      retf = (pd_aout_set_cv_clk(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSETEVNT:
      retf = (pd_aout_set_events(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSTATUS:
      retf = (pd_aout_get_status(board, (u32*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOCVEN:
      retf = (pd_aout_set_enable_conversion(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSTARTTRIG:
      retf = (pd_aout_sw_start_trigger(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSTOPTRIG:
      retf = (pd_aout_sw_stop_trigger(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOSWCVSTART:
      retf = (pd_aout_sw_cv_start(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOCLRDATA:
      retf = (pd_aout_clear_data(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AORESET:
      retf = (pd_aout_reset(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOPUTVALUE:
      retf = (pd_aout_put_value(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_AOPUTBLOCK:
      if (argcmd->dwParam[0]>AOUT_BLKSIZE*2)
      {
         retf = -EINVAL; break;
      }
      /*pd_copy_from_user32((u32*)pd_board[board].AoutSS.pXferBuf,
                          (u32*)argcmd->bufParam.buffer,
                          argcmd->bufParam.size*sizeof(u32));*/
      retf = (pd_aout_put_block(board,
                                argcmd->bufParam.size/sizeof(u32),    //num values
                                (u32*)argcmd->bufParam.buffer,       //pdwBuf
                                (u32*)&argcmd->bufParam.size          //pdwCount
                               ))? 0 : -EIO;
      break;

      // DIO
   case  IOCTL_PWRDAQ_DISETCFG:
      retf = (pd_din_set_config(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DISTATUS:
      retf = (pd_din_status(board, (u32*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DIREAD:
      retf = (pd_din_read_inputs(board, (u32*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DICLRDATA:
      retf = (pd_din_clear_data(board)? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_DIRESET:
      retf = (pd_din_reset(board)? 0 : -EIO);
      break;

   case  IOCTL_PWRDAQ_DOWRITE:
      retf = (pd_dout_write_outputs(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DORESET:
      retf = (pd_dout_write_outputs(board, 0))? 0 : -EIO; // reset writes all zeroes
      break;

   case  IOCTL_PWRDAQ_DIO256CMDWR:
      retf = (pd_dio256_write_output(board,
                                     argcmd->dwParam[0],
                                     argcmd->dwParam[1]))? 0 : -EIO;;
      break;

   case  IOCTL_PWRDAQ_DIO256CMDWR_ALL:
      retf = (pd_dio256_write_all(board, argcmd->dwParam))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DIO256CMDRD:
      retf = (pd_dio256_read_input(board,
                                   argcmd->dwParam[0],
                                   (u32*)&argcmd->dwParam[1]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DIO256CMDRD_ALL:
      retf = (pd_dio256_read_all(board, argcmd->dwParam))? 0 : -EIO;
      break;

      //---------------------------------------------------------------
      // PdDIODMASet  -- write 3 param, DIO256
   case IOCTL_PWRDAQ_DIODMASET:
      retf = (pd_dio_dmaSet(board, argcmd->dwParam[0],
                            argcmd->dwParam[1], argcmd->dwParam[2])
             )? 0 : -EIO;
      break;

      //---------------------------------------------------------------
      // PdAODMASet  -- write 3 param, DIO256
   case IOCTL_PWRDAQ_AODMASET:
      retf = (pd_aout_dmaSet(board, argcmd->dwParam[0],
                             argcmd->dwParam[1], argcmd->dwParam[2])
             )? 0 : -EIO;
      break;


      //---------------------------------------------------------------
      // PdDIO256SetIntrMask  -- write 2 param to DSP register
   case IOCTL_PWRDAQ_DIO256SETINTRMASK:
      memcpy((u32*)pd_board[board].DinSS.intrMask,
                          (u32*)argcmd->bufParam.buffer,
                          argcmd->bufParam.size);

      DPRINTK("DIO SetIntMask ");
      for (i=0; i<8; i++)
      {
         DPRINTK("reg%d:0x%x ", i, pd_board[board].DinSS.intrMask[i]);
      }
      DPRINTK("\n");

      retf = (pd_dio256_setIntrMask(board)) ? 0 : -EIO;
      break;

      //---------------------------------------------------------------
      // PdDIO256GetIntrData  -- write 2 param to DSP register
   case IOCTL_PWRDAQ_DIO256GETINTRDATA:
      retf = (pd_dio256_getIntrData(board))? 0 : -EIO;

      DPRINTK("DIO GetIntrData ");
      for (i=0; i<16; i++)
      {
         DPRINTK("reg%d:0x%x ", i, pd_board[board].DinSS.intrData[i]);
      }
      DPRINTK("\n");

      memcpy((u32*)argcmd->bufParam.buffer,
             (u32*)pd_board[board].DinSS.intrData,
             16*sizeof(u32));
      break;

      //---------------------------------------------------------------
      // PdDIO256IntrEnable -- write 2 param to DSP register
   case IOCTL_PWRDAQ_DIO256INTRREENABLE:
      DPRINTK("DIO Intr Enable %d\n", argcmd->dwParam[0]);
      retf = (pd_dio256_intrEnable(board, argcmd->dwParam[0]))? 0 : -EIO;;
      break;


      // UCT
   case  IOCTL_PWRDAQ_UCTSETCFG:
      retf = (pd_uct_set_config(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTSTATUS: retf = -ENOSYS;
      retf = (pd_uct_get_status(board, (u32*)&argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTWRITE: retf = -ENOSYS;
      retf = (pd_uct_write(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTREAD: retf = -ENOSYS;
      retf = (pd_uct_read(board,
                          argcmd->dwParam[0],
                          (u32*)&argcmd->dwParam[1])
             )? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTSWGATE: retf = -ENOSYS;
      retf = (pd_uct_set_sw_gate(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTSWCLK: retf = -ENOSYS;
      retf = (pd_uct_sw_strobe(board))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_UCTRESET: retf = -ENOSYS;
      retf = (pd_uct_reset(board))? 0 : -EIO;
      break;

      //CALDIAG
   case  IOCTL_PWRDAQ_CALSETCFG: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_CALDACWRITE:
      retf = (pd_cal_dac_write(board, argcmd->dwParam[0]))? 0 : -EIO;
      break;

   case  IOCTL_PWRDAQ_DIAGPCIECHO: retf = -ENOSYS;
      break;

   case  IOCTL_PWRDAQ_DIAGPCIINT: retf = -ENOSYS;
      break;

      // test only ioctls
   case IOCTL_PWRDAQ_TESTEVENTS:
      DPRINTK_I("test events from dispatch routine\n");
      //pd_adapter_enable_interrupt(board, 1);
      //retf = (pd_adapter_test_interrupt(board))? 0 : -EIO;
      //pd_adapter_enable_interrupt(board, 0);
#if defined(_PD_RTLPRO)
      rtl_sem_post(&rt_bh_sem[board]);
      retf = 0;
#endif
      break;

   case IOCTL_PWRDAQ_TESTBUFFERING:
      //test_pd_ain_buffering(board);
      retf = -ENOSYS; // 0
      break;

   case IOCTL_PWRDAQ_GETADCFIFOSIZE:
      argcmd->dwParam[0] =
      pd_board[board].Eeprom.u.Header.ADCFifoSize * 0x400;

      DPRINTK_I("pd_ioctl: ADC FIFO size =%d\n", argcmd->dwParam[0]);
      retf=0;
      break;

   case IOCTL_PWRDAQ_GETSERIALNUMBER:
      memcpy((u8*)(argcmd->bufParam.buffer), (u8*)&pd_board[board].Eeprom.u.Header.SerialNumber, PD_SERIALNUMBER_SIZE);
      argcmd->bufParam.size = PD_SERIALNUMBER_SIZE;
      retf=0;
      break;

   case IOCTL_PWRDAQ_GETMANFCTRDATE:
      memcpy((u8*)(argcmd->bufParam.buffer), (u8*)&pd_board[board].Eeprom.u.Header.ManufactureDate, PD_DATE_SIZE);
      argcmd->bufParam.size = PD_DATE_SIZE;
      retf=0;
      break;

   case IOCTL_PWRDAQ_GETCALIBRATIONDATE:
      memcpy((u8*)(argcmd->bufParam.buffer), (u8*)&pd_board[board].Eeprom.u.Header.CalibrationDate, PD_DATE_SIZE);
      argcmd->bufParam.size = PD_DATE_SIZE;
      retf=0;
      break;

#ifdef MEASURE_LATENCY
   case IOCTL_PWRDAQ_MEASURE_LATENCY:
      argcmd->dwParam[0] = pd_board[board].tscLow;
      argcmd->dwParam[1] = pd_board[board].tscHigh;
      retf=0;
      break;
#endif

   } // switch

   _fw_spinunlock

   return retf;
}


