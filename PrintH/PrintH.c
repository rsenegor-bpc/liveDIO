// PrintH.c - sends textual output to the Hercules or Monochrome 
// card installed on ISA bus.
// It's a pretty rare beast now, but extremely useful when you're
// about to debug driver and see it in dynamic
//
// Load PrintH module using insmod PrintH.o and send    
// driver debug output to the Hercules (or monochrome)
// monitor attached to your Linux system
//
// Copyright (C) 2000 UEI, Inc. by Alex Ivchenko

#include <linux/autoconf.h>

#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/init.h>
#include <linux/errno.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
   #include <linux/slab.h>
#else
   #include <linux/malloc.h>
#endif
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/tqueue.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include <asm/system.h>
#endif

#include "PrintH.h"

VIEWPORT    vp;     // default viewport
PVIEWPORT   pvp;

unsigned long* addrh;
int  xc, yc;
int hercmon_lines, hercmon_columns;
int linebytes;
char *HercVirtBase;

void hercmon_vp_calc(void);

//-----------------------------------------------------------------------------
// Scrolls Hercules screen by <int lines> lines
// Clears screen when int lines = 0;
// * function scrolls current viewport *
void hercmon_scroll(int lines)
{
    int i, j, offs, src;
    char linebuf[0x100];
    
    if ((lines < 0) || (lines > pvp->vlines))
        return;         // oops! wrong arg


    // clear screen
    if ((lines == 0) || (lines == pvp->vlines))
    {
        pvp->xc = pvp->yc = 0;
        xc = pvp->x1; yc = pvp->y1;
        offs = ((pvp->x1)<<1) + (pvp->y1*linebytes);
        
        for (i = offs, j = 0; j < pvp->vlines; i+=linebytes, j++)
            memset_io(HercVirtBase + i, 
                      0, 
                      pvp->linebytes);
        return;
    }


    // Scrol by several lines
    i = pvp->vlines - lines;                     // lines to move
    offs = ((pvp->x1)<<1) + (pvp->y1*linebytes); // viewport offset
    src = offs + lines*linebytes;                // start offset

    do {
        memcpy_fromio(linebuf, HercVirtBase+src, pvp->linebytes);
        memcpy_toio(HercVirtBase+offs, linebuf, pvp->linebytes);
        src += linebytes;
        offs += linebytes;
    } while (i--);
    
    // clear last line
    memset_io(HercVirtBase+src-linebytes, 0, pvp->linebytes);
}

//-----------------------------------------------------------------------------
// clears whole screen regardless of viewport
void hercmon_clear()
{
    xc = yc = 0;            // clear screen
    hercmon_vp_setptr(&vp); // original viewport
    
    memset_io(HercVirtBase, 0, linebytes*hercmon_lines);
    return;
}

//-----------------------------------------------------------------------------
// moves cursor to absolute position
void hercmon_crsr(xc, yc) 
{ 
    int val = xc + yc*hercmon_columns;

    outb(0xF,0x3B4); // Cursor Location Low register address
    outb(val & 0xFF,0x3B5);

    outb(0xE,0x3B4); // Cursor Location High register address
    outb((val & 0xFF00)>>8,0x3B5);
    
    return; 
}

//-----------------------------------------------------------------------------
// Print the string to the Hercules card w/o formatting
// supports only basic format characters:
// * function prints to the current viewport *
void hercmon_print(const char *str)
{
  int offs = 0;
  int i = 0;

  // note: all calcs in vp coords
  do {
      switch ((char)*(str+i))
      {
        case 0:     // end-of-the-ASCIIZ character
            break;

        case '\n':  // \n (CRLF, New line)
            pvp->xc = 0;
            if(pvp->yc < (pvp->vlines-1)) 
            {
               pvp->yc++;
            } else {
               hercmon_scroll(1);
            }
            break;

        case '\b':  // \b Backspace
            if(pvp->xc>0) pvp->xc--;
            break;

        case '\f':  // \f (Formfeed, clear screen)
               hercmon_scroll(0);
            break;

        case '\r':  // \r Carriage return
            if(pvp->xc>0) pvp->xc = 0;
            break;

        case '\t':  // \t Horizontal tab
            if (pvp->xc <= pvp->x2 + pvp->tab) pvp->xc += pvp->tab;
            break;

        case '\v':  // \v Vertical tab
            if (pvp->yc < pvp->y2) pvp->yc ++;
            break;

        default:    // character to print
            offs = ((pvp->x1+pvp->xc)<<1) + (pvp->y1+pvp->yc)*linebytes;
            writeb((char)*(str+i), HercVirtBase + offs);
            writeb(pvp->attrib, HercVirtBase + offs + 1);
            pvp->xc++;
            
            // adjust cursor position            
            if (pvp->xc >= pvp->vcolumns) {      // If we're @ last column
                pvp->xc = 0;
                if (pvp->yc < pvp->vlines - 1) {     // and last line
                   pvp->yc++;
                } else {
                   hercmon_scroll(1);
                }
            }            
            break;
      }
  } while((char)*(str+i++));

  // recalculate absolute xc and yc
  xc = pvp->xc + pvp->x1;
  yc = pvp->yc + pvp->y1;

  // move cursor to new position  
  hercmon_crsr(xc, yc);
    
  return;
}


