// TODO: In this file we will attempt to copy the original file and then include what we need from Gøtes changes.
// TODO: Remove what we don't need!
// TODO: Fix according to changes elsewhere!

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
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// RTP source for a common kind of payload format: Those that pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// Implementation

#include "include/DenseMultiFramedRTPSource.hh"
#include "include/DenseMediaSession.hh"
#include "include/DenseMediaSubsession.hh"

////////// DenseReorderingPacketBuffer definition //////////

class DenseReorderingPacketBuffer
{
public:
  DenseReorderingPacketBuffer(DenseBufferedPacketFactory *packetFactory);
  virtual ~DenseReorderingPacketBuffer();
  void reset();

  DenseBufferedPacket *getFreePacket(DenseMultiFramedRTPSource *ourSource);
  Boolean storePacket(DenseBufferedPacket *bPacket);
  DenseBufferedPacket *getNextCompletedPacket(Boolean &packetLossPreceded);
  void releaseUsedPacket(DenseBufferedPacket *packet);
  void freePacket(DenseBufferedPacket *packet)
  {
    if (packet != fSavedPacket)
    {
      delete packet;
    }
    else
    {
      fSavedPacketFree = True;
    }
  }
  Boolean isEmpty() const { return fHeadPacket == NULL; }

  void setThresholdTime(unsigned uSeconds) { fThresholdTime = uSeconds; }
  void resetHaveSeenFirstPacket() { fHaveSeenFirstPacket = False; }

  /** <Dense content> **/
  DenseBufferedPacket *getLastCompletedPacket();         // Note: only used in unreachable code.
  DenseBufferedPacket *getHead() { return fHeadPacket; } // Note: not used?
  DenseBufferedPacket *getTail() { return fTailPacket; } // Note: not used?
  unsigned short getChunk() { return curChunk; }         // Note: only used in unreachable code.
  void setChunk(unsigned short in) { curChunk = in; }    // Note: not used?
  int bufferSize;                                        // Note: not used?
  int bufferPackets;                                     // Note: not used?
  /** </Dense content> **/

private:
  DenseBufferedPacketFactory *fPacketFactory;
  unsigned fThresholdTime;      // uSeconds
  Boolean fHaveSeenFirstPacket; // used to set initial "fNextExpectedSeqNo"
  unsigned short fNextExpectedSeqNo;
  DenseBufferedPacket *fHeadPacket;
  DenseBufferedPacket *fTailPacket;
  DenseBufferedPacket *fSavedPacket;
  // to avoid calling new/free in the common case
  Boolean fSavedPacketFree;

  // <Dense Content>
  unsigned short curChunk; // Note: only used in unreachable code.
  // </Dense Content>
};

////////// DenseMultiFramedRTPSource implementation //////////

DenseMultiFramedRTPSource::DenseMultiFramedRTPSource(
    UsageEnvironment &env, Groupsock *RTPgs,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    DenseBufferedPacketFactory *packetFactory)
    : RTPSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency)
{
  reset();
  fReorderingBuffer = new DenseReorderingPacketBuffer(packetFactory);

  // Try to use a big receive buffer for RTP:
  increaseReceiveBufferTo(env, RTPgs->socketNum(), 50 * 1024);
}

void DenseMultiFramedRTPSource::reset()
{
  fCurrentPacketBeginsFrame = True;    // by default
  fCurrentPacketCompletesFrame = True; // by default
  fAreDoingNetworkReads = False;
  fPacketReadInProgress = NULL;
  fNeedDelivery = False;
  fPacketLossInFragmentedFrame = False;
}

DenseMultiFramedRTPSource::~DenseMultiFramedRTPSource()
{
  delete fReorderingBuffer;
}

Boolean DenseMultiFramedRTPSource::processSpecialHeader(
    DenseBufferedPacket * /*packet*/,
    unsigned &resultSpecialHeaderSize)
{
  // Default implementation: Assume no special header:
  resultSpecialHeaderSize = 0;
  return True;
}

Boolean DenseMultiFramedRTPSource::packetIsUsableInJitterCalculation(
    unsigned char * /*packet*/,
    unsigned /*packetSize*/)
{
  // Default implementation:
  return True;
}

void DenseMultiFramedRTPSource::doStopGettingFrames()
{
  if (fPacketReadInProgress != NULL)
  {
    fReorderingBuffer->freePacket(fPacketReadInProgress);
    fPacketReadInProgress = NULL;
  }
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  fRTPInterface.stopNetworkReading();
  fReorderingBuffer->reset();
  reset();
}

