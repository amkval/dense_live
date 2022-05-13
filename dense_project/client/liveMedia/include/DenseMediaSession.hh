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

////// DenseMediaSession implementation /////
class DenseMediaSession : public MediaSession
{
public:
  static DenseMediaSession *createNew(UsageEnvironment &env, std::string sdpDescription);

protected:
  DenseMediaSession(UsageEnvironment &env);
  virtual ~DenseMediaSession();

public:
  // Note: Not used anymore.
  //MediaSubsession *getSubHead() { return fSubsessionsHead; }
  //MediaSubsession *getSubTail() { return fSubsessionsTail; }

protected:
  Boolean initializeWithSDP(char const *sdpDescription);
  Boolean parseSDPLine_o(char const *sdpLine);
  Boolean parseSDPLine_xmisc(char const *sdpLine);

public:
  char *fxmisc;

  // Subsession variables:
  std::vector<DenseMediaSubsession *> fDenseMediaSubsessions;
  DenseMediaSubsession *fInControl; // Current subsession in control of the stream.
  DenseMediaSubsession *fDenseNext; // Targeted subsession after level move. TODO: Make sure that this works!

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
  unsigned short fPacketChunk;   // Chunk number of the most resent chunk.
  unsigned short fRTPChunk;      // TODO:?
};

#endif // _DENSE_MEDIA_SESSION_HH