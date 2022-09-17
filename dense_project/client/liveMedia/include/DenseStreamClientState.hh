// Note: Do we need this class with only one variable?
// Could we store other important variables here?

#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#define _DENSE_STREAM_CLIENT_STATE_HH

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class DenseStreamClientState
{
public:
  DenseStreamClientState();
  virtual ~DenseStreamClientState();

public:
  DenseMediaSession *fDenseMediaSession;
};

#endif // _DENSE_STREAM_CLIENT_STATE_HH