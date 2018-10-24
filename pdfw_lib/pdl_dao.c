//===========================================================================
//
// NAME:    pdl_dao.c:
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to Digital and Analog Output FW functions
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
// this file is not to be compiled independently
// but to be included into pdfw_lib.c

//==============================================================================
// Function writes data to PD-AO-96 channel
int pd_ao96_writex(int board, u32 dwDACNum, u16 wDACValue, int dwHold, int dwUpdAll) 
{
    u32   dwCmd;

    // Set/Clear Write&Hold bit
    if (!dwHold) dwCmd = AOB_DACBASE | dwDACNum;
        else dwCmd = AOB_DACBASE | dwDACNum | AOB_AO96WRITEHOLD;

    // Set Update All bit
    if (dwUpdAll) dwCmd = dwCmd | AOB_AO96UPDATEALL;

    // Use this function for now.
    // In future releases individual AO-96 functions will be available
    return pd_dio256_write_output(board, dwCmd, wDACValue);
}

//==============================================================================
// Function writes data to PD-AO-32 channel
int pd_ao32_writex(int board, u32 dwDACNum, u16 wDACValue, int dwHold, int dwUpdAll) 
{

    u32 xcfg, val;
    int ret;

    if (dwHold) xcfg = AO32_WRH; else xcfg = AO32_WRPR;

    ret = pd_dio256_write_output(board, (dwDACNum & 0x7F)|xcfg|AO32_BASE, wDACValue);
    if (dwUpdAll) ret = pd_dio256_read_input(board, AO32_UPDALL|AO32_BASE, &val)
;

    return ret;
}

//==============================================================================
// reset ao-96 board
int pd_ao96_reset(int board) 
{

    int    bRes;
    u32    i;

    // remove any update channels
    bRes = 1;
    bRes &= pd_dio256_read_input(board, 0, &i);

    // write zeroes to all DACs
    for (i = 0; i < 95; i++)
        bRes &= pd_dio256_write_output(board, AOB_DACBASE | i, 0x7fff);

    return bRes;
}

//==============================================================================
// reset ao-32 board
int pd_ao32_reset(int board) 
{

    int    bRes;
    u32    i;

    // remove any update channels
    bRes = 1;
    bRes &= pd_dio256_read_input(board, 0, &i);

    // write zeroes to all DACs
    for (i = 0; i < 32; i++)
        bRes &= pd_dio256_write_output(board, i|AO32_WRPR|AO32_BASE, 0x7fff);

    return bRes;
}

//==============================================================================
// write and update data
int pd_ao96_write(int board, u16 wChannel, u16 wValue)
{
    u32 dwCmd;

    dwCmd = AOB_DACBASE |(wChannel&0x7f);
    return pd_dio256_write_output(board, dwCmd, wValue);
}


//==============================================================================
// write and update data
int pd_ao32_write(int board, u16 wChannel, u16 wValue)
{
    return pd_dio256_write_output(board,
                             (wChannel & 0x7F)|AO32_WRPR|AO32_BASE, wValue);
}

//==============================================================================
// write and update data
int pd_ao96_write_hold(int board, u16 wChannel, u16 wValue)
{
    u32 dwCmd;

    dwCmd = AOB_DACBASE |(wChannel&0x7f) | AOB_AO96WRITEHOLD;
    return pd_dio256_write_output(board, dwCmd, wValue);
}


//==============================================================================
// write and update data
int pd_ao32_write_hold(int board, u16 wChannel, u16 wValue)
{
    return pd_dio256_write_output(board,
                             (wChannel & 0x7F)|AO32_WRH|AO32_BASE, wValue);
}


//==============================================================================
// clock update all lines
int pd_ao96_update(int board)
{
    u32   dwValue;
    return pd_dio256_read_input(board, AOB_DACBASE, &dwValue);
}

//==============================================================================
// clock update all lines
int pd_ao32_update(int board)
{
    u32   dwValue;
    return pd_dio256_read_input(board, AO32_UPDALL|AO32_BASE, &dwValue);
}

//==============================================================================
// select update channel
int pd_ao96_set_update_channel(int board, u16 wChannel, u32 Mode)
{

    return pd_dio256_write_output(board,
            AOB_CTRBASE | wChannel | (Mode << 7), 0);
}


