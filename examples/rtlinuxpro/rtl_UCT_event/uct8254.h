#ifndef __SCAN_UCT_H__
#define __SCAN_UCT_H__


typedef enum _clockSource
{
   clkInternal1MHz,
   clkExternal,
   clkSoftware,
   clkUct0Out
} tClockSource;

typedef enum _gateSource
{
   gateSoftware,
   gateHardware   
} tGateSource;

typedef enum _counterMode
{
   EventCounting,
   SinglePulse,
   PulseTrain,
   SquareWave,
   swTriggeredStrobe,
   hwTriggeredStrobe   
} tCounterMode;


typedef enum _state
{
   closed,
   unconfigured,
   configured,
   running
} tState;


typedef struct _uctGenSquareWaveData
{
   int board;                    // board number to be used for the AI operation
   int counter;                  // counter to be used
   tClockSource clockSource;
   tGateSource gateSource;
   tCounterMode mode;
   tState state;                 // state of the acquisition session
} tUctGenSquareWaveData;


int InitUctGenSquareWave(tUctGenSquareWaveData *pUctData);
int StartUctGenSquareWave(tUctGenSquareWaveData *pUctData, double frequency);
int StopUctGenSquareWave(tUctGenSquareWaveData *pUctData);
int CleanUpUctGenSquareWave(tUctGenSquareWaveData *pUctData);


#endif

