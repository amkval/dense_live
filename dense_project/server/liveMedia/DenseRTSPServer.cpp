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

#include "include/DenseRTSPServer.hh"

DenseRTSPServer *DenseRTSPServer::createNew(
    UsageEnvironment &env,
    Port port,
    UserAuthenticationDatabase *authDatabase,
    u_int reclamationSeconds,
    Boolean streamRTPOverTCP,
    int count,
    std::string name,
    std::string time,
    std::string alias)
{

  int socket = setUpOurSocket(env, port);
  if (socket == -1)
  {
    return NULL;
  }

  DenseRTSPServer *denseRTSPServer = new DenseRTSPServer(env, socket, port, authDatabase, reclamationSeconds, streamRTPOverTCP, count, name, alias);

  // TODO: Where do we move this and what values do we need?
  // denseRTSPServer->initDenseValues();

  // TODO: Move these assignments
  //denseRTSPServer->fTime = stoi(time);
  //denseRTSPServer->fNextServer = NULL;
  //denseRTSPServer->fBeforeServer = NULL;

  fprintf(stderr, "\n TODO: print values for easy access");
  gettimeofday(&denseRTSPServer->fStartTime, NULL);

  // Make a 'DenseSession' for each quality level
  for (int i = 0; i < denseRTSPServer->fCount; i++)
  {
    denseRTSPServer->make(i);
  }

  return denseRTSPServer;
}

DenseRTSPServer::DenseRTSPServer(
    UsageEnvironment &env, int socket, Port port,
    UserAuthenticationDatabase *authDatabase,
    unsigned reclamationSeconds,
    Boolean streamRTPOverTCP, int count, std::string name, std::string alias)
    : RTSPServer(env, socket, port, authDatabase, reclamationSeconds),
      fDenseTable(HashTable::create(ONE_WORD_HASH_KEYS)),
      fCount(count), fName(name), fAlias(alias), fStartPort(port.num())
{
}

DenseRTSPServer::~DenseRTSPServer()
{
  cleanup();
}

void DenseRTSPServer::make(int number)
{
  fprintf(stderr, "Making DenseSession with id: %i, name %s, and startPort %d\n", number, fAlias, fStartPort);

  // TODO: do we need a new UsageEnvironment? Try to use the same?
  UsageEnvironment &env = envir();

  DenseSession *denseSession = createNewDenseSession();

  // Create 'ServerMediaSession'
  std::string denseName(fAlias + std::to_string(number));

  ServerMediaSession *sms = ServerMediaSession::createNew(
      env, denseName.c_str(), NULL, "Session streamed by 'denseServer'", False,
      "miscSDPLines");
  addServerMediaSession(sms);

  // Sleep to give us some time to kill the program(?)
  if (fStartPort == 18888)
  {
    sleep(1);
  }

  denseSession->setServerMediaSession(sms);

  // Create 'groupsocks' for RTP and RTCP:
  struct in_addr destinaionAddress;
  destinaionAddress.s_addr = chooseRandomIPv4SSMAddress(env);

  const unsigned short rtpPortNum = fStartPort + ((number - 1) * 2);
  fprintf(stderr, "rtpPortNum: %hu\n", rtpPortNum);

  const unsigned short rtcpPortNum = rtpPortNum + 1;
  fprintf(stderr, "rtcpPortNum: %hu\n", rtcpPortNum);

  const unsigned char ttl = 255;

  Port rtpPort(rtpPortNum);
  Port rtcpPort(rtcpPortNum);

  denseSession->setRTPGroupsock(env, destinaionAddress, rtpPort, ttl);
  denseSession->setRTCPGroupsock(env, destinaionAddress, rtcpPort, ttl);

  fprintf(stderr, "Make Video Sink\n");

  // Create 'H264 Video RTP' sink from the RTP 'groupsock':
  OutPacketBuffer::maxSize = 100000; // TODO: Do we need to do this here every time?

  ManifestRTPSink *manifestRTPSink = ManifestRTPSink::createNew(env, denseSession->fRTPGroupsock, 96, 90000, "video", "MP2T", this, 1, True, False, denseName.c_str());
  denseSession->setVideoSink(manifestRTPSink);

  fprintf(stderr, "Make RTCP Instance\n");

  // Create and start a 'RTCP instance' for this RTP sink:
  const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen + 1];
  gethostname((char *)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case

  RTCPInstance *rtcpInstance = RTCPInstance::createNew(
      env, denseSession->fRTCPGroupsock, estimatedSessionBandwidth,
      CNAME, denseSession->fVideoSink, NULL, True);
  denseSession->setRTCP(rtcpInstance);

  fprintf(stderr, "Make Passive Server Media Subsession\n");

  sms->addSubsession(PassiveServerMediaSubsession::createNew(*denseSession->fVideoSink, denseSession->fRTCP));

  fprintf(stderr, "Making CheckSource\n");
  // TODO: make dynamic, argv or something!

  std::string base = fName;
  std::string one = "first.m3u8";
  std::string two = "second.m3u8";
  std::string three = "third.m3u8";
  switch (number)
  {
  case 1:
    base += one;
    break;
  case 2:
    base += two;
    break;
  case 3:
    base += three;
  default:
    fprintf(stderr, "The number is outside the expected values!\n");
    break;
  }

  strcpy(denseSession->fSessionManifest, base.c_str());
  unsigned const inputDataChunkSize = TRANSPORT_PACKETS_PER_NETWORK_PACKET * TRANSPORT_PACKET_SIZE;

  // Make 'FileSource'
  CheckSource *fileSource = CheckSource::createNew(env /*envir()*/, denseSession->fSessionManifest, inputDataChunkSize);
  if (fileSource == NULL)
  {
    //env << "Unable to open file \"" << sms->streamFile() << "\" as a byte-stream file source\n";
    env << "Unable to open file as a byte-stream file source\n";
    exit(1);
  }

  denseSession->setFileSource(fileSource);
  // denseSession->fFileSource->removeLookAside(); // TODO: Why? Might not be needed?
  denseSession->fVideoSink->setCheckSource(denseSession->fFileSource);

  // Make 'videoSource'
  MPEG2TransportStreamFramer *videoSource = MPEG2TransportStreamFramer::createNew(env, denseSession->fFileSource);
  denseSession->setVideoSource(videoSource);
  //denseSession->fVideoSource->removeLookAside(); // TODO: why??

  // Start Playing
  denseSession->fVideoSink->startPlaying(*videoSource, NULL/*afterPlaying*/, denseSession->fVideoSink);

  fprintf(stderr, "Add DenseSession to DenseTable.\n");

  fDenseTable->Add((char const *)number, denseSession);

  fprintf(stderr, "Finish Make.\n");
}

///// DenseSession /////

DenseRTSPServer::DenseSession *DenseRTSPServer::createNewDenseSession()
{
  fprintf(stderr, "Creating new 'DenseSession'\n");
  return new DenseSession();
}