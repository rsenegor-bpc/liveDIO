#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "gnuplot.h"
#include "StreamFile.h"

struct subsystemName
{
   char *name;
   unsigned short id;
};
struct subsystemName ssNames[] =
{ 
   { "AnalogInput", AI },
   { "AnalogOutput", AO },
   { "DigitalInput", DI },
   { "DigitalOutput", DO },
   { "CountergInput", CI },
   { "CounterOutput", CO },
   { NULL, 0}
};


int getSsIndex(unsigned short id)
{
   int i;
   for(i=0; ssNames[i].id != 0; i++)
   {
      if(ssNames[i].id == id)
         return i;
   }

   return -1;
}


int main(int argc, char *argv[])
{
   char s[256];
   FILE *fp = NULL;
   FILE *gnuplot;
   tStreamFileHeader hdr;
   double *buffer;
   int numScans, fileSize;
   int k=0;

   if(argc < 2)
   {
      printf("Usage: ReadStreamFile <stream file name>\n");
      exit(EXIT_FAILURE);
   }

   fp = fopen(argv[1], "rb");
   if(fp == NULL)
   {
      fprintf(stderr, "Error opening data file %s: %s\n", argv[1], strerror(errno));
      exit(EXIT_FAILURE);
   }

   // get file size and calculate total number of scans stored in the file
   fseek (fp , 0 , SEEK_END);
   fileSize = ftell (fp);
   rewind (fp);

   if(fread(&hdr, sizeof(tStreamFileHeader), 1, fp) < 1)
   {
      fprintf(stderr, "Error reading data: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   } 

   if(getSsIndex(hdr.subsystem) < 0)
      printf("Subsystem type = unknow\n");
   else
      printf("Subsystem type = %s\n", ssNames[getSsIndex(hdr.subsystem)].name);
   printf("Number of channels = %d\n", hdr.numChannels);
   printf("Number of scans Per Block = %d\n", hdr.numScansPerFrame);
   printf("Scan rate = %f\n", hdr.scanRate);
   numScans = (fileSize - sizeof(tStreamFileHeader)) / (hdr.numChannels * sizeof(double));
   printf("Total number of scans = %d\n", numScans);

   buffer = (double*)malloc(hdr.numChannels * numScans * sizeof(double));

   // Start gnuplot
   gnuplot=GnuPlotOpen();
   if(!gnuplot) 
   {
      fprintf(stderr, "Could not start Gnuplot: %s\n",  strerror(errno));
      exit(EXIT_FAILURE);
   }

   while(fread((buffer+k*hdr.numChannels * hdr.numScansPerFrame), sizeof(double),hdr.numChannels * hdr.numScansPerFrame, fp) > 0)
   {
      printf("Read block %d\n", k);

      GnuPlot(gnuplot, (k*hdr.numScansPerFrame) * 1.0/hdr.scanRate, 1.0/hdr.scanRate,(buffer+k*hdr.numChannels * hdr.numScansPerFrame),
              hdr.numChannels, hdr.numScansPerFrame);    
 
      //sleep(1);
      k++;
   }

   GnuPlot(gnuplot, 0.0, 1.0/hdr.scanRate,buffer, hdr.numChannels, numScans);


   free(buffer);
   fclose(fp);

   fprintf(stderr,"Press enter to continue program.\n");
   fgets(s,256,stdin);
   while(strcmp(s,"\n")!=0) 
   {
      fgets(s,256,stdin); 
   }

   GnuPlotClose(gnuplot);
   return 0;
}

