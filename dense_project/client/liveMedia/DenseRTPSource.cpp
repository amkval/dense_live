#include "include/DenseRTPSource.hh"

// Forward
int chunk(BufferedPacket *packet);

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
          env, RTPgs,
          rtpPayloadFormat, rtpTimestampFrequency, mimeTypeString, offset, doNormalMBitRule)
{
}

DenseRTPSource::~DenseRTPSource()
{
}

/**
 * @brief Spoofed function used to run our code every time a new packet is read
 * If we return false the packet will be dropped.
 */
Boolean DenseRTPSource::processSpecialHeader(BufferedPacket *packet, unsigned &resultSpecialHeaderSize)
{
  UsageEnvironment &env = envir();
  //env << "DenseRTPSource::processSpecialHeader():\n";

  // Detect packet loss:
  Boolean fUseMBitForFrameEnd = False; // Note: Remove
  Boolean packetLossPrecededThis = False;
  if (packet->isFirstPacket())
  {
    packetLossPrecededThis = True;
  }

  Boolean timeThresholdHasBeenExceeded;
  unsigned int thresholdTime = 10000; //Note: Bad hardcoded part
  if (thresholdTime == 0)
  {
    timeThresholdHasBeenExceeded = True;
  }
  else
  {
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    unsigned uSecondsSinceReceived = (timeNow.tv_sec - packet->timeReceived().tv_sec) * 1000000 + (timeNow.tv_usec - packet->timeReceived().tv_usec);
    timeThresholdHasBeenExceeded = uSecondsSinceReceived > thresholdTime;
  }

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
  fCurrentPacketCompletesFrame = !fUseMBitForFrameEnd || packet->rtpMarkerBit();
  resultSpecialHeaderSize = 0; // fOffset;

  // Can we use the incoming packet?
  //env << "We are here in the fancy function\n";
  int use = manageQualityLevels(packet);

  //env << "DenseRTPSource::processSpecialHeader():\n";

  // Note: The packet will be dropped by the caller if use us 2.
  if (use == 0 || use == 1)
  {
    return True;
  }
  else
  {
    return False;
  }
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
          << "\tIn PACKET:\n"
          << "\tid: " << htons(ourSocket->port().num()) << "\n"
          << "\tLevel: " << ourSubSession->fLevel << "\n"
          << "\tseqNo: " << packet->rtpSeqNo() << "\n"
          << "\tpacketChunk: " << chunk(packet) << "\n"
          << "\tIn SESSION:\n"
          << "\tSourcenowChunk " << parent->fRTPChunk << "\n"
          << "\tMain level " << parent->fCurrentLevel << "\n"
          << "\tInControl: " << (inCntrl ? "True" : "False") << "\n"
          << "\tNextInControl: " << (nextInCntrl ? "True" : "False") << "\n"
          << "\tLEVEL LOSS: " << parent->fLevelDrops << "\n"
          << "\tTOTAL LOSS: " << parent->fTotalDrops << "\n";
}

int DenseRTPSource::manageQualityLevels(BufferedPacket *packet)
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();
  
  //env << "We are here in the other function\n";

  //env << "ManageQualityLevels:\n"
  //    << "chunk(): " << chunk(packet) << "\n";

  if (chunk(packet) % 50 == 0)
  {
    printQLF(packet);
  }

  if (fDenseMediaSession->fRTPChunk == 65535)
  {
    fDenseMediaSession->fRTPChunk = chunk(packet);
    env << "This is the first packet, and we are changing the fOurSession->RTPChunk to " << fDenseMediaSession->fRTPChunk << "\n";
  }

  /*
  THERE ARE THREE CASES; The return values represent:
  0. The packet is usable -> its from the controlling entity and correct chunk
  1. The packet should be saved -> its from the next controlling entity, but its ahead
  2. The packet should be dropped -> its not from a controlling entity or ahead
  */

  if (fDenseMediaSession->fInControl == ourSubSession) // We are in control
  {
    if (chunk(packet) == fDenseMediaSession->fRTPChunk)
    {
      return 0;
    }
    else if (chunk(packet) > fDenseMediaSession->fRTPChunk)
    {
      if (fJustMoved)
      {
        fJustMoved = False;
        env << "This one JUST MOVED, give it a minute!\n";
        printQLF(packet);
        fDenseMediaSession->fRTPChunk = chunk(packet);
        return 0;
      }

      if (!fThisIsMoving)
      { // Start moving
        if (fDenseMediaSession->fLevelDrops >= 3)
        { // Start down, there are too many drops
          if (!startDown())
          {
            env << "Quitting after starDown\n";
            exit(0);
          }
        }
        else
        {
          if (!startUp())
          { // Start up, there is not many drops
            env << "Quitting after startUp()\n";
            exit(0);
          }
        }
        printQLF(packet);
        fDenseMediaSession->fRTPChunk = chunk(packet); // THE ONLY OF TWO PLACED THIS IS UPDATED
        return 0;
      }
      else
      { // Finish moving
        finish();
        env << "Have finished (packet follows)\n";
        printQLF(packet);
      }
    }
    else
    { // This is wrong
      env << "Something is wrong in manageQualityLevels() the packet chunk is behind the parent.streamChunk\n";
      exit(0);
    }
  }
  else
  { // We are not in control
    if (chunk(packet) > fDenseMediaSession->fRTPChunk)
    { // the packet should be saved for when we are in control
      fDenseMediaSession->fPutInLookAsideBuffer = True;
      fDenseMediaSession->fPacketChunk = chunk(packet);
      return 1;
    }
  }
  return 2;
}

