#ifndef _MANIFEST_RTP_SINK_HH
#define _MANIFEST_RTP_SINK_HH

#include "MultiFramedRTPSink.hh"

class DenseRTSPServer; // Forward

#ifndef _DENSE_RTSP_SERVER_HH
#include "DenseRTSPServer.hh"
#endif

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

class ManifestRTPSink : public MultiFramedRTPSink
{
public:
  static ManifestRTPSink *createNew(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      std::string sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels = 1,
      Boolean allowMultipleFramesPerPacket = True,
      Boolean doNormalMBitRule = True,
      std::string name = "");

protected:
  ManifestRTPSink(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      std::string sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels,
      Boolean allowMultipleFramesPerPacket,
      Boolean doNormalMBitRule,
      std::string name);
  // called only by createNew()

  virtual ~ManifestRTPSink();

public:
  void setCheckSource(CheckSource *checkSource)
  {
    fprintf(stderr, "ManifestRTPSink::setCheckSource()\n");
    fCheckSource = checkSource;
  }

protected:
  CheckSource *fCheckSource;
  // TODO: Are all of these used?
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnNextPacket;
  std::string fSDPMediaTypeString;
  std::string fName;
  Boolean fSetMBitOnLastFrames;
};

#endif