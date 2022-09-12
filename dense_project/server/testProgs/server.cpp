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
// Copyright (c) 1996-2019, Live Networks, Inc.  All rights reserved
// A test program for the dense server
// main program

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

  int port = 51273;                  // TODO: why this port?
  unsigned reclamationSeconds = 65U; // TODO: what does this signify?
  int levels = 3;                    // Number of quality levels for the server to serve
  std::string path = argv[1];        // Location of video files
  std::string fps = argv[2];        // Framerate for the transmission
  std::string alias = "makeStream"; // TODO: Is alias the most appropriate name? Just use name?

  *env << "\n\nStarting RTSP server!\n";

  DenseRTSPServer *denseRTSPServer = DenseRTSPServer::createNew(
      *env, port, NULL, reclamationSeconds, False, levels, (char *)path.c_str(), fps, alias); // Note: changed NULL to false 

  if (denseRTSPServer == NULL)
  {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(EXIT_FAILURE);
  }

  env->taskScheduler().doEventLoop(); // Does not return

  return 0; // Only to prevent compiler waring
}
