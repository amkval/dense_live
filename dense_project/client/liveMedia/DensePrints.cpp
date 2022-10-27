#include "include/DensePrints.hh"

#include <iostream>

////// Debug Print Functions //////

// A function that outputs a string that identifies each stream (for debugging output).
/*
UsageEnvironment &operator<<(UsageEnvironment &env, const DenseRTSPClient &denseRTSPClient)
{
  return env << "[URL:\"" << denseRTSPClient.url() << "\"]: ";
}
*/

// A function that outputs a string that identifies each subsession (for debugging output).
/*
UsageEnvironment &operator<<(UsageEnvironment &env, const DenseMediaSubsession &subsession)
{
  return env << subsession.mediumName() << "/" << subsession.codecName();
}
*/

// A function that outputs a string as a C string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, std::string string)
{
  return env << string.c_str();
}

// A function that outputs a long as a string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, long number)
{
  return env << std::to_string(number).c_str();
}

// A function that outputs a short unsigned int as a string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, short unsigned int number)
{
  return env << std::to_string(number).c_str();
}