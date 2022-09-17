#ifndef _DENSE_MEDIA_SUBSESSION_HH
#define _DENSE_MEDIA_SUBSESSION_HH

// Live Imports
#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif

#ifndef _GROUPSOCK_HELPER_HH
#include "GroupsockHelper.hh"
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
      DenseMediaSession &parent,
      DenseMediaSession *mediaSession);
  friend class DenseMediaSession;

protected:
  DenseMediaSubsession(
      UsageEnvironment &env,
      DenseMediaSession &parent,
      DenseMediaSession *mediaSession);
  virtual ~DenseMediaSubsession();

public:
  // Note: Function overriden to spoof fControlPath for RTSPClient
  // This is done to avoid having to edit / copy the whole file.
  // This may break another call... and may break things. Better check it out!
  char const *controlPath() const
  {
    std::string denseControlPath = fControlPath;
    int len = denseControlPath.size();
    std::string init = std::to_string(fInit);
    denseControlPath.replace(len - 2, 1, init);

    return denseControlPath.c_str();
  }

public:
  Boolean denseInitiate(int useSpecialRTPOffset); // TODO: is this variable necessary?
protected:
  Boolean denseCreateSourceObjects(int useSpecialRTPOffset); // TODO: ^

public:
  int fInit;                        // TODO: rename? What does it mean? Initial level? Does it serve several purposes? weird.
  int fLevel;                       // Quality level of subsession
  DenseMediaSession *fMediaSession; // Parent of subsession
  DenseMediaSubsession *fDenseNext; // TODO: is this used? Can it be replaced by the vector? Possibly remove completely!
};

#endif // _DENSE_MEDIA_SUBSESSION_HH