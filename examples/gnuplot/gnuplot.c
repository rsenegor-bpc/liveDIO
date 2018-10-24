#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "gnuplot.h"

FILE* GnuPlotOpen()
{
   /* Start gnuplot */
   FILE* gnuplot=popen("gnuplot","w");
   if(!gnuplot) 
   {
      fprintf(stderr, "Could not start Gnuplot: %s\n",  strerror(errno));
      return NULL;
   }

   fprintf(gnuplot,"\n");
   fprintf(gnuplot,"set terminal x11\n");
   fprintf(gnuplot,"set nokey\n");
   fprintf(gnuplot,"set autoscale x\n");
   fprintf(gnuplot,"set autoscale y\n"); 

   return gnuplot;
}

int GnuPlot(FILE* gnuplot, float x0, float dX, double *buffer, int numChannels, int numScans)
{
   int i, j; 

   fprintf(gnuplot, "plot ");
   for(i=0; i<numChannels-1; i++)
      fprintf(gnuplot, "'-' using 1:2 title 'channel %d' with lines,", i);
   fprintf(gnuplot, "'-' using 1:2 title 'channel %d' with lines\n", numChannels-1);

   for(j=0; j<numChannels; j++)
   {
      for(i=0; i<numScans; i++)
      {
         fprintf(gnuplot, "%f", x0 + i * dX);    
         {
            fprintf(gnuplot, " %f", buffer[i*numChannels + j]);
         }
   
         fprintf(gnuplot, "\n");
      }

      fprintf(gnuplot, "e\n");
   }
   fflush(gnuplot);

   return 0;
}

void GnuPlotClose(FILE* gnuplot)
{
   fclose(gnuplot);
}

int sendDataToGnuPlot(int handle, float x0, float dX, double *buffer, int nbOfChannels, int nbScans)
{
   FILE *dataFp = NULL;
   FILE *gpcmdFp = NULL;
   char dataFileName[128];
   char gpcmdFileName[128];
   char *argv[3];
   int fork_result = 0;
   int i, j;

   fork_result = fork();
   if (fork_result == -1)
   {
      printf("GnuPlot: fork failed\n");
      exit(EXIT_FAILURE);
   }

   if (fork_result != 0)  /* we are in the parent process */
   {
      /* wait for the child process that runs gnuplot to end */
      waitpid(fork_result, NULL, 0);
      return 0;
   }

   /* we are in the child process */
   /* close driver handle that was duplicated by the fork */
   if(handle >= 0)
   {
      close(handle);
   }

   /* create temporary file with data to display in Gnuplot */
   sprintf(dataFileName, "./powerdaq%d.dat", handle);
   dataFp = fopen(dataFileName, "w");
   if(dataFp == NULL)
   {
      printf("Can't open temp file\n");
      return -1;
   }

   for(i=0; i<nbScans; i++)
   {
      fprintf(dataFp, "%f", x0 + i*dX);

      for(j=0; j<nbOfChannels; j++)
      {
         fprintf(dataFp, " %f", buffer[i*nbOfChannels + j]);
      }

      fprintf(dataFp, "\n");
   }

   fclose(dataFp);

   /* call Gnuplot to display our datas */
   sprintf(gpcmdFileName, "./gnuplot%d.cmd", handle);
   gpcmdFp = fopen(gpcmdFileName, "w");
   if(gpcmdFp == NULL)
   {
      printf("Can't open GnuPlot temp file\n");
      return -1;
   }

   fprintf(gpcmdFp, "plot ");
   for(i=0; i<nbOfChannels-1; i++)
      fprintf(gpcmdFp, "'%s' using 1:%d title 'channel %d' with lines,", dataFileName, i+2, i);
   fprintf(gpcmdFp, "'%s' using 1:%d title 'channel %d' with lines\n", dataFileName, nbOfChannels+1, nbOfChannels-1);
   fprintf(gpcmdFp, "pause 5 'Hit Return to continue'\n");

   fclose(gpcmdFp);

   argv[0] = "gnuplot";
   argv[1] = gpcmdFileName;
   argv[2] = NULL;
   if(execvp("gnuplot", argv)<0)
   {
       printf("Can't start gnuplot, error %d\n", errno);
   }

   _exit(0);
}