void DenseMultiFramedRTPSource::doGetNextFrame()
{
  UsageEnvironment &env = envir();
  //env << "####:\t\tDenseMultiFramedRTPSource::doGetNextFrame()\n";

  if (!fAreDoingNetworkReads)
  {
    env << "\tTurn on background read handling of incoming packets:\n";
    // Turn on background read handling of incoming packets:
    fAreDoingNetworkReads = True;
    TaskScheduler::BackgroundHandlerProc *handler = (TaskScheduler::BackgroundHandlerProc *)&networkReadHandler;
    fRTPInterface.startNetworkReading(handler);
  }

  fSavedTo = fTo;
  fSavedMaxSize = fMaxSize;
  fFrameSize = 0; // for now
  fNeedDelivery = True;
  doGetNextFrame1();
  //env << "<<<<:\t\tDenseMultiFramedRTPSource::doGetNextFrame()\n";
}

// This function has been heavily modified for dense use.
void DenseMultiFramedRTPSource::doGetNextFrame1()
{
  UsageEnvironment &env = envir();
  //env << "####:\t\tDenseMultiFramedRTPSource::doGetNextFrame1()\n";

  while (fNeedDelivery)
  {
    env << "\tfNeedDelivery: True\n";
    // If we already have packet data available, then deliver it now.
    Boolean packetLossPrecededThis;
    DenseBufferedPacket *nextPacket = fReorderingBuffer->getNextCompletedPacket(packetLossPrecededThis);
    if (nextPacket == NULL)
    {
      env << "\tnextPacket: NULL\n";
      break;
    }

    env << "\tnextPacket: !NULL\n";

    fNeedDelivery = False;

    if (nextPacket->useCount() == 0)
    {
      // Before using the packet, check whether it has a special header
      // that needs to be processed:
      unsigned specialHeaderSize;
      if (!processSpecialHeader(nextPacket, specialHeaderSize))
      {
        // Something's wrong with the header; reject the packet:
        fReorderingBuffer->releaseUsedPacket(nextPacket);
        fNeedDelivery = True;
        continue;
      }
      nextPacket->skip(specialHeaderSize);
    }

    // Check whether we're part of a multi-packet frame, and whether
    // there was packet loss that would render this packet unusable:
    if (fCurrentPacketBeginsFrame)
    {
      if (packetLossPrecededThis || fPacketLossInFragmentedFrame)
      {
        // We didn't get all of the previous frame.
        // Forget any data that we used from it:
        fTo = fSavedTo;
        fMaxSize = fSavedMaxSize;
        fFrameSize = 0;
      }
      fPacketLossInFragmentedFrame = False;
    }
    else if (packetLossPrecededThis)
    {
      // We're in a multi-packet frame, with preceding packet loss
      fPacketLossInFragmentedFrame = True;
    }

    if (fPacketLossInFragmentedFrame)
    {
      // This packet is unusable; reject it:
      fReorderingBuffer->releaseUsedPacket(nextPacket);
      fNeedDelivery = True;
      continue;
    }

    // <Dense Content>
    if (packetLossPrecededThis)
    {
      fMediaSession->fLevelDrops++;
      fMediaSession->fTotalDrops++;
    }

    int use = manageQualityLevels(nextPacket);
    env << "use: " << use << "\n";

    // The packet is usable. Deliver all or part of it to our caller:
    unsigned frameSize;
    if (use == 0)
    { // The packet is usable. Deliver all or part of it to our caller:

      // Groupsock *ourSocket = RTPgs();
      fMediaSession->fPacketChunk = nextPacket->chunk();
      // env << "Use == 0\nName: " << fMediaSession->mainSessionName << "\nchunk: " << fMediaSession->fPacketChunk << "\n";

      nextPacket->use(fTo, fMaxSize, frameSize, fNumTruncatedBytes,
                      fCurPacketRTPSeqNum, fCurPacketRTPTimestamp,
                      fPresentationTime, fCurPacketHasBeenSynchronizedUsingRTCP,
                      fCurPacketMarkerBit);
      fFrameSize += frameSize;

      if (!nextPacket->hasUsableData())
      {
        // We're completely done with this packet now
        fReorderingBuffer->releaseUsedPacket(nextPacket);
      }
      // nowChunk = nextPacket->nowChunk();
    }
    else if (use == 1)
    { // This packet needs another round in the chain

      Groupsock *ourSocket = RTPgs();
      env << "(RTPSource)\ngo to the lookaside packet! bad packet!\nid: " << htons(ourSocket->port().num()) << "\nseq: " << fCurPacketRTPSeqNum << "\n";
      // FramedSource::setLookAside();
      if (fMediaSession->fPutInLookAsideBuffer == False)
      {
        fMediaSession->fPutInLookAsideBuffer = True;
      }
      else
      {
        env << "Something is wrong in outside of manageQualityLevels()\n";
        exit(EXIT_FAILURE);
      }
      fMediaSession->fPacketChunk = nextPacket->chunk();
      // FramedSource::setSourceAndSeq(htons(ourSocket->port().num()), fCurPacketRTPSeqNum);

      nextPacket->use(fTo, fMaxSize, frameSize, fNumTruncatedBytes,
                      fCurPacketRTPSeqNum, fCurPacketRTPTimestamp,
                      fPresentationTime, fCurPacketHasBeenSynchronizedUsingRTCP,
                      fCurPacketMarkerBit);
      fFrameSize += frameSize;

      if (!nextPacket->hasUsableData())
      {
        // We're completely done with this packet now
        fReorderingBuffer->releaseUsedPacket(nextPacket);
      }
    }
    else if (use == 2)
    {
      fReorderingBuffer->releaseUsedPacket(nextPacket);
    }
    else
    {
      env << "Something is wrong in outside of manageQualityLevels() 78\n";
      exit(EXIT_FAILURE);
    }

    // </Dense Content>

    if (fCurrentPacketCompletesFrame && fFrameSize > 0)
    {
      // We have all the data that the client wants.
      if (fNumTruncatedBytes > 0)
      {
        env << "MultiFramedRTPSource::doGetNextFrame1(): The total received frame size exceeds the client's buffer size ("
            << fSavedMaxSize << ").  "
            << fNumTruncatedBytes << " bytes of trailing data will be dropped!\n";
      }
      // Call our own 'after getting' function, so that the downstream object can consume the data:
      if (fReorderingBuffer->isEmpty())
      {
        // Common case optimization: There are no more queued incoming packets, so this code will not get
        // executed again without having first returned to the event loop.  Call our 'after getting' function
        // directly, because there's no risk of a long chain of recursion (and thus stack overflow):
        env << "AfterGetting Option 1\n";
        afterGetting(this);
      }
      else
      {
        env << "AfterGetting Option 2\n";
        // Special case: Call our 'after getting' function via the event loop.
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                                 (TaskFunc *)FramedSource::afterGetting, this);
      }
    }
    else
    {
      // This packet contained fragmented data, and does not complete
      // the data that the client wants.  Keep getting data:
      fTo += frameSize;
      fMaxSize -= frameSize;
      fNeedDelivery = True;
    }
  }
  //env << "<<<<:\t\tDenseMultiFramedRTPSource::doGetNextFrame1()\n";
}

