#ifndef _DENSE_MEDIA_SESSION_HH
#define _DENSE_MEDIA_SESSION_HH

// Live Imports
#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif

// Dense Imports
#ifndef _DENSE_MEDIA_SUBSESSION_HH
#include "DenseMediaSubsession.hh"
#endif

#include <string>
#include <vector>

////// DenseMediaSession Implementation /////
class DenseMediaSession : public MediaSession
{
public:
  static DenseMediaSession *createNew(UsageEnvironment &env, std::string sdpDescription);

protected:
  DenseMediaSession(UsageEnvironment &env);
  virtual ~DenseMediaSession();

protected:
  Boolean initializeWithSDP(char const *sdpDescription);
  Boolean parseSDPLine_o(char const *sdpLine);
  Boolean parseSDPLine_xmisc(char const *sdpLine);

public:
  char *fxmisc;

  // Subsession variables:
  std::vector<DenseMediaSubsession *> fDenseMediaSubsessions; // Vector containing all our subsessions.
  DenseMediaSubsession *fInControl;                           // Current subsession in control of the stream.
  DenseMediaSubsession *fDenseNext;                           // Targeted subsession after level move.

  // Session variables
  Boolean fFinishLookAside;      // If we need to transfer from look aside to file before ...
  Boolean fPacketLoss;           // Note: Not read, only written.
  Boolean fPutInLookAsideBuffer; // If a chunk is to be put in the look aside buffer.
  FILE *fOut;                    // The main output stream. better name?
  int fCurrentLevel;             // The current quality level
  int fLastOffset;               // Size of the last write to fOut?
  int fLevelDrops;               // Times the quality level has dropped since last level change.
  int fLookAsideSize;            // Bytes currently stored in the look aside buffer.
  int fTotalDrops;               // Total packet drops.
  int fWritten;                  // Bytes written to fOut.
  unsigned char *fLookAside;     // The look aside buffer.
  unsigned short fChunk;         // Current chunk number.
  unsigned short fPacketChunk;   // Chunk number of the most recent chunk.
  unsigned short fRTPChunk;      // TODO:?
};

#endif // _DENSE_MEDIA_SESSION_HH