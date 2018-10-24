//===========================================================================
//
// NAME:    pdl_int.c: 
//
// DESCRIPTION:
//
//          PowerDAQ Linux driver interface to interrupt processing functions
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


////////////////////////////////////////////////////////////////////////
//   
// INTERRUP PROCESSING SECTION
//
////////////////////////////////////////////////////////////////////////


//
// Function:    PdStopAndDisableAIn
//
// Parameters:  int board
//
// Returns:     VOID
//
// Description: Stop and disable AIn data acquisition operation.
//
//              Driver event eStopped is asserted.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//
void pd_stop_and_disable_ain(int board)
{
    // Stop acquisition and disable A/D conversions.
    if (!pd_ain_sw_stop_trigger(board))
        DPRINTK_F("pd_stop_and_disable_ain: cannot execute pd_ain_sw_stop_trigger\n");

    pd_board[board].AinSS.SubsysState = ssStopped;

    // Stop acquisition and disable A/D conversions.
    if (!pd_ain_set_enable_conversion(board, 0))
        DPRINTK_F("pd_stop_and_disable_ain: cannot execute pd_ain_enable_conversion\n");

    pd_board[board].AinSS.dwEventsNew |= eStopped;
    pd_board[board].AinSS.bAsyncMode = FALSE;

    DPRINTK_E("bh>pd_stop_and_disable_ain: stopped AIn.\n");
}