// <Dense Content>

void DenseMultiFramedRTPSource::printQLF(DenseBufferedPacket *packet)
{
  DenseMediaSubsession *ourSubSession = mediaSubsession();
  DenseMediaSession *parent = fMediaSession;
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
          << "\tpacketChunk: " << packet->chunk() << "\n"
          << "\tIn SESSION:\n"
          << "\tSourcenowChunk " << parent->fRTPChunk << "\n"
          << "\tMain level " << parent->fCurrentLevel << "\n"
          << "\tInControl: " << (inCntrl ? "True" : "False") << "\n"
          << "\tNextInControl: " << (nextInCntrl ? "True" : "False") << "\n"
          << "\tLEVEL LOSS: " << parent->fLevelDrops << "\n"
          << "\tTOTAL LOSS: " << parent->fTotalDrops << "\n";
}

int DenseMultiFramedRTPSource::manageQualityLevels(DenseBufferedPacket *packet)
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = mediaSubsession();
  if (packet->rtpSeqNo() % 50 == 0)
  {
    printQLF(packet);
  }

  if (fMediaSession->fRTPChunk == 65535)
  {
    fMediaSession->fRTPChunk = packet->chunk();
    env << "This is the first packet, and we are changing the fOurSession->RTPChunk to " << fMediaSession->fRTPChunk << "\n";
  }

  /*
  THERE ARE THREE CASES; The return values represent:
  0. The packet is usable -> its from the controlling entity and correct chunk
  1. The packet should be saved -> its from the next controlling entity, but its ahead
  2. The packet should be dropped -> its not from a controlling entity or ahead
  */

  if (fMediaSession->fInControl == ourSubSession) // We are in control
  {
    if (packet->chunk() == fMediaSession->fRTPChunk)
    {
      return 0;
    }
    else if (packet->chunk() > fMediaSession->fRTPChunk)
    {
      if (fJustMoved)
      {
        fJustMoved = False;
        env << "This one JUST MOVED, give it a minute!\n";
        printQLF(packet);
        fMediaSession->fRTPChunk = packet->chunk();
        return 0;
      }

      if (!fThisIsMoving)
      { // Start moving
        if (fMediaSession->fLevelDrops >= 3)
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
        fMediaSession->fRTPChunk = packet->chunk(); // THE ONLY OF TWO PLACED THIS IS UPDATED
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
    if (packet->chunk() > fMediaSession->fRTPChunk)
    { // the packet should be saved for when we are in control
      return 1;
    }
  }
  return 2;
}

void DenseMultiFramedRTPSource::finish()
{
  UsageEnvironment &env = envir();
  DenseMediaSubsession *ourSubSession = mediaSubsession();
  Groupsock *ourGS = RTPgs();

  fThisIsMoving = False;

  fMediaSession->fInControl = fMediaSession->fDenseNext;
  fMediaSession->fCurrentLevel = fMediaSession->fDenseNext->fLevel;
  fMediaSession->fDenseNext = NULL;
  fMediaSession->fLevelDrops = 0;
  DenseMediaSubsession *change = (DenseMediaSubsession *)fMediaSession->fInControl;

  DenseMultiFramedRTPSource *child = dynamic_cast<DenseMultiFramedRTPSource *>(change->rtpSource());
  if (!child)
    env << "RTPSource is not a DenseMultiFramedRTPSource";

  child->fJustMoved = True;
  fMediaSession->fFinishLookAside = True;

  if (ourSubSession != NULL)
  {
    socketLeaveGroup(envir(), ourGS->socketNum(), ourSubSession->connectionEndpointAddress());
  }

  env << "Finish from " << htons(ourGS->port().num()) << "\n";
  env << "The level after finish: " << fMediaSession->fCurrentLevel << "\n";
}

// Note: This could be made to support an arbitrary amount of levels.
Boolean DenseMultiFramedRTPSource::startDown()
{
  UsageEnvironment &env = envir();

  env << "DenseMultiFramedRTPSource::startDown()"
      << "\n";
  env << "fMediaSession->fCurrentLevel: " << fMediaSession->fCurrentLevel << "\n";
  env << "fMediaSession->fLevelDrops: " << fMediaSession->fLevelDrops << "\n";

  if (fMediaSession->fCurrentLevel == 1)
  {
    if (joinZero())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined ZERO\n";
      return True;
    }
  }
  else if (fMediaSession->fCurrentLevel == 2)
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
    fMediaSession->fLevelDrops = 0;

    return True;
  }

  return False;
}

