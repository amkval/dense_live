#include "MPEG2TransportStreamFramer.hh"

#include "include/DenseMediaSession.hh"
#include "include/DenseMediaSubsession.hh"
#include "include/DenseRTPSource.hh"

// Note: both parent and mediaSession is overkill. how do we limit?
////// DenseMediaSubsession //////
DenseMediaSubsession *DenseMediaSubsession::createNew(
    UsageEnvironment &env,
    DenseMediaSession &parent,
    DenseMediaSession *mediaSession)
{
  return new DenseMediaSubsession(env, parent, mediaSession);
}

DenseMediaSubsession::DenseMediaSubsession(
    UsageEnvironment &env,
    DenseMediaSession &parent,
    DenseMediaSession *mediaSession) : MediaSubsession(parent),
                                       fLevel(0), fMediaSession(mediaSession)
{
}

DenseMediaSubsession::~DenseMediaSubsession()
{
}

// Dense implementation of Initiate
Boolean DenseMediaSubsession::denseInitiate(int useSpecialRTPoffset)
{
  if (fReadSource != NULL)
    return True; // has already been initiated

  do
  {
    if (fCodecName == NULL)
    {
      env().setResultMsg("Codec is unspecified");
      break;
    }

    // Create RTP and RTCP 'Groupsocks' on which to receive incoming data.
    // (Groupsocks will work even for unicast addresses)
    struct in_addr tempAddr;
    tempAddr.s_addr = connectionEndpointAddress();
    // This could get changed later, as a result of a RTSP "SETUP"

    if (fClientPortNum != 0)
    // if (fClientPortNum != 0 && (honorSDPPortChoice || IsMulticastAddress(tempAddr.s_addr)))
    {
      // The sockets' port numbers were specified for us.  Use these:
      fprintf(stderr, "The sockets' port numbers were specified for us. Using: %hu\n", fClientPortNum);
      Boolean const protocolIsRTP = strcmp(fProtocolName, "RTP") == 0;
      if (protocolIsRTP && !fMultiplexRTCPWithRTP)
      {
        fClientPortNum = fClientPortNum & ~1;
        // use an even-numbered port for RTP, and the next (odd-numbered) port for RTCP
      }
      if (isSSM())
      {
        fprintf(stderr, "Setting up groupsock, ISSSM\n\n");
        fRTPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, fClientPortNum);
      }
      else
      {
        fprintf(stderr, "Setting up groupsock, IS NOT SSM\n\n");
        fRTPSocket = new Groupsock(env(), tempAddr, fClientPortNum, 255);

        // <Dense Section>
        // TODO: Make it more clear what is happening here.
        // Better variable names etc.

        if (!fInit)
        {
          // struct sockaddr_in sa;
          char buffer[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &tempAddr.s_addr, buffer, sizeof(buffer));
          printf("address:%s and socketnum: %d\n", buffer, fRTPSocket->socketNum());
          socketLeaveGroup(env(), fRTPSocket->socketNum(), tempAddr.s_addr);

          env() << "This is " << htons(fRTPSocket->port().num()) << " and we are leaving the group again because we don't flow from start\n";
        }
        else
        {
          env() << "This is " << htons(fRTPSocket->port().num()) << " and we flow from start\n";

          // This subsession is in control.
          fMediaSession->fInControl = this;
        }
        // </Dense Section>
      }
      if (fRTPSocket == NULL)
      {
        env().setResultMsg("Failed to create RTP socket");
        break;
      }

      if (protocolIsRTP)
      {
        if (fMultiplexRTCPWithRTP)
        {
          // Use the RTP 'groupsock' object for RTCP as well:
          fRTCPSocket = fRTPSocket;
        }
        else
        {
          // Set our RTCP port to be the RTP port + 1:
          portNumBits const rtcpPortNum = fClientPortNum | 1;
          if (isSSM())
          {
            fRTCPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, rtcpPortNum);
          }
          else
          {
            fRTCPSocket = new Groupsock(env(), tempAddr, rtcpPortNum, 255);
          }
        }
      }
    }
    else
    {
      // Port numbers were not specified in advance, so we use ephemeral port numbers.
      // Create sockets until we get a port-number pair (even: RTP; even+1: RTCP).
      // (However, if we're multiplexing RTCP with RTP, then we create only one socket,
      // and the port number can be even or odd.)
      // We need to make sure that we don't keep trying to use the same bad port numbers over
      // and over again, so we store bad sockets in a table, and delete them all when we're done.
      HashTable *socketHashTable = HashTable::create(ONE_WORD_HASH_KEYS);
      if (socketHashTable == NULL)
        break;
      Boolean success = False;
      NoReuse dummy(env());
      // ensures that our new ephemeral port number won't be one that's already in use

      while (1)
      {
        // Create a new socket:
        if (isSSM())
        {
          fRTPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, 0);
        }
        else
        {
          fRTPSocket = new Groupsock(env(), tempAddr, 0, 255);
        }
        if (fRTPSocket == NULL)
        {
          env().setResultMsg("MediaSession::initiate(): unable to create RTP and RTCP sockets");
          break;
        }

        // Get the client port number:
        Port clientPort(0);
        if (!getSourcePort(env(), fRTPSocket->socketNum(), clientPort))
        {
          break;
        }
        fClientPortNum = ntohs(clientPort.num());
        if (fMultiplexRTCPWithRTP)
        {
          // Use this RTP 'groupsock' object for RTCP as well:
          fRTCPSocket = fRTPSocket;
          success = True;
          break;
        }

        // To be usable for RTP, the client port number must be even:
        if ((fClientPortNum & 1) != 0)
        { // it's odd
          // Record this socket in our table, and keep trying:
          unsigned key = (unsigned)fClientPortNum;
          Groupsock *existing = (Groupsock *)socketHashTable->Add(std::to_string(key).c_str(), fRTPSocket);
          delete existing; // in case it wasn't NULL
          continue;
        }

        // Make sure we can use the next (i.e., odd) port number, for RTCP:
        portNumBits rtcpPortNum = fClientPortNum | 1;
        if (isSSM())
        {
          fRTCPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, rtcpPortNum);
        }
        else
        {
          fRTCPSocket = new Groupsock(env(), tempAddr, rtcpPortNum, 255);
        }
        if (fRTCPSocket != NULL && fRTCPSocket->socketNum() >= 0)
        {
          // Success! Use these two sockets.
          success = True;
          break;
        }
        else
        {
          // We couldn't create the RTCP socket (perhaps that port number's already in use elsewhere?).
          delete fRTCPSocket;
          fRTCPSocket = NULL;

          // Record the first socket in our table, and keep trying:
          unsigned key = (unsigned)fClientPortNum;
          Groupsock *existing = (Groupsock *)socketHashTable->Add(std::to_string(key).c_str(), fRTPSocket);
          delete existing; // in case it wasn't NULL
          continue;
        }
      }

      // Clean up the socket hash table (and contents):
      Groupsock *oldGS;
      while ((oldGS = (Groupsock *)socketHashTable->RemoveNext()) != NULL)
      {
        delete oldGS;
      }
      delete socketHashTable;

      if (!success)
        break; // a fatal error occurred trying to create the RTP and RTCP sockets; we can't continue
    }

    // Try to use a big receive buffer for RTP - at least 0.1 second of
    // specified bandwidth and at least 50 KB
    unsigned rtpBufSize = fBandwidth * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
    if (rtpBufSize < 50 * 1024)
      rtpBufSize = 50 * 1024;
    increaseReceiveBufferTo(env(), fRTPSocket->socketNum(), rtpBufSize);

    if (isSSM() && fRTCPSocket != NULL)
    {
      // Special case for RTCP SSM: Send RTCP packets back to the source via unicast:
      fRTCPSocket->changeDestinationParameters(fSourceFilterAddr, 0, ~0);
    }

    // Create "fRTPSource" and "fReadSource":
    if (!denseCreateSourceObjects(useSpecialRTPoffset))
      break;

    if (fReadSource == NULL)
    {
      env().setResultMsg("Failed to create read source");
      break;
    }

    // Finally, create our RTCP instance. (It starts running automatically)
    if (fRTPSource != NULL && fRTCPSocket != NULL)
    {
      // If bandwidth is specified, use it and add 5% for RTCP overhead.
      // Otherwise make a guess at 500 kbps.
      unsigned totSessionBandwidth = fBandwidth ? fBandwidth + fBandwidth / 20 : 500;

      env() << "\tCNAME: \"" << fParent.CNAME() << "\"\n";

      fRTCPInstance = RTCPInstance::createNew(env(), fRTCPSocket,
                                              totSessionBandwidth,
                                              (unsigned char const *)
                                                  fParent.CNAME(),
                                              NULL /* we're a client */,
                                              fRTPSource);
      if (fRTCPInstance == NULL)
      {
        env().setResultMsg("Failed to create RTCP instance");
        break;
      }
    }

    return True;
  } while (0);

  deInitiate();
  fClientPortNum = 0;
  return False;
}

// This replaces a more general function found in MediaSubsession
Boolean DenseMediaSubsession::denseCreateSourceObjects(int useSpecialRTPoffset)
{
  env() << "DenseMediaSubsession::denseCreateSourceObjects()\n";
  if (strcmp(fProtocolName, "UDP") != 0)
  { // RTP stream
    if (strcmp(fCodecName, "MP2T") == 0)
    { // MPEG-2 transport Stream
      fRTPSource = DenseRTPSource::createNew(
          env(), fRTPSocket, fRTPPayloadFormat,
          fRTPTimestampFrequency, "video/MP2T",
          0, False);

      if (fMediaSession == NULL)
      {
        env() << "WARNING: fMediaSession == NULL!\n";
        return False;
      }

      ((DenseRTPSource *)fRTPSource)->fDenseMediaSession = fMediaSession;
      ((DenseRTPSource *)fRTPSource)->setDenseMediaSubsession(this);

      fReadSource = MPEG2TransportStreamFramer::createNew(env(), fRTPSource);
      return True;
    }
  }

  return False;
}