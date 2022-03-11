#ifndef _DENSE_RTSP_SERVER_HH
#define _DENSE_RTSP_SERVER_HH

#include "RTSPServer.hh"
#include "PassiveServerMediaSubsession.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "GroupsockHelper.hh"

#include "CheckSource.hh"
#include "ManifestRTPSink.hh"

#include <string>

#define TRANSPORT_PACKET_SIZE 188
#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7

///// DENSE RTSP SERVER /////
class DenseRTSPServer : public RTSPServer
{
public:
  static DenseRTSPServer *createNew(
      UsageEnvironment &env,
      Port port = 554,
      UserAuthenticationDatabase *authDatabase = NULL,
      u_int reclamationSeconds = 65U,
      Boolean streamRTPOverTCP = False,
      int number = 1,
      std::string name = NULL,
      std::string time = NULL,
      std::string alias = NULL);

  int fCount;                // Quality level count
  std::string fName;         // Name of ???
  std::string fAlias;        // Alias of ???
  uint16_t fStartPort;       // TODO: verify type
  struct timeval fStartTime; // Start time of the server

  void makeNextTuple();

protected:
  DenseRTSPServer(
      UsageEnvironment &env,
      int socket,
      Port port,
      UserAuthenticationDatabase *authDatabase,
      u_int reclamationSeconds,
      Boolean streamRTPOverTCP,
      int number,
      std::string name,
      std::string alias);
  virtual ~DenseRTSPServer();

  void getQualityLevelSDP(std::string &linePointer);
  void make(int number);
  void getFile(int /*???*/, char *pointer);
  ServerMediaSession *findSession(char const *streamName);

private:
  static void afterPlaying1(void /* clientData */);

  // TODO: Are these three needed?
  Boolean fStreamRTPOverTCP;
  Boolean fAllowStreamingRTPOverTCP;
  HashTable *fTCPStreamingDatabase;

  HashTable *fDenseTable;

  // TODO: Is this class needed?
  // A data structure that is used to implement "fTCPStreamingDatabase"
  // (and the "noteTCPStreamingOnSocket()" and "stopTCPStreamingOnSocket()" member functions):
  class streamingOverTCPRecord
  {
  public:
    streamingOverTCPRecord(u_int32_t sessionId, unsigned trackNum, streamingOverTCPRecord *next)
        : fNext(next), fSessionId(sessionId), fTrackNum(trackNum)
    {
    }
    virtual ~streamingOverTCPRecord()
    {
      delete fNext;
    }

    streamingOverTCPRecord *fNext;
    u_int32_t fSessionId;
    unsigned fTrackNum;
  };

public:
  class DenseSession
  {
  public:
    DenseSession()
    {
    }

    ~DenseSession()
    {
    }

    void setRTPGroupsock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl);
    void setRTCPGroupsock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl);

    void setVideoSink(ManifestRTPSink *manifestRTPSink)
    {
      fVideoSink = manifestRTPSink;
    }

    void setServerMediaSession(ServerMediaSession *serverMediaSession)
    {
      fServerMediaSession = serverMediaSession;
    }

    void setRTCP(RTCPInstance *rtcp)
    {
      fRTCP = rtcp;
    }

    void setFileSource(CheckSource *fileSource)
    {
      fFileSource = fileSource;
    }

    void setVideoSource(MPEG2TransportStreamFramer *videoSource)
    {
      fVideoSource = videoSource;
    }

    // Sockets til RTP og RTCP
    Groupsock *fRTPGroupsock;
    Groupsock *fRTCPGroupsock;

    // RTP
    ManifestRTPSink *fVideoSink;

    // RTCP
    RTCPInstance *fRTCP;

    // Passive
    PassiveServerMediaSubsession *fPassiveSession;

    // Session
    ServerMediaSession *fServerMediaSession;

    CheckSource *fFileSource;

    MPEG2TransportStreamFramer *fVideoSource;

    DenseRTSPServer *fDenseServer;

    char fSessionManifest[100]; // TODO: can this be stored in another way?
  };

  DenseSession *createNewDenseSession();
};

#endif