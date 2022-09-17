#include "BasicUsageEnvironment.hh"
#include "../liveMedia/include/DenseRTSPServer.hh"
#include <string>

// A function that outputs a string as a C string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, std::string string)
{
  return env << string.c_str();
}

int main(int argc, char **argv)
{
  // Begin by setting up our usage environment
  TaskScheduler *taskScheduler = BasicTaskScheduler::createNew();
  UsageEnvironment *env = BasicUsageEnvironment::createNew(*taskScheduler);

  int port = 51273;                   // Note: Why this port?
  unsigned reclamationSeconds = 65U;  // Note: What is reclamation seconds?
  int levels = 3;                     // Number of quality levels for the server to serve
  std::string path = argv[1];         // Location of video files
  std::string fps = argv[2];          // Framerate for the transmission
  std::string alias = "makeStream";   // TODO: Is alias the most appropriate name? Just use name?

  *env << "\n\nStarting RTSP server!\n";

  DenseRTSPServer *denseRTSPServer = DenseRTSPServer::createNew(
      *env, port, NULL, reclamationSeconds, False, levels, (char *)path.c_str(), fps, alias);

  if (denseRTSPServer == NULL)
  {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(EXIT_FAILURE);
  }

  env->taskScheduler().doEventLoop(); // Does not return

  return 0; // Only to prevent compiler waring
}
