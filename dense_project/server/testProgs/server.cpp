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

/*
#include "../liveMedia/include/DenseRTSPServer.hh"
*/

#include <string>

UsageEnvironment *env;

int main(int argc, char **argv)
{
  // Begin by setting up our usage environment
  TaskScheduler *taskScheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*taskScheduler);
  
  int port = 51273; // TODO: why this port?
  unsigned reclamationSeconds = 65U; // TODO: what does this signify?
  int count = 3;
  std::string name = argv[1];
  std::string time = argv[2];
  std::string alias = "denseServer";

  fprintf(stderr, "main():\n");
  fprintf(stderr,
          "port: %d\nreclamationSeconds: %u\ncount: %d\nname: %s\ntime: %s\nalias: %s\n",
          port, reclamationSeconds, count, name.c_str(), time.c_str(), alias.c_str());

  /*
  DenseRTSPServer *denseRTSPServer = DenseRTSPServer::createNew(
      *env, port, NULL, reclamationSeconds, NULL, count, name, time, alias);

  if (denseRTSPServer == NULL)
  {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
  }
  */

  env->taskScheduler().doEventLoop(); // Does not return

  return 0; // Only to prevent compiler waring
}