//
// Function:    PdProcessAInGetSamples
//
// Parameters:  int board
//              int bFHFState      -- flag: FHF state
//
// Returns:     VOID
// Returns:     NTSTATUS    -- STATUS_SUCCESS
//                          -- STATUS_UNSUCCESSFUL
//
// Description: Process AIn Get Samples to get all samples from board.
//              This function is called from a DPC to process the AIn
//              FHF hardware interrupt event by transferring data from
//              board to designated DAQ buffer.
//
//              Three types of buffer operation to handle:
//              1. Straight buffer, stop acquisiton when end is reached.
//              2. Wrapped buffer, functions like a queue, we add new samples
//                 at the Head and user removes samples from the Tail. Stop
//                 acquisiton when Head reaches Tail.
//              3. Recycled buffer, same as wrapped buffer, except that the
//                 Tail moves (by frame size increments) when buffer becomes
//                 full. (This is a very small variation from wrapped buffer).
//                 Solution is to check if less than a frame remains, recycle
//                 by incrementing ValueIndex if so, then proceed as with #2.
//
//              Three types of copy conditions to handle:
//              1. Straight buffer, copy from ValueIndex up to MaxValues.
//              2. Wrapped buffer, copy from Head up to end of buffer.
//              3. Wrapped buffer had wrapped, copy from Head up to Tail.
//
//              Driver events are updated on change of status.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//
void pd_process_pd_ain_get_samples(int board, int bFHFState)
{
   u32   Count;                  // num samples in buffer (queue)
   u32   Head;                   // queue head (wrapped buffer)
   u32   Tail;                   // queue tail (wrapped buffer)
   u32   FrameValues;            // num values in frame
   u32   MaxValues;              // max buffer samples

   u32   NumSamplesRead;         // num samples read from board
   u32   NumToCopy;              // num samples to copy to buffer
   u32   NumCopied = 0;          // num samples already copied

   int  bWrapped = FALSE;
   int  res;

   u16* pBuf = (u16*)pd_board[board].AinSS.pXferBuf;

   DPRINTK_T("bh>pd_process_pd_ain_get_samples: board %d, FHF %d\n", board, bFHFState);

   // Verify that a driver buffer has been allocated.
   if (!pBuf)
   {
      // Stop acquisition and disable A/D conversions.
      pd_stop_and_disable_ain(board);
      DPRINTK_F("bh>pd_process_pd_ain_get_samples: no driver buffer allocated\n");
      return;
   }

   // Verify that a buffer has been registered.
   if (!pd_board[board].AinSS.BufInfo.databuf)
   {
      // Stop acquisition and disable A/D conversions.
      pd_stop_and_disable_ain(board);

      DPRINTK_F("bh>pd_process_pd_ain_get_samples: no registered Daq Buffer\n");
      return;
   }

   // Set parameters.
   Count = pd_board[board].AinSS.BufInfo.Count;
   Head  = pd_board[board].AinSS.BufInfo.Head;
   Tail  = pd_board[board].AinSS.BufInfo.Tail;
   FrameValues = pd_board[board].AinSS.BufInfo.FrameValues;
   MaxValues = pd_board[board].AinSS.BufInfo.MaxValues;

   DPRINTK_T("bh>pd_process_pd_ain_get_samples(1):Count 0x%x Head 0x%x Tail 0x%x FrameValues 0x%x MaxValues 0x%x UseHeavy=%d\n",
             Count, Head, Tail, FrameValues, MaxValues, pd_board[board].bUseHeavyIsr);

   if (!pd_board[board].bUseHeavyIsr)
   {
      if ( bFHFState )
      {
         DPRINTK_E("bh>pd_process_pd_ain_get_samples: bFHFState\n");

         // Get samples acquired and stored on board using AIn Block Transfer
         // We should get 512 samples in the BURST mode (4+512+4 clocks)
         // and rest of them in GETSAMPLES mode (samples*4 clocks)
         if ( !pd_ain_flush_fifo(board, 0))
         {
            // Error: cannot execute PdAInFlushFifo.
            DPRINTK_F("bh>pd_process_pd_ain_get_samples: cannot execute pd_ain_flush_fifo\n");
            return;
         }
      }
      else
      {
         DPRINTK_E("bh>pd_process_pd_ain_get_samples: !bFHFState\n");

         // Get all samples acquired and stored on board.
         res = pd_ain_get_samples(board, 
                                  pd_board[board].AinSS.XferBufValues,
                                  (u16*)pd_board[board].AinSS.pXferBuf,
                                  &pd_board[board].AinSS.XferBufValueCount);
         if (!res)
         {
            DPRINTK_F("bh>pd_process_pd_ain_get_samples: no samples\n");
            return;
         }
      }
   }

   DPRINTK_T("bh>pd_process_pd_ain_get_samples Xfer: requested: 0x%x returned 0x%x \n",
             pd_board[board].AinSS.XferBufValues,
             pd_board[board].AinSS.XferBufValueCount);

   NumSamplesRead = pd_board[board].AinSS.XferBufValueCount;

   //Alex: Imediate update fix 
   if ( (( pd_board[board].bUseHeavyIsr ) && ( !bFHFState ))
        || pd_board[board].bImmUpdate)
   {
      // Get all samples acquired when ImediateUpdate is called
      u16* pXferBuf = (u16*)pd_board[board].AinSS.pXferBuf;

      do
      {
         pd_ain_get_samples(board, 
                            pd_board[board].AinSS.XferBufValues,
                            pXferBuf,
                            &pd_board[board].AinSS.XferBufValueCount);

         pXferBuf = pXferBuf + pd_board[board].AinSS.XferBufValueCount;
         NumSamplesRead = NumSamplesRead + pd_board[board].AinSS.XferBufValueCount;
      } while(pd_board[board].AinSS.XferBufValueCount != 0);
   }

   if ( NumSamplesRead == 0 )
   {
      DPRINTK_F("bh>pd_process_pd_ain_get_samples: no samples acquired since we last checked\n");
      return;
   }

   //-----------------------------------------------------------------------
   // Check if we need to recycle a frame past NumSamples read.
   if ( pd_board[board].AinSS.BufInfo.bRecycle )
   {
      if ( (Count + NumSamplesRead) >= MaxValues )
      {
         // Increment Tail to the next frame boundry.
         u32 NewTail = ( ((Head + NumSamplesRead + FrameValues) % MaxValues)
                         / FrameValues) * FrameValues;
         Count -= (NewTail >= Tail) ? (NewTail - Tail)                // not wrapped
                  : ((MaxValues - Tail) + NewTail); // wrapped
         Tail = NewTail;

         pd_board[board].AinSS.dwEventsNew |= eFrameRecycled;
         DPRINTK_E("bh>pd_process_pd_ain_get_samples: eFrameRecycled.\n");
      }
   }

   //-----------------------------------------------------------------------
   // Check if buffer is full.
   DPRINTK_T("bh>pd_process_pd_ain_get_samples(2):Count 0x%x Head 0x%x Tail 0x%x FrameValues 0x%x MaxValues 0x%x\n",
             Count, Head, Tail, FrameValues, MaxValues );

   if ( !pd_board[board].AinSS.BufInfo.bRecycle && (Count == MaxValues) )
   {
      // Stop acquisition and disable A/D conversions.
      // if buffer is in wrap (continuous) mode it's an error condition
      // if buffer is in single-run mode it's a normal condition
      if (pd_board[board].AinSS.BufInfo.bWrap)
         pd_board[board].AinSS.dwEventsNew |= eFrameRecycled | eBufferError;
      
      pd_stop_and_disable_ain(board);

      DPRINTK_Q("bh>pd_process_pd_ain_get_samples: Buffer Full: eStopped.\n"); //????
      return;
   }

   // Copy samples to buffer starting at Head up to end of buffer.
   // (Cases #1 and #2a).
   if ( Head >= Tail )
   {
      NumToCopy = (NumSamplesRead < (MaxValues - Head))?
                  NumSamplesRead : (MaxValues - Head);

      memcpy((pd_board[board].AinSS.BufInfo.databuf + Head),
                                       pBuf, (NumToCopy * 2) );

      /*DPRINTK_T("0:0x%x 1:0x%x 2:0x%x 3:0x%x\n", 
                *(pd_board[board].AinSS.BufInfo.databuf + Head+0),
                *(pd_board[board].AinSS.BufInfo.databuf + Head+1),            
                *(pd_board[board].AinSS.BufInfo.databuf + Head+2),
                *(pd_board[board].AinSS.BufInfo.databuf + Head+3));*/

      Count += NumToCopy;
      Head = (Head + NumToCopy) % MaxValues;
      NumCopied = NumToCopy;

      // Check if we wrapped.
      if ( (NumCopied > 0) && (Head == 0) )
      {
         if (pd_board[board].AinSS.BufInfo.bWrap || 
             pd_board[board].AinSS.BufInfo.bRecycle)
         {
            ++pd_board[board].AinSS.BufInfo.WrapCount;
            pd_board[board].AinSS.dwEventsNew |= eBufferWrapped;
            DPRINTK_E("bh>pd_process_pd_ain_get_samples: eBufferWrapped\n");
         }
         bWrapped = TRUE;
      }
   }

   DPRINTK_T("bh>pd_process_pd_ain_get_samples(3):Count 0x%x Head 0x%x Tail 0x%x FrameValues 0x%x MaxValues 0x%x\n",
             Count, Head, Tail, FrameValues, MaxValues );

   // Wrap buffer: copy samples to buffer starting at Head up to Tail
   // (Cases #2b and #3).
   if ((NumSamplesRead - NumCopied > 0) && (Head < Tail))
   {
      NumToCopy = ((NumSamplesRead - NumCopied) < (Tail - Head))?
                  (NumSamplesRead - NumCopied) : (Tail - Head);

      memcpy((pd_board[board].AinSS.BufInfo.databuf + Head),
                                       (pBuf + NumCopied), (NumToCopy * 2) );

      /*DPRINTK_T("0:0x%x 1:0x%x 2:0x%x 3:0x%x\n", 
                *(pd_board[board].AinSS.BufInfo.databuf + Head+0),
                *(pd_board[board].AinSS.BufInfo.databuf + Head+1),            
                *(pd_board[board].AinSS.BufInfo.databuf + Head+2),
                *(pd_board[board].AinSS.BufInfo.databuf + Head+3)); */

      Count += NumToCopy;

      Head += NumToCopy;          // should never wrap (these cases cannot wrap buffer)
      NumCopied += NumToCopy;
   }

   // Set buffer event flags to alert DLL.
   if ( NumCopied > 0 )
   {
      pd_board[board].AinSS.dwEventsNew |= eDataAvailable;

      // Check if we reached end of buffer.
      if ( bWrapped )
      {
         pd_board[board].AinSS.dwEventsNew |= eBufferDone;
         DPRINTK_E("bh>pd_process_pd_ain_get_samples: eBufferDone\n");
      }

      // Check if we crossed a frame boundry.
      if ( bWrapped || ((Head / pd_board[board].AinSS.BufInfo.FrameValues) > 
                        (pd_board[board].AinSS.BufInfo.Head / 
                         pd_board[board].AinSS.BufInfo.FrameValues)))
      {
         pd_board[board].AinSS.dwEventsNew |= eFrameDone;
         DPRINTK_E("bh>pd_process_pd_ain_get_samples: eFrameDone\n");
      }

      pd_board[board].AinSS.BufInfo.Count = Count; // value count
      pd_board[board].AinSS.BufInfo.Head  = Head;
      pd_board[board].AinSS.BufInfo.Tail  = Tail;
   }

   DPRINTK_T("bh>pd_process_pd_ain_get_samples(4):Count 0x%x Head 0x%x Tail 0x%x\n",
             Count, Head, Tail );

   pd_board[board].AinSS.XferBufValueCount = 0;

   // Check if buffer is full and acquistion needs to be stopped.
   if (!pd_board[board].AinSS.BufInfo.bRecycle && (Count == MaxValues))
   {
      // Stop acquisition and disable A/D conversions.
      // if buffer is in wrap (continuous) mode it's an error condition
      // if buffer is in single-run mode it's a normal condition
      if (pd_board[board].AinSS.BufInfo.bWrap)
      {
          DPRINTK_E("bh>pd_process_pd_ain_get_samples: eBufferError\n");
          pd_board[board].AinSS.dwEventsNew |= eFrameRecycled | eBufferError;
      }

      // Buffer is full: stop acquisition.
      pd_stop_and_disable_ain(board);
      DPRINTK_E("bh>pd_process_pd_ain_get_samples: eStopped: values acquired\n");
   }

}


