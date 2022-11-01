#ifndef _DENSE_MEDIA_SUBSESSION_HH
#define _DENSE_MEDIA_SUBSESSION_HH

// Live Imports
#ifndef _GROUPSOCK_HELPER_HH
#include "GroupsockHelper.hh"
#endif

// Dense Imports
#ifndef _DENSE_FILE_SINK_HH
#include "DenseFileSink.hh"
#endif

#ifndef _DENSE_RTP_SOURCE_HH
#include "DenseRTPSource.hh"
#endif

// C++ Imports
#include <string>

// Forward class declaration
class DenseMediaSession;

////// DenseMediaSubsession //////
class DenseMediaSubsession : public MediaSubsession
{
public:
  static DenseMediaSubsession *createNew(
      UsageEnvironment &env,
      DenseMediaSession &mediaSession,
      int level);
  friend class DenseMediaSession;

protected:
  DenseMediaSubsession(
      UsageEnvironment &env,
      DenseMediaSession &mediaSession,
      int level);
  virtual ~DenseMediaSubsession();

public:
  Boolean denseInitiate();
protected:
  Boolean denseCreateSourceObjects();

public:
  int fLevel;                       // Quality level of subsession.
  DenseMediaSession &fMediaSession; // Parent session.
};

#endif // _DENSE_MEDIA_SUBSESSION_HH