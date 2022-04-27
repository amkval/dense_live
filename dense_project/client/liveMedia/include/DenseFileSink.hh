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

#ifndef _DENSE_FILE_SINK_HH
#define _DENSE_FILE_SINK_HH

class DenseMediaSession; // Forward

// Live Imports

#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif

// Dense Imports

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

//#include <cmath>
//#include <string>
//#include <cassert>

class DenseFileSink : public FileSink
{
public:
  static DenseFileSink *createNew(
      UsageEnvironment &env,
      char const *fileName,
      DenseMediaSession *mediaSession,
      unsigned bufferSize = 20000);

protected:
  DenseFileSink(
      UsageEnvironment &env,
      FILE *fid,
      DenseMediaSession *mediaSession,
      unsigned bufferSize);
  virtual ~DenseFileSink();

protected:
  virtual void addData(
      unsigned char const *data,
      unsigned dataSize,
      struct timeval presentationTime);

  long pullChunk(unsigned short chunkId);
  void pullPatch();
  void pullBeginning(int chunkCount);
  void pullAll();

  void finishFromLookAside();

public:
  DenseMediaSession *fMediaSession;
  Boolean fWriteFromPacket;
};

#endif // _DENSE_FILE_SINK_HH