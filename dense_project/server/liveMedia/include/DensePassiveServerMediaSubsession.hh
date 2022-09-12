#ifndef _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#define _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#include "PassiveServerMediaSubsession.hh"
#endif

#include "GroupsockHelper.hh"

class DensePassiveServerMediaSubsession : public PassiveServerMediaSubsession
{
public:
  static DensePassiveServerMediaSubsession *createNew(
      RTPSink &rtpSink,
      RTCPInstance *rtcpInstance = NULL);
  void getInstances(Port &serverRTPPort, Port &serverRTCPPort);
  Groupsock gs();
  Groupsock RTCPgs();

protected:
  DensePassiveServerMediaSubsession(RTPSink &rtpSink, RTCPInstance *rtcpInstance);
  // called only by createNew();
  virtual ~DensePassiveServerMediaSubsession();
};

#endif //_DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH