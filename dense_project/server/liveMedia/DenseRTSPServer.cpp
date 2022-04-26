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

#include "GroupsockHelper.hh"

#include "include/DenseRTSPServer.hh"

#include <string>

////// DenseRTSPServer //////

DenseRTSPServer *DenseRTSPServer::createNew(
    UsageEnvironment &env,
    Port port,
    UserAuthenticationDatabase *authDatabase,
    u_int reclamationSeconds,
    Boolean streamRTPOverTCP,
    int levels,
    std::string path,
    std::string fps,
    std::string alias)
{

  // Setup socket
  int socket = setUpOurSocket(env, port);
  if (socket == -1)
  {
    return NULL;
  }

  DenseRTSPServer *denseRTSPServer = new DenseRTSPServer(
      env, socket, port, authDatabase, reclamationSeconds,
      streamRTPOverTCP, levels, path, fps, alias);

  // TODO: Move initialization to a better place:
  denseRTSPServer->fNextServer = NULL; // What is this used for?
  //denseRTSPServer->fStartPort = port.num(); Moved

  env << "DenseRTSPServer: "
      << "TODO: print all densRTSPServer info\n";

  gettimeofday(&denseRTSPServer->fStartTime, NULL);

  // Make a 'DenseSession' for each quality level
  for (int i = 0; i < denseRTSPServer->fLevels; i++)
  {
    denseRTSPServer->make(i);
  }

  return denseRTSPServer;
}

DenseRTSPServer::DenseRTSPServer(
    UsageEnvironment &env, int socket, Port port,
    UserAuthenticationDatabase *authDatabase,
    unsigned reclamationSeconds,
    Boolean streamRTPOverTCP, int levels, std::string path, std::string fps, std::string alias)
    : RTSPServer(env, socket, port, authDatabase, reclamationSeconds),
      fDenseTable(HashTable::create(ONE_WORD_HASH_KEYS)),
      fLevels(levels), fPath(path), fAlias(alias), fStartPort(port.num()),
      fStartTime({0}), fFPS(std::stoi(fps)), fNextServer(NULL)
{
  //TODO: What about the other stuff that i left out?
}

DenseRTSPServer::~DenseRTSPServer()
{
  cleanup();
}

void DenseRTSPServer::make(int level)
{
  UsageEnvironment &env = envir(); // TODO: get from somewhere else?

  // TODO: use constom print
  env << "Making DenseSession with id: " << level << ", name " << fAlias.c_str() << ", and startPort " << fStartPort << "\n";

  DenseSession *denseSession = createNewDenseSession(); // TODO: why not createNew() ?

  // Create 'ServerMediaSession'
  std::string denseName = fAlias + std::to_string(level);
  env << "denseName: " << denseName.c_str() << "\n";

  ServerMediaSession *sms = ServerMediaSession::createNew(
      env, 
      denseName.c_str(),
      NULL,
      "Session streamed by \"denseServer\"",
      False,
      "a=x-qt-text-misc:yo yo mood"); // TODO: Change "yo yo mood" to something more appropriate
  addServerMediaSession(sms);

  // Sleep to give us some time to kill the program
  if (fStartPort == 18888) // TODO: can be removed.
  {
    env << "Sleeping for 1 second\n";
    sleep(1);
  }

  denseSession->setServerMediaSession(sms);

  // Create 'groupsocks' for RTP and RTCP:
  struct in_addr destinaionAddress;
  destinaionAddress.s_addr = chooseRandomIPv4SSMAddress(env);

  const unsigned short rtpPortNum = fStartPort + (level * 2);
  env << "rtpPortNum: " << rtpPortNum << "\n";

  const unsigned short rtcpPortNum = rtpPortNum + 1;
  env << "rtcpPortNum: " << rtcpPortNum << "\n";

  Port rtpPort(rtpPortNum);
  Port rtcpPort(rtcpPortNum);

  const unsigned char ttl = 255;

  denseSession->setRTPGroupsock(env, destinaionAddress, rtpPort, ttl);
  denseSession->setRTCPGroupsock(env, destinaionAddress, rtcpPort, ttl);

  // Make VideoSink or ManifestRTPSink
  env << "Make Video Sink\n";

  // Create 'H264 Video RTP' sink from the RTP 'groupsock':
  OutPacketBuffer::maxSize = 100000; // TODO: Do we need to do this here every time?

  ManifestRTPSink *manifestRTPSink = ManifestRTPSink::createNew(
      env, 
      denseSession->fRTPGroupsock, 
      96, 
      90000, 
      "video", 
      "MP2T",
      this, 
      1, 
      True, 
      False, 
      denseName.c_str());
  denseSession->setVideoSink(manifestRTPSink);

  // Make RTCP Instance 
  env << "Make RTCP Instance\n";

  // Create and start a 'RTCP instance' for this RTP sink:
  const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen + 1];
  gethostname((char *)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case

  RTCPInstance *rtcpInstance = RTCPInstance::createNew(
      env, 
      denseSession->fRTCPGroupsock, 
      estimatedSessionBandwidth,
      CNAME, 
      (RTPSink *)denseSession->fVideoSink, 
      NULL, 
      True);
  denseSession->setRTCP(rtcpInstance);

  // Make 'PassiveServerMediaSubsession'
  env << "Make Passive Server Media Subsession\n";

  sms->addSubsession(PassiveServerMediaSubsession::createNew((RTPSink &)*denseSession->fVideoSink, denseSession->fRTCP));

  // Make 'CheckSource'
  env << "Make CheckSource\n";

  // TODO: make dynamic?
  std::string base = fPath;
  std::string one = "first.m3u8";
  std::string two = "second.m3u8";
  std::string three = "third.m3u8";
  switch (level)
  {
  case 0:
    base += one;
    break;
  case 1:
    base += two;
    break;
  case 2:
    base += three;
  default:
    env << "Level \"" << level << "\" is outside the expected values!\n";
    exit(EXIT_FAILURE);
  }
  strcpy(denseSession->fSessionManifest, base.c_str()); // TODO: Better method than strcpy?

  unsigned const inputDataChunkSize = TRANSPORT_PACKETS_PER_NETWORK_PACKET * TRANSPORT_PACKET_SIZE;
  CheckSource *fileSource = CheckSource::createNew(env, denseSession->fSessionManifest, inputDataChunkSize);
  if (fileSource == NULL)
  {
    env << "Unable to open file as a byte-stream file source: " << env.getResultMsg() << "\n";
    exit(EXIT_FAILURE);
  }

  denseSession->setFileSource(fileSource);
  // denseSession->fFileSource->removeLookAside(); // TODO: Why? Might not be needed?
  denseSession->fVideoSink->setCheckSource(denseSession->fFileSource);

  // Make 'videoSource'
  MPEG2TransportStreamFramer *videoSource = MPEG2TransportStreamFramer::createNew(env, denseSession->fFileSource);
  denseSession->setVideoSource(videoSource);
  // denseSession->fVideoSource->removeLookAside(); // TODO: why??

  env << "Start Playing\n";

  denseSession->fVideoSink->startPlaying(*videoSource, NULL /*afterPlaying*/, denseSession->fVideoSink);

  env << "Add DenseSession to DenseTable\n";

  fDenseTable->Add((char const *)level, denseSession);

  env << "Make finished\n";
}

