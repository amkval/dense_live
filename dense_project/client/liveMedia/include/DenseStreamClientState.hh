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

// Note: Do we need this class with only one variable?
// Could we store other important variables here?

#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#define _DENSE_STREAM_CLIENT_STATE_HH

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class DenseStreamClientState
{
public:
  DenseStreamClientState();
  virtual ~DenseStreamClientState();

public:
  //MediaSubsessionIterator *fIter; // Note: Replaced by vector.
  DenseMediaSession *fDenseMediaSession;
  // DenseMediaSubsession *fDenseMediaSubsession; // Note: Replaced by vector.
  // TaskToken fStreamTimerTask; // Note: Not used outside caPLAY, and caPLAY was not used.
  // double fDuration; // Note: ^
  // int fSubsessionCount; // Note: Not used.
  // DenseMediaSubsession *fLinkSubSession; // Note: Not used.
};

#endif // _DENSE_STREAM_CLIENT_STATE_HH