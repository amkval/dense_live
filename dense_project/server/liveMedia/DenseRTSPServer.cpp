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

#include "RTSPCommon.hh"
#include "GroupsockHelper.hh"
#include "Base64.hh"

#include "include/DenseRTSPServer.hh"
#include "include/DenseServerMediaSession.hh"

#include <string>

static HashTable *commonDenseTable; // For static access.

// Forward Class Definitions
static Boolean parsePlayNowHeader(char const *buf);

static void parseTransportHeader(
    char const *buf,
    StreamingMode &streamingMode,
    char *&streamingModeString,
    char *&destinationAddressStr,
    u_int8_t &destinationTTL,
    portNumBits &clientRTPPortNum,  // if UDP
    portNumBits &clientRTCPPortNum, // if UDP
    unsigned char &rtpChannelId,    // if TCP
    unsigned char &rtcpChannelId    // if TCP
);

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

  env << "DenseRTSPServer:\n"
      << "\tport: " << port << "\n"
      << "\treclamationSeconds: " << reclamationSeconds << "\n"
      << "\tstreamRTPOverTCP: " << streamRTPOverTCP << "\n"
      << "\tlevels: " << levels << "\n"
      << "\tpath: " << path.c_str() << "\n"
      << "\tfps: " << fps.c_str() << "\n"
      << "\talias: " << alias.c_str() << "\n";

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
      fLevels(levels), fPath(path), fAlias(alias), fStartPort(port.num()),
      fStartTime({0}), fFPS(std::stoi(fps)), fNextServer(NULL),
      fStreamRTPOverTCP(streamRTPOverTCP), fAllowStreamingRTPOverTCP(True), // TODO: Are these needed?
      fDenseTable(HashTable::create(ONE_WORD_HASH_KEYS))
{
}

DenseRTSPServer::~DenseRTSPServer()
{
  cleanup();
}

void DenseRTSPServer::getQualityLevelSDP(std::string &linepointer)
{
  const char *line;

  int teller = 1;
  DenseSession *closeSessionPointer;
  closeSessionPointer = (DenseSession *)commonDenseTable->Lookup(std::to_string(teller).c_str());
  while (closeSessionPointer != NULL)
  {
    DenseServerMediaSession *session = dynamic_cast<DenseServerMediaSession *>(closeSessionPointer->fServerMediaSession);
    if (session == NULL)
    {
      fprintf(stderr, "Failed miserably to cast Session!. :(");
    }

    int subTeller = session->numSubsessions();

    if (subTeller != 1)
    {
      fprintf(stderr, "Something went wrong, there are more than one subsession on this serversession on this DenseSession"); // What ever that means.
      exit(0);
    }

    ServerMediaSubsession *subsession = *session->fServerMediaSubsessionVector.begin();
    if (subsession == NULL)
    {
      fprintf(stderr, "Subsession is NULL, something went wrong :(\n");
    }

    line = subsession->sdpLines();
    linepointer.append(line);
    if (line != NULL)
    {
      fprintf(stderr, "Her har vi en line fra en subsession: %s\n", line);
    }
    else
    {
      fprintf(stderr, "Line == NULL unfortunately :/\n");
    }
    closeSessionPointer = (DenseSession *)commonDenseTable->Lookup(std::to_string(++teller).c_str());
  }

  fprintf(stderr, "outside of the whileloop in getQualityLevelsSDP and have assembled:\n%s\n", linepointer.c_str());
}

void DenseRTSPServer::make(int level)
{
  UsageEnvironment &env = envir();

  env << "Make 'DenseSession':\n" 
      << "\tlevel: " << level << "\n" 
      << "\tname: " << fAlias.c_str() << "\n" 
      << "\tstartPort: " << fStartPort << "\n";

  DenseSession *denseSession = createNewDenseSession(this);

  // Create 'ServerMediaSession'
  std::string denseName = fAlias + std::to_string(level);
  env << "\tdenseName: \"" << denseName.c_str() << "\"\n";

  ServerMediaSession *sms = ServerMediaSession::createNew(
      env,
      denseName.c_str(),
      NULL,
      "Session streamed by \"denseServer\"",
      False,
      "a=x-qt-text-misc:yo yo mood"); // TODO: Change "yo yo mood" to something more appropriate
  addServerMediaSession(sms);

  char *url = rtspURL(sms);
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;

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
  env << "Make 'VideoSink'\n";

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
  env << "Make 'RTCPInstance'\n";

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
  // Note: This starts RTCP running automatically

  // Make 'PassiveServerMediaSubsession'
  env << "Make 'PassiveServerMediaSubsession'\n";
  sms->addSubsession(PassiveServerMediaSubsession::createNew(
      (RTPSink &)*denseSession->fVideoSink,
      denseSession->fRTCP));

  // Make 'CheckSource'
  env << "Make 'CheckSource'\n";

  // TODO: move hard coded values?
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
    break;
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
  denseSession->fVideoSink->setCheckSource(fileSource);
  // fileSource->removeLookAside(); // TODO: Do we need this?

  // Make 'videoSource'
  MPEG2TransportStreamFramer *videoSource = MPEG2TransportStreamFramer::createNew(env, denseSession->fFileSource);
  denseSession->setVideoSource(videoSource);
  // videoSource->removeLookAside(); // TODO: Do we need this?

  env << "Start Playing\n";

  denseSession->fVideoSink->startPlaying(*videoSource, afterPlaying1, denseSession->fVideoSink);

  env << "Add DenseSession to DenseTable\n";

  // std::string key = "makeStream" + std::to_string(level);
  // fprintf(stderr, "key: \"%s\"\n", key.c_str());
  fDenseTable->Add(std::to_string(level).c_str(), denseSession);
  commonDenseTable = fDenseTable;

  env << "Make finished\n";
}

ServerMediaSession *DenseRTSPServer::findSession(char const *streamName)
{

  int i = 0;

  DenseSession *session = (DenseSession *)fDenseTable->Lookup(std::to_string(i).c_str());

  while (session != NULL)
  {
    if (session->fServerMediaSession->streamName() != NULL || streamName != NULL)
    {
      fprintf(stderr, "\nlooking for match: %s and %s with i: %d\n", session->fServerMediaSession->streamName(), streamName, i);

      if (strcmp(session->fServerMediaSession->streamName(), streamName) == 0)
      {
        fprintf(stderr, "\nfound a match: %s and %s\n", session->fServerMediaSession->streamName(), streamName);

        return session->fServerMediaSession;

        if (strcmp(session->fServerMediaSession->streamName(), "makeStream1") == 0)
        {
          exit(0);
        }
      }
    }
    session = (DenseSession *)fDenseTable->Lookup(std::to_string(i++).c_str());
  }

  return NULL;
}