void DenseRTSPServer::DenseSession::setRTPGroupsock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl)
{
  env << "setRTPGroupsock\n";
  Groupsock *gsock = new Groupsock(env, destinationAddress, rtpPort, ttl);
  fRTPGroupsock = gsock;

  AddressString groupAddressStr(fRTPGroupsock->groupAddress());
  env << "       setRTPGSock -> AddressString groupAddressStr: " << groupAddressStr.val() << "\n";
  unsigned short portNum = ntohs(fRTPGroupsock->port().num());
  env << "       setRTPGSock -> portnum: " << portNum << "\n";
}

// TODO: Remove duplicate code?
void DenseRTSPServer::DenseSession::setRTCPGroupsock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl)
{
  env << "setRTCPGroupsock\n";
  Groupsock *gsock = new Groupsock(env, destinationAddress, rtpPort, ttl);
  fRTCPGroupsock = gsock;

  AddressString groupAddressStr(fRTCPGroupsock->groupAddress());
  env << "       setRTPCONTROL gsock -> AddressString groupAddressStr: " << groupAddressStr.val() << "\n";
  unsigned short portNum = ntohs(fRTCPGroupsock->port().num());
  env << "       setRTPCONTROL gsock -> portnum: " << portNum << "\n";
}

// TODO: implement
void DenseRTSPServer::afterPlaying(void * /*clientData*/)
{
  for (int i = 0; i < fLevels; i++)
  {
    DenseSession *denseSession = (DenseSession *)fDenseTable->Lookup((char const *)i);
    if (denseSession != NULL)
    {
      denseSession->fVideoSink->stopPlaying();
      Medium::close(denseSession->fVideoSink);
      Medium::close(denseSession->fRTCP);
    }
  }
}

void DenseRTSPServer::makeNextTuple()
{
  std::string newstream = "newStream";

  DenseRTSPServer *rtspServer = DenseRTSPServer::createNew(
      envir(),
      htons(fStartPort) + 10,
      NULL,
      65U,
      NULL,
      fLevels,
      fPath,
      std::to_string(fFPS),
      newstream);

  fNextServer = rtspServer;

  fprintf(stderr, "makeNextTuple() startPort: %d\n", fStartPort);
  sleep(2); // TODO: Remove?
}

///// DenseSession /////

DenseRTSPServer::DenseSession *DenseRTSPServer::createNewDenseSession()
{
  return new DenseSession();
}