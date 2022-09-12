#include "include/DenseServerMediaSession.hh"

static char const *const libNameStr = "LIVE555 Streaming Media v";
char const *const libVersionStr = LIVEMEDIA_LIBRARY_VERSION_STRING;

////////// DenseServerMediaSession //////////

DenseServerMediaSession::DenseServerMediaSession(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines) : ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines)
{
}

DenseServerMediaSession::~DenseServerMediaSession()
{
}

DenseServerMediaSession *DenseServerMediaSession::createNew(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines)
{
  return new DenseServerMediaSession(
      env,
      streamName,
      info,
      description,
      isSSM,
      miscSDPLines);
}

Boolean DenseServerMediaSession::addSubsession(ServerMediaSubsession *subsession)
{
  fServerMediaSubsessionVector.push_back(subsession);
  return ServerMediaSession::addSubsession(subsession);
}