// TODO: implement
void DenseRTSPServer::afterPlaying1(void * /*clientData*/)
{
  // TODO: Reimplement CommonDenseTable?
  for (int i = 0; i < 3; i++) // Note: Having 3 hardcoded is a bad idea.
  {
    DenseSession *denseSession = (DenseSession *)commonDenseTable->Lookup(std::to_string(i).c_str());
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
  std::string newstream = "makeStream";

  DenseRTSPServer *rtspServer = DenseRTSPServer::createNew(
      envir(),
      htons(fStartPort) + 10,
      NULL,
      65U,
      False, // Note: Repaced NULL with False
      fLevels,
      fPath,
      std::to_string(fFPS),
      newstream);

  fNextServer = rtspServer;

  fprintf(stderr, "makeNextTuple() startPort: %d\n", fStartPort);
  sleep(2); // TODO: Remove?
}

///// DenseSession /////

DenseRTSPServer::DenseSession *DenseRTSPServer::createNewDenseSession(DenseRTSPServer *denseServer)
{
  UsageEnvironment &env = envir();
  env << "Create new 'DenseSession'\n";
  return new DenseSession(denseServer);
}

DenseRTSPServer::DenseSession::DenseSession(DenseRTSPServer *denseRTSPServer)
    : fRTPGroupsock(NULL), fRTCPGroupsock(NULL), fVideoSink(NULL),
      fRTCP(NULL), fPassiveSession(NULL), fServerMediaSession(NULL),
      fFileSource(NULL), fVideoSource(NULL), fDenseServer(denseRTSPServer)
{
}

DenseRTSPServer::DenseSession::~DenseSession()
{
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

////// 'DenseRTSPClientConnection' //////
DenseRTSPServer::DenseRTSPClientConnection *DenseRTSPServer::createNewClientConnection(
    int clientSocket,
    struct sockaddr_in clientAddr)
{
  return new DenseRTSPClientConnection(*this, clientSocket, clientAddr);
}

DenseRTSPServer::DenseRTSPClientConnection::DenseRTSPClientConnection(
    DenseRTSPServer &ourServer,
    int clientSocket,
    struct sockaddr_in clientAddr)
    : RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr),
      fDenseRTSPServer(ourServer),
      fDenseRTSPClientSession(NULL)
{
  resetRequestBuffer();
}

DenseRTSPServer::DenseRTSPClientConnection::~DenseRTSPClientConnection()
{
  closeSocketsRTSP();
}

// TODO: Cleanup
void DenseRTSPServer::DenseRTSPClientConnection::handleRequestBytes(int newBytesRead)
{
  fprintf(stderr, "RTSPDenseClientConnection::handleRequestBytes\n");

  int numBytesRemaining = 0;
  ++fRecursionCount;

  do
  {
    DenseRTSPServer::DenseRTSPClientSession *clientSession = NULL;

    if (newBytesRead < 0 || (unsigned)newBytesRead >= fRequestBufferBytesLeft)
    {
      // Either the client socket has died, or the request was too big for us.
      // Terminate this connection:
      fIsActive = False;
      break;
    }
    Boolean endOfMsg = False;
    unsigned char *ptr = &fRequestBuffer[fRequestBytesAlreadySeen];

    if (fClientOutputSocket != fClientInputSocket && numBytesRemaining == 0)
    {
      // We're doing RTSP-over-HTTP tunneling, and input commands are assumed to have been Base64-encoded.
      // We therefore Base64-decode as much of this new data as we can (i.e., up to a multiple of 4 bytes).

      // But first, we remove any whitespace that may be in the input data:
      unsigned toIndex = 0;
      for (int fromIndex = 0; fromIndex < newBytesRead; ++fromIndex)
      {
        char c = ptr[fromIndex];
        if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n'))
        { // not 'whitespace': space,tab,CR,NL
          ptr[toIndex++] = c;
        }
      }
      newBytesRead = toIndex;

      unsigned numBytesToDecode = fBase64RemainderCount + newBytesRead;
      unsigned newBase64RemainderCount = numBytesToDecode % 4;
      numBytesToDecode -= newBase64RemainderCount;
      if (numBytesToDecode > 0)
      {
        ptr[newBytesRead] = '\0';
        unsigned decodedSize;
        unsigned char *decodedBytes = base64Decode((char const *)(ptr - fBase64RemainderCount), numBytesToDecode, decodedSize);
#ifdef DEBUG
        fprintf(stderr, "Base64-decoded %d input bytes into %d new bytes:", numBytesToDecode, decodedSize);
        for (unsigned k = 0; k < decodedSize; ++k)
          fprintf(stderr, "%c", decodedBytes[k]);
        fprintf(stderr, "\n");
#endif

        // Copy the new decoded bytes in place of the old ones (we can do this because there are fewer decoded bytes than original):
        unsigned char *to = ptr - fBase64RemainderCount;
        for (unsigned i = 0; i < decodedSize; ++i)
        {
          *to++ = decodedBytes[i];
        }

        // Then copy any remaining (undecoded) bytes to the end:
        for (unsigned j = 0; j < newBase64RemainderCount; ++j)
        {
          *to++ = (ptr - fBase64RemainderCount + numBytesToDecode)[j];
        }

        newBytesRead = decodedSize - fBase64RemainderCount + newBase64RemainderCount;
        // adjust to allow for the size of the new decoded data (+ remainder)
        delete[] decodedBytes;
      }
      fBase64RemainderCount = newBase64RemainderCount;
    }

    unsigned char *tmpPtr = fLastCRLF + 2;
    if (fBase64RemainderCount == 0)
    { // no more Base-64 bytes remain to be read/decoded
      // Look for the end of the message: <CR><LF><CR><LF>
      if (tmpPtr < fRequestBuffer)
      {
        tmpPtr = fRequestBuffer;
      }
      while (tmpPtr < &ptr[newBytesRead - 1])
      {
        if (*tmpPtr == '\r' && *(tmpPtr + 1) == '\n')
        {
          if (tmpPtr - fLastCRLF == 2)
          { // This is it:
            endOfMsg = True;
            break;
          }
          fLastCRLF = tmpPtr;
        }
        ++tmpPtr;
      }
    }

    fRequestBufferBytesLeft -= newBytesRead;
    fRequestBytesAlreadySeen += newBytesRead;

    if (!endOfMsg)
    {
      break; // subsequent reads will be needed to complete the request
    }

    // Parse the request string into command name and 'CSeq', then handle the command:
    fRequestBuffer[fRequestBytesAlreadySeen] = '\0';
    char cmdName[RTSP_PARAM_STRING_MAX];
    char urlPreSuffix[RTSP_PARAM_STRING_MAX];
    char urlSuffix[RTSP_PARAM_STRING_MAX];
    char cseq[RTSP_PARAM_STRING_MAX];
    char sessionIdStr[RTSP_PARAM_STRING_MAX];
    unsigned contentLength = 0;
    Boolean playAfterSetup = False;
    fLastCRLF[2] = '\0'; // temporarily, for parsing

    Boolean parseSucceeded = parseRTSPRequestString(
        (char *)fRequestBuffer,
        fLastCRLF + 2 - fRequestBuffer,
        cmdName,
        sizeof cmdName,
        urlPreSuffix,
        sizeof urlPreSuffix,
        urlSuffix,
        sizeof urlSuffix,
        cseq,
        sizeof cseq,
        sessionIdStr,
        sizeof sessionIdStr,
        contentLength);

    fLastCRLF[2] = '\r'; // restore its value
    // Check first for a bogus "Content-Length" value that would cause a pointer wraparound:
    if (tmpPtr + 2 + contentLength < tmpPtr + 2)
    {
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() returned a bogus \"Content-Length:\" value: 0x%x (%d)\n", contentLength, (int)contentLength);
#endif
      contentLength = 0;
      parseSucceeded = False;
    }
    if (parseSucceeded)
    {
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() succeeded, returning cmdName \"%s\", urlPreSuffix \"%s\", urlSuffix \"%s\", CSeq \"%s\", Content-Length %u, with %d bytes following the message.\n", cmdName, urlPreSuffix, urlSuffix, cseq, contentLength, ptr + newBytesRead - (tmpPtr + 2));
#endif
      // If there was a "Content-Length:" header, then make sure we've received all of the data that it specified:
      if (ptr + newBytesRead < tmpPtr + 2 + contentLength)
      {
        break; // we still need more data; subsequent reads will give it to us
      }

      // If the request included a "Session:" id, and it refers to a client session that's
      // current ongoing, then use this command to indicate 'liveness' on that client session:
      Boolean const requestIncludedSessionId = sessionIdStr[0] != '\0';
      if (requestIncludedSessionId)
      {
        fprintf(stderr, "handleRequestBytes -> lookupClientSession\n");
        clientSession = (DenseRTSPServer::DenseRTSPClientSession *)(fDenseRTSPServer.lookupClientSession(sessionIdStr));
        if (clientSession != NULL)
        {
          clientSession->noteLiveness();
        }
      }

      // We now have a complete RTSP request.
      // Handle the specified command (beginning with commands that are session-independent):

      fCurrentCSeq = cseq;
      if (strcmp(cmdName, "OPTIONS") == 0)
      {
        fprintf(stderr, "OPTIONS\n");
        // If the "OPTIONS" command included a "Session:" id for a session that doesn't exist,
        // then treat this as an error:
        if (requestIncludedSessionId && clientSession == NULL)
        {
#ifdef DEBUG
          fprintf(stderr, "Calling handleCmd_sessionNotFound() (case 1)\n");
#endif
          handleCmd_sessionNotFound();
        }
        else
        {
          // Normal case:
          handleCmd_OPTIONS(urlSuffix);
        }
      }
      else if (urlPreSuffix[0] == '\0' && urlSuffix[0] == '*' && urlSuffix[1] == '\0')
      {
        // The special "*" URL means: an operation on the entire server.  This works only for GET_PARAMETER and SET_PARAMETER:
        if (strcmp(cmdName, "GET_PARAMETER") == 0)
        {
          fprintf(stderr, "GET_PARAMETER\n");
          handleCmd_GET_PARAMETER((char const *)fRequestBuffer);
        }
        else if (strcmp(cmdName, "SET_PARAMETER") == 0)
        {
          fprintf(stderr, "SET_PARAMETER\n");
          handleCmd_SET_PARAMETER((char const *)fRequestBuffer);
        }
        else
        {
          handleCmd_notSupported();
        }
      }
      else if (strcmp(cmdName, "DESCRIBE") == 0)
      {
        fprintf(stderr, "DESCRIBE\n");
        handleCmd_DESCRIBE(urlPreSuffix, urlSuffix, (char const *)fRequestBuffer);
      }
      else if (strcmp(cmdName, "SETUP") == 0)
      {
        Boolean areAuthenticated = True;
        fprintf(stderr, "cmd name is setup\n");

        if (!requestIncludedSessionId)
        {
          // No session id was present in the request.
          // So create a new "RTSPClientSession" object for this request.

          // But first, make sure that we're authenticated to perform this command:
          char urlTotalSuffix[2 * RTSP_PARAM_STRING_MAX];
          // enough space for urlPreSuffix/urlSuffix'\0'
          urlTotalSuffix[0] = '\0';
          if (urlPreSuffix[0] != '\0')
          {
            strcat(urlTotalSuffix, urlPreSuffix);
            strcat(urlTotalSuffix, "/");
          }
          strcat(urlTotalSuffix, urlSuffix);
          if (authenticationOK("SETUP", urlTotalSuffix, (char const *)fRequestBuffer))
          {
            fprintf(stderr, "createnewClientsession\n");
            clientSession = (DenseRTSPServer::DenseRTSPClientSession *)fDenseRTSPServer.createNewClientSessionWithId();
          }
          else
          {
            areAuthenticated = False;
            fprintf(stderr, "not authenticated\n");
          }
        }
        if (clientSession != NULL)
        {
          clientSession->handleCmd_SETUP_1(this, urlPreSuffix, urlSuffix, (char const *)fRequestBuffer);
          playAfterSetup = clientSession->fStreamAfterSETUP;
        }
        else if (areAuthenticated)
        {
          handleCmd_sessionNotFound();
        }
      }
      else if (strcmp(cmdName, "TEARDOWN") == 0 || strcmp(cmdName, "PLAY") == 0 || strcmp(cmdName, "PAUSE") == 0 || strcmp(cmdName, "GET_PARAMETER") == 0 || strcmp(cmdName, "SET_PARAMETER") == 0)
      {
        if (clientSession != NULL)
        {
          fprintf(stderr, "handleCMD within session\n");
          clientSession->handleCmd_withinSession(this, cmdName, urlPreSuffix, urlSuffix, (char const *)fRequestBuffer);
        }
        else
        {
#ifdef DEBUG
          fprintf(stderr, "Calling handleCmd_sessionNotFound() (case 3)\n");
#endif
          handleCmd_sessionNotFound();
        }
      }
      else if (strcmp(cmdName, "REGISTER") == 0 || strcmp(cmdName, "DEREGISTER") == 0)
      {
        // Because - unlike other commands - an implementation of this command needs
        // the entire URL, we re-parse the command to get it:
        char *url = strDupSize((char *)fRequestBuffer);
        if (sscanf((char *)fRequestBuffer, "%*s %s", url) == 1)
        {
          // Check for special command-specific parameters in a "Transport:" header:
          Boolean reuseConnection, deliverViaTCP;
          char *proxyURLSuffix;
          parseTransportHeaderForREGISTER((const char *)fRequestBuffer, reuseConnection, deliverViaTCP, proxyURLSuffix);

          handleCmd_REGISTER(cmdName, url, urlSuffix, (char const *)fRequestBuffer, reuseConnection, deliverViaTCP, proxyURLSuffix);
          delete[] proxyURLSuffix;
        }
        else
        {
          handleCmd_bad();
        }
        delete[] url;
      }
      else
      {
        // The command is one that we don't handle:
        handleCmd_notSupported();
      }
    }
    else
    {
      // fprintf(stderr, "parse-failed\n");
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() failed; checking now for HTTP commands (for RTSP-over-HTTP tunneling)...\n");
#endif
      // The request was not (valid) RTSP, but check for a special case: HTTP commands (for setting up RTSP-over-HTTP tunneling):
      char sessionCookie[RTSP_PARAM_STRING_MAX];
      char acceptStr[RTSP_PARAM_STRING_MAX];
      *fLastCRLF = '\0'; // temporarily, for parsing
      parseSucceeded = parseHTTPRequestString(cmdName, sizeof cmdName,
                                              urlSuffix, sizeof urlPreSuffix,
                                              sessionCookie, sizeof sessionCookie,
                                              acceptStr, sizeof acceptStr);
      *fLastCRLF = '\r';
      if (parseSucceeded)
      {
#ifdef DEBUG
        fprintf(stderr, "parseHTTPRequestString() succeeded, returning cmdName \"%s\", urlSuffix \"%s\", sessionCookie \"%s\", acceptStr \"%s\"\n", cmdName, urlSuffix, sessionCookie, acceptStr);
#endif
        // Check that the HTTP command is valid for RTSP-over-HTTP tunneling: There must be a 'session cookie'.
        Boolean isValidHTTPCmd = True;
        if (strcmp(cmdName, "OPTIONS") == 0)
        {
          handleHTTPCmd_OPTIONS();
        }
        else if (sessionCookie[0] == '\0')
        {
          // There was no "x-sessioncookie:" header.  If there was an "Accept: application/x-rtsp-tunnelled" header,
          // then this is a bad tunneling request.  Otherwise, assume that it's an attempt to access the stream via HTTP.
          if (strcmp(acceptStr, "application/x-rtsp-tunnelled") == 0)
          {
            isValidHTTPCmd = False;
          }
          else
          {
            handleHTTPCmd_StreamingGET(urlSuffix, (char const *)fRequestBuffer);
          }
        }
        else if (strcmp(cmdName, "GET") == 0)
        {
          handleHTTPCmd_TunnelingGET(sessionCookie);
        }
        else if (strcmp(cmdName, "POST") == 0)
        {
          // We might have received additional data following the HTTP "POST" command - i.e., the first Base64-encoded RTSP command.
          // Check for this, and handle it if it exists:
          unsigned char const *extraData = fLastCRLF + 4;
          unsigned extraDataSize = &fRequestBuffer[fRequestBytesAlreadySeen] - extraData;
          if (handleHTTPCmd_TunnelingPOST(sessionCookie, extraData, extraDataSize))
          {
            // We don't respond to the "POST" command, and we go away:
            fIsActive = False;
            break;
          }
        }
        else
        {
          isValidHTTPCmd = False;
        }
        if (!isValidHTTPCmd)
        {
          handleHTTPCmd_notSupported();
        }
      }
      else
      {
#ifdef DEBUG
        fprintf(stderr, "parseHTTPRequestString() failed!\n");
#endif
        handleCmd_bad();
      }
    }

    fprintf(stderr, "sending response: %s", fResponseBuffer);
    send(fClientOutputSocket, (char const *)fResponseBuffer, strlen((char *)fResponseBuffer), 0);

    if (playAfterSetup)
    {
      // The client has asked for streaming to commence now, rather than after a
      // subsequent "PLAY" command.  So, simulate the effect of a "PLAY" command:
      clientSession->handleCmd_withinSession(this, "PLAY", urlPreSuffix, urlSuffix, (char const *)fRequestBuffer);
    }

    // Check whether there are extra bytes remaining in the buffer, after the end of the request (a rare case).
    // If so, move them to the front of our buffer, and keep processing it, because it might be a following, pipelined request.
    unsigned requestSize = (fLastCRLF + 4 - fRequestBuffer) + contentLength;
    numBytesRemaining = fRequestBytesAlreadySeen - requestSize;
    resetRequestBuffer(); // to prepare for any subsequent request

    if (numBytesRemaining > 0)
    {
      memmove(fRequestBuffer, &fRequestBuffer[requestSize], numBytesRemaining);
      newBytesRead = numBytesRemaining;
    }
  } while (numBytesRemaining > 0);

  --fRecursionCount;
  if (!fIsActive)
  {
    if (fRecursionCount > 0)
      closeSockets();
    else
      delete this;
    // Note: The "fRecursionCount" test is for a pathological situation where we reenter the event loop and get called recursively
    // while handling a command (e.g., while handling a "DESCRIBE", to get a SDP description).
    // In such a case we don't want to actually delete ourself until we leave the outermost call.
  }

  fprintf(stderr, "DenseRTSPClientConnection::handleRequestBytes - end\n");
}

