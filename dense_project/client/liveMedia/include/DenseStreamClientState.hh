#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#define _DENSE_STREAM_CLIENT_STATE_HH

// Dense Imports
#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

////// DenseStreamClientState //////
class DenseStreamClientState
{
public:
  DenseStreamClientState();
  virtual ~DenseStreamClientState();

public:
  DenseMediaSession *fDenseMediaSession;
};

#endif // _DENSE_STREAM_CLIENT_STATE_HH