//---------------------------------------------------------------------------
// Function:    pd_process_ain_move_samples
//
// Parameters:  int board -- pointer to device extension
//              BOOLEAN bFHFState      -- flag: FHF state
//
// Returns:     VOID
// Returns:     NTSTATUS    -- STATUS_SUCCESS
//                          -- STATUS_UNSUCCESSFUL
//
// Description: Process AIn Move Samples moves all the samples from the
//              physical bus-master page into the buffer. This function
//              cannot be used along with GetSamples and Immediate update
//
//---------------------------------------------------------------------------
void pd_process_ain_move_samples(int board, u32 page, u32 numready) 
{
    u32   Count, i;               // num samples in buffer (queue)
    u32   Head;                   // queue head (wrapped buffer)
    u32   Tail;                   // queue tail (wrapped buffer)
    u32   FrameValues;            // num values in frame
    u32   MaxValues;              // max buffer samples

    u32   NumSamplesRead;         // num samples read from board
    u32   NumToCopy;              // num samples to copy to buffer
    u32   NumCopied = 0;          // num samples already copied

    u16*  pBuf = (u16*)pd_board[board].AinSS.pXferBuf;
    //u32*  plBuf = (u32*)pd_board[board].AinSS.pXferBuf;

    BOOLEAN bWrapped = FALSE;
    BOOLEAN bLong = pd_board[board].AinSS.BufInfo.bDWValues;

    //-----------------------------------------------------------------------
    // Verify that a buffer has been registered.
    // Verify that a driver buffer has been allocated.
    if (!pBuf)
    {
        // Stop acquisition and disable A/D conversions.
        pd_stop_and_disable_ain(board);
        DPRINTK_F("bh>pd_process_ain_move_samples: no driver buffer allocated\n");
        return;
    }

    // Verify that a buffer has been registered.
    if (!pd_board[board].AinSS.BufInfo.databuf)
    {
        // Stop acquisition and disable A/D conversions.
        pd_stop_and_disable_ain(board);

        DPRINTK_F("bh>pd_process_ain_move_samples: no registered Daq Buffer\n");
        return;
    }

    // Set parameters.
    Count = pd_board[board].AinSS.BufInfo.Count;
    Head  = pd_board[board].AinSS.BufInfo.Head;
    Tail  = pd_board[board].AinSS.BufInfo.Tail;
    FrameValues = pd_board[board].AinSS.BufInfo.FrameValues;
    MaxValues = pd_board[board].AinSS.BufInfo.MaxValues;

    DPRINTK_T("bh>pd_process_ain_move_samples(1):Count 0x%x Head 0x%x Tail 0x%x FrameValues 0x%x MaxValues 0x%x\n",
             Count, Head, Tail, FrameValues, MaxValues);


    if (pd_board[board].AinSS.bImmUpdate) 
    {
        // Get all samples acquired when ImediateUpdate is called - TBI
        return; // that's for now
    }

    // OK, we got some samples
    NumSamplesRead = numready;

    //-----------------------------------------------------------------------
    // Check if we need to recycle a frame past NumSamples read.
    if (pd_board[board].AinSS.BufInfo.bRecycle)
    {
        if ((Count + NumSamplesRead) >= MaxValues)
        {
            // Increment Tail to the next frame boundry.
            u32 NewTail = ( ((Head + NumSamplesRead + FrameValues) % MaxValues)
                              / FrameValues) * FrameValues;
            Count -= (NewTail >= Tail) ? 
                     (NewTail - Tail) :              // not wrapped
                     ((MaxValues - Tail) + NewTail); // wrapped
            Tail = NewTail;

            pd_board[board].AinSS.dwEventsNew |= eFrameRecycled;
            DPRINTK_E("pd_process_ain_move_samples: eFrameRecycled.\n");
        }
    }

    DPRINTK_T("bh>pd_process_ain_move_samples(2):Count 0x%x Head 0x%x Tail 0x%x\n",
             Count, Head, Tail );


    //-----------------------------------------------------------------------
    // Check if buffer is full.
    if (!pd_board[board].AinSS.BufInfo.bRecycle && (Count == MaxValues))
    {
        // Buffer (queue) is full: stop acquisition.

        // Stop acquisition and disable A/D conversions.
        // if buffer is in wrap (continuous) mode it's an error condition
        // if buffer is in single-run mode it's a normal condition
        if (pd_board[board].AinSS.BufInfo.bWrap)
            pd_board[board].AinSS.dwEventsNew |= eFrameRecycled | eBufferError;

        pd_stop_and_disable_ain(board);

        DPRINTK_E("PdProcessAInMoveSamples: Driver Buffer Full: eStopped #1\n");
        return;
    }

    //-----------------------------------------------------------------------
    // Copy samples to buffer starting at Head up to end of buffer.
    // (Cases #1 and #2a).
    if ( Head >= Tail ) 
    {
       NumToCopy = (NumSamplesRead < (MaxValues - Head)) ? NumSamplesRead : (MaxValues - Head);
       if (!bLong)
       {
           for (i=0; i<NumToCopy; i++) 
           {
                *(pd_board[board].AinSS.BufInfo.databuf + Head + i) = 
                         (u16)*((u32*)pd_board[board].pSysBMB[page]+i);
           }
       }
       else
       {
           for (i=0; i<NumToCopy; i++) 
           {
                *((u32*)pd_board[board].AinSS.BufInfo.databuf + Head + i) = 
                         *((u32*)pd_board[board].pSysBMB[page]+i);
           }
       } 

       Count += NumToCopy;
       Head = (Head + NumToCopy) % MaxValues;
       NumCopied = NumToCopy;

       // Check if we wrapped
       if ((NumCopied > 0) && (Head == 0)) 
       {    
          if (pd_board[board].AinSS.BufInfo.bWrap || 
              pd_board[board].AinSS.BufInfo.bRecycle )
          {
              ++pd_board[board].AinSS.BufInfo.WrapCount;
              pd_board[board].AinSS.dwEventsNew |= eBufferWrapped;
              DPRINTK_E("eBufferWrapped.\n");
           }
           bWrapped = TRUE;
       }
    }

    DPRINTK_T("bh>pd_process_ain_move_samples(3):Count 0x%x Head 0x%x Tail 0x%x\n",
             Count, Head, Tail );


    //-----------------------------------------------------------------------
    // Wrap buffer: copy samples to buffer starting at Head up to Tail.
    // (Cases #2b and #3)
    if ((NumSamplesRead - NumCopied > 0) && (Head < Tail)) 
    {
       NumToCopy = ((NumSamplesRead - NumCopied) < (Tail - Head)) ?
                    (NumSamplesRead - NumCopied) : (Tail - Head);
       if (!bLong)
       {
           for (i = 0; i < NumToCopy; i++) 
           {
               *(pd_board[board].AinSS.BufInfo.databuf + Head + i) = 
                        (u16)*((u32*)pd_board[board].pSysBMB[page]+i+NumCopied);
           }
       }
       else
       {
           for (i = 0; i < NumToCopy; i++) 
           {
               *((PULONG)pd_board[board].AinSS.BufInfo.databuf + Head + i) = 
                        *((u32*)pd_board[board].pSysBMB[page]+i+NumCopied);
           }
       } 

       Count += NumToCopy;
       Head += NumToCopy;          // should never wrap (these cases cannot wrap buffer)
       NumCopied += NumToCopy;
    }

    //-----------------------------------------------------------------------
    // Set buffer event flags to alert DLL.
    if (NumCopied > 0)
    {
        pd_board[board].AinSS.dwEventsNew |= eDataAvailable;

        // Check if we reached end of buffer.
        if (bWrapped) 
        {
            pd_board[board].AinSS.dwEventsNew |= eBufferDone;
            DPRINTK_E("eBufferDone.\n");
        }

        // Check if we crossed a frame boundry.
        if (bWrapped || ((Head / pd_board[board].AinSS.BufInfo.FrameValues) > 
                         (pd_board[board].AinSS.BufInfo.Head / 
                          pd_board[board].AinSS.BufInfo.FrameValues)) )
        {
            pd_board[board].AinSS.dwEventsNew |= eFrameDone;
            DPRINTK_E("eFrameDone.\n");
        }

        pd_board[board].AinSS.BufInfo.Count = Count; // value count
        pd_board[board].AinSS.BufInfo.Head  = Head;
        pd_board[board].AinSS.BufInfo.Tail  = Tail;
    }

    DPRINTK_T("bh>pd_process_ain_move_samples(4):Count 0x%x Head 0x%x Tail 0x%x\n",
             Count, Head, Tail );


    pd_board[board].AinSS.XferBufValueCount = 0;

    //-----------------------------------------------------------------------
    // Check if buffer is full and acquistion needs to be stopped.
    if (!pd_board[board].AinSS.BufInfo.bRecycle && (Count == MaxValues)) 
    {
        // Buffer is full: stop acquisition.

        // Stop acquisition and disable A/D conversions.
        // if buffer is in wrap (continuous) mode it's an error condition
        // if buffer is in single-run mode it's a normal condition
        if (pd_board[board].AinSS.BufInfo.bWrap)
            pd_board[board].AinSS.dwEventsNew |= eFrameRecycled | eBufferError;

        pd_stop_and_disable_ain(board);

        DPRINTK_E("PdProcessAInMoveSamples: Driver Buffer Full. eStopped #2.\n");
    }
}


