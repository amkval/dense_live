#include "MultiFramedRTPSink.hh"

#include "DenseRTSPServer.hh"

class ManifestRTPSink : public MultiFramedRTPSink
{
public:
  static ManifestRTPSink *createNew(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels = 1,
      Boolean allowMultipleFramesPerPacket = True,
      Boolean doNormalMBitRule = True,
      const char *name = NULL);

protected:
  ManifestRTPSink(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels,
      Boolean allowMultipleFramesPerPacket,
      Boolean doNormalMBitRule,
      const char *name);
  // called only by createNew()

  virtual ~ManifestRTPSink();

public:
  void ManifestRTPSink::setCheckSource(CheckSource *checkSource)
  {
    fprintf(stderr, "ManifestRTPSink::setCheckSource()\n");
    fCheckSource = checkSource;
  }

protected:
  CheckSource *fCheckSource;
};
