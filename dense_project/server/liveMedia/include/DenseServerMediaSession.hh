#ifndef _DENSE_SERVER_MEDIA_SESSION_HH
#define _DENSE_SERVER_MEDIA_SESSION_HH

// Live Imports
#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif

#ifndef _GROUPSOCK_HELPER_HH
#include "GroupsockHelper.hh"
#endif

// Dense Imports
#ifndef _DENSE_PASSIVE_SERVER_MEDIA_SUBSESSION_HH
#include "DensePassiveServerMediaSubsession.hh"
#endif

// C++ Imports
#include <vector>

////// DenseServerMediaSession ///////
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
  // Note: These were moved from private to public, but they might not be needed anymore since we have the vector!
  ServerMediaSubsession *fSubsessionsHead;
  ServerMediaSubsession *fSubsessionsTail;

  std::vector<ServerMediaSubsession *> fServerMediaSubsessionVector;
};

#endif // _DENSE_SERVER_MEDIA_SESSION_HH