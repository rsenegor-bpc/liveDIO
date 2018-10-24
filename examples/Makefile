
CC=gcc
LD=ld

SUBDIRS=DIEvent \
	DIOEvent \
	DIOEvent_Async \
	DspIrq \
	SingleDI \
	SingleDIO \
	SingleDO \
	SingleAI \
	SingleAIEvent \
	SingleAO \
	SingleAO_SimUpdate \
	BufferedAI \
	BufferedAI_Async \
	BufferedAI_MultiBoards \
	BufferedAI_StreamToDisk \
	BufferedAO \
	BufferedAO_AudioFile \
	BufferedAIAO \
	BufferedAIAO_Replay \
	BufferedDSPCT \
	BufferedDI \
	BufferedDO \
	BufferedDO_TS \
	UCT_CountEvent \
	UCT_GenSinglePulse \
	UCT_GenPulseTrain \
	UCT_GenSquareWave \
	UCT_MeasFrequency \
	UCT_MeasPulseWidth \
	UctDsp \
	UCT_Async \
	UCT_Event

all:  $(SUBDIRS) 

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	cd $@ && $(MAKE)
	
	
install:
		
clean:
	for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done; 
	
