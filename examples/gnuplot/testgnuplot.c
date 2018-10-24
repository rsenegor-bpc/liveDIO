#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include "gnuplot.h"

#define NB_SCANS 2000

int bAbort = 0;

void SigInt(int signum)
{
   if(signum == SIGINT)
   {
      printf("CTRL+C detected, stopping acquisition\n");
      bAbort = 1;
   }
}


int main(int argc, char* argv[])
{
   int i, j;
   int nbCycles;
   double data[NB_SCANS*2];

   signal(SIGINT, SigInt);

   if(initGnuPlot() < 0)
      fprintf(stderr, "Failed initializing gnuplot");

   j=0;
   nbCycles = 0;
   while(!bAbort)
   {
      nbCycles = j++ % 10;

      for(i=0; i<NB_SCANS; i++)
      {
         data[i*2] = 10.0 * sin(3.14 * 2 * nbCycles * i/NB_SCANS);
         data[i*2 + 1] = nbCycles * cos(3.14 * 2 * nbCycles * i/NB_SCANS);
      }

      printf("plot %d\n", j);

      sendDataToGnuPlot(0, 0.0, 0.1, data, 2, NB_SCANS);
   }

   closeGnuPlot();

   return 0;

}
