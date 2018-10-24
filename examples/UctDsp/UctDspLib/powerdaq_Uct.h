typedef enum _dspUctSrc
{
   Clock,   // Use internal clock at 33MHz
   Clock2,  // Use internal clock / 2 
   CT0,    // Use timer  0
   CT1,    // Use timer  1
   CT2     // Use timer  2
} tDspUctSrc;

typedef enum _dspUctMode
{
   GenSinglePulse,
   GenPulseTrain,
   CountEvent,
   MeasPulseWidth,
   MeasPeriod
} tDspUctMode;


// Configure the selected counter to perform one of the counting
// timing operation
// 
// GenSinglePulse 
// GenPulseTrain
// CountEvent
// MeasPulseWidth
// MeasPeriod
// Capture
//
// set reload to 1 to automatically rearm the counter/timer
//
// set inverted to 1 to invert the polarity of the input/output
//
// Set the timebase divider
//
// For Capture, GenSinglePulse, GenPulseTrain, MeasPulseWidth and Measperiod
// it is used to divide the 33MHz internal clock

// For CountEvent
// it is used to divide the signal that is being counted
//
// it can be any value from 1 to 2^21-1
// the divider is shared by all counters
//
// ticks1 and ticks load parameters in the counter/timer
//
// GenSinglePulse: ticks1 is the delay before the pulse, ticks2 is the width of the pulse
// GenPulseTrain: ticks1 and ticks2 define the duty cycle of the pulse train
// CountEvent: ticks1 defines the number of event after which the counter will generate an event
// MeasPulseWidth: N/A
// MeasPeriod: N/A
// Capture: N/A
int PdDspUctConfig(int handle, 
                   unsigned long timerNb,  // id of the tiemr to configure
                   tDspUctSrc timerSrc, // clock source
                   tDspUctMode mode,
                   unsigned long divider,
                   unsigned long ticks1,
                   unsigned long ticks2,
                   unsigned long inverted);

// Start the operation configured with PdDspUctConfig on the specified
// counter/timer.
int PdDspUctStart(int handle, unsigned long timerNb);

// Configure the counter/timer to generate an interrupt 
// when the counter overflow or the configured operation
// is complete.
//int PdDspUctSetEvent(int handle, unsigned long events);
int PdDspUctWaitForEvent(int handle, int timerNb, tDspUctMode mode, int timeout);

// Reads counter value for the following operating modes:
// CountEvent, MeasPulseWidth, MeasPeriod, Capture
int PdDspUctRead(int handle, unsigned long timerNb, unsigned long *count);

// Stop the current operation and reset the counter/timer
int PdDspUctStop(int handle, unsigned long timerNb);