//---------------------------------------------------------------------------
// Function:    pd_stop_and_disable_aout
//
// Parameters:  int board
//
// Returns:     int
//
// Description: Stop and disable AOut data generation operation.
//
//              Driver event eStopped is asserted.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
void pd_stop_and_disable_aout(int board)
{
   // Stop generation and disable D/A conversions.
   if (!pd_aout_sw_stop_trigger(board))
      DPRINTK_F("pd_stop_and_disable_aout: cannot execute pd_aout_sw_stop_trigger\n");

   if (!pd_aout_set_enable_conversion(board, 0))
      DPRINTK_F("pd_stop_and_disable_aout: cannot execute pd_aout_set_enable_conversion\n");

   // eStopped is only needed in asynchronous mode
   if (pd_board[board].AoutSS.SubsysState == ssRunning)
      pd_board[board].AoutSS.dwEventsNew |= eStopped;

   pd_board[board].AoutSS.SubsysState = ssStopped;


   // switch to the standby state
   pd_board[board].AoutSS.bAsyncMode = FALSE;

   DPRINTK_E("bh>pd_stop_and_disable_aout: stopped AOut.\n");
}

//---------------------------------------------------------------------------
// Function:    pd_process_aout_put_samples
//
// Parameters:  int board -- board number
//              int bFHFState      -- flag: FHF state (1024 samples)
//
// Returns:     VOID
// Returns:     NTSTATUS    -- STATUS_SUCCESS
//                          -- STATUS_UNSUCCESSFUL
//
// Description: Process AIn Get Samples to put all needed samples to the
//               board.
//              This function is called from a DPC to process the AOut
//              FHF hardware interrupt event by transferring data from
//              board to designated DAQ buffer.
//
//              Three types of buffer operation to handle:
//              1. Straight buffer, stop output when end of buffer is reached.
//              2. Wrapped buffer, functions like a queue, user adds new samples
//                 at the Head and we move samples from the Tail to the board.
//                 Stop output when buffer becomes empty (underrun)
//              3. Recycled buffer, same as wrapped buffer, except that
//                 the driver output next chunk of data regardless either
//                 application were able to put anything into buffer or not.
//
//              Six types of buffer types to handle
//              1. PD2-MF(S) with WORD buffer
//                 We need to combine two left-alignment values into one 24-bit
//                 word dwOut = ((DWORD)wVal1 << 8)|(wVal1 >> 4)
//              2. PD2-MF(S) with DWORD buffer - output directly
//              3. PD2-AO with WORD buffer
//                 Combine value with the channel list
//                 dwOut = (ChList[i%ChListSize]<<16)|wVal
//              4. PD2-AO with DWORD buffer
//                 Output values directly, channel list is incorporated if
//                 ChListSize == 0. If not:
//                 dwOut = (ChList[i%ChListSize]<<16)| (dwVal & 0xFFFF);
//              5. PD2-AO with DWORD buffer and BUF_FIXEDDMA
//                 DMA is programmed. Use direct output.
//              6. PD2-AO with WORD buffer and BUF_FIXEDDMA
//
//              Driver events are updated on change of status.
//
// Notes:       * This routine must be called with device spinlock held! *
//
//---------------------------------------------------------------------------
void pd_process_aout_put_samples(int board, int bFHFState)
{
   PTBuf_Info pDaqBuf = &pd_board[board].AoutSS.BufInfo;

   u32   Head;                   // queue head (wrapped buffer)
   u32   Tail;                   // queue tail (wrapped buffer)
   u32   FrameValues;            // num values in frame
   u32   MaxValues;              // max buffer samples

   u32   NumToWrite;             // num samples to write to board
   u32   NumCopied = 0;          // num samples already copied

   u32*  pBuf = pd_board[board].AoutSS.pXferBuf;
   

   //-----------------------------------------------------------------------
   // Check whether we already finished putting blocks. Stop AOut
   // Note: PdProcessAOutPutSamples is called upon each AOut interrupt
   //       When we put the last block we set up
   //       pAdapter->AoutSubsystem.bAsyncMode = FALSE
   //       if on the next interrupt we see it FALSE then stop AOut
   if (pd_board[board].AoutSS.bAsyncMode == FALSE)
   {
      // stop right now!
      pd_stop_and_disable_aout(board);
      DPRINTK_F("bh>pd_process_aout_put_samples: stopped due to buffer end\n");
      return;
   }

   //-----------------------------------------------------------------------
   // Verify that a buffer has been registered.
   if (!pd_board[board].AoutSS.BufInfo.databuf)
   {
      // Stop generation and disable D/A conversions.
      pd_stop_and_disable_aout(board);
      DPRINTK_F("bh>pd_process_aout_put_samples: no registered Daq Buffer\n");
      return;
   }

   if (!pBuf)
   {
      // Stop generation and disable D/A conversions.
      pd_stop_and_disable_aout(board);
      DPRINTK_F("bh>pd_process_aout_put_samples: Daq Buffer pointer is null\n");
      return;
   }


   // Set parameters.
   Head  = pDaqBuf->Head;
   Tail  = pDaqBuf->Tail;
   FrameValues = pDaqBuf->FrameValues;
   MaxValues = pDaqBuf->MaxValues;

   // Straight buffer - stop if we copied enough
   if ((!pDaqBuf->bRecycle)&&(!pDaqBuf->bWrap)) // neither wrapped nor recycled
   {
      DPRINTK_F("bh>pd_process_aout_put_samples: count=%d, max=%d\n", pDaqBuf->Count, pDaqBuf->MaxValues);
      if (pDaqBuf->Count >= pDaqBuf->MaxValues)
      {
         pd_board[board].AoutSS.dwEventsNew |= eBufferDone;
         DPRINTK_F("bh>pd_process_aout_put_samples: Straight buffer is done\n");
         return;
      }
   }

   // Wrapped buffer - head reached the tail - nothing to output
   if ((!pDaqBuf->bRecycle)&&(pDaqBuf->bWrap)) // wrapped but not recycled
   {
      if ((pDaqBuf->Head == pDaqBuf->Tail)) //&&(pDaqBuf->Tail != 0))
      {
         //PdStopAndDisableAOut(pAdapter);
         pd_board[board].AoutSS.bAsyncMode = FALSE;
         pd_board[board].AoutSS.dwEventsNew |= eBufferError;
         DPRINTK_F("bh>pd_process_aout_put_samples: Nothing to output\n");
         return;
      }
   }

   // Now - output stuff
   NumToWrite = pd_board[board].AoutSS.TranSize/2;  // thru half a FIFO - we shall have it

   if (!(pd_board[board].AoutSS.dwEventsNew & eStopped))
   {
      if ( !pd_aout_put_xbuf(board, NumToWrite, &NumCopied) )
      {
         // Error: cannot execute PdAOutPutXBuf.
         DPRINTK_F("bh>pd_process_aout_put_samples: cannot execute PdAOutPutXBuf.\n");
      }
   }

   // Check, if we cross buffer boundaries (old head > new head - wrapped)
   //dprintf(("Head=0x%x pDaqBuf->Head=0x%x", Head, pDaqBuf->Head));
   if (Head > pDaqBuf->Head)
   {
      pd_board[board].AoutSS.dwEventsNew |= eBufferDone;
      pd_board[board].AoutSS.dwEventsNew |= eFrameDone;   // buffer_size = N * frame_size
      DPRINTK_F("bh>pd_process_aout_put_samples: eBD+eFD\n");
   }
   else
   {    // Check, if we cross frame boundaries
      if ( (pDaqBuf->Head / pDaqBuf->FrameValues) > (Head / pDaqBuf->FrameValues) )
      {
         pd_board[board].AoutSS.dwEventsNew |= eFrameDone;
         DPRINTK_F("bh>pd_process_aout_put_samples: eFD\n");
      }
   }
}


