//===========================================================================
//
// NAME:    pdfw_lib.c
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver firmware interface library
//
// AUTHOR: Frederic Villeneuve 
//         Alex Ivchenko
//
//---------------------------------------------------------------------------
//      Copyright (C) 2000,2004 United Electronic Industries, Inc.
//      All rights reserved.
//---------------------------------------------------------------------------
// For more informations on using and distributing this software, please see
// the accompanying "LICENSE" file.
//
#define __NO_VERSION__

#ifdef _PD_RTLPRO
   #define __RTCORE_POLLUTED_APP__
   #define __KERNEL__

   #include <linux/kernel.h>
   #include <linux/types.h>
   #include <string.h>
   #include <errno.h>
#else                      
   #include <linux/version.h>
   #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
      #include <generated/autoconf.h>
   #else
      #include <linux/autoconf.h>
   #endif
   #include <linux/init.h>
   #include <linux/kernel.h>
   #include <linux/errno.h>
   #include <linux/types.h>
   #include <linux/sched.h>
#endif


// Libraries needed to recompile
#include "../include/win_ddk_types.h"
#include "../include/pdfw_def.h"
#include "../include/pdpcidef.h"
#include "../include/powerdaq-internal.h"
#include "../include/powerdaq.h"
#include "../include/powerdaq_kernel.h"
#include "../include/pdfw_if.h"
#include "../include/pd_dsp_ct.h"
#include "../include/pd_stmem.h"
#include "../include/pd_boards.h"

pd_board_t pd_board[PD_MAX_BOARDS];
int num_pd_boards = 0;

extern void pd_kill_fasync(struct fasync_struct *fa, int sig);

extern unsigned long pd_copy_from_user32(u32* to, u32* from, u32 len);
extern unsigned long pd___copy_from_user32(u32* to, u32* from, u32 len);
extern unsigned long pd_copy_to_user32(u32* to, u32* from, u32 len);
extern unsigned long pd___copy_to_user32(u32* to, u32* from, u32 len);

extern unsigned long pd_copy_from_user16(u16* to, u16* from, u32 len);
extern unsigned long pd___copy_from_user16(u16* to, u16* from, u32 len);
extern unsigned long pd_copy_to_user16(u16* to, u16* from, u32 len);
extern unsigned long pd_copy_to_userbuf16(u16* to, u16* from, u32 len);
extern unsigned long pd___copy_to_user16(u16* to, u16* from, u32 len);

extern unsigned long pd_copy_from_user8(u8* to, u8* from, u32 len);
extern unsigned long pd___copy_from_user8(u8* to, u8* from, u32 len);
extern unsigned long pd_copy_to_user8(u8* to, u8* from, u32 len);
extern unsigned long pd___copy_to_user8(u8* to, u8* from, u32 len);

extern int pd_get_user32(u32* value, u32* address);
extern int pd___get_user32(u32* value, u32* address);
extern int pd_get_user16(u16* value, u16* address);
extern int pd___get_user16(u16* value, u16* address);

extern void pd_udelay(u32 usecs);
extern void pd_mdelay(u32 msecs);
extern void* pd_alloc_bigbuf(u32 size);
extern void pd_free_bigbuf(void* mem, u32 size);

// Firmware interface itself
#include "firmware.c"
#include "pdl_fwi.c"
#include "pdl_brd.c"
#include "pdl_ain.c"
#include "pdl_aio.c"
#include "pdl_ao.c"
#include "pdl_dio.c"
#include "pdl_dao.c"
#include "pdl_uct.c"
#include "pdl_int.c"
#include "pdl_event.c"
#include "pdl_init.c"
#include "pdl_dspuct.c"


