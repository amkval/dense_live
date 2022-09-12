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
  ServerMediaSubsession *fSubsessionsHead;
  ServerMediaSubsession *fSubsessionsTail;

  std::vector<ServerMediaSubsession *> fServerMediaSubsessionVector;
};

#endif // _DENSE_SERVER_MEDIA_SESSION_HH