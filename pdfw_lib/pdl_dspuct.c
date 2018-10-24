//===========================================================================
//
// NAME:    pdl_dspuct.c
//
// DESCRIPTION:
//
//          This file contains implementations for the PowerDAQ DSP
//          Counter/Timer access in kernel mode
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
//---------------------------------------------------------------------------
//
//      Copyright (C) 2002 United Electronic Industries, Inc.
//      All rights reserved.
//      United Electronic Industries Confidential Information.
//
//===========================================================================



// Load all registers of the selected counter
int pd_dspct_load(int board,
                  u32 dwCounter, 
                  u32 dwLoad, 
                  u32 dwCompare, 
                  u32 dwMode, 
                  u32 dwReload, 
                  u32 dwInverted,
                  u32 dwUsePrescaler)
{
   int dwError;
   u32 dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
        
    // Read current status value register 
    dwError = pd_dsp_reg_read(board, 
            pd_dspct_get_status_addr(dwCounter),
            &dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    // Leave Timer/Interrupt enable bits
    dwTCSR = dwTCSR & (M_TE | M_TCIE | M_TOIE);

    // Program counter
    // Load register
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_load_addr(dwCounter),
            dwLoad);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    // Compare register
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_compare_addr(dwCounter),
            dwCompare);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    if (dwReload) 
        dwTCSR = dwTCSR | M_TRM | dwMode ;
    else
        dwTCSR = dwTCSR | dwMode;
    if (dwInverted) dwTCSR = dwTCSR | M_INV ;
    if (dwUsePrescaler) dwTCSR = dwTCSR | M_PCE ;
    // Control register
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    return(dwError);    
}


// Enable(1)/Disable(x) counting for the selected counter 
int pd_dspct_enable_counter(int board, u32 dwCounter, u32 dwEnable)
{
   int dwError;
   u32 dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
        
    // Read current status value register 
    dwError = pd_dsp_reg_read(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            &dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    if (dwEnable) 
      dwTCSR = dwTCSR | M_TE;
    else
      dwTCSR = dwTCSR & (~M_TE);

    // Update control register
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    return(dwError); 
}
   
// Enable(1)/Disable(x) interrupts for the selected events for the selected 
// counter (only one event can be enabled at the time)
int pd_dspct_enable_interrupts(int board,u32 dwCounter, u32 dwCompare, u32 dwOverflow)
{
   int dwError;
   u32 dwTCSR;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Check if both interrupts enabled at the same time
    if ((dwCompare) && (dwOverflow)) return(-1);
        
    // Read current status value register 
    dwError = pd_dsp_reg_read(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            &dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    if (dwCompare) dwTCSR = dwTCSR | M_TCIE;
    if (dwOverflow) dwTCSR = dwTCSR | M_TOIE;

    // Update control register
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            dwTCSR);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    return(dwError); 
}

// Get count register value from the selected counter
int pd_dspct_get_count(int board, u32 dwCounter, u32 *dwCount)
{
    int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_read(
            board, 
            pd_dspct_get_count_addr(dwCounter),
            dwCount);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    return(dwError); 
}

// Get compare register value from the selected counter
int pd_dspct_get_compare(int board, u32 dwCounter, u32 *dwCompare)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_read(
            board, 
            pd_dspct_get_compare_addr(dwCounter),
            dwCompare);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    return(dwError); 
}

// Get control/status register value from the selected counter
int pd_dspct_get_status(int board, u32 dwCounter, u32 *dwStatus)
{
   int dwError;
   
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_read(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            dwStatus);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    return(dwError); 
}

// Set compare register value from the selected counter
int pd_dspct_set_compare(int board,u32 dwCounter, u32 dwCompare)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_compare_addr(dwCounter),
            dwCompare);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");

    return(dwError); 
}

// Set load register value from the selected counter
int pd_dspct_set_load(int board,u32 dwCounter, u32 dwLoad)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_load_addr(dwCounter),
            dwLoad);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");

    return(dwError); 
}
// Set control/status register value from the selected counter
int pd_dspct_set_status(int board,u32 dwCounter, u32 dwStatus)
{
   int dwError;
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    
    // Read value
    dwError = pd_dsp_reg_write(
            board, 
            pd_dspct_get_status_addr(dwCounter),
            dwStatus);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");

    return(dwError); 
}


// Load prescaler
int pd_dsp_PS_load(int board, u32 dwLoad, u32 dwSource)
{
   int dwError;
   u32 dwTPLR;

    dwTPLR = dwLoad | dwSource ;
    // Program prescaler
    dwError = pd_dsp_reg_write(
            board, 
            M_TPLR,
            dwTPLR);
    if (!dwError) DPRINTK("_PdDspRegWrite error\n");
    return(dwError);
}

// Get prescaler count register value 
int pd_dsp_PS_get_count(int board, u32 *dwCount)
{
   int dwError;
    
    // Read value
    dwError = pd_dsp_reg_read(
            board, 
            M_TPCR,
            dwCount);
    if (!dwError) DPRINTK("_PdDspRegRead error\n");

    return(dwError); 
}

// Get address of the count register of the selected counter
u32 pd_dspct_get_count_addr(u32 dwCounter)
{
   u32 dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the load register of the selected counter
u32 pd_dspct_get_load_addr(u32 dwCounter)
{
   u32 dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TLR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TLR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TLR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the control/status register of the selected counter
u32 pd_dspct_get_status_addr(u32 dwCounter)
{
   u32 dwAddress;    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(0);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCSR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCSR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCSR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// Get address of the compare register of the selected counter
u32 pd_dspct_get_compare_addr(u32 dwCounter)
{
    u32 dwAddress;    
    
    // Check counter number 
    if ((dwCounter < DCT_UCT0) || (dwCounter > DCT_UCT2)) return(-1);
    // Calculate address
    switch(dwCounter) 
    {
        case DCT_UCT0 :
            dwAddress = M_TCPR0;
            break;
        case DCT_UCT1 :
            dwAddress = M_TCPR1;
            break;
        case DCT_UCT2 :
            dwAddress = M_TCPR2;
            break;
        default :
            dwAddress = 0;
    }
    return(dwAddress);
}

// end of DSPUCTFn.c

