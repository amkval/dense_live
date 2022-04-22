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

#include "include/DenseStreamClientState.hh"


DenseStreamClientState::DenseStreamClientState()
    : fIter(NULL), fDenseMediaSession(NULL), fDenseMediaSubsession(NULL), fStreamTimerTask(NULL), fDuration(0.0)
{
}

DenseStreamClientState::~DenseStreamClientState()
{
  delete fIter;
  if (fDenseMediaSession != NULL)
  {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment &env = fDenseMediaSession->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(fStreamTimerTask);
    Medium::close(fDenseMediaSession);
  }
}