void DenseRTPSource::finish()
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();
  Groupsock *ourGS = RTPgs();

  fThisIsMoving = False;

  fDenseMediaSession->fInControl = fDenseMediaSession->fDenseNext;
  fDenseMediaSession->fCurrentLevel = fDenseMediaSession->fDenseNext->fLevel;
  fDenseMediaSession->fDenseNext = NULL;
  fDenseMediaSession->fLevelDrops = 0;
  DenseMediaSubsession *change = (DenseMediaSubsession *)fDenseMediaSession->fInControl;

  DenseRTPSource *child = dynamic_cast<DenseRTPSource *>(change->rtpSource());
  if (!child)
    env << "RTPSource is not a DenseMultiFramedRTPSource";

  child->fJustMoved = True;
  fDenseMediaSession->fFinishLookAside = True;

  if (ourSubSession != NULL)
  {
    socketLeaveGroup(envir(), ourGS->socketNum(), ourSubSession->connectionEndpointAddress());
  }

  env << "Finish from " << htons(ourGS->port().num()) << "\n";
  env << "The level after finish: " << fDenseMediaSession->fCurrentLevel << "\n";
}

// Note: This could be made to support an arbitrary amount of levels.
Boolean DenseRTPSource::startDown()
{
  UsageEnvironment &env = envir();

  env << "DenseMultiFramedRTPSource::startDown()"
      << "\n";
  env << "fMediaSession->fCurrentLevel: " << fDenseMediaSession->fCurrentLevel << "\n";
  env << "fMediaSession->fLevelDrops: " << fDenseMediaSession->fLevelDrops << "\n";

  if (fDenseMediaSession->fCurrentLevel == 1)
  {
    if (joinZero())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined ZERO\n";
      return True;
    }
  }
  else if (fDenseMediaSession->fCurrentLevel == 2)
  {
    if (joinOne())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined ONE\n";
      return True;
    }
  }
  else
  {
    env << "We are already at zero and is not moving anywhere\n";
    fDenseMediaSession->fLevelDrops = 0;

    return True;
  }

  return False;
}

Boolean DenseRTPSource::startUp()
{
  UsageEnvironment &env = envir();
  env << "DenseMultiFramedRTPSource::startUp()"
      << "\n";
  env << "fMediaSession->fCurrentLevel: " << fDenseMediaSession->fCurrentLevel << "\n";

  if (fDenseMediaSession->fCurrentLevel == 0)
  {
    if (joinOne())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined ONE\n";
      return True;
    }
  }
  else if (fDenseMediaSession->fCurrentLevel == 1)
  {
    if (joinTwo())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined TWO\n";
      return True;
    }
  }
  else
  {
    env << "We are already at two and is not moving anywhere\n";
    fDenseMediaSession->fLevelDrops = 0;
    return True;
  }

  return False;
}

// Note: The three following functions could be merged.
Boolean DenseRTPSource::joinZero()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();
  DenseMediaSubsession *next = fDenseMediaSession->fDenseMediaSubsessions.at(0);

  env << "Joining Quality Level 0!\n";

  fDenseMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinZero() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fDenseMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

Boolean DenseRTPSource::joinOne()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();
  DenseMediaSubsession *next = fDenseMediaSession->fDenseMediaSubsessions.at(1);

  env << "Joining Quality Level 1!\n";

  fDenseMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinOne() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fDenseMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

Boolean DenseRTPSource::joinTwo()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();

  env << "Joining Quality Level 2\n";

  DenseMediaSubsession *next = fDenseMediaSession->fDenseMediaSubsessions.at(2);
  fDenseMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinTwo() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fDenseMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

// Extract chunk number from packet
int chunk(BufferedPacket *packet)
{
  unsigned char *data = packet->data();

  // TODO: Make sure that this works in all cases
  data = data - 8;
  unsigned extHdr = ntohl(*(u_int32_t *)(data));

  //uint32_t temp = extHdr >> 16;
  //fprintf(stdout, "temp: %d\n", temp);
  return (extHdr) >> 16;
}