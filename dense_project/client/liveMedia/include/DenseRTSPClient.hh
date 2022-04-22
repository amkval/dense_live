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

#ifndef _DENSE_RTSP_CLIENT_HH
#define _DENSE_RTSP_CLIENT_HH

#ifndef _RTSP_CLIENT_HH
#include "RTSPClient.hh"
#endif

#ifndef _DENSE_STREAM_CLIENT_STATE_HH
#include "DenseStreamClientState.hh"
#endif

class DenseRTSPClient : public RTSPClient
{
public:
  static DenseRTSPClient *createNew(
      UsageEnvironment &env, char const *rtspURL,
      int verbosityLevel = 0,
      char const *applicationName = NULL,
      portNumBits tunnelOverHTTPPortNum = 0);

protected:
  DenseRTSPClient(
      UsageEnvironment &env, char const *rtspURL,
      int verbosityLevel, char const *applicationName,
      portNumBits tunnelOverHTTPPortNum);
  // called only by createNew();
  virtual ~DenseRTSPClient();

public:
  DenseStreamClientState fDenseStreamClientState;
};

#endif