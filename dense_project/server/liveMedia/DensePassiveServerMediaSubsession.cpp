#include "include/DensePassiveServerMediaSubsession.hh"

DensePassiveServerMediaSubsession::DensePassiveServerMediaSubsession(RTPSink &rtpSink, RTCPInstance *rtcpInstance) : PassiveServerMediaSubsession(rtpSink, rtcpInstance)
{
}

DensePassiveServerMediaSubsession::~DensePassiveServerMediaSubsession()
{
}

void DensePassiveServerMediaSubsession::getInstances(Port &serverRTPPort, Port &serverRTCPPort)
{
  Groupsock &gs = fRTPSink.groupsockBeingUsed();

  serverRTPPort = gs.port();
  Groupsock const &rtcpGS = *(fRTCPInstance->RTCPgs());

  serverRTCPPort = rtcpGS.port();

  AddressString ipAddressStr(ourIPAddress(envir()));
  unsigned ipAddressStrSize = strlen(ipAddressStr.val());

  ipAddressStrSize = ipAddressStrSize + 1;

  char *adr = new char[ipAddressStrSize + 2];
  strcpy(adr, ipAddressStr.val());
  adr[ipAddressStrSize + 1] = '\0';
}

Groupsock DensePassiveServerMediaSubsession::gs()
{
  return fRTPSink.groupsockBeingUsed();
}

Groupsock DensePassiveServerMediaSubsession::RTCPgs()
{
  return *(fRTCPInstance->RTCPgs());
}

// Note: sdpLines is also modified, but does it actually do anything (differently?)?