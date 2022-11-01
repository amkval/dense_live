#ifndef _CHECK_SOURCE_HH
#define _CHECK_SOURCE_HH

// Live Imports
#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

// C++ Imports
#include <string>

////// CheckSource //////
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
  int currentChunk() { return fCurrentChunk; }

protected:
  static int stripChunks(char (*chunks)[1000][100], FILE *fid);
  static std::string stripPath(std::string path);

  virtual void doGetNextFrame();
  static void fileReadableHandler(CheckSource *source, int mask);
  virtual void doReadFromFile();
  int manageManifest();

protected:
  u_int64_t fFileSize;
  uint64_t fReadSoFar;
  char fChunks[1000][100] = {0};
  std::string fPath;
  int fCurrentChunk;
  int fChunkCount;

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