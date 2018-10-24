#ifndef __PARSE_PARAMS_H__
#define __PARSE_PARAMS_H__

typedef struct _pd_params
{
    int board;
    int numChannels;
    int channels[128];
    float frequency;
    int trigger;
    int numSamplesPerChannel;
    int regenerate;
    char streamFileName[260];
} PD_PARAMS;

void Usage();
int ParseParameters(int argc, char **argv, PD_PARAMS *params);
int ParseChannelList(char *channelList, int channels[], int *nbChannels);

#endif /* __PARSE_PARAMS_H__ */