void DenseRTSPServer::DenseRTSPClientConnection::handleCmd_OPTIONS(char *urlSuffix)
{
  UsageEnvironment &env = envir();
  env << "\n###### handleCmd_OPTIONS ######\n";

  // Note: Unused
  // ServerMediaSession *session = fOurServer.lookupServerMediaSession(urlSuffix);

  snprintf((char *)fResponseBuffer, sizeof fResponseBuffer,
           "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
           fCurrentCSeq, dateHeader(), fDenseRTSPServer.allowedCommandNames());
}

// TODO: Cleanup
void DenseRTSPServer::DenseRTSPClientConnection::handleCmd_DESCRIBE(
    char const *urlPreSuffix,
    char const *urlSuffix,
    char const *fullRequestStr)
{
  fprintf(stderr, "/////////////// YOUR DESCRIBE 1 /////////////\n");

  ServerMediaSession *session = NULL;
  char *sdpDescription = NULL;
  char *rtspURL = NULL;
  do
  {
    fprintf(stderr, "/////////////// YOUR DESCRIBE 2 /////////////\n");

    char urlTotalSuffix[2 * RTSP_PARAM_STRING_MAX];
    // enough space for urlPreSuffix/urlSuffix'\0'
    urlTotalSuffix[0] = '\0';
    if (urlPreSuffix[0] != '\0')
    {
      strcat(urlTotalSuffix, urlPreSuffix);
      strcat(urlTotalSuffix, "/");
    }
    strcat(urlTotalSuffix, urlSuffix);

    fprintf(stderr, "     Have assembled url-total-suffix: %s\n", urlTotalSuffix);

    if (!authenticationOK("DESCRIBE", urlTotalSuffix, fullRequestStr))
    {
      break;
    }

    // We should really check that the request contains an "Accept:" #####
    // for "application/sdp", because that's what we're sending back #####

    unsigned sdpDescriptionSize;

    int i = 0;
    DenseSession *closeSessionPointer;
    DenseRTSPServer *correct = &fDenseRTSPServer;

    while (correct->fNextServer != NULL)
    {
      correct = correct->fNextServer;
      i++;
    }

    DenseRTSPServer *cast = (DenseRTSPServer *)correct;
    closeSessionPointer = (DenseSession *)cast->fDenseTable->Lookup((char const *)0);
    fprintf(stderr, "\nTrying to find the correct tuple to return on the describe: %d\n", i);

    // Begin by looking up the "ServerMediaSession" object for the specified "urlTotalSuffix":
    session = closeSessionPointer->fServerMediaSession;

    if (session == NULL)
    {
      fprintf(stderr, "This is describe -> the lookupservermediasession = NULL\n");
      handleCmd_notFound();
      break;
    }

    // Note: Unused
    // int fNumStreamStates = session->numSubsessions();

    // Increment the "ServerMediaSession" object's reference count, in case someone removes it
    // while we're using it:
    session->incrementReferenceCount();

    // Then, assemble a SDP description for this session:
    sdpDescription = session->generateSDPDescription();
    if (sdpDescription == NULL)
    {
      // This usually means that a file name that was specified for a
      // "ServerMediaSubsession" does not exist.
      setRTSPResponse("404 File Not Found, Or In Incorrect Format");
      break;
    }

    sdpDescriptionSize = strlen(sdpDescription);

    fprintf(stderr, "   have assembled a sdpDescription: \n%s\n", sdpDescription);

    std::string fetch;
    fDenseRTSPServer.getQualityLevelSDP(fetch);

    fprintf(stderr, "   have assembled a sdplevels: \n%s\n", fetch.c_str());

    sdpDescriptionSize = strlen(sdpDescription) + fetch.size();

    // Also, generate our RTSP URL, for the "Content-Base:" header
    // (which is necessary to ensure that the correct URL gets used in subsequent "SETUP" requests).
    rtspURL = fDenseRTSPServer.rtspURL(session, fClientInputSocket);

    snprintf((char *)fResponseBuffer, sizeof fResponseBuffer,
             "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
             "%s"
             "Content-Base: %s/\r\n"
             "Content-Type: application/sdp\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s"
             "%s",
             fCurrentCSeq,
             dateHeader(),
             rtspURL,
             sdpDescriptionSize,
             sdpDescription, fetch.c_str());

    fprintf(stderr, "   the total in the fResponseBuffer is now%s\n", (char *)fResponseBuffer);

  } while (0);

  if (session != NULL)
  {
    // Decrement its reference count, now that we're done using it:
    session->decrementReferenceCount();
    if (session->referenceCount() == 0 && session->deleteWhenUnreferenced())
    {
      fOurServer.removeServerMediaSession(session);
    }
  }

  delete[] sdpDescription;
  delete[] rtspURL;
}