//
// Function:    PdProcessDriverEvents
//
// Parameters:  int board
//              pEvents     -- asserted board events struct
//
// Returns:     VOID
//
// Description: Process board events interrupt. This function is called
//              from a DPC to initiate processing of all driver handled
//              board events and re-enable the processed events.
//
// Notes:       * This routine must be called with device spinlock held! *
//
void pd_process_driver_events(int board, tEvents* pEvents)
{
   tEvents ClearEvents = {0};
   ULONG bAOPutData = FALSE;

   DPRINTK("bh>pd_process_driver_events: Entering\n");

   if ((pd_board[board].dwXFerMode == XFERMODE_BM) || 
      (pd_board[board].dwXFerMode == XFERMODE_BM8WORD)) 
   {
       // check for non-recoverable errors
       if (pEvents->AInIntr & AIB_BMErrSC) 
       {
          DPRINTK("bh>pd_process_driver_events: BM Error\n");
          pd_stop_and_disable_ain(board);
          ClearEvents.AInIntr |= AIB_BMErrSC;
          pd_board[board].AinSS.dwEventsNew |= eStopped | eBufferError;
       } 
       else // is there new page of data available?
       if ((pEvents->AInIntr & AIB_BMPg0DoneSC)||(pEvents->AInIntr & AIB_BMPg1DoneSC)) 
       {
          // reset current events
          DPRINTK_E("Ev=0x%x\n", pEvents->AInIntr);
          ClearEvents.AInIntr |= (pEvents->AInIntr & (AIB_BMPgDoneIm | 
                                                      AIB_BMErrIm |
                                                      AIB_StartIm | 
                                                      AIB_StopIm | 
                                                      AIB_BMEnabled | 
                                                      AIB_BMActive));
          if (pEvents->AInIntr & AIB_BMPg0DoneSC) 
          {
             ClearEvents.AInIntr |= AIB_BMPg0DoneSC;
             pd_board[board].cp = AI_PAGE0;
          } 
          else if (pEvents->AInIntr & AIB_BMPg1DoneSC)
          {
             ClearEvents.AInIntr |= AIB_BMPg1DoneSC;
             pd_board[board].cp = AI_PAGE1;
          }
          
          DPRINTK_N("bh>pd_process_driver_events: Page%d is done\n", pd_board[board].cp);
          pd_process_ain_move_samples(board, pd_board[board].cp, pd_board[board].AinSS.BmPageXFers*pd_board[board].AinSS.AIBMTXSize);
       }
       else
       {
          DPRINTK("bh>pd_process_driver_events: Not a BM event\n");
       }
   }
   else
   {
      // Check AIn FF hardware interrupt event.
      if (pd_board[board].AinSS.bCheckFifoError &&
          ((pEvents->AIOIntr & AIB_FFSC) ||(pEvents->ADUIntr & AIB_FF)))
      {
         // Process AIn Fifo Full (Overrun Error) hardware interrupt event.
         //if ( pd_board[board].AinSS.dwChListChan > 1 )
         {
            // Stop acquisition and disable A/D conversions.
            pd_stop_and_disable_ain(board);
            pd_board[board].AinSS.dwEventsNew |= eStopped | eBufferError;
         }
   
         //TODO: MORE WORK HERE:
         // Apparently add restart flag and if it's set:
         // stop ; clear FIFO; adjust buffer; start 
   
         if ( !(pd_board[board].AinSS.dwEventsNew & eStopped) )
         {
            ClearEvents.AIOIntr |= AIB_FFSC;
            pd_board[board].AinSS.dwEventsNew |= eBufferError;
         }
         DPRINTK_E("bh>pd_process_driver_events: AIB_FF\n");
      }
   
      //--------------------------------------------------------------------
      // AIn
      // Check AIn FHF hardware interrupt event.
      DPRINTK_T("bh|AIB_FHF:%ld AIB_FHFSC:%ld\n",(pEvents->ADUIntr & AIB_FHF),(pEvents->AIOIntr & AIB_FHFSC));
   
      if (pd_board[board].AinSS.bCheckHalfDone)
      {
         if((pEvents->ADUIntr & AIB_FHF) || (pEvents->AIOIntr & AIB_FHFSC))
         {
            // Process AIn Fifo Half Full hardware interrupt event.
            pd_process_pd_ain_get_samples(board, TRUE);
      
            if ( !(pd_board[board].AinSS.dwEventsNew & eStopped) )
               ClearEvents.AIOIntr |= AIB_FHFSC;
         }
         else
         {
            // Process any samples acquired upto this point.
            pd_process_pd_ain_get_samples(board, FALSE);
         }
      }
   }


   //--------------------------------------------------------------------
   // AOut
   // Check AOut Underrun Error hardware interrupt event.
   pd_board[board].AoutSS.dwEventsNew = 0;

   if (!pd_board[board].AoutSS.bRev3Mode) // OLD WAY
   {
      // Check AOut Underrun Error hardware interrupt event.
      if ( pd_board[board].AoutSS.bCheckFifoError && (pEvents->AOutIntr & AOB_UndRunErrSC) )
      {
         DPRINTK_E("pd_process_driver_events: AOB_UndRunErr\n");

         // Process AOut Underrun Error hardware interrupt event.
         if (!(pd_board[board].AoutSS.dwEventsNew & eStopped))
            ClearEvents.AOutIntr |= AOB_UndRunErrSC;

            pd_board[board].AoutSS.dwEventsNew |= eBufferError;
      }

      if (pEvents->AOutIntr & AOB_HalfDoneSC) //AI90819
      {
         DPRINTK_E("pd_process_driver_events: AOB_HalfDone\n");

         // Process AOut Half Done hardware interrupt event.
         if (!(pd_board[board].AoutSS.dwEventsNew & eFrameDone))
            ClearEvents.AOutIntr |= AOB_HalfDoneSC;

         pd_board[board].AoutSS.dwEventsNew |= eFrameDone;
      }

      // Check AOut Buffer done hardware interrupt event
      if (pEvents->AOutIntr & AOB_BufDoneSC) //AI90819
      {
         DPRINTK_E("pd_process_driver_events: AOB_BufDone\n");

         // Process AOut Buffer Done hardware interrupt event.
         if (!(pd_board[board].AoutSS.dwEventsNew & eBufferDone))
            ClearEvents.AOutIntr |= AOB_BufDoneSC;

         pd_board[board].AoutSS.dwEventsNew |= eBufferDone;
      }
   } 
   else
   {
      // Check AOut Start Trigger interrupt event
      if (pEvents->AOutIntr & AOB_StartSC) 
      {
         if (pEvents->AOutIntr & AOB_StartIm) 
         {
            ClearEvents.AOutIntr |= AOB_StartSC;
            pd_board[board].AoutSS.dwEventsNew |= eStartTrig;
         }
      }

      // Check AOut Stop Trigger interrupt event
      if (pEvents->AOutIntr & AOB_StopSC) 
      {
         if (pEvents->AOutIntr & AOB_StopIm) 
         {
            ClearEvents.AOutIntr |= AOB_StopSC;
            pd_board[board].AoutSS.dwEventsNew |= eStopTrig;
         }
      }

      if (pEvents->AOutIntr & AOB_HalfDoneSC)
      {
         DPRINTK_E("bh>pd_process_driver_events: AOB_HalfDoneSC\n");
         ClearEvents.AOutIntr |= AOB_HalfDoneSC;
   
         // output analog output event
         if (pd_board[board].AoutSS.SubsysState == ssRunning)
         {
            pd_process_aout_put_samples(board, TRUE);
         }
         else
         {
            pd_board[board].AoutSS.dwEventsNew |= eFrameDone;
            DPRINTK_E("bh>pd_process_driver_events: eFrameDone. You shouldn't see it in Async mode\n");
         }
      }
   
      // Check AOut Buffer done hardware interrupt event
      if (!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE))
      {
         if ( pEvents->AOutIntr & AOB_BufDoneSC)
         {
            DPRINTK_E("bh>pd_process_driver_events: AOB_BufDoneSC\n");
            ClearEvents.AOutIntr |= AOB_BufDoneSC;
   
            // if buffer is empty the eBufferError is certainly happens in async mode
   
            if (pd_board[board].AoutSS.SubsysState == ssRunning)
            {
               pd_process_aout_put_samples(board, TRUE);
               if (pd_board[board].AoutSS.bCheckFifoError)
                  pd_board[board].AoutSS.dwEventsNew |= eBufferError;
            }
            else
            {
               pd_board[board].AoutSS.dwEventsNew |= eBufferError | eBufferDone;
            }
         }
      } // AOB_REGENERATE
   
      // Check AOut Underrun Error hardware interrupt event - underrun error check
      // should be enabled to stop driver
      // in any case buffer underrun should be notices by user
      if (!(pd_board[board].AoutSS.SubsysConfig & AOB_REGENERATE))
      {
         if (pEvents->AOutIntr & AOB_UndRunErrSC)
         {
            DPRINTK_E("bh>pd_process_driver_events: AOB_UndRunErrSC\n");
            if (pd_board[board].AoutSS.bAsyncMode)
               ClearEvents.AOutIntr |= AOB_UndRunErrSC;
   
            // if we care about DAC FIFO errors - stop output. StopAndDisable
            // sets eStopped event itself
            //if ((pd_board[board].AoutSS.bCheckFifoError)&&
            //    (pd_board[board].AoutSS.SubsysState != ssRunning))
            {
               // inform that underrun happens: if we're in single-shot mode
               pd_board[board].AoutSS.dwEventsNew |= eBufferError;
               pd_stop_and_disable_aout(board);
               bAOPutData = TRUE;
               DPRINTK_E("bh>pd_process_driver_events: underrun!\n");
            }
         }
      } // AOB_REGENERATE
   }

   //--------------------------------------------------------------------
   // DIn
   // Check digital input hardware interrupt event
   if (pEvents->ADUIntr & DIB_IntrSC) //AI90821
   {
      // Process DIn hardware interrupt event.
      if ( !(pd_board[board].DinSS.dwEventsNew & eDInEvent) )
         ClearEvents.ADUIntr |= DIB_IntrSC;

      pd_board[board].DinSS.dwEventsNew |= eDInEvent;

      DPRINTK_E("bh>pd_process_driver_events: eDinEvent!\n");
   }

   //-----------------------------------------------------------------------
   //ScanDone event for non-buffered acquisition
   //
   
   if (pEvents->AInIntr & AIB_ScanDoneSC)  //AI 20030918
   {
	  DPRINTK_E("PdProcessDriverEvents:AIB_ScanDoneSC\n");

      if(!(pd_board[board].AinSS.dwEventsNew & eScanDone))
	     ClearEvents.AInIntr |= AIB_ScanDoneSC;

	  pd_board[board].AinSS.dwEventsNew |= eScanDone; 
   }


   //--------------------------------------------------------------------
   // UCT
   // Check UCT countdown hardware interrupt event
   if (pEvents->ADUIntr & (UTB_Uct0IntrSC|UTB_Uct1IntrSC|UTB_Uct2IntrSC))
   {
      DPRINTK_E("bh>PdProcessDriverEvents: UTB_UctXIntrSC\n");

      // Process Uct hardware interrupt event.
      if ( !(pd_board[board].UctSS.dwEventsNew & eUct0Event) )
      {

         if (pEvents->ADUIntr & UTB_Uct0IntrSC)
         {
            ClearEvents.ADUIntr |= UTB_Uct0IntrSC;
            pd_board[board].UctSS.dwEventsNew |= eUct0Event;
         }
      }

      if ( !(pd_board[board].UctSS.dwEventsNew & eUct1Event) )
      {
         if (pEvents->ADUIntr & UTB_Uct1IntrSC)
         {
            ClearEvents.ADUIntr |= UTB_Uct1IntrSC;
            pd_board[board].UctSS.dwEventsNew |= eUct1Event;
         }
      }

      if ( !(pd_board[board].UctSS.dwEventsNew & eUct2Event) )
      {
         if (pEvents->ADUIntr & UTB_Uct2IntrSC)
         {
            ClearEvents.ADUIntr |= UTB_Uct2IntrSC;
            pd_board[board].UctSS.dwEventsNew |= eUct2Event;
         }
      }
   }

   // re-enable events 
   pd_enable_events(board, &ClearEvents);
}


