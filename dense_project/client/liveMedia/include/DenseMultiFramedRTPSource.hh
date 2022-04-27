// This is an attempt copy the original file and include the modifications. while potentially removing some essessive stuff.

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
// RTP source for a common kind of payload format: Those which pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// C++ header

#ifndef _DENSE_MULTI_FRAMED_RTP_SOURCE_HH
#define _DENSE_MULTI_FRAMED_RTP_SOURCE_HH

#ifndef _RTP_SOURCE_HH
#include "RTPSource.hh"
#endif

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

#ifndef _DENSE_MEDIA_SUBSESSION_HH
#include "DenseMediaSubsession.hh"
#endif

#include "RTCP.hh"
#include "GroupsockHelper.hh"

#include <string.h>

class DenseBufferedPacket;        // forward
class DenseBufferedPacketFactory; // forward

class DenseMultiFramedRTPSource : public RTPSource
{
protected:
  DenseMultiFramedRTPSource(UsageEnvironment &env, Groupsock *RTPgs,
                            unsigned char rtpPayloadFormat,
                            unsigned rtpTimestampFrequency,
                            DenseBufferedPacketFactory *packetFactory = NULL);
  // virtual base class
  virtual ~DenseMultiFramedRTPSource();

  virtual Boolean processSpecialHeader(DenseBufferedPacket *packet,
                                       unsigned &resultSpecialHeaderSize);
  // Subclasses redefine this to handle any special, payload format
  // specific header that follows the RTP header.

  virtual Boolean packetIsUsableInJitterCalculation(unsigned char *packet,
                                                    unsigned packetSize);
  // The default implementation returns True, but this can be redefined

public:
  // Moved from RTPSource
  DenseMediaSubsession *mediaSubsession() { return fMediaSubsession; };
  void setMediaSubsession(DenseMediaSubsession *mediaSubsession) { fMediaSubsession = mediaSubsession; };
  // </Dense Section>

protected:
  Boolean fCurrentPacketBeginsFrame;
  Boolean fCurrentPacketCompletesFrame;

protected:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

  // <Dense Section>
  // For managing quality levels
protected:
  int manageQualityLevels(DenseBufferedPacket *packet);
  void finish();
  Boolean startDown();
  Boolean startUp();
  Boolean joinZero();
  Boolean joinOne();
  Boolean joinTwo();
  void printQLF(DenseBufferedPacket *packet);
  void printQLT(DenseBufferedPacket *packet);

private:
  // redefined virtual functions:
  virtual void setPacketReorderingThresholdTime(unsigned uSeconds);

private:
  void reset();
  void doGetNextFrame1();

  static void networkReadHandler(DenseMultiFramedRTPSource *source, int /*mask*/);
  void networkReadHandler1();

  Boolean fAreDoingNetworkReads;
  DenseBufferedPacket *fPacketReadInProgress;
  Boolean fNeedDelivery;
  Boolean fPacketLossInFragmentedFrame;
  unsigned char *fSavedTo;
  unsigned fSavedMaxSize;

  // A buffer to (optionally) hold incoming pkts that have been reordered
  class DenseReorderingPacketBuffer *fReorderingBuffer;

  // <Dense Section>
  class DenseReorderingPacketBuffer *QLBuffer;

  // Moved from RTPSource ˇ
public:
  Boolean fThisIsMoving;
  Boolean fJustMoved;

  DenseMediaSession *fMediaSession;

protected:
  DenseMediaSubsession *fMediaSubsession;
  // </Dense Section>
};

// A 'packet data' class that's used to implement the above.
// Note that this can be subclassed - if desired - to redefine
// "nextEnclosedFrameParameters()".

class DenseBufferedPacket
{
public:
  DenseBufferedPacket();
  virtual ~DenseBufferedPacket();

  Boolean hasUsableData() const { return fTail > fHead; }
  unsigned useCount() const { return fUseCount; }

  Boolean fillInData(RTPInterface &rtpInterface, struct sockaddr_in &fromAddress, Boolean &packetReadWasIncomplete);
  void assignMiscParams(unsigned short rtpSeqNo, unsigned rtpTimestamp,
                        struct timeval presentationTime,
                        Boolean hasBeenSyncedUsingRTCP,
                        Boolean rtpMarkerBit, struct timeval timeReceived,
                        unsigned short chunkRef, struct sockaddr_in *addr);
  void skip(unsigned numBytes);          // used to skip over an initial header
  void removePadding(unsigned numBytes); // used to remove trailing bytes
  void appendData(unsigned char *newData, unsigned numBytes);
  void use(unsigned char *to, unsigned toSize,
           unsigned &bytesUsed, unsigned &bytesTruncated,
           unsigned short &rtpSeqNo, unsigned &rtpTimestamp,
           struct timeval &presentationTime,
           Boolean &hasBeenSyncedUsingRTCP, Boolean &rtpMarkerBit);

  DenseBufferedPacket *&nextPacket() { return fNextPacket; }

  unsigned short rtpSeqNo() const { return fRTPSeqNo; }
  struct timeval const &timeReceived() const { return fTimeReceived; }

  unsigned char *data() const { return &fBuf[fHead]; }
  unsigned dataSize() const { return fTail - fHead; }
  Boolean rtpMarkerBit() const { return fRTPMarkerBit; }
  Boolean &isFirstPacket() { return fIsFirstPacket; }
  unsigned bytesAvailable() const { return fPacketSize - fTail; }

  // <Dense Section>
  unsigned short chunk() const { return fChunkRef; }
  // </Dense Section>

protected:
  virtual void reset();
  virtual unsigned nextEnclosedFrameSize(unsigned char *&framePtr,
                                         unsigned dataSize);
  // The above function has been deprecated.  Instead, new subclasses should use:
  virtual void getNextEnclosedFrameParameters(unsigned char *&framePtr,
                                              unsigned dataSize,
                                              unsigned &frameSize,
                                              unsigned &frameDurationInMicroseconds);

  unsigned fPacketSize;
  unsigned char *fBuf;
  unsigned fHead;
  unsigned fTail;

private:
  // <Dense Section>
  unsigned short fChunkRef;
  struct sockaddr_in *fAddr;
  // </Dense Section>

  DenseBufferedPacket *fNextPacket; // used to link together packets

  unsigned fUseCount;
  unsigned short fRTPSeqNo;
  unsigned fRTPTimestamp;
  struct timeval fPresentationTime; // corresponding to "fRTPTimestamp"
  Boolean fHasBeenSyncedUsingRTCP;
  Boolean fRTPMarkerBit;
  Boolean fIsFirstPacket;
  struct timeval fTimeReceived;
};

// A 'factory' class for creating "DenseBufferedPacket" objects.
// If you want to subclass "DenseBufferedPacket", then you'll also
// want to subclass this, to redefine createNewPacket()

class DenseBufferedPacketFactory
{
public:
  DenseBufferedPacketFactory();
  virtual ~DenseBufferedPacketFactory();

  virtual DenseBufferedPacket *createNewPacket(DenseMultiFramedRTPSource *ourSource);
};

#endif // _DENSE_MULTI_FRAMED_RTP_SOURCE_HH
