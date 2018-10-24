#ifndef __STREAMFILE_H__
#define __STREAMFILE_H__

typedef enum _subSystems
{
   AI = 0x4941,
   AO = 0x4F41,
   DI = 0x4944,
   DO = 0x4F44,
   CI = 0x4943,
   CO = 0x4F43
} tSubSystems;

typedef struct _streamFileHeader
{
   short subsystem;
   int  numChannels;
   int numScansPerFrame;;
   double scanRate;
} tStreamFileHeader;

#endif
