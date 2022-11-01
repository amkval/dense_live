#ifndef _DENSE_RTP_SINK_HH
#define _DENSE_RTP_SINK_HH

#ifndef _SIMPLE_RTP_SINK_HH
#include "SimpleRTPSink.hh"
#endif

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

class DenseRTPSink : public SimpleRTPSink
{
public:
  static DenseRTPSink *createNew(
      UsageEnvironment &env, Groupsock *RTPgs,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      unsigned numChannels = 1,
      Boolean allowMultipleFramesPerPacket = True,
      Boolean doNormalMBitRule = False);

protected:
  DenseRTPSink(UsageEnvironment &env, Groupsock *RTPgs,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      unsigned numChannels,
      Boolean allowMultipleFramesPerPacket,
      Boolean doNormalMBitRule);
  // Called only by createNew()

  virtual ~DenseRTPSink();

public:
  void setCheckSource(CheckSource *checkSource)
  {
    fCheckSource = checkSource;
  }

protected:
  virtual void doSpecialFrameHandling(
      unsigned fragmentationOffset,
      unsigned char *frameStart,
      unsigned numBytesInFrame,
      struct timeval framePresentationTime,
      unsigned numRemainingBytes);
      
  virtual unsigned specialHeaderSize() const;
  virtual unsigned frameSpecificHeaderSize() const;

protected:
  CheckSource *fCheckSource;
};

#endif //_DENSE_RTP_SINK_HH