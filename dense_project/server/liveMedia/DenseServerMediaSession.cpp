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

#include "include/DenseServerMediaSession.hh"

static char const *const libNameStr = "LIVE555 Streaming Media v";
char const *const libVersionStr = LIVEMEDIA_LIBRARY_VERSION_STRING;

////////// DenseServerMediaSession //////////

DenseServerMediaSession::DenseServerMediaSession(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines) : ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines)
{
}

DenseServerMediaSession::~DenseServerMediaSession()
{
}

DenseServerMediaSession *DenseServerMediaSession::createNew(
    UsageEnvironment &env,
    char const *streamName,
    char const *info,
    char const *description,
    Boolean isSSM,
    char const *miscSDPLines)
{
  return new DenseServerMediaSession(
      env,
      streamName,
      info,
      description,
      isSSM,
      miscSDPLines);
}

// TODO: This function had only one small change and may not need to be copied completely
/*
char *DenseServerMediaSession::generateSDPDescription()
{

  AddressString ipAddressStr(ourIPAddress(envir()));
  unsigned ipAddressStrSize = strlen(ipAddressStr.val());
  // For a SSM sessions, we need a "a=source-filter: incl ..." line also:
  char *sourceFilterLine;
  if (fIsSSM)
  {
    char const *const sourceFilterFmt =
        "a=source-filter: incl IN IP4 * %s\r\n"
        "a=rtcp-unicast: reflection\r\n";
    unsigned const sourceFilterFmtSize = strlen(sourceFilterFmt) + ipAddressStrSize + 1;

    sourceFilterLine = new char[sourceFilterFmtSize];
    sprintf(sourceFilterLine, sourceFilterFmt, ipAddressStr.val());
  }
  else
  {
    sourceFilterLine = strDup("");
  }

  char *rangeLine = NULL; // for now
  char *sdp = NULL;       // for now

  do
  {
    // Count the lengths of each subsession's media-level SDP lines.
    // (We do this first, because the call to "subsession->sdpLines()"
    // causes correct subsession 'duration()'s to be calculated later.)
    unsigned sdpLength = 0;
    ServerMediaSubsession *subsession;
    for (subsession = fSubsessionsHead; subsession != NULL;
         subsession = subsession->fNext)
    {
      char const *sdpLines = subsession->sdpLines();
      if (sdpLines == NULL)
        continue; // the media's not available

      sdpLength += strlen(sdpLines);
    }
    if (sdpLength == 0)
      break; // the session has no usable subsessions

    // Unless subsessions have differing durations, we also have a "a=range:" line:
    float dur = duration();
    if (dur == 0.0)
    {
      rangeLine = strDup("a=range:npt=0-\r\n");
    }
    else if (dur > 0.0)
    {
      char buf[100];
      sprintf(buf, "a=range:npt=0-%.3f\r\n", dur);
      rangeLine = strDup(buf);
    }
    else
    { // subsessions have differing durations, so "a=range:" lines go there
      rangeLine = strDup("");
    }
    char const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %ld%06ld %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:*\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "a=x-qt-text-misc:%s\r\n"; // This is was changed for the Dense Server
    sdpLength += strlen(sdpPrefixFmt) + 20 + 6 + 20 + ipAddressStrSize +
                 strlen(fDescriptionSDPString) + strlen(fInfoSDPString) +
                 strlen(libNameStr) + strlen(libVersionStr) +
                 strlen(sourceFilterLine) + strlen(rangeLine) +
                 strlen(fDescriptionSDPString) + strlen(fInfoSDPString) +
                 strlen(fMiscSDPLines);
    sdpLength += 1000; // in case the length of the "subsession->sdpLines()" calls below change

    sdp = new char[sdpLength];
    if (sdp == NULL)
      break;

    //  Generate the SDP prefix (session-level lines):
    snprintf(sdp, sdpLength, sdpPrefixFmt,
             fCreationTime.tv_sec, fCreationTime.tv_usec, // o= <session id>
             1,                                           // o= <version> // (needs to change if params are modified)
             ipAddressStr.val(),                          // o= <address>
             fDescriptionSDPString,                       // s= <description>
             fInfoSDPString,                              // i= <info>
             libNameStr, libVersionStr,                   // a=tool:
             sourceFilterLine,                            // a=source-filter: incl (if a SSM session)
             rangeLine,                                   // a=range: line
             fDescriptionSDPString,                       // a=x-qt-text-nam: line
             fInfoSDPString,                              // a=x-qt-text-inf: line
             fMiscSDPLines);                              // miscellaneous session SDP lines (if any)

    // Then, add the (media-level) lines for each subsession:
    char *mediaSDP = sdp;
    for (subsession = fSubsessionsHead; subsession != NULL;
         subsession = subsession->fNext)
    {
      unsigned mediaSDPLength = strlen(mediaSDP);
      mediaSDP += mediaSDPLength;
      sdpLength -= mediaSDPLength;
      if (sdpLength <= 1)
        break; // the SDP has somehow become too long

      char const *sdpLines = subsession->sdpLines();
      if (sdpLines != NULL)
        snprintf(mediaSDP, sdpLength, "%s", sdpLines);
    }
  } while (0);

  delete[] rangeLine;
  delete[] sourceFilterLine;
  return sdp;
}
*/

Boolean DenseServerMediaSession::addSubsession(ServerMediaSubsession *subsession)
{
  ServerMediaSession::addSubsession(subsession);
  fServerMediaSubsessionVector.push_back(subsession);
}