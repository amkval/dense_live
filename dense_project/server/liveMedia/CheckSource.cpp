#include "InputFile.hh"
#include "GroupsockHelper.hh"

#include "include/CheckSource.hh"

// Dummy for gdb. just call this function anywhere!
void dummy()
{
  /* I am dummy */
}

CheckSource *CheckSource::createNew(
    UsageEnvironment &env,
    std::string fileName,
    unsigned preferredFrameSize,
    unsigned playTimePerFrame)
{
  // Open Manifest File
  FILE *fid = OpenInputFile(env, (const char *)fileName.c_str());
  if (fid == NULL) return NULL;
  env << "\n\nOpen Manifest File:\t" << fileName.c_str() << "\n";

  char chunks[1000][100] = {0};
  int chunkCount = stripChunks(&chunks, fid);
  std::string path = stripPath(fileName);

  // Find path to first file
  std::string newPath = path + (char *)chunks[0];
  // Remove newline character
  newPath.erase(newPath.length() - 1);

  env << "\tnewPath: " << newPath.c_str() << "\n"
      << "\tpath: " << path.c_str() << "\n"
      << "\tchunks[0]: " << chunks[0] << "\n";

  // Close Manifest File
  CloseInputFile(fid);

  // Open first video file in preparation to start
  FILE *newFid = OpenInputFile(env, newPath.c_str());
  if (newFid == NULL) return NULL;
  env << "Open first input file:\t" << newPath.c_str() << "\n";


  CheckSource *checkSource = new CheckSource(
      env,
      newFid,
      preferredFrameSize,
      playTimePerFrame);

  // Set values, note: consider using setters!
  checkSource->fChunkCount = chunkCount;
  checkSource->fPath = path;
  checkSource->fFileSize = GetFileSize((const char *)newPath.c_str(), newFid);
  std::copy(&chunks[0][0], &chunks[0][0] + 100 * 1000, &checkSource->fChunks[0][0]);

  env << "\tcheckSource->fFileSize: " << std::to_string(checkSource->fFileSize).c_str() << "\n";

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

int CheckSource::stripChunks(char (*chunks)[1000][100], FILE *fid)
{
  fprintf(stderr, "CheckSource::stripChunks()\n");

  int chunkCount = 0;
  //int lineCount = 0;
  size_t size = 0;
  char *line = NULL; // Note: Should this be an empty string instead?

  getline(&line, &size, fid);
  if (line[0] != '#')
  {
    fprintf(stderr, "This line does not adhere to manifest format!");
    exit(0);
  }

  while (getline(&line, &size, fid) > 0)
  {
    if (line[0] != '#')
    {
      memcpy((*chunks)[chunkCount], line, strlen(line) + 1);
      chunkCount++;
    }
    //lineCount++;
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
    //fprintf(stdout, "Add Background Read Handling!\n");
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
  //fprintf(stdout, "File read handler has been called!\n");
  if (!source->isCurrentlyAwaitingData())
  {
    source->doStopGettingFrames();
    return;
  }
  source->doReadFromFile();
  //fprintf(stderr, "After fileReadableHandler\n");
}

void CheckSource::doReadFromFile()
{
  //fprintf(stdout, "Do read from file has been called!\n");
  //[[maybe_unused]] u_int32_t firstTimestamp; // note: not used!
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
  
  // Change input file if needed
  manageManifest();

  if (fFrameSize <= 0)
  {
    fprintf(stdout, "We are exiting!\n");
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
  //fprintf(stderr, "After doReadFromFile()\n");
  FramedSource::afterGetting(this);
#endif
}

int CheckSource::manageManifest()
{
  //fprintf(stdout, "Manage manifest has been called!\n");
  //fprintf(stdout, "fFileSize %d, fReadSoFar %d, fCurrentChunk %d, fChunkCount %d\n", fFileSize, fReadSoFar, fCurrentChunk, fChunkCount);

  if ((fFileSize - fReadSoFar) == 0 && fCurrentChunk <= fChunkCount)
  {
    fCurrentChunk++;

    std::string newPath = fPath + (char *)fChunks[fCurrentChunk]; // TODO: do we need cast?
    newPath.erase(newPath.length() - 1);                          // TODO: Make better solution
    
    fprintf(stderr, "Read: %s\n", newPath.c_str());

    FILE *newFid = OpenInputFile(envir(), newPath.c_str());
    if (newFid == NULL)
    {
      fprintf(stderr, "In the CheckSource::manageManifest(): Could not open file!\n");
      exit(1);
    }
    //fprintf(stderr, "FILE* OpenInputFile 3: %s\n",  newPath.c_str());
    
    // Replace old file with new.
    FILE *oldFid = fFid;
    fFid = newFid;
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    envir().taskScheduler().turnOffBackgroundReadHandling(fileno(oldFid));
#endif
    CloseInputFile(oldFid);

    fFileSize = GetFileSize(newPath.c_str(), newFid);
    fReadSoFar = 0;
    fHaveStartedReading = False;
    //fprintf(stderr, "Finish ManageManifest\n");
  }

  return 1;
}