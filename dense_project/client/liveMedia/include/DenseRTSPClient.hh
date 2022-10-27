#ifndef _DENSE_RTSP_CLIENT_HH
#define _DENSE_RTSP_CLIENT_HH

#ifndef _RTSP_CLIENT_HH
#include "RTSPClient.hh"
#endif

#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#include "DenseStreamClientState.hh"
#endif

// C++ imports
#include <string>

////// DenseRTSPClient //////

class DenseRTSPClient : public RTSPClient
{
public:
  static DenseRTSPClient *createNew(
      UsageEnvironment &env, std::string rtspURL,
      std::string applicationName,
      int verbosityLevel = 0,
      portNumBits tunnelOverHTTPPortNum = 0);

protected:
  DenseRTSPClient(
      UsageEnvironment &env, std::string rtspURL,
      std::string applicationName, int verbosityLevel,
      portNumBits tunnelOverHTTPPortNum);
  // called only by createNew();
  virtual ~DenseRTSPClient();

public:
  DenseStreamClientState fDenseStreamClientState;
};

#endif