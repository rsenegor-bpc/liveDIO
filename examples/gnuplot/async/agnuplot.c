#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "gnuplot.h"

void* plot_thread_routine(void* arg);

struct _plot_thread_data
{
   // event that will wake-up plot thread when data
   // is available
   pthread_cond_t  plot_data_event;
   pthread_mutex_t plot_data_event_lock;
   float x0;
   float dX;
   double *buffer;
   int nbOfChannels;
   int nbScans;

   int abort;
   int ready;
};


FILE *gpcmd = NULL;
FILE *gpfifo = NULL;
pthread_t plot_thread;
struct _plot_thread_data plot_data;

int initGnuPlot()
{
   int retval;

   gpcmd = popen("gnuplot","w");
   if(gpcmd == NULL)
   {
      fprintf(stderr, "Failed to open gnuplot");
      return -1;
   }

   // create condition that will synchronize the plot thread
   retval = pthread_cond_init(&plot_data.plot_data_event, NULL);
   if (retval != 0)
   {
      fprintf(stderr, "ai server: Error %d, can't create conditional variable\n", retval);
      return -1;
   }
      
   // Initializes the mutex that protects access to the condition
   retval = pthread_mutex_init(&plot_data.plot_data_event_lock, NULL);
   if (retval != 0)
   {
      fprintf(stderr, "ai server: Error %d, can't create mutex\n", retval);
      return -1;
   }

   plot_data.abort = 0;
   plot_data.buffer = NULL;
   plot_data.dX = 0;
   plot_data.nbOfChannels = 0;
   plot_data.nbScans = 0;
   plot_data.ready = 0;
   plot_data.x0 = 0;
   
   // starts thread that will communicate with gnuplot
   retval = pthread_create(&plot_thread, NULL, plot_thread_routine, &plot_data);  
   if(retval != 0)
   {
      fprintf(stderr, "Could not create server thread\n");
      return -1;
   }

   return 0;
}


int closeGnuPlot()
{
   // signal thread that it's time to terminate
   fprintf(stderr, "thread ready = %d\n", plot_data.ready);
   
   while(plot_data.ready != 1)
   { }

   fprintf(stderr, "thread ready = %d\n", plot_data.ready);

   plot_data.abort = 1;
   pthread_cond_signal(&plot_data.plot_data_event);
   
   // wait for thread to terminate
   pthread_join(plot_thread, NULL);

   pthread_mutex_destroy(&plot_data.plot_data_event_lock);    
   pthread_cond_destroy(&plot_data.plot_data_event);
   
   if(gpcmd != NULL)
   { 
      pclose(gpcmd);
      gpcmd = NULL;
   }

   return 0;
}
    

int sendDataToGnuPlot(int handle, float x0, float dX, double *buffer, int nbOfChannels, int nbScans)
{
   // if plot thread is ready for new data, copy buffer and signal
   if(plot_data.ready == 1)
   {
      plot_data.x0 = x0;
      plot_data.dX = dX;
      plot_data.nbOfChannels = nbOfChannels;
      plot_data.nbScans = nbScans;
      plot_data.buffer = (double*) malloc(nbOfChannels * nbScans * sizeof(double));
      memcpy(plot_data.buffer, buffer, nbOfChannels * nbScans * sizeof(double));

      pthread_cond_signal(&plot_data.plot_data_event);
   }

   return 0;
}

void* plot_thread_routine(void* arg)
{
   int i, j;
   int retval;
   char *tmp;
   char *temp_name[64];
   struct _plot_thread_data *data = (struct _plot_thread_data*)arg;
   sigset_t new_set;

   sigemptyset(&new_set);
   sigaddset(&new_set, SIGINT);
   pthread_sigmask(SIG_BLOCK, &new_set, NULL);

   while(!data->abort)
   {
      // signal that this thread is ready for new data
      data->ready = 1;

      // wait for the date ready signal
      // Get the lock, pthrea_cond_wait will unlock it while waiting and
      // relock it when it gets signaled
      retval = pthread_mutex_lock(&data->plot_data_event_lock);
      if (retval != 0)
      {
         fprintf(stderr, "client thread: Could not acquire data ready lock\n");
         pthread_exit(NULL);
      }

      fprintf(stderr, "waiting for condition\n");
        
      retval = pthread_cond_wait(&data->plot_data_event, &data->plot_data_event_lock);
      if (retval != 0)
      {
         fprintf(stderr, "uei8xx_wait_for_event: Unexpected error\n");
         pthread_exit(NULL);
      } 

      fprintf(stderr, "Received condition\n");
      
      pthread_mutex_unlock(&data->plot_data_event_lock);

      if(data->abort)
      {
         fprintf(stderr, "aborting\n");
         break;
      }
      else
         data->ready = 0;

      //fprintf(gpcmd, "clear\n");
      fprintf(gpcmd, "plot ");

      for(i=0; i<data->nbOfChannels; i++)
      {
         temp_name[i] = (char *)malloc(260 * sizeof(char));
         
         tmp = tmpnam(temp_name[i]);
         if (temp_name == NULL)
         {
            fprintf(stderr, "tmpnam failed");
            pthread_exit(NULL);
         }
      
         if(mkfifo(temp_name[i], S_IRUSR | S_IWUSR) != 0)
         {
            fprintf(stderr, "mkfifo failed");
            pthread_exit(NULL);
         }
      }
      
      for(i=0; i<data->nbOfChannels-1; i++)
         fprintf(gpcmd, "'%s' title 'channel %d' with lines, ", temp_name[i], i);
      fprintf(gpcmd, "'%s' title 'channel %d' with lines\n", temp_name[data->nbOfChannels-1], data->nbOfChannels-1);
      fflush(gpcmd);
   
         
      for(i=0; i<data->nbOfChannels; i++)
      {
         gpfifo = fopen(temp_name[i], "w");
         if(gpfifo == NULL)                          
         {
            printf("Can't open temp fifo\n");
            pthread_exit(NULL);
         } 
   
         for(j=0; j<data->nbScans; j++)
         {
            fprintf(gpfifo, "%f %f\n", data->x0 + j*data->dX, data->buffer[j*data->nbOfChannels + i]);
         }
   
         fprintf(gpfifo, "\n");
   
         fflush(gpfifo);
   
         fclose(gpfifo);
      }
   
      for(j=0; j<data->nbOfChannels; j++)
      {
         unlink(temp_name[j]);
         free(temp_name[j]);
      }

      free(data->buffer);
   }

   fprintf(stderr, "thread aborted\n");
   
   return 0;
}



