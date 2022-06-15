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
#include "../liveMedia/include/DenseFileSink.hh"

#ifndef _DENSE_PRINTS_HH
#include "../liveMedia/include/DensePrints.hh"
#endif

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

// Other event handler functions:
void subsessionAfterPlaying(void *clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void *clientData, char const *reason);

// Used to shut down and close a stream (including its "DenseRTSPClient" object):
void shutdownStream(DenseRTSPClient *denseRTSPClient);

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
  // Note: Does it make sense to accept more than one url?
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
    0);

  env << "\tURL: \"" << rtspURL << "\"\n";

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
    UsageEnvironment &env = denseRTSPClient->envir();
    DenseStreamClientState &scs = denseRTSPClient->fDenseStreamClientState;

    if (resultCode != 0)
    {
      env << *denseRTSPClient << "Failed to get a SDP description: " << resultString << "\n";
      break;
    }

    //env << *denseRTSPClient << "Got a SDP description:\n" << resultString << "\n";

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
    if (!denseMediaSubsession->denseInitiate(-1))
    {
      env << *denseRTSPClient << "Failed to initiate the \"" << *denseMediaSubsession << "\" subsession: " << env.getResultMsg() << "\n";
      continue; // Note: Should we shut down here?
    }

    env /*<< *denseRTSPClient*/ << "Initiated the \"" << *denseMediaSubsession << "\" subsession (";
    if (denseMediaSubsession->rtcpIsMuxed()) // TODO: do we need both?
    {
      env << "client port " << denseMediaSubsession->clientPortNum();
    }
    else
    {
      env << "client ports " << denseMediaSubsession->clientPortNum() << "-" << denseMediaSubsession->clientPortNum() + 1;
    }
    env << ")\n";

    // Used for us to be able to access the 'DenseRTSPClient' from a 'DenseSubsession'
    denseMediaSubsession->miscPtr = denseRTSPClient;
  }

  env << "We've finished setting up all of the subsessions!\n";

  // Check if each subsession works and start them.
  for (auto &denseMediaSubsession : scs.fDenseMediaSession->fDenseMediaSubsessions)
  {
    if (denseMediaSubsession->readSource() == NULL)
    {
      env << *denseMediaSubsession << " has no 'readSource'!\n";
      exit(EXIT_FAILURE);
    }
    if (denseMediaSubsession->rtpSource() == NULL)
    {
      env << *denseMediaSubsession << " has no 'rtpSource'!\n";
      exit(EXIT_FAILURE);
    }
    if (denseMediaSubsession->rtcpInstance() == NULL)
    {
      env << *denseMediaSubsession << " has no 'rtcpInstance'!\n";
      exit(EXIT_FAILURE);
    }

    // Start playing the subsessions.
    env /*<< *denseMediaSubsession*/ << " Starting to play\n";
    DenseFileSink *sink = dynamic_cast<DenseFileSink *>(denseMediaSubsession->sink);
    sink->startPlaying(*(denseMediaSubsession->readSource()), subsessionAfterPlaying, denseMediaSubsession);

    env << "TEMP: AFTER SUPPOSED PLAY.\n";

    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (denseMediaSubsession->rtcpInstance() != NULL)
    {
      denseMediaSubsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, denseMediaSubsession);
    }
  }
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void *clientData)
{
  DenseMediaSubsession *subsession = (DenseMediaSubsession *)clientData;
  DenseRTSPClient *rtspClient = (DenseRTSPClient *)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  DenseMediaSession *session = subsession->fMediaSession;

  for (auto &denseMediaSubsession : session->fDenseMediaSubsessions)
  {
    if (denseMediaSubsession->sink != NULL)
      return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData, char const *reason)
{
  DenseMediaSubsession *subsession = (DenseMediaSubsession *)clientData;
  DenseRTSPClient *rtspClient = (DenseRTSPClient *)subsession->miscPtr;
  UsageEnvironment &env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL)
  {
    env << " (reason:\"" << reason << "\")";
    delete[] reason;
  }
  env << " on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void shutdownStream(DenseRTSPClient *denseRTSPClient)
{
  UsageEnvironment &env = denseRTSPClient->envir(); // Alias
  DenseStreamClientState &scs = denseRTSPClient->fDenseStreamClientState;
  
  if (scs.fDenseMediaSession != NULL)
  {
    Boolean someSubsessionsWereActive = False;

    // Check whether any subsessions have still to be closed:
    for (auto &denseMediaSubsession : scs.fDenseMediaSession->fDenseMediaSubsessions)
    {
      if (denseMediaSubsession->sink != NULL)
      {
        Medium::close(denseMediaSubsession->sink);
        denseMediaSubsession->sink = NULL;

        if (denseMediaSubsession->rtcpInstance() != NULL)
        {
          // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
          denseMediaSubsession->rtcpInstance()->setByeHandler(NULL, NULL);
        }

        someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive)
    {
      // Send a RTSP "TEARDOWN" command, tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN"
      // Note: Does this make sense in a multicast scenario?
      denseRTSPClient->sendTeardownCommand(*scs.fDenseMediaSession, NULL);
    }
  }

  env << *denseRTSPClient << "closing the stream.\n";
  Medium::close(denseRTSPClient);
  // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--denseRTSPClientCount == 0)
  {
    exit(EXIT_SUCCESS);
  }
}