////// 'DenseRTSPClientSession' //////
DenseRTSPServer::DenseRTSPClientSession *DenseRTSPServer::createNewClientSession(u_int32_t sessionId)
{
  return new DenseRTSPClientSession(*this, sessionId);
}

DenseRTSPServer::DenseRTSPClientSession::DenseRTSPClientSession(
    DenseRTSPServer &ourServer,
    u_int32_t sessionId)
    : RTSPServer::RTSPClientSession(ourServer, sessionId),
      fDenseRTSPServer(ourServer),
      fDenseRTSPClientConnection(NULL),
      fNewDenseRequest(False)
{
}

DenseRTSPServer::DenseRTSPClientSession::~DenseRTSPClientSession()
{
  reclaimStreamStates();
}

// TODO: Cleanup
void DenseRTSPServer::DenseRTSPClientSession::handleCmd_SETUP_1(
    DenseRTSPServer::DenseRTSPClientConnection *ourClientConnection,
    char const *urlPreSuffix,
    char const *urlSuffix,
    char const *fullRequestStr)
{
  // Normally, "urlPreSuffix" should be the session (stream) name, and "urlSuffix" should be the subsession (track) name.
  // However (being "liberal in what we accept"), we also handle 'aggregate' SETUP requests (i.e., without a track name),
  // in the special case where we have only a single track.  I.e., in this case, we also handle:
  //    "urlPreSuffix" is empty and "urlSuffix" is the session (stream) name, or
  //    "urlPreSuffix" concatenated with "urlSuffix" (with "/" inbetween) is the session (stream) name.
  char const *streamName = urlPreSuffix; // in the normal case
  char const *trackId = urlSuffix;       // in the normal case
  char *concatenatedStreamName = NULL;   // in the normal case

  fprintf(stderr, "############### DIN SETUP ###############\n");
  fprintf(stderr, "urlPreSuffix(streamname): %s        urlSuffix(subsession/track): %s \n", urlPreSuffix, urlSuffix);

  do
  {
    // First, make sure the specified stream name exists:
    ServerMediaSession *sms = fOurServer.lookupServerMediaSession(streamName, fOurServerMediaSession == NULL);
    if (sms == NULL)
    {
      fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL first time -> %s\n", streamName);
      // Check for the special case (noted above), before we give up:
      if (urlPreSuffix[0] == '\0')
      {
        streamName = urlSuffix;
      }
      else
      {
        concatenatedStreamName = new char[strlen(urlPreSuffix) + strlen(urlSuffix) + 2]; // allow for the "/" and the trailing '\0'
        sprintf(concatenatedStreamName, "%s/%s", urlPreSuffix, urlSuffix);
        streamName = concatenatedStreamName;
      }
      trackId = NULL;

      // Check again:
      fprintf(stderr, "   handleCmd_SETUP Server Media Session = checking the concatenated stream name: %s\n", streamName);
      sms = fOurServer.lookupServerMediaSession(streamName, fOurServerMediaSession == NULL);
    }
    if (sms == NULL)
    {
      if (fOurServerMediaSession == NULL)
      {
        // The client asked for a stream that doesn't exist (and this session descriptor has not been used before):
        fprintf(stderr, "   handleCmd_SETUP Server Media Session = fOurServerMediaSession == NULL: %s\n", streamName);
        ourClientConnection->handleCmd_notFound();
      }
      else
      {
        // The client asked for a stream that doesn't exist, but using a stream id for a stream that does exist. Bad request:
        fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL badRequest()\n");
        ourClientConnection->handleCmd_bad();
      }

      break;
    }
    else
    {
      fprintf(stderr, "   handleCmd_SETUP Server Media Session -> sms is not NULL!\n");
      if (fOurServerMediaSession == NULL)
      {
        // We're accessing the "ServerMediaSession" for the first time.
        fprintf(stderr, "   handleCmd_SETUP Server Media Session -> fOurServerMediaSession = sms;\n");
        fOurServerMediaSession = sms;
        fprintf(stderr, "   handleCmd_SETUP Server Media Session -> fOurServerMediaSession->incrementReferenceCount();\n");
        fOurServerMediaSession->incrementReferenceCount();
      }
      else if (sms != fOurServerMediaSession)
      {
        // The client asked for a stream that's different from the one originally requested for this stream id.  Bad request:
        fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL badRequest()\n");
        ourClientConnection->handleCmd_bad();

        break;
      }
    }

    if (fStreamStates == NULL)
    {
      // This is the first "SETUP" for this session.  Set up our array of states for all of this session's subsessions (tracks):
      // fprintf(stderr, "   This is the first SETUP for this session.  Set up our array of states for all tracks\n");

      fNumStreamStates = fOurServerMediaSession->numSubsessions();
      // fprintf(stderr, "         fNumStreamStates: %d\n", fNumStreamStates);
      fStreamStates = new struct streamState[fNumStreamStates];

      ServerMediaSubsessionIterator iter(*fOurServerMediaSession);
      ServerMediaSubsession *subsession;
      for (unsigned i = 0; i < fNumStreamStates; ++i)
      {
        subsession = iter.next();
        fStreamStates[i].subsession = subsession;
        fStreamStates[i].tcpSocketNum = -1;  // for now; may get set for RTP-over-TCP streaming
        fStreamStates[i].streamToken = NULL; // for now; it may be changed by the "getStreamParameters()" call that comes later
      }
    }

    // Look up information for the specified subsession (track):
    ServerMediaSubsession *subsession = NULL;
    unsigned trackNum;
    if (trackId != NULL && trackId[0] != '\0')
    { // normal case
      for (trackNum = 0; trackNum < fNumStreamStates; ++trackNum)
      {
        subsession = fStreamStates[trackNum].subsession;
        if (subsession != NULL && strcmp(trackId, subsession->trackId()) == 0)
        {
          break;
        }
      }
      if (trackNum >= fNumStreamStates)
      {
        // The specified track id doesn't exist, so this request fails:
        ourClientConnection->handleCmd_notFound();
        break;
      }
    }
    else
    {
      // Weird case: there was no track id in the URL.
      // This works only if we have only one subsession:
      if (fNumStreamStates != 1 || fStreamStates[0].subsession == NULL)
      {
        ourClientConnection->handleCmd_bad();
        break;
      }
      trackNum = 0;
      subsession = fStreamStates[trackNum].subsession;
    }
    // ASSERT: subsession != NULL

    void *&token = fStreamStates[trackNum].streamToken; // alias
    if (token != NULL)
    {
      // We already handled a "SETUP" for this track (to the same client),
      // so stop any existing streaming of it, before we set it up again:
      subsession->pauseStream(fOurSessionId, token);
      // fDenseRTSPServer.unnoteTCPStreamingOnSocket(fStreamStates[trackNum].tcpSocketNum, this, trackNum);
      subsession->deleteStream(fOurSessionId, token);
    }

    // Look for a "Transport:" header in the request string, to extract client parameters:
    // fprintf(stderr, "   handleCmd_SETUP Looking for client parameters\n");
    StreamingMode streamingMode;
    char *streamingModeString = NULL; // set when RAW_UDP streaming is specified
    char *clientsDestinationAddressStr;
    u_int8_t clientsDestinationTTL;
    portNumBits clientRTPPortNum, clientRTCPPortNum;
    unsigned char rtpChannelId, rtcpChannelId;
    parseTransportHeader(fullRequestStr, streamingMode, streamingModeString,
                         clientsDestinationAddressStr, clientsDestinationTTL,
                         clientRTPPortNum, clientRTCPPortNum,
                         rtpChannelId, rtcpChannelId);
    if ((streamingMode == RTP_TCP && rtpChannelId == 0xFF) ||
        (streamingMode != RTP_TCP && ourClientConnection->fClientOutputSocket != ourClientConnection->fClientInputSocket))
    {
      // An anomalous situation, caused by a buggy client.  Either:
      //     1/ TCP streaming was requested, but with no "interleaving=" fields.  (QuickTime Player sometimes does this.), or
      //     2/ TCP streaming was not requested, but we're doing RTSP-over-HTTP tunneling (which implies TCP streaming).
      // In either case, we assume TCP streaming, and set the RTP and RTCP channel ids to proper values:
      streamingMode = RTP_TCP;
      rtpChannelId = fTCPStreamIdCount;
      rtcpChannelId = fTCPStreamIdCount + 1;
    }
    if (streamingMode == RTP_TCP)
    {
      fTCPStreamIdCount += 2;
    }

    Port clientRTPPort(clientRTPPortNum);
    Port clientRTCPPort(clientRTCPPortNum);

    // Next, check whether a "Range:" or "x-playNow:" header is present in the request.
    // This isn't legal, but some clients do this to combine "SETUP" and "PLAY":
    double rangeStart = 0.0, rangeEnd = 0.0;
    char *absStart = NULL;
    char *absEnd = NULL;
    Boolean startTimeIsNow;
    if (parseRangeHeader(fullRequestStr, rangeStart, rangeEnd, absStart, absEnd, startTimeIsNow))
    {
      delete[] absStart;
      delete[] absEnd;
      fStreamAfterSETUP = True;
    }
    else if (parsePlayNowHeader(fullRequestStr))
    {
      fStreamAfterSETUP = True;
    }
    else
    {
      fStreamAfterSETUP = False;
    }

    // Then, get server parameters from the 'subsession':
    if (streamingMode == RTP_TCP)
    {
      // Note that we'll be streaming over the RTSP TCP connection:
      fStreamStates[trackNum].tcpSocketNum = ourClientConnection->fClientOutputSocket;
      // fDenseRTSPServer.noteTCPStreamingOnSocket(fStreamStates[trackNum].tcpSocketNum, this, trackNum);
    }
    netAddressBits destinationAddress = 0;
    u_int8_t destinationTTL = 255;
#ifdef RTSP_ALLOW_CLIENT_DESTINATION_SETTING
    if (clientsDestinationAddressStr != NULL)
    {
      // Use the client-provided "destination" address.
      // Note: This potentially allows the server to be used in denial-of-service
      // attacks, so don't enable this code unless you're sure that clients are
      // trusted.
      destinationAddress = our_inet_addr(clientsDestinationAddressStr);
    }
    // Also use the client-provided TTL.
    destinationTTL = clientsDestinationTTL;
#endif
    delete[] clientsDestinationAddressStr;
    Port serverRTPPort(0);
    Port serverRTCPPort(0);

    // Make sure that we transmit on the same interface that's used by the client (in case we're a multi-homed server):
    struct sockaddr_in sourceAddr;
    socklen_t namelen = sizeof sourceAddr;
    getsockname(ourClientConnection->fClientInputSocket, (struct sockaddr *)&sourceAddr, &namelen);
    netAddressBits origSendingInterfaceAddr = SendingInterfaceAddr;
    netAddressBits origReceivingInterfaceAddr = ReceivingInterfaceAddr;

    char sessionIdStr[8 + 1];
    sprintf(sessionIdStr, "%08X", fOurSessionId);
    sessionIdStr[8] = '\0';

    subsession->getStreamParameters(
        fOurSessionId,
        ourClientConnection->fClientAddr.sin_addr.s_addr,
        clientRTPPort,
        clientRTCPPort,
        fStreamStates[trackNum].tcpSocketNum,
        rtpChannelId,
        rtcpChannelId,
        destinationAddress,
        destinationTTL,
        fIsMulticast,
        serverRTPPort,
        serverRTCPPort,
        fStreamStates[trackNum].streamToken);

    SendingInterfaceAddr = origSendingInterfaceAddr;
    ReceivingInterfaceAddr = origReceivingInterfaceAddr;

    AddressString destAddrStr(destinationAddress);
    AddressString sourceAddrStr(sourceAddr);
    char timeoutParameterString[100];
    if (fDenseRTSPServer.fReclamationSeconds > 0)
    {
      sprintf(timeoutParameterString, ";timeout=%u", fDenseRTSPServer.fReclamationSeconds);
    }
    else
    {
      timeoutParameterString[0] = '\0';
    }
    if (fIsMulticast)
    {
      fprintf(stderr, "   handleCmd_SETUP - this setup is multicast, if you get something else here there is something wrong\n");
      switch (streamingMode)
      {
      case RTP_UDP:
      {
        fprintf(stderr, "   handleCmd_SETUP - this setup is RTP_UDP ok\n");
        snprintf((char *)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%d-%d;ttl=%d\r\n"
                 "Session: %08X%s\r\n\r\n",
                 ourClientConnection->fCurrentCSeq,
                 dateHeader(),
                 destAddrStr.val(), sourceAddrStr.val(), ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()), destinationTTL,
                 fOurSessionId, timeoutParameterString);

        fprintf(stderr, "   handleCmd_SETUP - this setup is RTP_UDP ok\n%s\n", (char *)ourClientConnection->fResponseBuffer);

        break;
      }
      case RTP_TCP:
      {
        // multicast streams can't be sent via TCP
        ourClientConnection->handleCmd_unsupportedTransport();
        break;
      }
      case RAW_UDP:
      {
        snprintf((char *)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: %s;multicast;destination=%s;source=%s;port=%d;ttl=%d\r\n"
                 "Session: %08X%s\r\n\r\n",
                 ourClientConnection->fCurrentCSeq,
                 dateHeader(),
                 streamingModeString, destAddrStr.val(), sourceAddrStr.val(), ntohs(serverRTPPort.num()), destinationTTL,
                 fOurSessionId, timeoutParameterString);
        break;
      }
      }
    }
    else
    {
      switch (streamingMode)
      {
      case RTP_UDP:
      {
        snprintf((char *)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
                 "Session: %08X%s\r\n\r\n",
                 ourClientConnection->fCurrentCSeq,
                 dateHeader(),
                 destAddrStr.val(), sourceAddrStr.val(), ntohs(clientRTPPort.num()), ntohs(clientRTCPPort.num()), ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()),
                 fOurSessionId, timeoutParameterString);
        break;
      }
      case RTP_TCP:
      {
        if (!fDenseRTSPServer.fAllowStreamingRTPOverTCP)
        {
          ourClientConnection->handleCmd_unsupportedTransport();
        }
        else
        {
          snprintf((char *)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
                   "RTSP/1.0 200 OK\r\n"
                   "CSeq: %s\r\n"
                   "%s"
                   "Transport: RTP/AVP/TCP;unicast;destination=%s;source=%s;interleaved=%d-%d\r\n"
                   "Session: %08X%s\r\n\r\n",
                   ourClientConnection->fCurrentCSeq,
                   dateHeader(),
                   destAddrStr.val(), sourceAddrStr.val(), rtpChannelId, rtcpChannelId,
                   fOurSessionId, timeoutParameterString);
        }
        break;
      }
      case RAW_UDP:
      {
        snprintf((char *)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: %s;unicast;destination=%s;source=%s;client_port=%d;server_port=%d\r\n"
                 "Session: %08X%s\r\n\r\n",
                 ourClientConnection->fCurrentCSeq,
                 dateHeader(),
                 streamingModeString, destAddrStr.val(), sourceAddrStr.val(), ntohs(clientRTPPort.num()), ntohs(serverRTPPort.num()),
                 fOurSessionId, timeoutParameterString);
        break;
      }
      }
    }
    delete[] streamingModeString;
  } while (0);

  delete[] concatenatedStreamName;
}

