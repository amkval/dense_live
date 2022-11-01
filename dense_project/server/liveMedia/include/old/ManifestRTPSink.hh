#ifndef _MANIFEST_RTP_SINK_HH
#define _MANIFEST_RTP_SINK_HH

class DenseRTSPServer; // Forward

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

#ifndef _DENSE_MULTI_FRAMED_RTP_SINK_HH
#include "DenseMultiFramedRTPSink.hh"
#endif

#ifndef _DENSE_RTSP_SERVER_HH
#include "DenseRTSPServer.hh"
#endif

class ManifestRTPSink : public DenseMultiFramedRTPSink
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

protected: // redefined virtual functions
  virtual void doSpecialFrameHandling(
      unsigned fragmentationOffset,
      unsigned char *frameStart,
      unsigned numBytesInFrame,
      struct timeval framePresentationTime,
      unsigned numRemainingBytes);

protected: // redefined virtual functions:
  virtual Boolean continuePlaying(); //TODO: Unused? NO, very used!
  void buildAndSendPacket(Boolean isFirstPacket);
  static void sendNext(void *firstArg);
  friend void sendNext(void *);
  void packFrame();
  void sendPacketIfNecessary();

  static void afterGettingFrame(
      void *clientData,
      unsigned numBytesRead,
      unsigned numTruncatedBytes,
      struct timeval presentationTime,
      unsigned durationInMicroseconds);
  void afterGettingFrame1(
      unsigned numBytesRead,
      unsigned numTruncatedBytes,
      struct timeval presentationTime,
      unsigned durationInMicroseconds);

private:
  char const *fSDPMediaTypeString;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames, fSetMBitOnNextPacket;
  CheckSource *fCheckSource;
  const char *fName;

public:
  DenseRTSPServer *fOurServer; //TODO: Rename
};

#endif