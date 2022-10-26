#include "include/DenseRTPSource.hh"

// Forward TODO: Move to header
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
          env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency,
          mimeTypeString, offset, doNormalMBitRule)
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
  resultSpecialHeaderSize = 0; // fOffset;

  // Can we use the incoming packet?
  int use = manageQualityLevels(packet);

  // Tell caller to drop packet if use is two.
  if (use == 2)
  {
    return False;
  }
  fDenseMediaSession->fPacketChunk = chunk(packet);
  return True;
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
  /*
  envir() << "\tManageQualityLevels:\n"
          << "\tIn PACKET:\n"
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
        */
}

int DenseRTPSource::manageQualityLevels(BufferedPacket *packet)
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = denseMediaSubsession();

  //env << "ManageQualityLevels:\n"
  //    << "chunk(): " << chunk(packet) << "\n";

  // Print QLF every 50 packets
  if (chunk(packet) % 50 == 0)
  {
    printQLF(packet);
  }

  if (fDenseMediaSession->fRTPChunk == 65535)
  {
    fDenseMediaSession->fRTPChunk = chunk(packet);
    env << "This is the first packet, setting fOurSession->RTPChunk to " << fDenseMediaSession->fRTPChunk << "\n";
  }

  /*
  There are three possible return values, the return values represent:
  0. The packet is usable -> its from the current level and the correct chunk
  1. The packet should be saved -> its from the next level, but its ahead
  2. The packet should be dropped -> its not from the current level or ahead
  */

  // If we are in control
  if (fDenseMediaSession->fInControl == ourSubSession)
  { 
    // If it is the expected packet
    if (chunk(packet) == fDenseMediaSession->fRTPChunk)
    {
      return 0;
    }
    // If the packet is ahead
    else if (chunk(packet) > fDenseMediaSession->fRTPChunk)
    { 
      // If we just moved into this level
      if (fJustMoved)
      {
        fJustMoved = False;
        env << "Entered Level: " << fDenseMediaSession->fCurrentLevel << "\n";
        printQLF(packet);
        fDenseMediaSession->fRTPChunk = chunk(packet);
        return 0;
      }

      // Start moving level if we are not currently moving
      
      if (!fThisIsMoving)
      {
        // Move down a level if we have had enough packet loss
        if (fDenseMediaSession->fLevelDrops >= 3)
        {
          if (!startDown())
          {
            env << "Quitting after starDown\n";
            exit(EXIT_FAILURE);
          }
        }
        else // Move up a level
        {
          if (!startUp())
          {
            env << "Quitting after startUp()\n";
            exit(EXIT_FAILURE);
          }
        }
        printQLF(packet);
        fDenseMediaSession->fRTPChunk = chunk(packet);
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
      env << "Something is wrong in manageQualityLevels() the packet chunk is behind\n";
      exit(EXIT_FAILURE);
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

  // Set edit media session
  fDenseMediaSession->fInControl = fDenseMediaSession->fDenseNext;
  fDenseMediaSession->fCurrentLevel = fDenseMediaSession->fInControl->fLevel;
  fDenseMediaSession->fDenseNext = NULL;
  fDenseMediaSession->fLevelDrops = 0;
  fDenseMediaSession->fFinishLookAside = True;

  DenseRTPSource *denseRTPSource = dynamic_cast<DenseRTPSource *>(fDenseMediaSession->fInControl->rtpSource());
  if (!denseRTPSource)
  {
    env << "RTPSource is not a DenseRTPSource!";
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

// Note: This could be made to support an arbitrary amount of levels.
Boolean DenseRTPSource::startDown()
{
  UsageEnvironment &env = envir();

  env << "#### Go down a quality level ####\n";
  env << "fMediaSession->fCurrentLevel: " << fDenseMediaSession->fCurrentLevel << "\n";
  env << "fMediaSession->fLevelDrops: " << fDenseMediaSession->fLevelDrops << "\n";

  if (fDenseMediaSession->fCurrentLevel == 1)
  {
    if (joinZero())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Joined Level 0\n";
      return True;
    }
  }
  else if (fDenseMediaSession->fCurrentLevel == 2)
  {
    if (joinOne())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Joined Level 1\n";
      return True;
    }
  }
  else
  {
    //env << "We are already at zero and is not moving anywhere\n";
    fDenseMediaSession->fLevelDrops = 0;

    return True;
  }

  return False;
}

Boolean DenseRTPSource::startUp()
{
  UsageEnvironment &env = envir();
  if (fDenseMediaSession->fCurrentLevel == 0)
  {
    if (joinOne())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Joined Level 1\n";
      return True;
    }
  }
  else if (fDenseMediaSession->fCurrentLevel == 1)
  {
    if (joinTwo())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Joined Level 2\n";
      return True;
    }
  }
  else
  {
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
    env << "The next level is: " << fDenseMediaSession->fDenseNext->fLevel << "\n";
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
// Note: This is not an optimal solution and may break if the rtp header changes.
int chunk(BufferedPacket *packet)
{
  unsigned char *data = packet->data();
  data = data - 8;
  unsigned extHdr = ntohl(*(u_int32_t *)(data));
  return (extHdr) >> 16;
}