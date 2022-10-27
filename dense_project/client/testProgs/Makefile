LIVE_DIR = ../../../live/
INCLUDES = -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a
MACROS = -DBSD=1  -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int


CLIENT = client

DFSINK = ../liveMedia/DenseFileSink
DMSESSION = ../liveMedia/DenseMediaSession
DMSUBSESSION = ../liveMedia/DenseMediaSubsession
DPRINTS = ../liveMedia/DensePrints
DRTPSCLIENT = ../liveMedia/DenseRTSPClient
DSCSTATE = ../liveMedia/DenseStreamClientState
DRTPSOURCE = ../liveMedia/DenseRTPSource

client:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./$(CLIENT).cpp
	c++ -g -o $(CLIENT) -L. $(CLIENT).o $(DFSINK).o $(DMSESSION).o $(DMSUBSESSION).o $(DPRINTS).o $(DRTPSCLIENT).o $(DRTPSOURCE).o $(DSCSTATE).o $(LIBS)

.PHONY: clean all
all: client

clean:
	rm -f $(CLIENT)
	rm -f *.o
	rm -f *.a