#include "include/DenseStreamClientState.hh"

DenseStreamClientState::DenseStreamClientState()
    : fDenseMediaSession(NULL)
{
}

DenseStreamClientState::~DenseStreamClientState()
{
  // Close DenseMediaSession
  if (fDenseMediaSession != NULL)
  {
    Medium::close(fDenseMediaSession);
  }
}