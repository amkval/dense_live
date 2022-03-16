LIVE_DIR = ../../../live/
MACROS = -DBSD=1  -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int
OWN = /include
INCLUDES = -I$(OWN) -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a


all:

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./CheckSource.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseRTSPServer.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./ManifestRTPSink.cpp
	
clean:
	rm -f *.o
