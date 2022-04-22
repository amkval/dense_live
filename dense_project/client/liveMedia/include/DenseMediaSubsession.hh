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

#ifndef _DENSE_MEDIA_SUBSESSION_HH
#define _DENSE_MEDIA_SUBSESSION_HH

// Live Imports

#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif

#include "GroupsockHelper.hh"

#include <string>

// Forward class declaration
class DenseMediaSession;

////// DenseMediaSubsession //////
class DenseMediaSubsession : public MediaSubsession
{
public:
  friend class DenseMediaSession;
  static DenseMediaSubsession *createNew(UsageEnvironment &env, DenseMediaSession &parent);

protected:
  DenseMediaSubsession(UsageEnvironment &env, DenseMediaSession &parent); // TODO: is the passing of parent right?
  virtual ~DenseMediaSubsession();

public:
  // Function overriden to spoof fControlPath for RTSPClient
  // This is done to avoid having to edit / copy the whole file.
  char const *controlPath() const {
    std::string denseControlPath = fControlPath;
    int len = denseControlPath.size();
    std::string init = std::to_string(fInit);
    denseControlPath.replace(len - 2, 1, init);

    return denseControlPath.c_str();
  }

  DenseMediaSubsession *getNext() { return (DenseMediaSubsession *)fNext; }

protected:
  Boolean denseInitiate(int useSpecialRTPOffset);            // TODO: is this variable necessary?
  Boolean denseCreateSourceObjects(int useSpecialRTPOffset); // TODO: ^

public:
  int fInit;                        // TODO: rename? What does it mean? Initial level? Does it serve several purposes? weird.
  int fLevel;                       // Level of subsession
  DenseMediaSession *fMediaSession; // Parent of subsession
  DenseMediaSubsession *fDenseNext; // TODO: is this used? Can it be replaced by the vector?
};

#endif // _DENSE_MEDIA_SUBSESSION_HH