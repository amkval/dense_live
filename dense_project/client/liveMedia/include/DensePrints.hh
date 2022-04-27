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
// TODO: Additional info.

#ifndef _DENSE_PRINTS_HH
#define _DENSE_PRINTS_HH

// Live Imports
#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

// Dense Imports
#ifndef _DENSE_RTSP_CLIENT_HH
#include "DenseRTSPClient.hh"
#endif

#ifndef _DENSE_MEDIA_SESSION_HH
#include "DenseMediaSession.hh"
#endif

// Other Imports
#include <string>

////// Debug functions //////

// A function that outputs a string that identifies each stream (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, const DenseRTSPClient &denseRTSPClient);

// A function that outputs a string that identifies each subsession (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession);

// A function that outputs a string as a C string (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, std::string string);

// A function that outputs a string as a long (for debugging output).
UsageEnvironment &operator<<(UsageEnvironment &env, long number);

#endif // _DENSE_PRINTS_HH