//==============================================================================
// select update channel
int pd_ao32_set_update_channel(int board, u16 wChannel, u32 bEnable) 
{
    u32   dwValue;
    return pd_dio256_read_input(board,
                            wChannel |
                            ((bEnable)?AO32_SETUPDMD|AO32_BASE:0),
                            &dwValue);
}

//==============================================================================
// aux
u32 pd_dio256_make_cmd(u32 dwRegister)
{
    u32 dwCmd = 0;
    switch (dwRegister)
    {
        case 0: dwCmd |= DIO_REG0; break;
        case 1: dwCmd |= DIO_REG1; break;
        case 2: dwCmd |= DIO_REG2; break;
        case 3: dwCmd |= DIO_REG3; break;
        case 4: dwCmd |= DIO_REG4; break;
        case 5: dwCmd |= DIO_REG5; break;
        case 6: dwCmd |= DIO_REG6; break;
        case 7: dwCmd |= DIO_REG7; break;
        default: dwCmd |= DIO_REG0; break;
    }
    return dwCmd;
}

int pd_dio256_make_reg_mask(u32 bank, u32 regMask)
{
    u32   reg = bank;
    reg = (reg & 0x4);
    reg |= (regMask & 0x3);
    reg |= ((regMask << 1) & 0x18);
    return reg;
}

//==============================================================================
// reset PD2-DIO board (to all tristate)
int pd_dio_reset(int board) 
{
    u32    bRes = TRUE;
    bRes &= pd_dio256_write_output(board, DIO_DIS_0_3,
                              0);
    bRes &= pd_dio256_write_output(board, DIO_DIS_4_7,
                              0);
    return bRes;
}

//==============================================================================
//
int pd_dio_enable_output(int board, u32 dwRegMask) 
{
     u32 dwRegM = ~dwRegMask;
     u32  bRes = TRUE;

     bRes &= pd_dio256_write_output(board,
                               pd_dio256_make_reg_mask(DIO_REG0 & 0xF, (dwRegM & 0xF)) | DIO_WOE,
                               0);
     bRes &= pd_dio256_write_output(board,
                               pd_dio256_make_reg_mask(DIO_REG4 & 0xF, (dwRegM & 0xF0)>>4 ) | DIO_WOE,
                               0);
     return bRes;
}

//==============================================================================
// latch all ports in bank
int pd_dio_latch_all(int board, u32 dwRegister) 
{
    u32   dwResult;
    return pd_dio256_read_input (board, DIO_LAL | (dwRegister & 0x4),
                            &dwResult);
}

//==============================================================================
// read but not latch
int pd_dio_simple_read(int board, u32 dwRegister, u32 *pdwValue) 
{
    return pd_dio256_read_input(board,
                            (pd_dio256_make_cmd(dwRegister) | DIO_SRD),
                            pdwValue);
}

//==============================================================================
// latch and read
int pd_dio_read(int board, u32 dwRegister, u32 *pdwValue) 
{
    return pd_dio256_read_input(board,
                            (pd_dio256_make_cmd(dwRegister) | DIO_LAR),
                            pdwValue);
}

//==============================================================================
//
int pd_dio_write(int board, u32 dwRegister, u32 dwValue)
{
    return pd_dio256_write_output(board,
                            (pd_dio256_make_cmd(dwRegister) | DIO_SWR),
                            (dwValue & 0xFFFF));
}

//==============================================================================
//
int pd_dio_prop_enable(int board, u32 dwRegMask) 
{
     u32 dwRegM = ~dwRegMask;
     u32  bRes = TRUE;

     bRes &= pd_dio256_write_output(board,
                               pd_dio256_make_reg_mask(DIO_REG0 & 0xF, (dwRegM & 0xF)) | DIO_PWR,
                               0);

     bRes &= pd_dio256_write_output(board,
                               pd_dio256_make_reg_mask(DIO_REG4 & 0xF, (dwRegM & 0xF0)>>4 ) | DIO_PWR,
                               0);
     return bRes;
}

//==============================================================================
//
int pd_dio_latch_enable(int board, u32 dwRegister, u32 bEnable)
{
    return pd_dio256_write_output(board,
                            (pd_dio256_make_cmd(dwRegister & 0x4) | DIO_LWR | ((bEnable)?1:0)<<1),
                            0);
}