Boolean DenseMultiFramedRTPSource::startUp()
{
  UsageEnvironment &env = envir();
  env << "DenseMultiFramedRTPSource::startUp()"
      << "\n";
  env << "fMediaSession->fCurrentLevel: " << fMediaSession->fCurrentLevel << "\n";

  if (fMediaSession->fCurrentLevel == 0)
  {
    if (joinOne())
    {
      // Update variables
      fThisIsMoving = True;
      env << "Have joined ONE\n";
      return True;
    }
  }
  else if (fMediaSession->fCurrentLevel == 1)
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
    fMediaSession->fLevelDrops = 0;
    return True;
  }

  return False;
}

// Note: The three following functions could be merged.
Boolean DenseMultiFramedRTPSource::joinZero()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();
  DenseMediaSubsession *next = fMediaSession->fDenseMediaSubsessions.at(0);

  env << "Joining Quality Level 0!\n";

  fMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinZero() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

Boolean DenseMultiFramedRTPSource::joinOne()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();
  DenseMediaSubsession *next = fMediaSession->fDenseMediaSubsessions.at(1);

  env << "Joining Quality Level 1!\n";

  fMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinOne() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

Boolean DenseMultiFramedRTPSource::joinTwo()
{
  UsageEnvironment &env = envir();
  Groupsock *ourGS = RTPgs();

  env << "Joining Quality Level 2\n";

  DenseMediaSubsession *next = fMediaSession->fDenseMediaSubsessions.at(2);
  fMediaSession->fDenseNext = next;
  if (next != NULL)
  {
    struct in_addr nextAddr;
    nextAddr.s_addr = next->connectionEndpointAddress();
    RTPSource *nextSource = next->rtpSource();
    Groupsock *nextGs = nextSource->RTPgs();
    socketJoinGroup(envir(), nextGs->socketNum(), nextAddr.s_addr);
    env << "joinTwo() from " << htons(ourGS->port().num()) << " to " << htons(nextGs->port().num()) << "\n";
    env << "The next level is: " << fMediaSession->fDenseNext->fLevel << "\n ";
    return True;
  }
  return False;
}

// </Dense Content> ^^^

void DenseMultiFramedRTPSource::setPacketReorderingThresholdTime(unsigned uSeconds)
{
  fReorderingBuffer->setThresholdTime(uSeconds);
}

