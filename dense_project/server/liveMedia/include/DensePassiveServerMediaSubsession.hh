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