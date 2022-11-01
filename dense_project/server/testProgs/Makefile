LIVE_DIR = ../../../live/
MACROS = -DBSD=1 -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int
INCLUDES = -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a

# Test programs
SERVER = server

# Dense files
CSOURCE = "../liveMedia/CheckSource"
#DMFRTPSINK = "../liveMedia/DenseMultiFramedRTPSink"
DPSMSUBSESSION = "../liveMedia/DensePassiveServerMediaSubsession"
DRTPSINK = "../liveMedia/DenseRTPSink"
DRTSPSERVER = "../liveMedia/DenseRTSPServer"
DSMSESSION = "../liveMedia/DenseServerMediaSession"
#MRTPSINK = "../liveMedia/ManifestRTPSink"

# enable optimization by changing -O0 to -O
server:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./$(SERVER).cpp
	c++ -g -o $(SERVER) -L. $(SERVER).o $(CSOURCE).o $(DPSMSUBSESSION).o $(DRTPSINK).o $(DRTSPSERVER).o $(DSMSESSION).o $(LIBS)

.PHONY: clean all
all: server

clean:
	rm -f $(SERVER)
	rm -f *.o
	rm -f *.a