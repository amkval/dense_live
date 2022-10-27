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
  // Note: This function is overriden to spoof fControlPath for RTSPClient
  // This is done to avoid having to edit / copy the whole file.
  // This may break another call... and may break things. Better check it out!

  // What is the point rally?

  // Is it even called anywhere?!
  char const *controlPath() const
  {
    std::string denseControlPath = fControlPath;
    fprintf(stdout, "#### original ControlPath:%s\n", denseControlPath.c_str());
    int len = denseControlPath.size();
    std::string level = std::to_string(fLevel);
    denseControlPath.replace(len - 2, 1, level);
    fprintf(stdout, "#### new ControlPath:%s\n", denseControlPath.c_str());
    return denseControlPath.c_str();
  }

public:
  Boolean denseInitiate();
protected:
  Boolean denseCreateSourceObjects();

public:
  int fLevel;                       // Quality level of subsession
  DenseMediaSession &fMediaSession; // Dense Media Session
};

#endif // _DENSE_MEDIA_SUBSESSION_HH