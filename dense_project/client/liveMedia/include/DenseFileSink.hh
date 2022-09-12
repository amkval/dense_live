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

// Note: not needed?
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

public:
  virtual void addData(
      unsigned char const *data,
      unsigned dataSize,
      struct timeval presentationTime);

protected: // redefined virtual functions:
  virtual Boolean continuePlaying();

protected:
  static void afterGettingFrame(
      void *clientData, unsigned frameSize,
      unsigned numTruncatedBytes,
      struct timeval presentationTime,
      unsigned durationInMicroseconds);
  virtual void afterGettingFrame(
      unsigned frameSize,
      unsigned numTruncatedBytes,
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