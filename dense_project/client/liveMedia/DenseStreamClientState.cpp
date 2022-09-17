#include "include/DenseStreamClientState.hh"

DenseStreamClientState::DenseStreamClientState() : fDenseMediaSession(NULL)
{
}

DenseStreamClientState::~DenseStreamClientState()
{
  if (fDenseMediaSession != NULL)
  {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    // UsageEnvironment &env = fDenseMediaSession->envir(); // alias

    // env.taskScheduler().unscheduleDelayedTask(fStreamTimerTask); Note: Not used.
    Medium::close(fDenseMediaSession);
  }
}