#define ADVANCE(n)    \
  do                  \
  {                   \
    bPacket->skip(n); \
  } while (0)

void DenseMultiFramedRTPSource::networkReadHandler(DenseMultiFramedRTPSource *source, int /*mask*/)
{
  // NOTE: Do I have to cast instead of getting DDMFRTPS directly? it is actually a void *
  fprintf(stderr, "TEST: DenseMultiFramedRTPSource::networkReadHandler()\n");
  source->networkReadHandler1();
}

void DenseMultiFramedRTPSource::networkReadHandler1()
{
  UsageEnvironment &env = envir();
  env << "TEST: DenseMultiFramedRTPSource::networkReadHandler1()\n";
  unsigned chunkY;

  DenseBufferedPacket *bPacket = fPacketReadInProgress;
  if (bPacket == NULL)
  {
    // Normal case: Get a free DenseBufferedPacket descriptor to hold the new network packet:
    bPacket = fReorderingBuffer->getFreePacket(this);
  }

  // Read the network packet, and perform sanity checks on the RTP header:
  Boolean readSuccess = False;
  do
  {
    struct sockaddr_in fromAddress;
    Boolean packetReadWasIncomplete = fPacketReadInProgress != NULL;
    if (!bPacket->fillInData(fRTPInterface, fromAddress, packetReadWasIncomplete))
    {
      if (bPacket->bytesAvailable() == 0)
      { // should not happen??
        envir() << "MultiFramedRTPSource internal error: Hit limit when reading incoming packet over TCP\n";
      }
      fPacketReadInProgress = NULL;
      break;
    }
    if (packetReadWasIncomplete)
    {
      // We need additional read(s) before we can process the incoming packet:
      fPacketReadInProgress = bPacket;
      return;
    }
    else
    {
      fPacketReadInProgress = NULL;
    }

#ifdef TEST_LOSS
    setPacketReorderingThresholdTime(0);
    // don't wait for 'lost' packets to arrive out-of-order later
    if ((our_random() % 10) == 0)
      break; // simulate 10% packet loss
#endif

    // Check for the 12-byte RTP header:
    if (bPacket->dataSize() < 12){
      break;
    }

    unsigned rtpHdr = ntohl(*(u_int32_t *)(bPacket->data()));
    ADVANCE(4);
    Boolean rtpMarkerBit = (rtpHdr & 0x00800000) != 0;
    unsigned rtpTimestamp = ntohl(*(u_int32_t *)(bPacket->data()));
    ADVANCE(4);
    unsigned rtpSSRC = ntohl(*(u_int32_t *)(bPacket->data()));
    ADVANCE(4);

    // Check the RTP version number (it should be 2):
    if ((rtpHdr & 0xC0000000) != 0x80000000)
      break;

    // Check the Payload Type.
    unsigned char rtpPayloadType = (unsigned char)((rtpHdr & 0x007F0000) >> 16);
    if (rtpPayloadType != rtpPayloadFormat())
    {
      if (fRTCPInstanceForMultiplexedRTCPPackets != NULL && rtpPayloadType >= 64 && rtpPayloadType <= 95)
      {
        // This is a multiplexed RTCP packet, and we've been asked to deliver such packets.
        // Do so now:
        fRTCPInstanceForMultiplexedRTCPPackets
            ->injectReport(bPacket->data() - 12, bPacket->dataSize() + 12, fromAddress);
      }
      break;
    }

    // Skip over any CSRC identifiers in the header:
    unsigned cc = (rtpHdr >> 24) & 0x0F;
    if (bPacket->dataSize() < cc * 4)
      break;
    ADVANCE(cc * 4);

    // Check for (& ignore) any RTP header extension
    if (rtpHdr & 0x10000000)
    {
      if (bPacket->dataSize() < 4)
        break;
      unsigned extHdr = ntohl(*(u_int32_t *)(bPacket->data()));
      ADVANCE(4);
      unsigned remExtSize = 4 * (extHdr & 0xFFFF);
      chunkY = extHdr >> 16;
      if (bPacket->dataSize() < remExtSize)
        break;
      ADVANCE(remExtSize);
    }

    // Discard any padding bytes:
    if (rtpHdr & 0x20000000)
    {
      if (bPacket->dataSize() == 0)
        break;
      unsigned numPaddingBytes = (unsigned)(bPacket->data())[bPacket->dataSize() - 1];
      if (bPacket->dataSize() < numPaddingBytes)
        break;
      bPacket->removePadding(numPaddingBytes);
    }

    // The rest of the packet is the usable data.  Record and save it:
    if (rtpSSRC != fLastReceivedSSRC)
    {
      // The SSRC of incoming packets has changed.  Unfortunately we don't yet handle streams that contain multiple SSRCs,
      // but we can handle a single-SSRC stream where the SSRC changes occasionally:
      fLastReceivedSSRC = rtpSSRC;
      fReorderingBuffer->resetHaveSeenFirstPacket();
    }
    unsigned short rtpSeqNo = (unsigned short)(rtpHdr & 0xFFFF);
    Boolean usableInJitterCalculation = packetIsUsableInJitterCalculation((bPacket->data()),
                                                                          bPacket->dataSize());
    struct timeval presentationTime; // computed by:
    Boolean hasBeenSyncedUsingRTCP;  // computed by:
    receptionStatsDB()
        .noteIncomingPacket(rtpSSRC, rtpSeqNo, rtpTimestamp,
                            timestampFrequency(),
                            usableInJitterCalculation, presentationTime,
                            hasBeenSyncedUsingRTCP, bPacket->dataSize());

    // Fill in the rest of the packet descriptor, and store it:
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    bPacket->assignMiscParams(rtpSeqNo, rtpTimestamp, presentationTime,
                              hasBeenSyncedUsingRTCP, rtpMarkerBit,
                              timeNow, chunkY, &fromAddress);
    if (!fReorderingBuffer->storePacket(bPacket))
      break;

    readSuccess = True;
  } while (0);
  if (!readSuccess)
    fReorderingBuffer->freePacket(bPacket);

  doGetNextFrame1();
  // If we didn't get proper data this time, we'll get another chance
}

