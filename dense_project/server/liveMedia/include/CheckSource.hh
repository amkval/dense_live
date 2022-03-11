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

protected:
  char fChunks[1000][100] = {0}; // TODO: is this sensible?
  int fChunkCount;
  u_int64_t fFileSize;
  std::string fPath;

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
