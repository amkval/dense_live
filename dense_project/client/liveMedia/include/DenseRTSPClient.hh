#ifndef _DENSE_RTSP_CLIENT_HH
#define _DENSE_RTSP_CLIENT_HH

#ifndef _RTSP_CLIENT_HH
#include "RTSPClient.hh"
#endif

#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#include "DenseStreamClientState.hh"
#endif

class DenseRTSPClient : public RTSPClient
{
public:
  static DenseRTSPClient *createNew(
      UsageEnvironment &env, char const *rtspURL,
      int verbosityLevel = 0,
      char const *applicationName = NULL,
      portNumBits tunnelOverHTTPPortNum = 0);

protected:
  DenseRTSPClient(
      UsageEnvironment &env, char const *rtspURL,
      int verbosityLevel, char const *applicationName,
      portNumBits tunnelOverHTTPPortNum);
  // called only by createNew();
  virtual ~DenseRTSPClient();

public:
  DenseStreamClientState fDenseStreamClientState;
};

#endif