////////// DenseBufferedPacket and DenseBufferedPacketFactory implementation /////

#define MAX_PACKET_SIZE 65536

DenseBufferedPacket::DenseBufferedPacket()
    : fPacketSize(MAX_PACKET_SIZE),
      fBuf(new unsigned char[MAX_PACKET_SIZE]),
      fNextPacket(NULL)
{
}

DenseBufferedPacket::~DenseBufferedPacket()
{
  delete fNextPacket;
  delete[] fBuf;
}

void DenseBufferedPacket::reset()
{
  fHead = fTail = 0;
  fUseCount = 0;
  fIsFirstPacket = False; // by default
}

// The following function has been deprecated:
unsigned DenseBufferedPacket ::nextEnclosedFrameSize(unsigned char *& /*framePtr*/, unsigned dataSize)
{
  // By default, use the entire buffered data, even though it may consist
  // of more than one frame, on the assumption that the client doesn't
  // care.  (This is more efficient than delivering a frame at a time)
  return dataSize;
}

void DenseBufferedPacket ::getNextEnclosedFrameParameters(unsigned char *&framePtr, unsigned dataSize,
                                                          unsigned &frameSize,
                                                          unsigned &frameDurationInMicroseconds)
{
  // By default, use the entire buffered data, even though it may consist
  // of more than one frame, on the assumption that the client doesn't
  // care.  (This is more efficient than delivering a frame at a time)

  // For backwards-compatibility with existing uses of (the now deprecated)
  // "nextEnclosedFrameSize()", call that function to implement this one:
  frameSize = nextEnclosedFrameSize(framePtr, dataSize);

  frameDurationInMicroseconds = 0; // by default.  Subclasses should correct this.
}

Boolean DenseBufferedPacket::fillInData(RTPInterface &rtpInterface, struct sockaddr_in &fromAddress,
                                        Boolean &packetReadWasIncomplete)
{
  if (!packetReadWasIncomplete)
    reset();

  unsigned const maxBytesToRead = bytesAvailable();
  if (maxBytesToRead == 0)
    return False; // exceeded buffer size when reading over TCP

  unsigned numBytesRead;
  int tcpSocketNum;                 // not used
  unsigned char tcpStreamChannelId; // not used
  if (!rtpInterface.handleRead(&fBuf[fTail], maxBytesToRead,
                               numBytesRead, fromAddress,
                               tcpSocketNum, tcpStreamChannelId,
                               packetReadWasIncomplete))
  {
    return False;
  }
  fTail += numBytesRead;
  return True;
}

void DenseBufferedPacket::assignMiscParams(
    unsigned short rtpSeqNo, unsigned rtpTimestamp,
    struct timeval presentationTime,
    Boolean hasBeenSyncedUsingRTCP, Boolean rtpMarkerBit,
    struct timeval timeReceived,
    unsigned short chunkRef,
    struct sockaddr_in *addr)
{
  fRTPSeqNo = rtpSeqNo;
  fRTPTimestamp = rtpTimestamp;
  fPresentationTime = presentationTime;
  fHasBeenSyncedUsingRTCP = hasBeenSyncedUsingRTCP;
  fRTPMarkerBit = rtpMarkerBit;
  fTimeReceived = timeReceived;

