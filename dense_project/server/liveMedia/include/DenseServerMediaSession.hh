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

#ifndef _DENSE_SERVER_MEDIA_SESSION_HH
#define _DENSE_SERVER_MEDIA_SESSION_HH

#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif

#ifndef _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#include "DensePassiveServerMediaSubsession.hh"
#endif

#include "GroupsockHelper.hh"

#include <vector>

class DenseServerMediaSession : public ServerMediaSession
{
public:
  Boolean addTimestamp(char const *timestamp);
  static DenseServerMediaSession *createNew(
      UsageEnvironment &env,
      char const *streamName,
      char const *info,
      char const *description,
      Boolean isSSM,
      char const *miscSDPLines);

protected:
  DenseServerMediaSession(
      UsageEnvironment &env,
      char const *streamName,
      char const *info,
      char const *description,
      Boolean isSSM,
      char const *miscSDPLines);
  ~DenseServerMediaSession();

public:
  // char *DenseServerMediaSession::generateSDPDescription();
  Boolean addSubsession(ServerMediaSubsession *subsession);

public:
  // TODO: These are moved from private to public... is this needed?
  // and how do we deal with it?
  // ServerMediaSubsession *fDenseHead;
  // ServerMediaSubsession *fDenseTail;

  std::vector<ServerMediaSubsession *> fServerMediaSubsessionVector;
};

#endif // _DENSE_SERVER_MEDIA_SESSION_HH