void DenseRTSPServer::DenseRTSPClientSession::handleCmd_SETUP_2(DenseRTSPServer::DenseRTSPClientConnection *ourClientConnection,
                                                                char const *urlPreSuffix, char const *urlSuffix, char const *fullRequestStr)
{
  // Normally, "urlPreSuffix" should be the session (stream) name, and "urlSuffix" should be the subsession (track) name.
  // However (being "liberal in what we accept"), we also handle 'aggregate' SETUP requests (i.e., without a track name),
  // in the special case where we have only a single track.  I.e., in this case, we also handle:
  //    "urlPreSuffix" is empty and "urlSuffix" is the session (stream) name, or
  //    "urlPreSuffix" concatenated with "urlSuffix" (with "/" inbetween) is the session (stream) name.
  char const *streamName = urlPreSuffix; // in the normal case
  char const *trackId = urlSuffix;       // in the normal case
  char *concatenatedStreamName = NULL;   // in the normal case

  fprintf(stderr, "############### DIN SETUP ###############\n");
  fprintf(stderr, "urlPreSuffix(streamname): %s        urlSuffix(subsession/track): %s\n", streamName, trackId);

  do
  {
    // First, make sure the specified stream name exists:

    DenseServerMediaSession *sms = dynamic_cast<DenseServerMediaSession *>(fDenseRTSPServer.findSession(streamName));

    if (sms == NULL)
    {
      fprintf(stderr, "   handleCmd_SETUP Server Media Session = did not find a sms\n");
      ourClientConnection->handleCmd_notFound();
    }
    else
    {
      sms->incrementReferenceCount();
      fprintf(stderr, "   handleCmd_SETUP Server Media Session -> sms != NULL refcount er: %u\n", sms->referenceCount());
    }

    ServerMediaSubsession *subsession = NULL;
    //unsigned trackNum; //Note: unused

    ServerMediaSubsessionIterator iter(*sms);
    subsession = iter.next();
    // iter.reset();

    if (subsession == NULL)
    {
      fprintf(stderr, "there are no subsessions in this\n");
      subsession = sms->fSubsessionsHead;
      if (subsession == NULL)
      {
        fprintf(stderr, "there are REALLY no subsessions in this\n");
        subsession = sms->fSubsessionsHead;
      }
    }

    fprintf(stderr, "  see dynamic cast!\n%s\n", subsession->sdpLines());
    fprintf(stderr, "   handleCmd_SETUP Looking for client parameters\n%s\n", fullRequestStr);

    DensePassiveServerMediaSubsession *cast = dynamic_cast<DensePassiveServerMediaSubsession *>(subsession);
    //netAddressBits destinationAddress = 0; // TODO: Get this from subsession! Note: But is it not used?
    Groupsock gs = cast->gs();
    AddressString groupAddressStr(gs.groupAddress());

    struct sockaddr_in sourceAddr; //
    socklen_t namelen = sizeof sourceAddr;
    getsockname(ourClientConnection->fClientInputSocket, (struct sockaddr *)&sourceAddr, &namelen);

    AddressString sourceAddrStr(sourceAddr);
    Port serverRTPPort(gs.port().num());

    // Groupsock rtcpgs = cast->RTCPgs();
    Port serverRTCPPort(0);
    u_int8_t destinationTTL = 255;

    char timeoutParameterString[100];
    if (fDenseRTSPServer.fReclamationSeconds > 0)
    {
      sprintf(timeoutParameterString, ";timeout=%u", fDenseRTSPServer.fReclamationSeconds);
    }
    else
    {
      timeoutParameterString[0] = '\0';
    }

    snprintf((char *)ourClientConnection->fResponseBuffer, sizeof(ourClientConnection->fResponseBuffer),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %s\r\n"
             "%s"
             "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%d-%d;ttl=%d\r\n"
             "Session: %08X%s\r\n\r\n",
             ourClientConnection->fCurrentCSeq,
             dateHeader(),
             groupAddressStr.val(), sourceAddrStr.val(), serverRTPPort.num(), serverRTCPPort.num(), destinationTTL,
             fOurSessionId, timeoutParameterString);

  } while (0);

  fprintf(stderr, "Sending response to setup\n%s\n\n", (char *)ourClientConnection->fResponseBuffer);

  delete[] concatenatedStreamName;
}