  // <Dense Content>
  fChunkRef = chunkRef;
  fAddr = addr; // Note: Not used ... :(
  // </Dense Content>
}

void DenseBufferedPacket::skip(unsigned numBytes)
{
  fHead += numBytes;
  if (fHead > fTail)
    fHead = fTail;
}

void DenseBufferedPacket::removePadding(unsigned numBytes)
{
  if (numBytes > fTail - fHead)
    numBytes = fTail - fHead;
  fTail -= numBytes;
}

void DenseBufferedPacket::appendData(unsigned char *newData, unsigned numBytes)
{
  if (numBytes > fPacketSize - fTail)
    numBytes = fPacketSize - fTail;
  memmove(&fBuf[fTail], newData, numBytes);
  fTail += numBytes;
}

void DenseBufferedPacket::use(unsigned char *to, unsigned toSize,
                              unsigned &bytesUsed, unsigned &bytesTruncated,
                              unsigned short &rtpSeqNo, unsigned &rtpTimestamp,
                              struct timeval &presentationTime,
                              Boolean &hasBeenSyncedUsingRTCP,
                              Boolean &rtpMarkerBit)
{
  unsigned char *origFramePtr = &fBuf[fHead];
  unsigned char *newFramePtr = origFramePtr; // may change in the call below
  unsigned frameSize, frameDurationInMicroseconds;
  getNextEnclosedFrameParameters(newFramePtr, fTail - fHead,
                                 frameSize, frameDurationInMicroseconds);
  if (frameSize > toSize)
  {
    bytesTruncated += frameSize - toSize;
    bytesUsed = toSize;
  }
  else
  {
    bytesTruncated = 0;
    bytesUsed = frameSize;
  }

  memmove(to, newFramePtr, bytesUsed);
  fHead += (newFramePtr - origFramePtr) + frameSize;
  ++fUseCount;

  rtpSeqNo = fRTPSeqNo;
  rtpTimestamp = fRTPTimestamp;
  presentationTime = fPresentationTime;
  hasBeenSyncedUsingRTCP = fHasBeenSyncedUsingRTCP;
  rtpMarkerBit = fRTPMarkerBit;

  // Update "fPresentationTime" for the next enclosed frame (if any):
  fPresentationTime.tv_usec += frameDurationInMicroseconds;
  if (fPresentationTime.tv_usec >= 1000000)
  {
    fPresentationTime.tv_sec += fPresentationTime.tv_usec / 1000000;
    fPresentationTime.tv_usec = fPresentationTime.tv_usec % 1000000;
  }
}

DenseBufferedPacketFactory::DenseBufferedPacketFactory()
{
}

DenseBufferedPacketFactory::~DenseBufferedPacketFactory()
{
}

DenseBufferedPacket *DenseBufferedPacketFactory ::createNewPacket(DenseMultiFramedRTPSource * /*ourSource*/)
{
  return new DenseBufferedPacket;
}

////////// DenseReorderingPacketBuffer implementation //////////

DenseReorderingPacketBuffer ::DenseReorderingPacketBuffer(DenseBufferedPacketFactory *packetFactory)
    : fThresholdTime(100000) /* default reordering threshold: 100 ms */,
      fHaveSeenFirstPacket(False), fHeadPacket(NULL), fTailPacket(NULL), fSavedPacket(NULL), fSavedPacketFree(True)
{
  fPacketFactory = (packetFactory == NULL)
                       ? (new DenseBufferedPacketFactory)
                       : packetFactory;
}

DenseReorderingPacketBuffer::~DenseReorderingPacketBuffer()
{
  reset();
  delete fPacketFactory;
}

void DenseReorderingPacketBuffer::reset()
{
  if (fSavedPacketFree)
    delete fSavedPacket; // because fSavedPacket is not in the list
  delete fHeadPacket;    // will also delete fSavedPacket if it's in the list
  resetHaveSeenFirstPacket();
  fHeadPacket = fTailPacket = fSavedPacket = NULL;
}

DenseBufferedPacket *DenseReorderingPacketBuffer::getFreePacket(DenseMultiFramedRTPSource *ourSource)
{
  if (fSavedPacket == NULL)
  { // we're being called for the first time
    fSavedPacket = fPacketFactory->createNewPacket(ourSource);
    fSavedPacketFree = True;
  }

  if (fSavedPacketFree == True)
  {
    fSavedPacketFree = False;
    return fSavedPacket;
  }
  else
  {
    return fPacketFactory->createNewPacket(ourSource);
  }
}

