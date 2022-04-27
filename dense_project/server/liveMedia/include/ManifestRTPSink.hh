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

#ifndef _MANIFEST_RTP_SINK_HH
#define _MANIFEST_RTP_SINK_HH

class DenseRTSPServer; // Forward

#ifndef _CHECK_SOURCE_HH
#include "CheckSource.hh"
#endif

#ifndef _DENSE_MULTI_FRAMED_RTP_SINK_HH
#include "DenseMultiFramedRTPSink.hh"
#endif

#ifndef _DENSE_RTSP_SERVER_HH
#include "DenseRTSPServer.hh"
#endif

class ManifestRTPSink : public DenseMultiFramedRTPSink
{
public:
  static ManifestRTPSink *createNew(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      std::string sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels = 1,
      Boolean allowMultipleFramesPerPacket = True,
      Boolean doNormalMBitRule = True,
      std::string name = "");

protected:
  ManifestRTPSink(
      UsageEnvironment &env, Groupsock *RTPgroupsock,
      unsigned char rtpPayloadFormat,
      unsigned rtpTimestampFrequency,
      std::string sdpMediaTypeString,
      char const *rtpPayloadFormatName,
      DenseRTSPServer *denseRTSPServer,
      unsigned numChannels,
      Boolean allowMultipleFramesPerPacket,
      Boolean doNormalMBitRule,
      std::string name);
  // called only by createNew()

  virtual ~ManifestRTPSink();

public:
  void setCheckSource(CheckSource *checkSource)
  {
    fprintf(stderr, "ManifestRTPSink::setCheckSource()\n");
    fCheckSource = checkSource;
  }

protected: // redefined virtual functions
  virtual void doSpecialFrameHandling(
      unsigned fragmentationOffset,
      unsigned char *frameStart,
      unsigned numBytesInFrame,
      struct timeval framePresentationTime,
      unsigned numRemainingBytes);

protected: // redefined virtual functions:
  virtual Boolean continuePlaying(); //TODO: Unused? NO, very used!
  void buildAndSendPacket(Boolean isFirstPacket);
  static void sendNext(void *firstArg);
  friend void sendNext(void *);
  void packFrame();
  void sendPacketIfNecessary();

  static void afterGettingFrame(
      void *clientData,
      unsigned numBytesRead,
      unsigned numTruncatedBytes,
      struct timeval presentationTime,
      unsigned durationInMicroseconds);
  void afterGettingFrame1(
      unsigned numBytesRead,
      unsigned numTruncatedBytes,
      struct timeval presentationTime,
      unsigned durationInMicroseconds);

private:
  char const *fSDPMediaTypeString;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames, fSetMBitOnNextPacket;
  CheckSource *fCheckSource;
  const char *fName;

public:
  DenseRTSPServer *fOurServer; //TODO: Rename
};

#endif