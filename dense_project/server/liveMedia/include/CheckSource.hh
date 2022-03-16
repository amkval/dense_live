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
#ifndef _CHECK_SOURCE_HH
#define _CHECK_SOURCE_HH

#include "FramedFileSource.hh"

#include <string>

class CheckSource : public FramedFileSource
{
public:
  static CheckSource *createNew(
      UsageEnvironment &env,
      std::string fileName,
      unsigned preferredFrameSize = 0,
      unsigned playTimePerFrame = 0);

protected:
  CheckSource(UsageEnvironment &env, FILE *fid,
              unsigned preferredFrameSize, unsigned playTimePerFrame);
  // Called only by create new
  virtual ~CheckSource();

  int stripChunks();
  std::string stripPath(std::string path);

  virtual void doGetNextFrame();
  static void fileReadableHandler(CheckSource *source, int mask);
  virtual void doReadFromFile();
  int manageManifest();

protected:
  u_int64_t fFileSize;
  uint64_t fReadSoFar;
  char fChunks[1000][100] = {0}; // TODO: is this sensible?
  std::string fPath;
  int fCurrentChunk;
  int fChunkCount;

  // TODO: Where are these used? Do we need them?
private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

#endif