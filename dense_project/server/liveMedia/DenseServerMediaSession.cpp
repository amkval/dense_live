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
// TODO: additional information

#include "include/DenseServerMediaSession.hh"

static char const *const libNameStr = "LIVE555 Streaming Media v";
char const *const libVersionStr = LIVEMEDIA_LIBRARY_VERSION_STRING;

////////// DenseServerMediaSession //////////

DenseServerMediaSession::DenseServerMediaSession(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines) : ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines)
{
}

DenseServerMediaSession::~DenseServerMediaSession()
{
}

DenseServerMediaSession *DenseServerMediaSession::createNew(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines)
{
  return new DenseServerMediaSession(
      env,
      streamName,
      info,
      description,
      isSSM,
      miscSDPLines);
}

Boolean DenseServerMediaSession::addSubsession(ServerMediaSubsession *subsession)
{
  ServerMediaSession::addSubsession(subsession);
  fServerMediaSubsessionVector.push_back(subsession);
}