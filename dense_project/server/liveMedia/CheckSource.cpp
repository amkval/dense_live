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

#include "InputFile.hh"

#include "include/CheckSource.hh"
#include "GroupsockHelper.hh"

CheckSource *CheckSource::createNew(UsageEnvironment &env, std::string fileName, unsigned preferredFrameSize, unsigned playTimePerFrame)
{
  fprintf(stderr, "Constructing 'CheckSource' \n");

  FILE *fid = OpenInputFile(env, (const char *)fileName.c_str()); // TODO: is this cast legal?
  if (fid == NULL)
  {
    return NULL; // TODO: verify error handeling!
  }

  CheckSource *checkSource = new CheckSource(env, fid, preferredFrameSize, playTimePerFrame);

  checkSource->fChunkCount = checkSource->stripChunks();
  checkSource->fPath = checkSource->stripPath(fileName);

  std::string newPath = checkSource->fPath + (char *)checkSource->fChunks[0]; // TODO: do we need cast?

  fprintf(stderr, "newPath: %s\n", newPath);
  fprintf(stderr, "checkSource->fPath: %s, %s\n", checkSource->fPath, checkSource->fChunks[0]);

  FILE *newFid = OpenInputFile(env, (const char *)newPath.c_str());
  if (newFid == NULL)
  {
    return NULL; // TODO: Verify error handeling
  }

  checkSource->fFid = newFid;
  checkSource->fFileSize = GetFileSize((const char *)newPath.c_str(), newFid);

  fprintf(stderr, "checkSource->fFileSize: %d\n", checkSource->fFileSize);

  return checkSource;
}

CheckSource::CheckSource(
    UsageEnvironment &env, FILE *fid,
    unsigned preferredFrameSize,
    unsigned playTimePerFrame)
    : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
      fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
      fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0)
{
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  makeSocketNonBlocking(fileno(fFid));
#endif

  // Test whether the file is seekable
  fFidIsSeekable = FileIsSeekable(fFid);
}

CheckSource::~CheckSource()
{
}

int CheckSource::stripChunks()
{
  fprintf(stderr, "CheckSource::stripChunks()\n");

  int chunkCount = 0;
  int lineCount = 0;
  size_t size = 0;
  char *line = NULL; // TODO: is there any point in NULL initializing?

  getline(&line, &size, fFid);

  fprintf(stderr, "line: %s\nline[0]: %c\n", line, line[0]);

  if (line[0] != '#')
  {
    fprintf(stderr, "This line does not adhere to manifest format!");
    exit(0);
  }

  while (getline(&line, &size, fFid) > 0)
  {
    fprintf(stderr, "line: %s\n", line);
    if (line[0] != '#')
    {
      memcpy(fChunks[chunkCount], line, strlen(line) + 1);
      fprintf(stderr, "added: chunkCount: %d, lineCount: %d, line: %d\n", chunkCount, lineCount, line);
      chunkCount++;
    }
    else
    {
      fprintf(stderr, "skipped: lineCount: %d, line: %d\n", chunkCount, lineCount, line);
    }
    lineCount++;
  }

  return chunkCount;
}

std::string CheckSource::stripPath(std::string fileName) {
  size_t slash = fileName.find_last_of('/');
  return (slash == std::string::npos) ? fileName : fileName.substr(slash);
}