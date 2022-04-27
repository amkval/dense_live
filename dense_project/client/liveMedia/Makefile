LIVE_DIR = ../../../live/
MACROS = -DBSD=1  -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int
OWN = /include
INCLUDES = -I$(OWN) -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a

all:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseFileSink.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseMediaSession.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseMediaSubsession.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseMultiFramedRTPSource.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DensePrints.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseRTSPClient.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseSimpleRTPSource.cpp

	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./DenseStreamClientState.cpp

clean:
	rm -f *.o