//
// Function:    PdNotifyUserEvents
//
// Parameters:  int board
//              PPD_EVENTS pEvents     -- asserted board events struct
//
// Returns:     BOOLEAN status  -- TRUE:  there are user events to report
//                                 FALSE: no events to report
//
// Description: Notify DLL of user events. This function is called from
//              a DPC to notify DLL of selected user events.
//
//              User events are not automatically re-enabled. Clearing
//              and thus re-enabling of user events is initiated by DLL.
//
//              The algorithm reports events to DLL/User for which the
//              notification bit is set and the new event bit is set.
//              The event notification bits are cleared for which events
//              asserted and the driver event status bits are cleared.
//
// Notes:       This function is called for each hardware interrupt,
//              therefore, we need to be as efficient as possible here.
//
//              * This routine must be called with device spinlock held! *
//
//
int pd_notify_user_events(int board, tEvents* pNewFwEvents)
{
   tEvents* pFwEventsNotify = &(pd_board[board].FwEventsNotify);
   int bNotifyUser = 0;

   // Check if there are any hardware interrupt events to report.
   if (  (pFwEventsNotify->ADUIntr  & pNewFwEvents->ADUIntr)
         | (pFwEventsNotify->AIOIntr  & pNewFwEvents->AIOIntr)
         | (pFwEventsNotify->AInIntr  & pNewFwEvents->AInIntr)
         | (pFwEventsNotify->AOutIntr & pNewFwEvents->AOutIntr) )
   {
      // Report hardware interrupt event to pending user IRP.
      bNotifyUser = TRUE;

      // Clear notification of asserted hardware interrupt events.
      pFwEventsNotify->ADUIntr  &= ~pNewFwEvents->ADUIntr;
      pFwEventsNotify->AIOIntr  &= ~pNewFwEvents->AIOIntr;
      pFwEventsNotify->AInIntr  &= ~pNewFwEvents->AInIntr;
      pFwEventsNotify->AOutIntr &= ~pNewFwEvents->AOutIntr;
   }

   //----------------------------------------------------------------------------
   // AIn
   // Check if there are any new AIn Driver generated events to report.
   if ( pd_board[board].AinSS.dwEventsNotify & pd_board[board].AinSS.dwEventsNew )
   {
      // Report AIn Driver generated events.
      bNotifyUser = TRUE;
      pd_notify_event(board, AnalogIn, pd_board[board].AinSS.dwEventsNew);

      // Clear notification of asserted AIn Driver events.
      pd_board[board].AinSS.dwEventsNotify &= ~pd_board[board].AinSS.dwEventsNew;
      pd_board[board].AinSS.dwEventsStatus |= pd_board[board].AinSS.dwEventsNew;
      pd_board[board].AinSS.dwEventsNew = 0;
   }

   DPRINTK_E("bh>pd_notify_user_events AI: dwEventsNotify: 0x%x dwEventsStatus: 0x%x dwEventsNew: 0x%x\n", 
             pd_board[board].AinSS.dwEventsNotify,
             pd_board[board].AinSS.dwEventsStatus,
             pd_board[board].AinSS.dwEventsNew);

   //----------------------------------------------------------------------------
   // AOut
   // Check if there are any new AOut Driver generated events to report.
   if ( pd_board[board].AoutSS.dwEventsNotify & pd_board[board].AoutSS.dwEventsNew )
   {
      // Report AOut Driver generated events.
      bNotifyUser = TRUE;
      pd_notify_event(board, AnalogOut, pd_board[board].AoutSS.dwEventsNew);

      // Clear notification of asserted AOut Driver events.
      pd_board[board].AoutSS.dwEventsNotify &= ~pd_board[board].AoutSS.dwEventsNew;
      pd_board[board].AoutSS.dwEventsStatus |= pd_board[board].AoutSS.dwEventsNew;
      pd_board[board].AoutSS.dwEventsNew = 0;
   }

   DPRINTK_E("bh>pd_notify_user_events AO: dwEventsNotify: 0x%x dwEventsStatus: 0x%x dwEventsNew: 0x%x\n", 
             pd_board[board].AoutSS.dwEventsNotify,
             pd_board[board].AoutSS.dwEventsStatus,
             pd_board[board].AoutSS.dwEventsNew
            );


   //----------------------------------------------------------------------------
   // DIn
   // Check if there are any DIn Driver generated events to report.
   if ( pd_board[board].DinSS.dwEventsNotify & pd_board[board].DinSS.dwEventsNew )
   {
      // Report DIn Driver generated events.
      bNotifyUser = TRUE;
      pd_notify_event(board, DigitalIn, pd_board[board].DinSS.dwEventsNew);

      // Clear notification of asserted DIn Driver events.
      pd_board[board].DinSS.dwEventsNotify &= ~pd_board[board].DinSS.dwEventsNew;
      pd_board[board].DinSS.dwEventsStatus |= pd_board[board].DinSS.dwEventsNew;
      pd_board[board].DinSS.dwEventsNew = 0;
   }

   DPRINTK_E("bh>pd_notify_user_events DI: dwEventsNotify: 0x%x dwEventsStatus: 0x%x\n", 
             pd_board[board].DinSS.dwEventsNotify,
             pd_board[board].DinSS.dwEventsStatus
            );

   
   //----------------------------------------------------------------------------
   // UCT
   // Check if there are any UCT Driver generated events to report.
   if ( pd_board[board].UctSS.dwEventsNotify & pd_board[board].UctSS.dwEventsNew )
   {
      // Report UCT Driver generated events.
      bNotifyUser = TRUE;
      pd_notify_event(board, CounterTimer, pd_board[board].UctSS.dwEventsNew);

      // Clear notification of asserted UCT Driver events.
      pd_board[board].UctSS.dwEventsNotify &= ~pd_board[board].UctSS.dwEventsNew;
      pd_board[board].UctSS.dwEventsStatus |= pd_board[board].UctSS.dwEventsNew;
      pd_board[board].UctSS.dwEventsNew = 0;
   }

   DPRINTK_E("bh>pd_notify_user_events UCT: dwEventsNotify: 0x%x dwEventsStatus: 0x%x\n", 
             pd_board[board].UctSS.dwEventsNotify,
             pd_board[board].UctSS.dwEventsStatus
            );

   DPRINTK_E("bh>pd_notify_user_events: bNotifyUser = %d\n", bNotifyUser);

   return bNotifyUser;
}


