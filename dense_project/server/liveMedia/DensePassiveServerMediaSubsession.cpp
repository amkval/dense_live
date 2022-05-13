/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// TODO: additional information

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

// TODO: sdpLines is also modified, but does it actually do anything (differently?)?