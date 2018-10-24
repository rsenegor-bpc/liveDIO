//
// this file contains function for debug/test purpoes
//

#include "win_ddk_types.h"
#include "powerdaq-internal.h"
#include "powerdaq.h"
#include "pdfw_if.h"

static u16 wBuf[0x8000];  // temporarly storage

void test_pd_ain_events(int board)
{
    //u32  dwConfig;
    //u32  dwCLSize = 4;
    //u32  dwChList[4] = {0x100, 0x101, 0x102, 0x103};
    //u32  dwIntrCfg;
    int  res;
    int  i;
    TAinAsyncCfg AinCfg;
    TScan_Info ScanInfo; 

    // Should be set up on register_buffer
    pd_board[board].AinSS.BufInfo.ScanSize = 4;
    pd_board[board].AinSS.BufInfo.FrameSize = 0x800;
    pd_board[board].AinSS.BufInfo.NumFrames = 0x4;
    pd_board[board].AinSS.BufInfo.databuf = wBuf;
    pd_board[board].AinSS.BufInfo.bWrap = 0;
    pd_board[board].AinSS.BufInfo.bRecycle = 0;

    res = pd_ain_reset(board);

    AinCfg.dwAInCfg = AIB_INPTYPE | AIB_INPRANGE | AIB_CVSTART0 | AIB_CLSTART0 | AIB_SELMODE0;
    AinCfg.dwEventsNotify = eFrameDone | eBufferDone | eDataError | eBufferError |
                            eStartTrig | eStopTrig | eStopped;
    AinCfg.dwAInPreTrigCount = 0;
    AinCfg.dwAInPostTrigCount = 0;
    AinCfg.dwCvRate = 1100;
    AinCfg.dwClRate = 5500;
    AinCfg.dwChListSize = 4;
    AinCfg.dwChList[0] = 0x100;
    AinCfg.dwChList[1] = 0x101;
    AinCfg.dwChList[2] = 0x102;
    AinCfg.dwChList[3] = 0x103;

    res = pd_ain_async_init(board, &AinCfg);
    DPRINTK_T("pd_ain_async_init = 0x%x\n", res);

    res = pd_ain_async_start(board);
    DPRINTK_T("pd_ain_async_start = 0x%x\n", res);

    ScanInfo.NumScans = pd_board[board].AinSS.BufInfo.FrameSize;

    for (i = 0; i < 10; i++)
    {
        pd_ain_get_scans(board, &ScanInfo);
        DPRINTK_T("tst[%d]: pd_ain_get_scans: NumScans = 0x%x, ScanIndex = 0x%x, NumValidScans = 0x%x\n",
                i,
                ScanInfo.NumScans,
                ScanInfo.ScanIndex,
                ScanInfo.NumValidScans
                );
        mdelay(100);

    }

    res = pd_ain_async_stop(board);
    DPRINTK_T("pd_ain_async_stop = 0x%x\n", res);

    res = pd_ain_async_term(board);
    DPRINTK_T("pd_ain_async_term = 0x%x\n", res);

/*
    // Variables for pd_ain_async_init
    pd_board[board].AinSS.BufInfo.Count = 0;
    pd_board[board].AinSS.BufInfo.Head = 0;
    pd_board[board].AinSS.BufInfo.Tail = 0;
    pd_board[board].AinSS.BufInfo.FrameValues = pd_board[board].AinSS.BufInfo.FrameSize *
                                                pd_board[board].AinSS.BufInfo.ScanSize;
    pd_board[board].AinSS.BufInfo.MaxValues = pd_board[board].AinSS.BufInfo.FrameValues *
                                              pd_board[board].AinSS.BufInfo.NumFrames;


    // program board to perform acquisition
    res = pd_ain_reset(board);
    DPRINTK_T("tst: pd_ain_reset() returns %d\n", res);

    dwConfig = AIB_INPTYPE | AIB_INPRANGE | AIB_CVSTART0 | AIB_CLSTART0 | AIB_SELMODE0;
    res = pd_ain_set_config(board, dwConfig, 0, 0);
    DPRINTK_T("tst: pd_ain_set_config() returns %d\n", res);

    res = pd_ain_set_channel_list(board, dwCLSize, dwChList);
    DPRINTK_T("tst: pd_ain_set_channel_list() returns %d\n", res);

    res = pd_ain_set_cl_clock(board, 550);
    DPRINTK_T("tst: pd_ain_set_cl_clock() returns %d\n", res);

    res = pd_ain_set_cv_clock(board, 110);
    DPRINTK_T("tst: pd_ain_set_cv_clock() returns %d\n", res);

    pd_board[board].FwEventsConfig.AIOIntr = AIB_FHFIm | AIB_FHFSC | AIB_FFIm | AIB_FFSC;
    if (!pd_enable_events(board, &pd_board[board].FwEventsConfig)) 
    {
	    DPRINTK_T("pd_ain_async_init bails!  pd_enable_events failed!\n");
    	return;
    }

    DPRINTK_T("tst: pd_ain_set_events() with 0x%x returns %d\n", dwIntrCfg, res);
    res = pd_adapter_enable_interrupt(board, 1);

    DPRINTK_T("tst: pd_adapter_enable_interrupt() returns %d\n", res);

    res = pd_ain_set_enable_conversion(board, 1);
 
    res = pd_ain_sw_start_trigger(board);
*/

}


// test ain buffering
void test_pd_ain_buffering(int board)
{

 

}

/*
// print out event status
void pd_debug_show_events (TEvents Event, char* msg)
{
    // prints out the events received
    DPRINTK_T("pd_debug_show_events: %s\n", msg);



    // ADUIntr -----------------------------
    DPRINTK_T("--- ADUIntr --> 0x%x\n", Event.ADUIntr);
    if (Event.ADUIntr & UTB_Uct0Im)
        DPRINTK_T("ADUIntr: UTB_Uct0Im\n");
    if (Event.ADUIntr & UTB_Uct1Im)
        DPRINTK_T("ADUIntr: UTB_Uct1Im\n");
    if (Event.ADUIntr & UTB_Uct2Im)
        DPRINTK_T("ADUIntr: UTB_Uct2Im\n");
    if (Event.ADUIntr & UTB_Uct0IntrSC)
        DPRINTK_T("ADUIntr: UTB_Uct0IntrSC\n");
    if (Event.ADUIntr & UTB_Uct1IntrSC)
        DPRINTK_T("ADUIntr: UTB_Uct1IntrSC\n");
    if (Event.ADUIntr & UTB_Uct2IntrSC)
        DPRINTK_T("ADUIntr: UTB_Uct2IntrSC\n");
    if (Event.ADUIntr & DIB_IntrIm)
        DPRINTK_T("ADUIntr: DIB_IntrIm\n");
    if (Event.ADUIntr & DIB_IntrSC)
        DPRINTK_T("ADUIntr: DIB_IntrSC\n");
    if (Event.ADUIntr & BRDB_ExTrigIm)
        DPRINTK_T("ADUIntr: BRDB_ExTrigIm\n");
    if (Event.ADUIntr & BRDB_ExTrigReSC)
        DPRINTK_T("ADUIntr: BRDB_ExTrigReSC\n");
    if (Event.ADUIntr & BRDB_ExTrigFeSC)
        DPRINTK_T("ADUIntr: BRDB_ExTrigFeSC\n");
    if (Event.ADUIntr & AIB_FNE)
        DPRINTK_T("ADUIntr: AIB_FNE\n");
    if (Event.ADUIntr & AIB_FHF)
        DPRINTK_T("ADUIntr: AIB_FHF\n");
    if (Event.ADUIntr & AIB_FF)
        DPRINTK_T("ADUIntr: AIB_FF\n");
    if (Event.ADUIntr & AIB_CVDone)
        DPRINTK_T("ADUIntr: AIB_CVDone\n");
    if (Event.ADUIntr & AIB_CLDone)
        DPRINTK_T("ADUIntr: AIB_CLDone\n");
    if (Event.ADUIntr & UTB_Uct0Out)
        DPRINTK_T("ADUIntr: UTB_Uct0Out\n");
    if (Event.ADUIntr & UTB_Uct1Out)
        DPRINTK_T("ADUIntr: UTB_Uct1Out\n");
    if (Event.ADUIntr & UTB_Uct2Out)
        DPRINTK_T("ADUIntr: UTB_Uct2Out\n");
    if (Event.ADUIntr & BRDB_ExTrigLevel)
        DPRINTK_T("ADUIntr: BRDB_ExTrigLevel\n");


    // AIOIntr -----------------------------
    DPRINTK_T("--- AIOIntr --> 0x%x\n", Event.AIOIntr);
    if (Event.AIOIntr & AIB_FNEIm)
        DPRINTK_T("AIOIntr: AIB_FNEIm\n");
    if (Event.AIOIntr & AIB_FHFIm)
        DPRINTK_T("AIOIntr: AIB_FHFIm\n");
    if (Event.AIOIntr & AIB_CLDoneIm)
        DPRINTK_T("AIOIntr: AIB_CLDoneIm\n");
    if (Event.AIOIntr & AIB_FNESC)
        DPRINTK_T("AIOIntr: AIB_FNESC\n");
    if (Event.AIOIntr & AIB_FHFSC)
        DPRINTK_T("AIOIntr: AIB_FHFSC\n");
    if (Event.AIOIntr & AIB_CLDoneSC)
        DPRINTK_T("AIOIntr: AIB_CLDoneSC\n");
    if (Event.AIOIntr & AOB_CVDoneIm)
        DPRINTK_T("AIOIntr: AOB_CVDoneIm\n");
    if (Event.AIOIntr & AOB_CVDoneSC)
        DPRINTK_T("AIOIntr: AOB_CVDoneSC\n");
    if (Event.AIOIntr & AIB_FFIm)
        DPRINTK_T("AIOIntr: AIB_FFIm\n");
    if (Event.AIOIntr & AIB_CVStrtErrIm)
        DPRINTK_T("AIOIntr: AIB_CVStrtErrIm\n");
    if (Event.AIOIntr & AIB_CLStrtErrIm)
        DPRINTK_T("AIOIntr: AIB_CLStrtErrIm\n");
    if (Event.AIOIntr & AIB_OTRLowIm)
        DPRINTK_T("AIOIntr: AIB_OTRLowIm\n");
    if (Event.AIOIntr & AIB_OTRHighIm)
        DPRINTK_T("AIOIntr: AIB_OTRHighIm\n");
    if (Event.AIOIntr & AIB_FFSC)
        DPRINTK_T("AIOIntr: AIB_FFSC\n");
    if (Event.AIOIntr & AIB_CVStrtErrSC)
        DPRINTK_T("AIOIntr: AIB_CVStrtErrSC\n");
    if (Event.AIOIntr & AIB_CLStrtErrSC)
        DPRINTK_T("AIOIntr: AIB_CLStrtErrSC\n");
    if (Event.AIOIntr & AIB_OTRLowSC)
        DPRINTK_T("AIOIntr: AIB_OTRLowSC\n");
    if (Event.AIOIntr & AIB_OTRHighSC)
        DPRINTK_T("AIOIntr: AIB_OTRHighSC\n");

    // AInIntr -----------------------------
    DPRINTK_T("--- AInIntr --> 0x%x\n", Event.AInIntr);
    if (Event.AInIntr & AIB_StartIm)
        DPRINTK_T("AInIntr: AIB_StartIm\n");
    if (Event.AInIntr & AIB_StopIm)
        DPRINTK_T("AInIntr: AIB_StopIm\n");
    if (Event.AInIntr & AIB_SampleIm)
        DPRINTK_T("AInIntr: AIB_SampleIm\n");
    if (Event.AInIntr & AIB_ScanDoneIm)
        DPRINTK_T("AInIntr: AIB_ScanDoneIm\n");
    if (Event.AInIntr & AIB_ErrIm)
        DPRINTK_T("AInIntr: AIB_ErrIm\n");
    if (Event.AInIntr & AIB_BMDoneIm)
        DPRINTK_T("AInIntr: AIB_BMDoneIm\n");
    if (Event.AInIntr & AIB_BMErrIm)
        DPRINTK_T("AInIntr: AIB_BMErrIm\n");
    if (Event.AInIntr & AIB_BMEmptyIm)
        DPRINTK_T("AInIntr: AIB_BMEmptyIm\n");
    if (Event.AInIntr & AIB_StartSC)
        DPRINTK_T("AInIntr: AIB_StartSC\n");
    if (Event.AInIntr & AIB_StopSC)
        DPRINTK_T("AInIntr: AIB_StopSC\n");
    if (Event.AInIntr & AIB_SampleSC)
        DPRINTK_T("AInIntr: AIB_SampleSC\n");
    if (Event.AInIntr & AIB_ScanDoneSC)
        DPRINTK_T("AInIntr: AIB_ScanDoneSC\n");
    if (Event.AInIntr & AIB_ErrSC)
        DPRINTK_T("AInIntr: AIB_ErrSC\n");
    if (Event.AInIntr & AIB_BMDoneSC)
        DPRINTK_T("AInIntr: AIB_BMDoneSC\n");
    if (Event.AInIntr & AIB_Enabled)
        DPRINTK_T("AInIntr: AIB_Enabled\n");
    if (Event.AInIntr & AIB_Active)
        DPRINTK_T("AInIntr: AIB_Active\n");
    if (Event.AInIntr & AIB_BMEnabled)
        DPRINTK_T("AInIntr: AIB_BMEnabled\n");
    if (Event.AInIntr & AIB_BMActive)
        DPRINTK_T("AInIntr: AIB_BMActive\n");


    // AOutIntr -----------------------------
    DPRINTK_T("--- AOutIntr --> 0x%x\n", Event.AOutIntr);
    if (Event.AOutIntr & AOB_StartIm)
        DPRINTK_T("AOutIntr: AOB_StartIm\n");
    if (Event.AOutIntr & AOB_StopIm)
        DPRINTK_T("AOutIntr: AOB_StopIm\n");
    if (Event.AOutIntr & AOB_ScanDoneIm)
        DPRINTK_T("AOutIntr: AOB_ScanDoneIm\n");
    if (Event.AOutIntr & AOB_HalfDoneIm)
        DPRINTK_T("AOutIntr: AOB_HalfDoneIm\n");
    if (Event.AOutIntr & AOB_BufDoneIm)
        DPRINTK_T("AOutIntr: AOB_BufDoneIm\n");
    if (Event.AOutIntr & AOB_BlkXDoneIm)
        DPRINTK_T("AOutIntr: AOB_BlkXDoneIm\n");
    if (Event.AOutIntr & AOB_BlkYDoneIm)
        DPRINTK_T("AOutIntr: AOB_BlkYDoneIm\n");
    if (Event.AOutIntr & AOB_UndRunErrIm)
        DPRINTK_T("AOutIntr: AOB_UndRunErrIm\n");
    if (Event.AOutIntr & AOB_CVStrtErrIm)
        DPRINTK_T("AOutIntr: AOB_CVStrtErrIm\n");
    if (Event.AOutIntr & AOB_StartSC)
        DPRINTK_T("AOutIntr: AOB_StartSC\n");
    if (Event.AOutIntr & AOB_StopSC)
        DPRINTK_T("AOutIntr: AOB_StopSC\n");
    if (Event.AOutIntr & AOB_ScanDoneSC)
        DPRINTK_T("AOutIntr: AOB_ScanDoneSC\n");
    if (Event.AOutIntr & AOB_HalfDoneSC)
        DPRINTK_T("AOutIntr: AOB_HalfDoneSC\n");
    if (Event.AOutIntr & AOB_BufDoneSC)
        DPRINTK_T("AOutIntr: AOB_BufDoneSC\n");
    if (Event.AOutIntr & AOB_BlkXDoneSC)
        DPRINTK_T("AOutIntr: AOB_BlkXDoneSC\n");
    if (Event.AOutIntr & AOB_BlkYDoneSC)
        DPRINTK_T("AOutIntr: AOB_BlkYDoneSC\n");
    if (Event.AOutIntr & AOB_UndRunErrSC)
        DPRINTK_T("AOutIntr: AOB_UndRunErrSC\n");
    if (Event.AOutIntr & AOB_CVStrtErrSC)
        DPRINTK_T("AOutIntr: AOB_CVStrtErrSC\n");
    if (Event.AOutIntr & AOB_Enabled)
        DPRINTK_T("AOutIntr: AOB_Enabled\n");
    if (Event.AOutIntr & AOB_Active)
        DPRINTK_T("AOutIntr: AOB_Active\n");
    if (Event.AOutIntr & AOB_BufFull)
        DPRINTK_T("AOutIntr: AOB_BufFull\n");
    if (Event.AOutIntr & AOB_QEMPTY)
        DPRINTK_T("AOutIntr: AOB_QEMPTY\n");
    if (Event.AOutIntr & AOB_QHF)
        DPRINTK_T("AOutIntr: AOB_QHF\n");
    if (Event.AOutIntr & AOB_QFULL)
        DPRINTK_T("AOutIntr: AOB_QFULL\n");

}

*/

