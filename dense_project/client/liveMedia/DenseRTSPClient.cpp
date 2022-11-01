#include "include/DenseRTSPClient.hh"

////// DenseRTSPClient //////
DenseRTSPClient *DenseRTSPClient::createNew(
    UsageEnvironment &env,
    std::string rtspURL,
    std::string applicationName,
    int verbosityLevel,
    portNumBits tunnelOverHTTPPortNum)
{
    env << "#### Create new DenseRTSPClient ####\n"
        << "\trtspURL: " << rtspURL << "\n"
        << "\tapplicationName: " << applicationName << "\n"
        << "\tverbosityLevel: " << verbosityLevel << "\n"
        << "\ttunnelOverHTTPPortNum: " << tunnelOverHTTPPortNum << "\n";

    return new DenseRTSPClient(
        env, rtspURL, applicationName, verbosityLevel, tunnelOverHTTPPortNum);
}

DenseRTSPClient::DenseRTSPClient(
    UsageEnvironment &env,
    std::string rtspURL,
    std::string applicationName,
    int verbosityLevel,
    portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env, rtspURL.c_str(),
                 verbosityLevel, applicationName.c_str(),
                 tunnelOverHTTPPortNum, -1)
{
}

DenseRTSPClient::~DenseRTSPClient()
{
}