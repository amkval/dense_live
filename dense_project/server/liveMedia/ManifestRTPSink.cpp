#include "include/ManifestRTPSink.hh"

ManifestRTPSink *ManifestRTPSink::createNew(
    UsageEnvironment &env, Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule,
    const char *name)
{
  return new ManifestRTPSink(
      env, RTPGroupsock, rtpPayloadFormat, rtpTimestampFrequency,
      sdpMediaTypeString, rtpPayloadFormatName, denseRTSPServer,
      numChannels, allowMultipleFramesPerPacket, doNormalMBitRule, name);
  // Init some values??? TODO:
}

ManifestRTPSink::ManifestRTPSink(
    UsageEnvironment &env, Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const *sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule,
    const char *name)
    : MultiFramedRTPSink(env, RTPGroupsock, rtpPayloadFormat,
                         rtpTimestampFrequency, rtpPayloadFormatName, numChannels),
      fAllowMultipleFramesPerPacket(allowMultipleFramesPerPacket),
      fSetMbitOnNextPacket(False)
{
  fSDPMediaTypeString = sdpMediaTypeString;

  fName = // TODO: check if name is null and replace with unknown.
  fSetMBitOnLastFrames = doNormalMBitRule && strcmp(fSDPMediaTypeString, "audio") != 0;
}
