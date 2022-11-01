
#include "include/DenseRTPSink.hh"

////// DenseRTPSink //////

DenseRTPSink *DenseRTPSink::createNew(
    UsageEnvironment &env, Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule)
{
  return new DenseRTPSink(
      env,
      RTPGroupsock,
      rtpPayloadFormat,
      rtpTimestampFrequency,
      sdpMediaTypeString,
      rtpPayloadFormatName,
      denseRTSPServer,
      numChannels,
      allowMultipleFramesPerPacket,
      doNormalMBitRule);
}

DenseRTPSink::DenseRTPSink(
    UsageEnvironment &env,
    Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule)
    : SimpleRTPSink(
          env,
          RTPGroupsock,
          rtpPayloadFormat,
          rtpTimestampFrequency,
          sdpMediaTypeString,
          rtpPayloadFormatName,
          numChannels,
          allowMultipleFramesPerPacket,
          doNormalMBitRule),
      fCheckSource(NULL), fDenseRTSPServer(denseRTSPServer)
{
}

DenseRTPSink::~DenseRTPSink()
{
}

void DenseRTPSink::doSpecialFrameHandling(
    unsigned fragmentationOffset,
    unsigned char *frameStart,
    unsigned numBytesInFrame,
    struct timeval framePresentationTime,
    unsigned numRemainingBytes)
{
  // Add chunk number as special header extension!
  unsigned short chunkRef = fCheckSource->getNowChunk();
  unsigned vdhdr = chunkRef << 16;
  vdhdr |= 1;
  setSpecialHeaderWord(vdhdr);

  // Start new server if 'fTime' time has passed:
  if (fDenseRTSPServer->fNextServer == NULL)
  {
    struct timeval now;
    gettimeofday(&now, NULL);

    int diff = now.tv_sec - fDenseRTSPServer->fStartTime.tv_sec;

    // Start a new session if fFPS time has passed.
    if (diff >= fDenseRTSPServer->fFPS)
    {
      fDenseRTSPServer->makeNextTuple();
    }
  }

  MultiFramedRTPSink::doSpecialFrameHandling(
      fragmentationOffset,
      frameStart, numBytesInFrame,
      framePresentationTime,
      numRemainingBytes);
}

// First header extension
unsigned DenseRTPSink::specialHeaderSize() const
{
  return 4; // Note: Correct?
}

// Per frame header extension
unsigned DenseRTPSink::frameSpecificHeaderSize() const
{
  return 0;
}