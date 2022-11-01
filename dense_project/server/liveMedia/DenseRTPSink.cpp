
#include "include/DenseRTPSink.hh"

////// DenseRTPSink //////

DenseRTPSink *DenseRTPSink::createNew(
    UsageEnvironment &env, Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *sdpMediaTypeString,
    char const *rtpPayloadFormatName,
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
      fCheckSource(NULL)
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

  //fprintf(stdout, "################## Special Frame Handling! ##################\n");
  // if (isFirstFrameInPacket())
  //{
  unsigned short chunkRef = fCheckSource->getNowChunk();
  unsigned vdhdr = chunkRef << 16;
  vdhdr |= 1;
  setSpecialHeaderWord(vdhdr);
  //}

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(
      fragmentationOffset,
      frameStart, numBytesInFrame,
      framePresentationTime,
      numRemainingBytes);
}

unsigned DenseRTPSink::specialHeaderSize() const
{
  return 4; // Note: Correct?
}

unsigned DenseRTPSink::frameSpecificHeaderSize() const
{
  return 0; // Note: What is this for? Same as before
}