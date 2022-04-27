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

#include "InputFile.hh"
#include "GroupsockHelper.hh"

#include "include/CheckSource.hh"

CheckSource *CheckSource::createNew(
    UsageEnvironment &env,
    std::string fileName,
    unsigned preferredFrameSize,
    unsigned playTimePerFrame)
{
  env << "Constructing 'CheckSource' \n";

  FILE *fid = OpenInputFile(env, (const char *)fileName.c_str()); // TODO: is this cast legal?
  if (fid == NULL)
  {
    return NULL;
  }

  CheckSource *checkSource = new CheckSource(
      env,
      fid,
      preferredFrameSize,
      playTimePerFrame);

  // TODO: use setters?
  checkSource->fChunkCount = checkSource->stripChunks();
  checkSource->fPath = checkSource->stripPath(fileName);

  std::string newPath = checkSource->fPath + (char *)checkSource->fChunks[0]; // TODO: do we need cast?
  newPath.erase(newPath.length() - 1);                                        // TODO: Make better solution

  env << "newPath: " << newPath.c_str() << "\n";
  env << "checkSource->fPath: " << checkSource->fPath.c_str() << ", " << checkSource->fChunks[0] << "\n";

  FILE *newFid = OpenInputFile(env, newPath.c_str());
  if (newFid == NULL)
  {
    return NULL;
  }

  checkSource->fFid = newFid;
  checkSource->fFileSize = GetFileSize((const char *)newPath.c_str(), newFid);

  // TODO: potentially print uint64_t
  env << "checkSource->fFileSize: " <<  std::to_string(checkSource->fFileSize).c_str() << "\n";

  return checkSource;
}

CheckSource::CheckSource(
    UsageEnvironment &env, FILE *fid,
    unsigned preferredFrameSize,
    unsigned playTimePerFrame)
    : FramedFileSource(env, fid), fFileSize(0), fReadSoFar(0), fCurrentChunk(0),
      fChunkCount(0), fPreferredFrameSize(preferredFrameSize),
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
  if (fFid == NULL)
  {
    return;
  }

#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
#endif

  CloseInputFile(fFid);
}

int CheckSource::stripChunks()
{
  fprintf(stderr, "CheckSource::stripChunks()\n");

  int chunkCount = 0;
  int lineCount = 0;
  size_t size = 0;
  char *line = NULL; // TODO: is there any point in NULL initializing?

  getline(&line, &size, fFid);

  fprintf(stderr, "line: %s", line);

  if (line[0] != '#')
  {
    fprintf(stderr, "This line does not adhere to manifest format!");
    exit(0);
  }

  while (getline(&line, &size, fFid) > 0)
  {
    fprintf(stderr, "line: %s", line);
    if (line[0] != '#')
    {
      memcpy(fChunks[chunkCount], line, strlen(line) + 1);
      chunkCount++;
      // fprintf(stderr, "added: chunkCount: %d, lineCount: %d, line: %s\n", chunkCount, lineCount, line);
    }
    else
    {
      // fprintf(stderr, "skipped: lineCount: %d, line: %s\n", lineCount + 1, line);
    }
    lineCount++;
  }

  return chunkCount;
}

std::string CheckSource::stripPath(std::string fileName)
{
  fprintf(stderr, "fileName: %s\n", fileName.c_str());
  size_t slash = fileName.find_last_of('/');
  return (slash == std::string::npos) ? fileName : fileName.substr(0, slash + 1);
}

void CheckSource::doGetNextFrame()
{
  if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0))
  {
    handleClosure();
    return;
  }

#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  doReadFromFile();
#else
  if (!fHaveStartedReading)
  {
    // Await readable data from the file:
    envir().taskScheduler().turnOnBackgroundReadHandling(
        fileno(fFid),
        (TaskScheduler::BackgroundHandlerProc *)&fileReadableHandler, this);
    fHaveStartedReading = True;
  }
#endif
}

void CheckSource::fileReadableHandler(CheckSource *source, int /*mask*/)
{
  if (!source->isCurrentlyAwaitingData())
  {
    source->doStopGettingFrames();
    return;
  }
  source->doReadFromFile();
}

void CheckSource::doReadFromFile()
{
  [[maybe_unused]] u_int32_t firstTimestamp; // TODO: not used!
  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize)
  {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize)
  {
    fMaxSize = fPreferredFrameSize;
  }

  fFrameSize = fread(fTo, 1, fMaxSize, fFid);
  fReadSoFar += fFrameSize;
  [[maybe_unused]] int readSize = manageManifest(); // TODO: Not used

  if (fFrameSize == 0)
  {
    handleClosure();
    return;
  }
  fNumBytesToStream -= fFrameSize;

  // Set the 'presentation time':
  if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0)
  {
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)
    {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);
    }
    else
    {
      // Increment by the play time of the previous data:
      unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
      fPresentationTime.tv_sec += uSeconds / 1000000;
      fPresentationTime.tv_usec = uSeconds % 1000000;
    }
    // Remember the play time of this data:
    fLastPlayTime = (fPlayTimePerFrame * fFrameSize) / fPreferredFrameSize;
    fDurationInMicroseconds = fLastPlayTime;
  }
  else
  {
    // We don't know a specific play time duration for this data,
    // so just record the current time as being the 'presentation time':
    gettimeofday(&fPresentationTime, NULL);
  }

  // Inform the reader that he has data:
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                           (TaskFunc *)FramedSource::afterGetting, this);
#else
  // Because the file read was done from the event loop, we can call the
  // 'after getting' function directly, without risk of infinite recursion:
  FramedSource::afterGetting(this);
#endif
}

int CheckSource::manageManifest()
{
  if ((fFileSize - fReadSoFar) == 0 && fCurrentChunk <= fChunkCount)
  {
    fCurrentChunk++;

    std::string newPath = fPath + (char *)fChunks[fCurrentChunk]; // TODO: do we need cast?
    newPath.erase(newPath.length() - 1);                          // TODO: Make better solution

    FILE *newFid = OpenInputFile(envir(), newPath.c_str());
    if (newFid == NULL)
    {
      fprintf(stderr, "In the CheckSource::manageManifest(): Could not open file!\n");
      exit(1);
    }
    fFid = newFid;
    fFileSize = GetFileSize(newPath.c_str(), newFid);
    fReadSoFar = 0;
  }

  return 1;
}