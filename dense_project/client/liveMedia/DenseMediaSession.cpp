#include "include/DenseMediaSession.hh"
#include "include/DenseFileSink.hh"

DenseMediaSession *DenseMediaSession::createNew(
    UsageEnvironment &env,
    std::string sdpDescription)
{
  DenseMediaSession *denseMediaSession = new DenseMediaSession(env);
  if (denseMediaSession == NULL)
  {
    env << "Failed to create new DenseMediaSession\n";
    exit(EXIT_FAILURE);
  }

  if (!denseMediaSession->initializeWithSDP(sdpDescription.c_str()))
  {
    env << "Failed to initialize DenseMediaSession with sdpDescription\n";
    delete denseMediaSession;
    exit(EXIT_FAILURE);
  }

  return denseMediaSession;
}

DenseMediaSession::DenseMediaSession(UsageEnvironment &env)
    : MediaSession(env), fInControl(NULL), fDenseNext(NULL),
      fFinishLookAside(False), fPacketLoss(False), fPutInLookAsideBuffer(False),
      fOut(NULL), fCurrentLevel(0), fLastOffset(0), fLevelDrops(0), fLookAsideSize(0),
      fTotalDrops(0), fWritten(0),
      fLookAside(new unsigned char[2000000]), // TODO: centralize buffer size!
      fChunk(0), fPacketChunk(0), fRTPChunk(65535)
{
  fOut = fopen("result.mp4", "wbr");
  if (fOut == NULL)
  {
    env << "Failed to open file 'result.mp4' in DenseMediaSession \n";
    exit(EXIT_FAILURE);
  }
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

  // Dense Modification
  int level = 0;
  // Dense Modification ^

  while (sdpLine != NULL)
  {
    // We have a "m=" line, representing a new subsession:

    // Dense Modification
    // Note: Fix!
    DenseMediaSubsession *subsession = DenseMediaSubsession::createNew(envir(), *this, this);
    // Dense Modification ^

    if (subsession == NULL)
    {
      envir().setResultMsg("Unable to create new DenseMediaSubsession");
      return False;
    }

    // Dense Modification
    if (level == 0)
    {
      subsession->fInit = 1;
    }
    else
    {
      subsession->fInit = 0;
    }
    subsession->fLevel = level++; // TODO: this should be in a constructor!
    
    // Dense Modification ^

    // Parse the line as "m=<medium_name> <client_portNum> RTP/AVP <fmt>"
    // or "m=<medium_name> <client_portNum>/<num_ports> RTP/AVP <fmt>"
    // (Should we be checking for >1 payload format number here?)#####
    char *mediumName = strDupSize(sdpLine); // ensures we have enough space
    char const *protocolName = NULL;
    unsigned payloadFormat;

    if ((sscanf(sdpLine, "m=%s %hu RTP/AVP %u",
                mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
         sscanf(sdpLine, "m=%s %hu/%*u RTP/AVP %u",
                mediumName, &subsession->fClientPortNum, &payloadFormat) == 3) &&
        payloadFormat <= 127)
    {
      protocolName = "RTP";
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
      ((DenseMediaSubsession *)fSubsessionsTail)->fDenseNext = subsession;
      fSubsessionsTail = subsession;
    }
    // Dense Modification
    // This enables the use of a subsession vector, meaning we can disregard all the old next pointers.
    // Note: The old pointers may be removed as we don't need them anymore!
    fDenseMediaSubsessions.push_back(subsession);
    // Dense Modification ^

    subsession->serverPortNum = subsession->fClientPortNum; // by default

    char const *mStart = sdpLine;
    subsession->fSavedSDPLines = strDup(mStart);

    subsession->fMediumName = strDup(mediumName);
    delete[] mediumName;
    subsession->fProtocolName = strDup(protocolName);
    subsession->fRTPPayloadFormat = payloadFormat;

    // Dense Modifications
    // TODO: Why do we have two+ sinks?
    std::string denseName = "fileSink.txt";
    subsession->sink = DenseFileSink::createNew(envir(), denseName.c_str(), this);
    // Dense Modifications ^

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

/**
 * @brief looks for an 'oLine' in the 'sdpLine'
 *
 * @param sdpLine line to search.
 * @return Boolean True if found, False if not.
 */
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

/**
 * @brief looks for a 'misc' line in the 'sdpLine'
 *
 * @param sdpLine Line to search.
 * @return Boolean True if found, False if not.
 */
Boolean DenseMediaSession::parseSDPLine_xmisc(char const *sdpLine)
{ // a=x-qt-text-misc:yo yo mood // TODO: change to something more sensible
  // Check for "a=x-qt-text-misc" line
  Boolean parseSuccess = False;
  char *buffer = strDupSize(sdpLine); // ensures we have enough space

  if (sscanf(sdpLine, "a=x-qt-text-misc:%[^\r\n]", buffer) == 1)
  {
    delete[] fxmisc;
    fxmisc = strDup(buffer);
    parseSuccess = True;
  }
  delete[] buffer;

  return parseSuccess;
}