//
//
//
// Function:    PdProcessEvents
//
// Parameters:  int board
//
// Returns:     VOID
//
// Description: Process interrupt events. This function is called from
//              a DPC to get all board events, initiate processing of
//              driver handled events, notify DLL of user events, and
//              finally re-enable adapter interrupt.
//
void pd_process_events(int board)
{
   DPRINTK_E("bh>pd_process_events: get it\n");

   // Get board status if not already done so in ISR.
   if ((!pd_board[board].bUseHeavyIsr) ||
       (pd_board[board].AinSS.bImmUpdate ) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM) ||
       (pd_board[board].dwXFerMode == XFERMODE_BM8WORD))
   {
      tEvents EventsNew;

      if(!pd_adapter_get_board_status(board, &EventsNew))
         return;

      memcpy(&pd_board[board].FwEventsStatus, &EventsNew, sizeof(tEvents));
   }

#ifdef PD_DEBUG_T
   pd_debug_show_events( &pd_board[board].FwEventsStatus, "bh>events from bottom half");
#endif

   // Process driver handled events.
   pd_process_driver_events(board, &pd_board[board].FwEventsStatus);

   DPRINTK_S("bh>pd_process_events: AIOIntr: 0x%x\n", pd_board[board].FwEventsStatus.AIOIntr);

   if (pd_notify_user_events(board, &pd_board[board].FwEventsStatus))
   {
#if !(defined(_PD_RTL) || defined(_PD_RTLPRO) || defined(_PD_RTAI) || defined(_PD_XENOMAI))
      // if any process has registered for asynchronous notification,
      // notify them
      if (pd_board[board].fasync)
      {
         pd_kill_fasync(pd_board[board].fasync, SIGIO);
         DPRINTK_S("bh>pd_notify_user_events\n");
      }
#endif
   }
}





