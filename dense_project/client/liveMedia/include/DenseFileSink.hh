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

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

#include <cmath>
#include <string>
#include <assert.h> // TODO: use cassert instead?

class DenseMediaSession; // Forward

#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

class DenseFileSink : public FileSink
{
public:
  static DenseFileSink *createNew(
      UsageEnvironment &env,
      char const *fileName,
      unsigned bufferSize = 20000,
      Boolean oneFilePerFrame = False);

protected:
  DenseFileSink(
      UsageEnvironment &env,
      FILE *fid,
      unsigned bufferSize,
      char const *perFrameFileNamePrefix,
      DenseMediaSession *mediaSession);
  virtual ~DenseFileSink();

protected:
  virtual void addData(
      unsigned char const *data,
      unsigned dataSize,
      struct timeval presentationTime);

  // TODO: are these used properly?
  long pullChunk(unsigned short numb);
  void pullPatch();
  void pullBeginning(int ant);
  void pullAll();

  void finishFromLookAside();

public:
  DenseMediaSession *fMediaSession;
  Boolean fWriteFromPacket;
};

#endif // _DENSE_FILE_SINK_HH