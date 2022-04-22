/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// TODO: Additional information.

#include "BasicUsageEnvironment.hh"

#include "../liveMedia/include/DenseRTSPClient.hh"
#include "../liveMedia/include/DenseMediaSession.hh"
#include "../liveMedia/include/DenseMediaSubsession.hh"

#include <string>
#include <vector>
#include <iostream>

char eventLoopWatchVariable = 0;
// TODO: Keep these variables in some kind of object / state management?
static unsigned denseRTSPClientCount = 0; // Only used for cleanup
#define RTSP_CLIENT_VERBOSITY_LEVEL 1     // By default, print verbose output from each "DenseRTSPClient"

// Forward function definitions

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment &env, std::string applicationName, std::string rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupSubsessions(DenseRTSPClient *denseRTSPClient);

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);

// Used to shut down and close a stream (including its "DenseRTSPClient" object):
void shutdownStream(DenseRTSPClient *denseRTSPClient);

////// Debug functions //////
// TODO: Move to separate file
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

void usage(UsageEnvironment &env, std::string applicationName)
{
  env << "Usage: " << applicationName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

////// Program entry point /////

int main(int argc, char *argv[])
{
  // Begin by setting up our usage environment:
  TaskScheduler *scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

  // We need at least one "rtsp://" URL argument:
  if (argc < 2) // TODO: make better check?
  {
    usage(*env, argv[0]);
    exit(1);
  }

  // There are argc-1 URLs: argv[1] through argv[argc-1]. Open and start streaming each one:
  // TODO: Does it make sense to accept more than one url?
  for (int i = 1; i <= argc - 1; i++)
  {
    openURL(*env, argv[0], argv[i]);
  }

  // All subsequent activity takes place within the event loop
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

  return 0;
}

void openURL(UsageEnvironment &env, std::string applicationName, std::string rtspURL)
{
  DenseRTSPClient *denseRTSPClient = DenseRTSPClient::createNew(
    env, 
    rtspURL.c_str(), 
    RTSP_CLIENT_VERBOSITY_LEVEL, 
    applicationName.c_str(), 
    0); // TODO: The order is different!

  env << "URL: \"" << rtspURL << "\"\n";

  if (denseRTSPClient == NULL)
  {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\"" << env.getResultMsg() << "\n";
    exit(EXIT_FAILURE);
  }

  denseRTSPClientCount++;

  denseRTSPClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  DenseRTSPClient *denseRTSPClient = dynamic_cast<DenseRTSPClient *>(rtspClient);
  do {
    UsageEnvironment &env = denseRTSPClient->envir(); // TODO: should we pass it or do this?
    DenseStreamClientState &scs = denseRTSPClient->fDenseStreamClientState;

    if (resultCode != 0)
    {
      env << *denseRTSPClient << "Failed to get a SDP description: " << resultString << "\n";
      break;
    }

    env << *denseRTSPClient << "Got a SDP description:\n" << resultString << "\n";

    scs.fDenseMediaSession = DenseMediaSession::createNew(env, resultString);

    if (scs.fDenseMediaSession == NULL)
    {
      env << *denseRTSPClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    }

    if (!scs.fDenseMediaSession->hasSubsessions())
    {
      env << *denseRTSPClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    setupSubsessions(denseRTSPClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(denseRTSPClient);
}

void setupSubsessions(DenseRTSPClient *denseRTSPClient)
{
  UsageEnvironment &env = denseRTSPClient->envir();
  DenseStreamClientState &scs = denseRTSPClient->fDenseStreamClientState;

  for (auto &denseMediaSubsession : scs.fDenseMediaSession->fDenseMediaSubsessions)
  {
    if (!denseMediaSubsession->initiate())
    {
      env << *denseRTSPClient << "Failed to initiate the \"" << denseMediaSubsession << "\" subsession: " << env.getResultMsg() << "\n";
      // TODO: Should we shut down here?
      continue;
    }

    env << *denseRTSPClient << "Initiated the \"" << denseMediaSubsession << "\" subsession (";
    if (denseMediaSubsession->rtcpIsMuxed()) // TODO: do we need both?
    {
      env << "client port " << denseMediaSubsession->clientPortNum();
    }
    else
    {
      env << "client ports " << denseMediaSubsession->clientPortNum() << "-" << denseMediaSubsession->clientPortNum() + 1;
    }
    env << ")\n";
  }
}

void shutdownStream(DenseRTSPClient *denseRTSPClient)
{
  UsageEnvironment &env = denseRTSPClient->envir(); // TODO: should we do this?
  DenseStreamClientState &scs = denseRTSPClient->fDenseStreamClientState;

  // First, check whether any subsessions have still to be closed:
  if (scs.fDenseMediaSession != NULL)
  {
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.fDenseMediaSession); // TODO: is there a better way to iterate through these?

    // Iteration part

    if (someSubsessionsWereActive)
    {
      // Send a RTSP "TEARDOWN" command, tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN"
      denseRTSPClient->sendTeardownCommand(*scs.fDenseMediaSession, NULL);
    }
  }

  env << *denseRTSPClient << "closing the stream.\n";
  Medium::close(denseRTSPClient);

  if (--denseRTSPClientCount == 0)
  {
    exit(EXIT_SUCCESS);
  }
}