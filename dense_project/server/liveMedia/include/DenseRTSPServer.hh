/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// TODO: additional information

#ifndef _DENSE_RTSP_SERVER_HH
#define _DENSE_RTSP_SERVER_HH

#include "RTSPServer.hh"
#include "PassiveServerMediaSubsession.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "GroupsockHelper.hh"

class ManifestRTPSink; // Forward

#ifndef _MANIFEST_RTP_SINK_HH
#include "ManifestRTPSink.hh"
#endif

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

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
      int levels = 1,
      std::string path = "", //TODO: do something else?
      std::string fps = "", //TODO: do something else?
      std::string alias = ""); //TODO: do something else?

  int fLevels;               // Quality level count
  std::string fPath;         // used to be name
  std::string fAlias;        // Alias of ???, file descriptor? TODO: Better name or understanding!
  uint16_t fStartPort;       // TODO: verify type // TODO: is this needed? port is already a thing, no?
  struct timeval fStartTime; // Start time of the server.

  int fFPS;                     // Used to be 'time'
  DenseRTSPServer *fNextServer; // TODO: What is this used for?

  void makeNextTuple();

protected:
  DenseRTSPServer(
      UsageEnvironment &env,
      int socket,
      Port port,
      UserAuthenticationDatabase *authDatabase,
      u_int reclamationSeconds,
      Boolean streamRTPOverTCP,
      int levels,
      std::string path,
      std::string fps,
      std::string alias);
  virtual ~DenseRTSPServer();

  void getQualityLevelSDP(std::string &linePointer);
  void make(int number);
  void getFile(int /*???*/, char *pointer);
  ServerMediaSession *findSession(char const *streamName);

private:
  void afterPlaying(void */* clientData */); // TODO: used to be afterPlaying1

  // TODO: Are these three needed?
  Boolean fStreamRTPOverTCP;
  Boolean fAllowStreamingRTPOverTCP;
  HashTable *fTCPStreamingDatabase;

  HashTable *fDenseTable; // Used for cleanup

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

    // Sockets for RTP and RTCP
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

    // FileSource
    CheckSource *fFileSource;

    // VideoSource
    MPEG2TransportStreamFramer *fVideoSource;

    
    DenseRTSPServer *fDenseServer;

    char fSessionManifest[100]; // TODO: can this be stored in another way?
  };

  DenseSession *createNewDenseSession();
};

#endif