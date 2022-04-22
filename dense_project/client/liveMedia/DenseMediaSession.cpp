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
// TODO: Additional information.

#include "include/DenseMediaSession.hh"
#include "include/DenseFileSink.hh"

DenseMediaSession *DenseMediaSession::createNew(
    UsageEnvironment &env,
    std::string sdpDescription)
{
  DenseMediaSession *denseMediaSession = new DenseMediaSession(env);
  if (denseMediaSession == NULL)
  {
    env << "Failed to create new MediaSession\n";
    exit(EXIT_FAILURE);
  }

  if(!denseMediaSession->initializeWithSDP(sdpDescription.c_str()))
  {
    env << "Failed to initialize DenseMEdiaSession with sdpDescription\n";
    delete denseMediaSession;
    exit(EXIT_FAILURE);
  }

  denseMediaSession->fLookAside = new unsigned char[2000000]; // TODO: Use setter? Is the size appropriate?
  return denseMediaSession;
}

DenseMediaSession::DenseMediaSession(UsageEnvironment &env) : MediaSession(env)
{
  // TODO: Remember to initialize variables!
}

DenseMediaSession::~DenseMediaSession()
{

}

Boolean DenseMediaSession::initializeWithSDP(char const *sdpDescription)
{
  if (sdpDescription == NULL)
    return False;

  char const *sdpLine = sdpDescription;
  char const *nextSDPLine;

  while (True)
  {
    if (!parseSDPLine(sdpLine, nextSDPLine))
      return False;
    if (sdpLine[0] == 'm')
      break;

    sdpLine = nextSDPLine;

    if (sdpLine == NULL)
      break; // There are no "m=" lines at all

    // Check for various special SDP lines that we understand:
    if (parseSDPLine_xmisc(sdpLine)) // Dense addition
      continue;
    if (parseSDPLine_o(sdpLine)) // Dense addition
      continue;
    if (parseSDPLine_i(sdpLine))
      continue;
    if (parseSDPLine_c(sdpLine))
      continue;
    if (parseSDPAttribute_control(sdpLine))
      continue;
    if (parseSDPAttribute_range(sdpLine))
      continue;
    if (parseSDPAttribute_type(sdpLine))
      continue;
    if (parseSDPAttribute_source_filter(sdpLine))
      continue;
  }

  // GØTE
  int level = 0;
  fOut = fopen("mainStream", "wbr"); // TODO: is this really the place for this?
  // GØTE ^

  while (sdpLine != NULL)
  {
    // We have a "m=" line, representing a new subsession:

    // Dense
    DenseMediaSubsession *subsession = DenseMediaSubsession::createNew(envir(), *this); //TODO: did we really need to change this?
    // Dense ^

    if (subsession == NULL)
    {
      envir().setResultMsg("Unable to create new DenseMediaSubsession");
      return False;
    }

    // Gøte
    subsession->fLevel = level++; // TODO: this should be in a constructor!
    // Gøte ^

    // Parse the line as "m=<medium_name> <client_portNum> RTP/AVP <fmt>"
    // or "m=<medium_name> <client_portNum>/<num_ports> RTP/AVP <fmt>"
    // (Should we be checking for >1 payload format number here?)#####
    char *mediumName = strDupSize(sdpLine); // ensures we have enough space
    char const *protocolName = NULL;
    unsigned payloadFormat;

    // GØTE
    char *addressName = strDupSize(sdpLine);
    int port;
    // GØTE

    if ((sscanf(sdpLine, "m=%s %hu RTP/AVP %u",
                mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
         sscanf(sdpLine, "m=%s %hu/%*u RTP/AVP %u",
                mediumName, &subsession->fClientPortNum, &payloadFormat) == 3) &&
        payloadFormat <= 127)
    {
      protocolName = "RTP";

      // GØTE, TODO: Yes, but it is not used? what, why, how, keep?
      sscanf(sdpLine, "m=data %d RTP/AVP 96\nc=IN IP4 %s", &port, addressName);
      /*
      if (fLevelAddr == NULL)
      {
        fLevelAddr = strdup(addressName);
        int length = strlen(fLevelAddr);
        fLevelAddr[length - 4] = '\0';
      }
      else if (fLevelAddr1 == NULL)
      {
        fLevelAddr1 = strdup(addressName);
        int length = strlen(fLevelAddr1);
        fLevelAddr1[length - 4] = '\0';
      }
      else if (fLevelAddr2 == NULL)
      {
        fLevelAddr2 = strdup(addressName);
        int length = strlen(fLevelAddr2);
        fLevelAddr2[length - 4] = '\0';
      }
      */
      // GØTE ^

    }
    else if ((sscanf(sdpLine, "m=%s %hu UDP %u",
                     mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
              sscanf(sdpLine, "m=%s %hu udp %u",
                     mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
              sscanf(sdpLine, "m=%s %hu RAW/RAW/UDP %u",
                     mediumName, &subsession->fClientPortNum, &payloadFormat) == 3) &&
             payloadFormat <= 127)
    {
      // This is a RAW UDP source
      protocolName = "UDP";
    }
    else
    {
      // This "m=" line is bad; output an error message saying so:
      char *sdpLineStr;
      if (nextSDPLine == NULL)
      {
        sdpLineStr = (char *)sdpLine;
      }
      else
      {
        sdpLineStr = strDup(sdpLine);
        sdpLineStr[nextSDPLine - sdpLine] = '\0';
      }
      envir() << "Bad SDP \"m=\" line: " << sdpLineStr << "\n";
      if (sdpLineStr != (char *)sdpLine)
        delete[] sdpLineStr;

      delete[] mediumName;
      delete subsession;

      // GØTE
      delete[] addressName;
      // GØTE ^

      // Skip the following SDP lines, up until the next "m=":
      while (1)
      {
        sdpLine = nextSDPLine;
        if (sdpLine == NULL)
          break; // we've reached the end
        if (!parseSDPLine(sdpLine, nextSDPLine))
          return False;

        if (sdpLine[0] == 'm')
          break; // we've reached the next subsession
      }
      continue;
    }

    // Insert this subsession at the end of the list:
    if (fSubsessionsTail == NULL)
    {
      fSubsessionsHead = fSubsessionsTail = subsession;
    }
    else
    {
      ((DenseMediaSubsession *)fSubsessionsTail)->fDenseNext = subsession; // TODO: Tidy up.
      fSubsessionsTail = subsession;
    }

    subsession->serverPortNum = subsession->fClientPortNum; // by default

    char const *mStart = sdpLine;
    subsession->fSavedSDPLines = strDup(mStart);

    subsession->fMediumName = strDup(mediumName);
    delete[] mediumName;
    subsession->fProtocolName = strDup(protocolName);
    subsession->fRTPPayloadFormat = payloadFormat;

    // GØTE
    std::string densename = "fileSink.txt"; // TODO: What kind of file is this supposed to be?
    subsession->sink = DenseFileSink::createNew(envir(), densename.c_str());
    ((DenseFileSink *)subsession->sink)->fMediaSession = this;
    // GØTE ^

    // Process the following SDP lines, up until the next "m=":
    while (1)
    {
      sdpLine = nextSDPLine;
      if (sdpLine == NULL)
        break; // we've reached the end
      if (!parseSDPLine(sdpLine, nextSDPLine))
        return False;

      if (sdpLine[0] == 'm')
        break; // we've reached the next subsession

      // Check for various special SDP lines that we understand:
      if (subsession->parseSDPLine_c(sdpLine))
        continue;
      if (subsession->parseSDPLine_b(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_rtpmap(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_rtcpmux(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_control(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_range(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_fmtp(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_source_filter(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_x_dimensions(sdpLine))
        continue;
      if (subsession->parseSDPAttribute_framerate(sdpLine))
        continue;

      // (Later, check for malformed lines, and other valid SDP lines#####)
    }

    if (sdpLine != NULL)
      subsession->fSavedSDPLines[sdpLine - mStart] = '\0';

    // If we don't yet know the codec name, try looking it up from the
    // list of static payload types:
    if (subsession->fCodecName == NULL)
    {
      subsession->fCodecName = lookupPayloadFormat(subsession->fRTPPayloadFormat,
                                                   subsession->fRTPTimestampFrequency,
                                                   subsession->fNumChannels);
      if (subsession->fCodecName == NULL)
      {
        char typeStr[20];
        sprintf(typeStr, "%d", subsession->fRTPPayloadFormat);
        envir().setResultMsg("Unknown codec name for RTP payload type ", typeStr);
        return False;
      }
    }

    // If we don't yet know this subsession's RTP timestamp frequency
    // (because it uses a dynamic payload type and the corresponding
    // SDP "rtpmap" attribute erroneously didn't specify it),
    // then guess it now:
    if (subsession->fRTPTimestampFrequency == 0)
    {
      subsession->fRTPTimestampFrequency = guessRTPTimestampFrequency(subsession->fMediumName,
                                                                      subsession->fCodecName);
    }
  }

  return True;
}

Boolean DenseMediaSession::parseSDPLine_o(char const *sdpLine)
{
  Boolean oLine = False;
  char *buffer = strDupSize(sdpLine); // ensures we have enough space
  int s1 = 0;
  int s2 = 0;

  if (sscanf(sdpLine, "o=%[^\r\n]", buffer) == 1)
  {
    sscanf(sdpLine, "o=- %d %d IN IP4 %s", &s1, &s2, buffer);
    oLine = True;
  }

  delete[] buffer;

  return oLine;
}

Boolean DenseMediaSession::parseSDPLine_xmisc(char const *sdpLine)
{ // a=x-qt-text-misc:yo yo mood // TODO: change to more sensible
  // Check for "a=x-qt-text-misc" line
  char *buffer = strDupSize(sdpLine);
  Boolean parseSuccess = False;

  if (sscanf(sdpLine, "a=x-qt-text-misc:%[^\r\n]", buffer) == 1)
  {
    delete[] fxmisc;
    fxmisc = strDup(buffer);
    parseSuccess = True;
  }
  delete[] buffer;

  return parseSuccess;
}