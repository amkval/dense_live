LIVE_DIR = ../../../live/
MACROS = -DBSD=1 -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int
INCLUDES = -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a

# Test programs
SERVER = server
CLIENT = client

# Dense files
DENSE_RTSP_SERVER = "../liveMedia/DenseRTSPServer"
MANIFEST_RTP_SINK = "../liveMedia/ManifestRTPSink"
CHECK_SOURCE = "../liveMedia/CheckSource"

# enable optimization by changing -O0 to -O
server:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./$(SERVER).cpp
	c++ -g -o $(SERVER) -L. $(DENSE_RTSP_SERVER).o $(MANIFEST_RTP_SINK).o $(CHECK_SOURCE).o $(SERVER).o $(LIBS)

client:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./$(CLIENT).cpp
	c++ -g -o $(CLIENT) -L. $(CLIENT).o $(LIBS)

.PHONY: clean all server2
server2:
	c++ -c -g -O0 -Wall $(INCLUDES) $(MACROS) ./$(SERVER).cpp
	c++ -g -o $(SERVER) -L. $(SERVER).o $(LIBS)

all: server client

clean:
	rm -f $(SERVER)
	rm -f $(CLIENT)
	rm -f *.o
	rm -f *.a