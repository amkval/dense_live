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

// Forward class Definitions
class ManifestRTPSink;

// Live Imports
#include "RTSPServer.hh"
#include "PassiveServerMediaSubsession.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "GroupsockHelper.hh"

// Dense Imports
#ifndef _MANIFEST_RTP_SINK_HH
#include "ManifestRTPSink.hh"
#endif

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

#ifndef _DENSE_SERVER_MEDIA_SESSION
#include "DenseServerMediaSession.hh"
#endif

// Other Imports
#include <string>

#define TRANSPORT_PACKET_SIZE 188
#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7

///// DENSE RTSP SERVER /////
class DenseRTSPClientConnection; // Forward
class DenseRTSPClientSession;    // Forward
class DenseSession;              // Forward
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
      std::string path = "",   // TODO: do something else?
      std::string fps = "",    // TODO: do something else?
      std::string alias = ""); // TODO: do something else?

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

public:
  void makeNextTuple();

protected:
  void getQualityLevelSDP(std::string &linePointer);
  void make(int number);
  // void getFile(int /*???*/, char *pointer); TODO: Not used?
  ServerMediaSession *findSession(char const *streamName);

private:
  static void afterPlaying1(void * /* clientData */); // TODO: name?

public:
  int fLevels;               // Quality level count
  std::string fPath;         // used to be name
  std::string fAlias;        // Alias of ???, file descriptor? TODO: Better name or understanding!
  uint16_t fStartPort;       // TODO: verify type // TODO: is this needed? port is already a thing, no?
  struct timeval fStartTime; // Start time of the server.

  int fFPS;                     // Used to be 'time'
  DenseRTSPServer *fNextServer; // TODO: What is this used for?

private:
  Boolean fStreamRTPOverTCP;         // TODO: Used?
  Boolean fAllowStreamingRTPOverTCP; // TODO: Used?
  HashTable *fTCPStreamingDatabase;  // TODO: Used?

  HashTable *fDenseTable; // Storage for denseSessions.

  ////// DenseSession //////
public:
  // Class that keeps track of session variables.
  class DenseSession
  {
  public:
    DenseSession(DenseRTSPServer *denseServer);
    ~DenseSession();

  public:
    void setRTPGroupsock(
        UsageEnvironment &env,
        in_addr destinationAddress,
        Port rtpPort, u_int8_t ttl);
    void setRTCPGroupsock(
        UsageEnvironment &env,
        in_addr destinationAddress,
        Port rtpPort, u_int8_t ttl);

    void setVideoSink(ManifestRTPSink *manifestRTPSink)
    {
      fVideoSink = manifestRTPSink;
    }

    void setServerMediaSession(DenseServerMediaSession *serverMediaSession)
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

  public:
    Groupsock *fRTPGroupsock;                      // Socket for RTP
    Groupsock *fRTCPGroupsock;                     // Socket for RTCP
    ManifestRTPSink *fVideoSink;                   // RTP VideoSink
    RTCPInstance *fRTCP;                           // RTCP Instance
    PassiveServerMediaSubsession *fPassiveSession; // Passive Server Media Subsession
    DenseServerMediaSession *fServerMediaSession;  // Server Media Session
    CheckSource *fFileSource;                      // File Source
    MPEG2TransportStreamFramer *fVideoSource;      // Video Source
    DenseRTSPServer *fDenseServer;                 // Dense Server
    char fSessionManifest[100];                    // TODO: can this be stored in another way?
  };

  DenseSession *createNewDenseSession(DenseRTSPServer *denseServer); // TODO: why is this not a part of DenseSession?

public:
  ////// DenseClientConnection //////
  // The state of a TCP connection used by a RTSP client:
  class DenseRTSPClientSession; // forward
  class DenseRTSPClientConnection : public RTSPServer::RTSPClientConnection
  {

  public:
    void handleRequestBytes(int newBytesRead);
    void handleCmd_OPTIONS(char *urlSuffix);
    void handleCmd_DESCRIBE(char const *urlPreSuffix, char const *urlSuffix, char const *fullRequestStr);

  protected:
    DenseRTSPClientConnection(DenseRTSPServer &ourServer, int clientSocket, struct sockaddr_in clientAddr);
    virtual ~DenseRTSPClientConnection();

    friend class DenseRTSPServer;
    friend class RTSPServer;

    DenseRTSPServer &fDenseRTSPServer;
    DenseRTSPClientSession *fDenseRTSPClientSession;

  private:
    friend class DenseRTSPClientSession;
  };

protected: // redefined virtual functions
  // If you subclass "RTSPClientConnection", then you must also redefine this virtual function in order
  // to create new objects of your subclass:
  virtual DenseRTSPClientConnection *createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr);

  ////// RTSP Dense CLient Session //////
public:
  class DenseRTSPClientSession : public RTSPServer::RTSPClientSession
  {
  public:
    void handleCmd_SETUP_1(DenseRTSPServer::DenseRTSPClientConnection *ourClientConnection,
                           char const *urlPreSuffix, char const *urlSuffix, char const *fullRequestStr);

    void handleCmd_SETUP_2(DenseRTSPServer::DenseRTSPClientConnection *ourClientConnection,
                           char const *urlPreSuffix, char const *urlSuffix, char const *fullRequestStr);

  protected:
    DenseRTSPClientSession(DenseRTSPServer &ourServer, u_int32_t sessionId);
    virtual ~DenseRTSPClientSession();

    friend class DenseRTSPServer;
    friend class RTSPServer;

    DenseRTSPServer &fDenseRTSPServer;
    DenseRTSPClientConnection *fDenseRTSPClientConnection;
    Boolean fNewDenseRequest;

  private:
    friend class RTSPDenseClientConnection;
  };

protected:
  // If you subclass "DenseRTSPClientSession", then you must also redefine this virtual function in order
  // to create new objects of your subclass:
  virtual DenseRTSPClientSession *createNewClientSession(u_int32_t sessionId);
};

typedef enum StreamingMode
{
  RTP_UDP,
  RTP_TCP,
  RAW_UDP
} StreamingMode;

// A special version of "parseTransportHeader()", used just for parsing the "Transport:" header
// in an incoming "REGISTER" command:
void parseTransportHeaderForREGISTER(
    char const *buf,          // in
    Boolean &reuseConnection, // out
    Boolean &deliverViaTCP,   // out
    char *&proxyURLSuffix);   // out

#endif