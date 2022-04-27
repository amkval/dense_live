#ifndef _DENSE_PRINTS_HH
#include "include/DensePrints.hh"
#endif

#include <iostream>

////// Debug functions //////

// A function that outputs a string that identifies each stream (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, const DenseRTSPClient &denseRTSPClient)
{
  return env << "[URL:\"" << denseRTSPClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession)
{
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

// A function that outputs a string as a C string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, std::string string)
{
  return env << string.c_str();
}

// A function that outputs a string as a long (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, long number)
{
  return env << std::to_string(number).c_str();
}