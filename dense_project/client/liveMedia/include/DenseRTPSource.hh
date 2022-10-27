#ifndef _DENSE_RTP_SOURCE_HH
#define _DENSE_RTP_SOURCE_HH

// Live Imports
#ifndef _SIMPLE_RTP_SOURCE_HH
#include "SimpleRTPSource.hh"
#endif

// Dense Imports
#ifndef _DENSE_MEDIA_SUBSESSION_HH
#include "DenseMediaSubsession.hh"
#endif

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

// Forward class declaration
class DenseMediaSession;
class DenseMediaSubsession;

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
  int chunk(BufferedPacket *packet);
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