Boolean DenseReorderingPacketBuffer::storePacket(DenseBufferedPacket *bPacket)
{
  unsigned short rtpSeqNo = bPacket->rtpSeqNo();

  if (!fHaveSeenFirstPacket)
  {
    fNextExpectedSeqNo = rtpSeqNo; // initialization
    bPacket->isFirstPacket() = True;
    fHaveSeenFirstPacket = True;
  }

  // Ignore this packet if its sequence number is less than the one
  // that we're looking for (in this case, it's been excessively delayed).
  if (seqNumLT(rtpSeqNo, fNextExpectedSeqNo))
    return False;

  if (fTailPacket == NULL)
  {
    // Common case: There are no packets in the queue; this will be the first one:
    bPacket->nextPacket() = NULL;
    fHeadPacket = fTailPacket = bPacket;
    return True;
  }

  if (seqNumLT(fTailPacket->rtpSeqNo(), rtpSeqNo))
  {
    // The next-most common case: There are packets already in the queue; this packet arrived in order => put it at the tail:
    bPacket->nextPacket() = NULL;
    fTailPacket->nextPacket() = bPacket;
    fTailPacket = bPacket;
    return True;
  }

  if (rtpSeqNo == fTailPacket->rtpSeqNo())
  {
    // This is a duplicate packet - ignore it
    return False;
  }

  // Rare case: This packet is out-of-order.  Run through the list (from the head), to figure out where it belongs:
  DenseBufferedPacket *beforePtr = NULL;
  DenseBufferedPacket *afterPtr = fHeadPacket;
  while (afterPtr != NULL)
  {
    if (seqNumLT(rtpSeqNo, afterPtr->rtpSeqNo()))
      break; // it comes here
    if (rtpSeqNo == afterPtr->rtpSeqNo())
    {
      // This is a duplicate packet - ignore it
      return False;
    }

    beforePtr = afterPtr;
    afterPtr = afterPtr->nextPacket();
  }

  // Link our new packet between "beforePtr" and "afterPtr":
  bPacket->nextPacket() = afterPtr;
  if (beforePtr == NULL)
  {
    fHeadPacket = bPacket;
  }
  else
  {
    beforePtr->nextPacket() = bPacket;
  }

  return True;
}

void DenseReorderingPacketBuffer::releaseUsedPacket(DenseBufferedPacket *packet)
{
  // ASSERT: packet == fHeadPacket
  // ASSERT: fNextExpectedSeqNo == packet->rtpSeqNo()
  ++fNextExpectedSeqNo; // because we're finished with this packet now

  fHeadPacket = fHeadPacket->nextPacket();
  if (!fHeadPacket)
  {
    fTailPacket = NULL;
  }
  packet->nextPacket() = NULL;

  freePacket(packet);
}

DenseBufferedPacket *DenseReorderingPacketBuffer::getNextCompletedPacket(Boolean &packetLossPreceded)
{
  if (fHeadPacket == NULL)
  {
    fprintf(stderr, "fHeadPacket == NULL!\n");
    return NULL;
  }

  // Check whether the next packet we want is already at the head
  // of the queue:
  // ASSERT: fHeadPacket->rtpSeqNo() >= fNextExpectedSeqNo
  if (fHeadPacket->rtpSeqNo() == fNextExpectedSeqNo)
  {
    packetLossPreceded = fHeadPacket->isFirstPacket();
    // (The very first packet is treated as if there was packet loss beforehand.)
    return fHeadPacket;
  }

  // We're still waiting for our desired packet to arrive.  However, if
  // our time threshold has been exceeded, then forget it, and return
  // the head packet instead:
  Boolean timeThresholdHasBeenExceeded;
  if (fThresholdTime == 0)
  {
    timeThresholdHasBeenExceeded = True; // optimization
  }
  else
  {
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    unsigned uSecondsSinceReceived = (timeNow.tv_sec - fHeadPacket->timeReceived().tv_sec) * 1000000 + (timeNow.tv_usec - fHeadPacket->timeReceived().tv_usec);
    timeThresholdHasBeenExceeded = uSecondsSinceReceived > fThresholdTime;
  }
  if (timeThresholdHasBeenExceeded)
  {
    fNextExpectedSeqNo = fHeadPacket->rtpSeqNo();
    // we've given up on earlier packets now
    packetLossPreceded = True;
    return fHeadPacket;
  }

  // Otherwise, keep waiting for our desired packet to arrive:
  return NULL;
}

DenseBufferedPacket *DenseReorderingPacketBuffer::getLastCompletedPacket()
{
  return fTailPacket;
}
