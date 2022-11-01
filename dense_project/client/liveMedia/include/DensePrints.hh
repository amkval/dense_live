#ifndef _DENSE_PRINTS_HH
#define _DENSE_PRINTS_HH

// Live Imports
#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

#ifdef _DENSE_RTSP_CLIENT_HH
#include "DenseRTSPClient.hh"
#endif

// C++ Imports
#include <string>

////// Debug functions //////

// A function that outputs a string that identifies each stream (for debugging output).
//UsageEnvironment &
//operator<<(UsageEnvironment &env, const DenseRTSPClient &denseRTSPClient);

// A function that outputs a string that identifies each subsession (for debugging output).
//sageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession);

// A function that outputs a string as a C string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, std::string string);

// A function that outputs a string as a long (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, long number);

#endif // _DENSE_PRINTS_HH