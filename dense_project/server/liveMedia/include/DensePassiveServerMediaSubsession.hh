#ifndef _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#define _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH

// Live Imports
#ifndef _PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#include "PassiveServerMediaSubsession.hh"
#endif

#ifndef _GROUPSOCK_HELPER_HH
#include "GroupsockHelper.hh"
#endif

////// DensePassiveServerMediaSubsession //////
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
  DensePassiveServerMediaSubsession(
    RTPSink &rtpSink, RTCPInstance *rtcpInstance);
  // called only by createNew();
  virtual ~DensePassiveServerMediaSubsession();
};

#endif //_DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH