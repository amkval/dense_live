#include "include/DenseRTPSource.hh"

////// DenseRTPSource //////
DenseRTPSource *DenseRTPSource::createNew(
    UsageEnvironment &env,
    Groupsock *RTPgs,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *mimeTypeString,
    unsigned offset, Boolean doNormalMBitRule)
{
  return new DenseRTPSource(
      env, RTPgs, rtpPayloadFormat,
      rtpTimestampFrequency,
      mimeTypeString, offset, doNormalMBitRule);
}

DenseRTPSource::DenseRTPSource(
    UsageEnvironment &env, Groupsock *RTPgs,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *mimeTypeString,
    unsigned offset, Boolean doNormalMBitRule)
    : SimpleRTPSource(
          env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency,
          mimeTypeString, offset, doNormalMBitRule)
{
}

DenseRTPSource::~DenseRTPSource()
{
}

// Convenient function that runs every time we receive a packet.
Boolean DenseRTPSource::processSpecialHeader(BufferedPacket *packet, unsigned &resultSpecialHeaderSize)
{
  //UsageEnvironment &env = envir();

  // Detect packet loss:
  Boolean packetLossPrecededThis = False;
  if (packet->isFirstPacket())
  {
    packetLossPrecededThis = True;
  }

  Boolean timeThresholdHasBeenExceeded;
  unsigned int thresholdTime = 10000;
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  unsigned uSecondsSinceReceived = (timeNow.tv_sec - packet->timeReceived().tv_sec) * 1000000 + (timeNow.tv_usec - packet->timeReceived().tv_usec);
  timeThresholdHasBeenExceeded = uSecondsSinceReceived > thresholdTime;

  if (timeThresholdHasBeenExceeded)
  {
    packetLossPrecededThis = True;
  }

  if (packetLossPrecededThis)
  {
    fDenseMediaSession->fLevelDrops++;
    fDenseMediaSession->fTotalDrops++;
  }

  // The things the function is actually supposed to do:
  fCurrentPacketCompletesFrame = True;
  resultSpecialHeaderSize = 4;

  // Can we use the incoming packet?
  // It will be discarded otherwise
  return manageQualityLevels(packet);
}

void DenseRTPSource::printQLF(BufferedPacket *packet)
{
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();
  DenseMediaSession *parent = fDenseMediaSession;
  Groupsock *ourSocket = RTPgs();

  Boolean inCntrl = False;
  Boolean nextInCntrl = False;
  if (parent->fInControl == ourSubSession)
  {
    inCntrl = True;
  }
  if (parent->fDenseNext == ourSubSession)
  {
    nextInCntrl = True;
  }

  envir() << "\tManageQualityLevels:\n"
          << "\tid: " << htons(ourSocket->port().num()) << "\n"
          << "\tfLevel: " << ourSubSession->fLevel << "\n"
          << "\trtpSeqNo: " << packet->rtpSeqNo() << "\n"
          << "\tpacketChunk: " << chunk(packet) << "\n"
          << "\tIn SESSION:\n"
          << "\tfRTPChunk " << parent->fRTPChunk << "\n"
          << "\tfCurrentLevel " << parent->fCurrentLevel << "\n"
          << "\tInControl: " << (inCntrl ? "True" : "False") << "\n"
          << "\tNextInControl: " << (nextInCntrl ? "True" : "False") << "\n"
          << "\tfLevelDrops: " << parent->fLevelDrops << "\n"
          << "\tfTotalDrops: " << parent->fTotalDrops << "\n";
}

/*
  There are three possible cases:
    0. The packet should be used -> it's from the current level and the correct chunk
    1. The packet should be saved -> it's from the next level, but its ahead
    2. The packet should be dropped -> it's not from the current level or ahead
  There are two possible return values:
    True: The packet can be used.
    False: The packet is to be discarded.
*/
int DenseRTPSource::manageQualityLevels(BufferedPacket *packet)
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();

  // If this is the first packet we have received
  if (fDenseMediaSession->fRTPChunk == 65535)
  {
    fDenseMediaSession->fRTPChunk = chunk(packet);
  }

  // If we are not in control
  if (fDenseMediaSession->fInControl != ourSubSession)
  {
    // If the packet is ahead we should save it for later use.
    if (chunk(packet) > fDenseMediaSession->fRTPChunk)
    {
      fDenseMediaSession->fPutInLookAsideBuffer = True;
      fDenseMediaSession->fPacketChunk = chunk(packet);
      return True;
    }

    return False;
  }

  // Finish moving if we have started already
  if (fThisIsMoving)
  {
    finish();
    return False;
  }

  // If the packet is behind, just discard it.
  if (chunk(packet) < fDenseMediaSession->fRTPChunk)
  {
    //env << "chunk(packet): " << chunk(packet) << "\n";
    //env << "Something is wrong in manageQualityLevels() the packet chunk is behind\n";
    //exit(EXIT_FAILURE);
    return False;
  }

  // Set PacketChunk to current chunk ref.
  fDenseMediaSession->fPacketChunk = chunk(packet);

  // If the packet belongs to the current chunk
  if (fDenseMediaSession->fPacketChunk == fDenseMediaSession->fRTPChunk)
  {
    return True;
  }

  // Set the current RTPChunk to the new PacketChunk
  fDenseMediaSession->fRTPChunk = fDenseMediaSession->fPacketChunk;

  // If this level just gained control
  if (fJustMoved)
  {
    fJustMoved = False;
    return True;
  }

  // If we have lost too many packets, move down a level
  if (fDenseMediaSession->fLevelDrops >= 3)
  {
    if (!startDown())
    {
      env << "Quitting after starDown\n";
      exit(EXIT_FAILURE);
    }
    return True;
  }
  
  // Move up a level
  if (!startUp())
  {
    env << "Quitting after startUp()\n";
    exit(EXIT_FAILURE);
  }
  return True;
}

void DenseRTPSource::finish()
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();
  Groupsock *ourGS = RTPgs();

  fThisIsMoving = False;

  // Set edit media session
  fDenseMediaSession->fInControl = fDenseMediaSession->fDenseNext;
  fDenseMediaSession->fCurrentLevel = fDenseMediaSession->fInControl->fLevel;
  fDenseMediaSession->fDenseNext = NULL;
  fDenseMediaSession->fLevelDrops = 0;
  fDenseMediaSession->fFinishLookAside = True;

  DenseRTPSource *denseRTPSource = dynamic_cast<DenseRTPSource *>(fDenseMediaSession->fInControl->rtpSource());
  if (!denseRTPSource)
  {
    env << "RTPSource is not a 'DenseRTPSource'!";
    exit(EXIT_FAILURE);
  }

  // Prepare next level
  denseRTPSource->fJustMoved = True;

  // Exit multicast on current level
  if (ourSubSession != NULL)
  {
    socketLeaveGroup(envir(), ourGS->socketNum(), ourSubSession->connectionEndpointAddress());
  }

  env << "Closed socket on: " << htons(ourGS->port().num()) << "\n";
  env << "Next Level: " << fDenseMediaSession->fCurrentLevel << "\n";
}

Boolean DenseRTPSource::startDown()
{
  UsageEnvironment &env = envir();
  int newLevel = fDenseMediaSession->fCurrentLevel - 1;

  env << "#### Go down a quality level ####\n";
  env << "fMediaSession->fCurrentLevel: " << fDenseMediaSession->fCurrentLevel << "\n";
  env << "fMediaSession->fLevelDrops: " << fDenseMediaSession->fLevelDrops << "\n";

  // Go lower
  if (newLevel >= 0)
  {
    if (joinLevel(newLevel))
    {
      fThisIsMoving = True;
      env << "Joining level " << newLevel << "\n";
      return True;
    }
  }

  // We are already as low as we can go
  if (newLevel == -1)
  {
    fDenseMediaSession->fLevelDrops = 0;
    return True;
  }

  return False;
}

Boolean DenseRTPSource::startUp()
{
  UsageEnvironment &env = envir();
  int newLevel = fDenseMediaSession->fCurrentLevel + 1;

  if (newLevel < 3)
  {
    if (joinLevel(newLevel))
    {
      fThisIsMoving = True;
      env << "Joining level " << newLevel << "\n";
      return True;
    }
  }
  
  if (newLevel == 3)
  {
    fDenseMediaSession->fLevelDrops = 0;
    return True;
  }

  return False;
}

Boolean DenseRTPSource::joinLevel(int level)
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();

  env << "Joining Quality Level " << level << "\n";

  DenseMediaSubsession *next = fDenseMediaSession->fDenseMediaSubsessions.at(level);
  fDenseMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinLevel() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << level << "\n";
    return True;
  }
  return False;
}

// Extract chunk number from packet
int DenseRTPSource::chunk(BufferedPacket *packet)
{
  unsigned char *data = packet->data();
  unsigned extHdr = ntohl(*(u_int32_t *)(data));
  return (extHdr) >> 16;
}