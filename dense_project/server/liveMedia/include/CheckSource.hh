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
  CheckSource(
      UsageEnvironment &env,
      FILE *fid,
      unsigned preferredFrameSize,
      unsigned playTimePerFrame);
  // Called only by 'createNew'
  virtual ~CheckSource();

public:
  int getNowChunk() { return fCurrentChunk; } // TODO: rename to getter?

protected:
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
  unsigned fLastPlayTime;
  Boolean fFidIsSeekable;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

#endif