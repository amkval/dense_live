// This is a copy of the original file with small changes
// This is bcause we need to access private values.

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
// RTP Sinks
// C++ header

#ifndef _DENSE_RTP_SINK_HH
#define _DENSE_RTP_SINK_HH

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif
#ifndef _RTP_INTERFACE_HH
#include "RTPInterface.hh"
#endif

#include "GroupsockHelper.hh"

class DenseRTPTransmissionStatsDB; // forward

class DenseRTPSink : public MediaSink
{
public:
  static Boolean lookupByName(
      UsageEnvironment &env, char const *sinkName,
      DenseRTPSink *&resultSink);

  // Dense: The following two lines was previously changed.
  // used by RTSP servers:
  Groupsock const &groupsockBeingUsed() const { return *(fRTPInterface.gs()); }
  Groupsock &groupsockBeingUsed() { return *(fRTPInterface.gs()); }

  unsigned char rtpPayloadType() const { return fRTPPayloadType; }
  unsigned rtpTimestampFrequency() const { return fTimestampFrequency; }
  void setRTPTimestampFrequency(unsigned freq)
  {
    fTimestampFrequency = freq;
  }
  char const *rtpPayloadFormatName() const { return fRTPPayloadFormatName; }

  unsigned numChannels() const { return fNumChannels; }

  virtual char const *sdpMediaType() const; // for use in SDP m= lines
  virtual char *rtpmapLine() const;         // returns a string to be delete[]d
  virtual char const *auxSDPLine();
  // optional SDP line (e.g. a=fmtp:...)

  u_int16_t currentSeqNo() const { return fSeqNo; }
  u_int32_t presetNextTimestamp();
  // ensures that the next timestamp to be used will correspond to
  // the current 'wall clock' time.

  // <Dense Section>
  // Note: Are these used anywhere?
  u_int32_t getBase() const { return fTimestampBase; }
  u_int32_t getFirstTimeStamp() const { return fFirstTimeStamp; }
  unsigned char getFreq() const { return fTimestampBase; }
  // </Dense Section>

  DenseRTPTransmissionStatsDB &transmissionStatsDB() const
  {
    return *fTransmissionStatsDB;
  }

  Boolean nextTimestampHasBeenPreset() const { return fNextTimestampHasBeenPreset; }
  Boolean &enableRTCPReports() { return fEnableRTCPReports; }

  void getTotalBitrate(unsigned &outNumBytes, double &outElapsedTime);
  // returns the number of bytes sent since the last time that we
  // were called, and resets the counter.

  struct timeval const &creationTime() const { return fCreationTime; }
  struct timeval const &initialPresentationTime() const { return fInitialPresentationTime; }
  struct timeval const &mostRecentPresentationTime() const { return fMostRecentPresentationTime; }
  void resetPresentationTimes();

  // Hacks to allow sending RTP over TCP (RFC 2236, section 10.12):
  void setStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.setStreamSocket(sockNum, streamChannelId);
  }
  void addStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.addStreamSocket(sockNum, streamChannelId);
  }
  void removeStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.removeStreamSocket(sockNum, streamChannelId);
  }
  unsigned &estimatedBitrate() { return fEstimatedBitrate; } // kbps; usually 0 (i.e., unset)

  u_int32_t SSRC() const { return fSSRC; }
  // later need a means of changing the SSRC if there's a collision #####

protected:
  DenseRTPSink(UsageEnvironment &env,
               Groupsock *rtpGS, unsigned char rtpPayloadType,
               u_int32_t rtpTimestampFrequency,
               char const *rtpPayloadFormatName,
               unsigned numChannels);
  // abstract base class

  virtual ~DenseRTPSink();

  // used by RTCP:
  friend class RTCPInstance;
  friend class DenseRTPTransmissionStats;
  u_int32_t convertToRTPTimestamp(struct timeval tv);
  unsigned packetCount() const { return fPacketCount; }
  unsigned octetCount() const { return fOctetCount; }

protected:
  RTPInterface fRTPInterface;
  unsigned char fRTPPayloadType;
  unsigned fPacketCount, fOctetCount, fTotalOctetCount /*incl RTP hdr*/;
  struct timeval fTotalOctetCountStartTime, fInitialPresentationTime, fMostRecentPresentationTime;
  u_int32_t fCurrentTimestamp;
  u_int16_t fSeqNo;

  // <Dense Section> TODO: Is this initialized properly? It is read, this may be a problem.
  // Note: Not used?
  u_int32_t fFirstTimeStamp;
  // <Dense Section>

private:
  // redefined virtual functions:
  virtual Boolean isRTPSink() const;

