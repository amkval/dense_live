#include "include/ManifestRTPSink.hh"

ManifestRTPSink *ManifestRTPSink::createNew(
    UsageEnvironment &env, Groupsock *RTPGroupsock,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    std::string sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule,
    std::string name)
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
    std::string sdpMediaTypeString,
    char const *rtpPayloadFormatName,
    DenseRTSPServer *denseRTSPServer,
    unsigned numChannels,
    Boolean allowMultipleFramesPerPacket,
    Boolean doNormalMBitRule,
    std::string name)
    : MultiFramedRTPSink(env, RTPGroupsock, rtpPayloadFormat,
                         rtpTimestampFrequency, rtpPayloadFormatName, numChannels),
      fAllowMultipleFramesPerPacket(allowMultipleFramesPerPacket),
      fSetMBitOnNextPacket(False)
{
  fSDPMediaTypeString = sdpMediaTypeString.empty() ? "unknown" : sdpMediaTypeString;
  fName = name.empty() ? "unknown" : name;
  fSetMBitOnLastFrames = doNormalMBitRule && fSDPMediaTypeString.compare("audio") != 0;
}

ManifestRTPSink::~ManifestRTPSink()
{
}
