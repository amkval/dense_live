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

// TODO: Can should these be here?

#include <string>
#include <vector>

// Forward class definitions
//class DenseMediaSubsession;

////// DenseMediaSession implementation /////
class DenseMediaSession : public MediaSession
{
public:
  static DenseMediaSession *createNew(UsageEnvironment &env, std::string sdpDescription);

protected:
  DenseMediaSession(UsageEnvironment &env);
  virtual ~DenseMediaSession();

// TODO: Is this right?
public:
  MediaSubsession *getSubhead() { return fSubsessionsHead; }
  MediaSubsession *getSubTail() { return fSubsessionsTail; }

protected:
  Boolean initializeWithSDP(char const *sdpDescription);
  Boolean parseSDPLine_o(char const *sdpLine);
  Boolean parseSDPLine_xmisc(char const *sdpLine);

public:
  char *fxmisc;
  // TODO: do we need the level addresses?

  // Subsession variables:
  std::vector<DenseMediaSubsession *> fDenseMediaSubsessions;
  DenseMediaSubsession *fInControl;
  DenseMediaSubsession *fDenseNext;

  Boolean fPutInLookAsideBuffer;
  Boolean fFinishLookAside;
  unsigned short fPacketChunk;
  unsigned short fRTPChunk;
  unsigned char *fLookAside;
  int fLookAsideSize;
  int fCurrentLevel;
  // Pull patching variables:
  int fLevelDrops;
  int fTotalDrops;
  int fLastOffset;
  int fWritten;
  unsigned short fChunk;
  FILE *fOut;
  Boolean fPacketLoss;

};

#endif // _DENSE_MEDIA_SESSION_HH