private:
  u_int32_t fSSRC, fTimestampBase;
  unsigned char fTimestampFrequency; // TODO: Check if this actually needs to be char...
  Boolean fNextTimestampHasBeenPreset;
  Boolean fEnableRTCPReports; // whether RTCP "SR" reports should be sent for this sink (default: True)
  char const *fRTPPayloadFormatName;
  unsigned fNumChannels;
  struct timeval fCreationTime;
  unsigned fEstimatedBitrate; // set on creation if known; otherwise 0

  DenseRTPTransmissionStatsDB *fTransmissionStatsDB;
};

class DenseRTPTransmissionStats; // forward

class DenseRTPTransmissionStatsDB
{
public:
  unsigned numReceivers() const { return fNumReceivers; }

  class Iterator
  {
  public:
    Iterator(DenseRTPTransmissionStatsDB &receptionStatsDB);
    virtual ~Iterator();

    DenseRTPTransmissionStats *next();
    // NULL if none

  private:
    HashTable::Iterator *fIter;
  };

  // The following is called whenever a RTCP RR packet is received:
  void noteIncomingRR(u_int32_t SSRC, struct sockaddr_in const &lastFromAddress,
                      unsigned lossStats, unsigned lastPacketNumReceived,
                      unsigned jitter, unsigned lastSRTime, unsigned diffSR_RRTime);

  // The following is called when a RTCP BYE packet is received:
  void removeRecord(u_int32_t SSRC);

  DenseRTPTransmissionStats *lookup(u_int32_t SSRC) const;

private: // constructor and destructor, called only by DenseRTPSink:
  friend class DenseRTPSink;
  DenseRTPTransmissionStatsDB(DenseRTPSink &rtpSink);
  virtual ~DenseRTPTransmissionStatsDB();

private:
  void add(u_int32_t SSRC, DenseRTPTransmissionStats *stats);

private:
  friend class Iterator;
  unsigned fNumReceivers;
  DenseRTPSink &fOurDenseRTPSink;
  HashTable *fTable;
};

class DenseRTPTransmissionStats
{
public:
  u_int32_t SSRC() const { return fSSRC; }
  struct sockaddr_in const &lastFromAddress() const { return fLastFromAddress; }
  unsigned lastPacketNumReceived() const { return fLastPacketNumReceived; }
  unsigned firstPacketNumReported() const { return fFirstPacketNumReported; }
  unsigned totNumPacketsLost() const { return fTotNumPacketsLost; }
  unsigned jitter() const { return fJitter; }
  unsigned lastSRTime() const { return fLastSRTime; }
  unsigned diffSR_RRTime() const { return fDiffSR_RRTime; }
  unsigned roundTripDelay() const;
  // The round-trip delay (in units of 1/65536 seconds) computed from
  // the most recently-received RTCP RR packet.
  struct timeval const &timeCreated() const { return fTimeCreated; }
  struct timeval const &lastTimeReceived() const { return fTimeReceived; }
  void getTotalOctetCount(u_int32_t &hi, u_int32_t &lo);
  void getTotalPacketCount(u_int32_t &hi, u_int32_t &lo);

  // Information which requires at least two RRs to have been received:
  unsigned packetsReceivedSinceLastRR() const;
  u_int8_t packetLossRatio() const { return fPacketLossRatio; }
  // as an 8-bit fixed-point number
  int packetsLostBetweenRR() const;

private:
  // called only by DenseRTPTransmissionStatsDB:
  friend class DenseRTPTransmissionStatsDB;
  DenseRTPTransmissionStats(DenseRTPSink &rtpSink, u_int32_t SSRC);
  virtual ~DenseRTPTransmissionStats();

  void noteIncomingRR(struct sockaddr_in const &lastFromAddress,
                      unsigned lossStats, unsigned lastPacketNumReceived,
                      unsigned jitter,
                      unsigned lastSRTime, unsigned diffSR_RRTime);

private:
  DenseRTPSink &fOurDenseRTPSink;
  u_int32_t fSSRC;
  struct sockaddr_in fLastFromAddress;
  unsigned fLastPacketNumReceived;
  u_int8_t fPacketLossRatio;
  unsigned fTotNumPacketsLost;
  unsigned fJitter;
  unsigned fLastSRTime;
  unsigned fDiffSR_RRTime;
  struct timeval fTimeCreated, fTimeReceived;
  Boolean fAtLeastTwoRRsHaveBeenReceived;
  unsigned fOldLastPacketNumReceived;
  unsigned fOldTotNumPacketsLost;
  Boolean fFirstPacket;
  unsigned fFirstPacketNumReported;
  u_int32_t fLastOctetCount, fTotalOctetCount_hi, fTotalOctetCount_lo;
  u_int32_t fLastPacketCount, fTotalPacketCount_hi, fTotalPacketCount_lo;
};

#endif // _DENSE_RTP_SINK_HH
