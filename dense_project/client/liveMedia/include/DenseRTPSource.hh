#ifndef _DENSE_RTP_SOURCE_HH
#define _DENSE_RTP_SOURCE_HH

#ifndef _SIMPLE_RTP_SOURCE_HH
#include "SimpleRTPSource.hh"
#endif

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

class DenseRTPSource : public SimpleRTPSource
{
public:
  static DenseRTPSource *createNew(
      UsageEnvironment &env, Groupsock *RTPgs,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *mimeTypeString,
      unsigned offset = 0,
      Boolean doNormalMBitRule = True);

protected:
  DenseRTPSource(
      UsageEnvironment &env, Groupsock *RTPgs,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      char const *mimeTypeString,
      unsigned offset,
      Boolean doNormalMBitRule);
  virtual ~DenseRTPSource();

public:
  DenseMediaSubsession *denseMediaSubsession() { return fDenseMediaSubsession; };
  void setDenseMediaSubsession(DenseMediaSubsession *denseMediaSubsession) { fDenseMediaSubsession = denseMediaSubsession; };

protected:
  virtual Boolean processSpecialHeader(BufferedPacket *packet, unsigned &resultSpecialHeaderSize);
  int manageQualityLevels(BufferedPacket *packet);
  void finish();
  Boolean startDown();
  Boolean startUp();
  Boolean joinZero();
  Boolean joinOne();
  Boolean joinTwo();
  void printQLF(BufferedPacket *packet);

public:
  Boolean fThisIsMoving;
  Boolean fJustMoved;
  DenseMediaSession *fDenseMediaSession;
  DenseMediaSubsession *fDenseMediaSubsession;
};

#endif //_DENSE_RTP_SOURCE_HH