// Misc
Boolean parsePlayNowHeader(char const *buf)
{
  // Find "x-playNow:" header, if present
  while (1)
  {
    if (*buf == '\0')
      return False; // not found
    if (_strncasecmp(buf, "x-playNow:", 10) == 0)
      break;
    ++buf;
  }

  return True;
}

void parseTransportHeader(
    char const *buf,
    StreamingMode &streamingMode,
    char *&streamingModeString,
    char *&destinationAddressStr,
    u_int8_t &destinationTTL,
    portNumBits &clientRTPPortNum,  // if UDP
    portNumBits &clientRTCPPortNum, // if UDP
    unsigned char &rtpChannelId,    // if TCP
    unsigned char &rtcpChannelId    // if TCP
)
{
  // Initialize the result parameters to default values:
  streamingMode = RTP_UDP;
  streamingModeString = NULL;
  destinationAddressStr = NULL;
  destinationTTL = 255;
  clientRTPPortNum = 0;
  clientRTCPPortNum = 1;
  rtpChannelId = rtcpChannelId = 0xFF;

  portNumBits p1, p2;
  unsigned ttl, rtpCid, rtcpCid;
  fprintf(stderr, "     parseTransportHeader() - extracting info on client\n");
  // First, find "Transport:"
  while (1)
  {
    if (*buf == '\0')
      return; // not found
    if (*buf == '\r' && *(buf + 1) == '\n' && *(buf + 2) == '\r')
      return; // end of the headers => not found
    if (_strncasecmp(buf, "Transport:", 10) == 0)
    {
      break;
    }
    ++buf;
  }

  // Transport: RTP/AVP;multicast;port=18888-18889

  // Then, run through each of the fields, looking for ones we handle:
  char const *fields = buf + 10;
  fprintf(stderr, "pling 0: %s\n", fields);
  while (*fields == ' ')
    ++fields;
  char *field = strDupSize(fields);
  while (sscanf(fields, "%[^;\r\n]", field) == 1)
  {
    if (strcmp(field, "RTP/AVP/TCP") == 0)
    {
      fprintf(stderr, "pling 1\n");

      streamingMode = RTP_TCP;
    }
    else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
             strcmp(field, "MP2T/H2221/UDP") == 0)
    {
      fprintf(stderr, "pling 2\n");
      streamingMode = RAW_UDP;
      streamingModeString = strDup(field);
    }
    else if (_strncasecmp(field, "destination=", 12) == 0)
    {
      fprintf(stderr, "pling 3\n");
      delete[] destinationAddressStr;
      destinationAddressStr = strDup(field + 12);
    }
    else if (sscanf(field, "ttl%u", &ttl) == 1)
    {
      fprintf(stderr, "pling 4\n");
      destinationTTL = (u_int8_t)ttl;
    } /*else if (sscanf(field, "port=%hu-%hu", &p1, &p2) == 2) {
      fprintf(stderr, "pling 5\n");

      clientRTPPortNum = p1;
      clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p2; // ignore the second port number if the client asked for raw UDP
    }*/
    else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2)
    {
      fprintf(stderr, "pling 5\n");

      clientRTPPortNum = p1;
      clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p2; // ignore the second port number if the client asked for raw UDP
    }
    else if (sscanf(field, "client_port=%hu", &p1) == 1)
    {
      fprintf(stderr, "pling 6\n");
      clientRTPPortNum = p1;
      clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p1 + 1;
    }
    else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2)
    {
      fprintf(stderr, "pling 7\n");
      rtpChannelId = (unsigned char)rtpCid;
      rtcpChannelId = (unsigned char)rtcpCid;
    }

    fields += strlen(field);
    while (*fields == ';' || *fields == ' ' || *fields == '\t')
      ++fields; // skip over separating ';' chars or whitespace
    if (*fields == '\0' || *fields == '\r' || *fields == '\n')
      break;
  }
  delete[] field;
}
