#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ParseParams.h"

void Usage(char *prog_name)
{
    printf("%s, Copyright (C) 2003, United Electronic Industries, Inc.\n", prog_name);
    printf("usage: %s [options]\n", prog_name);
    printf("\t-h : display help\n");
    printf("\t-b n: selects the board to use (default: 0)\n");
    printf("\t-n n: selects the size of a frame (default: 4096)\n");
    printf("\t-f n.nn : set the rate of the DAQ operation (default: 1000 Hz)\n");
    printf("\t-c \"x,y,z,...\" : select the channels to use (default: channel 0)\n");
    printf("\t-t : configure digital trigger (only for Buffered examples)\n \t\t0 -> Software trigger\n \t\t1 -> start on raising edge\n \t\t2 -> start on falling edge\n");
    printf("\t-r : Turn-on regeneration (only for buffered output examples)\n");
    printf("\t-s : Stream acquired/generated data to/from the specified file\n");
}

int ParseParameters(int argc, char **argv, PD_PARAMS *params)
{
    int i = 1;
    int j;
    
    while(i<argc)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'b':
                i++;
                params->board = atoi(argv[i]);
                printf("Board is set to %d\n", params->board);
                break;
            case 'n':
                i++;
                params->numSamplesPerChannel = atoi(argv[i]);
                printf("Board is set to %d\n", params->numSamplesPerChannel);
                break;
            case 'f':
                i++;
                params->frequency = atof(argv[i]);
                printf("Frequency is set to %f\n", params->frequency);
                break;
            case 'c':
                i++;
                ParseChannelList(argv[i], params->channels, &params->numChannels);
                printf("There are %d channels specified: ", params->numChannels);
                for(j=0; j<params->numChannels; j++)
                    printf("%d ", params->channels[j]);
                printf("\n"); 
                break;
            case 't':
                i++;
                params->trigger = atoi(argv[i]);
                printf("Trigger is set to %d\n", params->trigger);
                break;
            case 'r':
                params->regenerate = 1;
                printf("Output regeneration is set to %d\n", params->trigger);
                break;
            case 's':
                i++;
                strcpy(params->streamFileName, argv[i]);
                printf("Stream file is set to %s\n", params->streamFileName);
                break;
            case 'h':
            default:
                Usage(argv[0]);
                exit(0);
                break;
            }
            
            i++;
        }
        else
        {
            Usage(argv[0]);
            exit(0);
        }
    }
    
    return 0;
}


int ParseChannelList(char *channelList, int channels[], int *nbChannels)
{
    char *p = channelList;
    char *comma;
    int i=0;
    
    while(p != NULL)
    {
        channels[i] = atoi(p);
        comma = strchr(p, ',');
        if(comma == NULL)
            break;
        
        p = comma+1;
        i++; 
    }
    
    *nbChannels = i+1;
    
    return 0;
}
