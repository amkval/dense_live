#include "BasicUsageEnvironment.hh"

#include <string>

char eventLoopVariable = 0;

int main(int argc, char argv[])
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
  for (int i = 1; i <= argc - 1; i++)
  {
    openURL(*env, argv[0], argv[i]);
  }

  // All subsequent activity takes place within the event loop
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

  return 0;

  // TODO: Do some cleanup? or is it unncessessary? 
}

void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL)
{
  RTSPClient *rtspClient = DenseRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  env << "URL \"" << rtspURL << "\"\n";

  if (rtspClient == NULL)
  {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\"" << env.getResultMsg() << "\n";
    exit(1);
  }

  rtspClientCount++;

  rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  do {
    UsageEnvironment &env = rtspClinet->envir();
    StreamClientState &scs = ((DenseRTSPServer *)rtspClient)->scs; // TODO: Will this work?

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char *const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n"
        << sdpDescription << "\n";

    scs.session = MediaSession::createNew(env, sdpDescription);

    delete[] sdpDescription;
    if (scs.session == NULL)
    {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    }
    else if (!scs.session->hasSubsessions())
    {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }
  } while (0);

  shutdownStream(rtspClient);
}