//-----------------------------------------------------------------------------
// Print the string to the Hercules card viewport - formatted
void hercmon_printf(const char *fmt,...)
{
    char ppbuf[1024];
    va_list args;
    int i;
    
    va_start(args, fmt);            // retrieve arguments
    i = vsprintf(ppbuf, fmt, args);
    va_end(args);                   // reset arg_ptr

    hercmon_print(ppbuf);
}

//-----------------------------------------------------------------------------
// Set current cursor position on the Hercules card regardless of viewport
void hercmon_set_cursor(int x, int y)
{
    if (x>=0 && x<hercmon_columns) xc = x;
    if (y>=0 && y<hercmon_lines) yc = y;
    
    // move cursor to the new position
    hercmon_crsr(xc, yc);    
}

//-----------------------------------------------------------------------------
// Set current cursor position on the Hercules card 
// * coords are in viewport origins *
void hercmon_vp_set_cursor(int x, int y)
{
    if (x>=0 && x<pvp->vcolumns) {
        pvp->xc = x;
        xc = pvp->x1 + pvp->xc;
    }
    if (y>=0 && y<pvp->vlines) {
        pvp->yc = y;
        yc = pvp->y1 + pvp->yc;
    }
    // move cursor to the new position    
    hercmon_crsr(xc, yc);
}

//-----------------------------------------------------------------------------
// Calculate viewport vars
void hercmon_vp_calc( void )
{
  pvp->vlines = (pvp->y2 - pvp->y1 + 1);
  pvp->vcolumns = (pvp->x2 - pvp->x1 + 1);
  pvp->linebytes = (pvp->x2 - pvp->x1 + 1) << 1;
  pvp->offset = (pvp->x1 <<1) + (pvp->y1 * linebytes);
  pvp->xc = pvp->yc = 0;  
  return;
}

//-----------------------------------------------------------------------------
// Set up viewport size to deal with
void hercmon_vp_set(int x1, int y1, int x2, int y2)
{
  pvp->x1 = (x1>0 && x1<hercmon_columns) ? x1 : 0;
  pvp->y1 = (y1>0 && y1<hercmon_lines) ? y1 : 0;
  pvp->x2 = (x2>0 && x2<hercmon_columns) ? x2 : hercmon_columns;
  pvp->y2 = (y2>0 && y2<hercmon_lines) ? y2 : hercmon_lines;
  hercmon_vp_calc();
}

//-----------------------------------------------------------------------------
// set up char attribute
void hercmon_vp_set_attrib(int attribute)
{
  pvp->attrib = attribute & 0xff;
}

//-----------------------------------------------------------------------------
// Set pointer to current viewport structure located in external code
// Make sure the following fields are filled:
//  pvp->x1
//  pvp->y1
//  pvp->x2
//  pvp->y2
//  pvp->tab
//  pvp->attrib
//
void hercmon_vp_setptr(PVIEWPORT pVP)
{
    if (pVP) {
          pvp = pVP;
          hercmon_vp_calc();    
    }
}

//-----------------------------------------------------------------------------
// Initialize the module - register the proc file 
int init_module()
{
  printk("initializing PrintH module...\n");
//  int* pbuf;
  
  // remap hercules base address
  HercVirtBase = ioremap(HercAddrBase, 0x8000);
  
  xc = yc = 0;
  hercmon_columns = HERCULES_COLUMNS;
  hercmon_lines = HERCULES_LINES;
  
  pvp = &vp;    // set up viewport to use

  linebytes = hercmon_columns << 1;

  // default viewport is a full screen viewport
  pvp->x1 = 0;
  pvp->y1 = 0;
  pvp->x2 = hercmon_columns - 1;
  pvp->y2 = hercmon_lines - 1;
  pvp->tab = 4;
  pvp->attrib = HGCMON_NORMAL;

  hercmon_vp_calc();

  hercmon_scroll(0);

  hercmon_printf("Module PrintH is inserted (c) UEI, 2000\n");

  return 0;
}

//-----------------------------------------------------------------------------
// Cleanup - unregister our file from /proc 
void cleanup_module()
{
  hercmon_printf("Module PrintH is Removed\n");    
  hercmon_scroll(1);  

  // unmap virtual address
  iounmap((void *)HercVirtBase);

}
                 

