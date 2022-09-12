#include "include/DenseRTSPClient.hh"

////// DenseRTSPClient //////

DenseRTSPClient *DenseRTSPClient::createNew(
    UsageEnvironment &env,
    char const *rtspURL,
    int verbosityLevel,
    char const *applicationName,
    portNumBits tunnelOverHTTPPortNum)
{
  env << "DenseRTSPClient::createNew():\n"
      << "\trtspURL: " << rtspURL << "\n"
      << "\tverbositylevel: " << verbosityLevel << "\n"
      << "\tapplicationName: " << applicationName << "\n";

  return new DenseRTSPClient(
      env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

DenseRTSPClient::DenseRTSPClient(
    UsageEnvironment &env,
    char const *rtspURL,
    int verbosityLevel,
    char const *applicationName,
    portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

DenseRTSPClient::~DenseRTSPClient()
{
}