#include "OutputFile.hh"

#include "include/DenseFileSink.hh"

#include <string>
#include <iostream>
#include <stdio.h>

DenseFileSink *DenseFileSink::createNew(
    UsageEnvironment &env,
    char const *fileName,
    DenseMediaSession *mediaSession,
    unsigned bufferSize)
{
  FILE *fid = OpenOutputFile(env, fileName);
  return new DenseFileSink(env, fid, mediaSession, bufferSize);
}

DenseFileSink::DenseFileSink(
    UsageEnvironment &env,
    FILE *fileName,
    DenseMediaSession *mediaSession,
    unsigned bufferSize)
    : FileSink(env, fileName, bufferSize, NULL),
      fMediaSession(mediaSession), fWriteFromPacket(True)
{
}

DenseFileSink::~DenseFileSink()
{
}

Boolean DenseFileSink::continuePlaying()
{
  UsageEnvironment &env = envir();

  // Check if FramedSource is present
  if (fSource == NULL)
  {
    env << "There is no fSource (FramedSource) here!\n";
    return False;
  }

  // Get the next frame
  fSource->getNextFrame(
      fBuffer, fBufferSize,
      afterGettingFrame, this,
      onSourceClosure, this);
  return True;
}

void DenseFileSink::afterGettingFrame(
    void *clientData, unsigned frameSize,
    unsigned numTruncatedBytes,
    struct timeval presentationTime,
    unsigned /*durationInMicroseconds*/)
{

  DenseFileSink *sink = (DenseFileSink *)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void DenseFileSink::afterGettingFrame(
    unsigned frameSize,
    unsigned numTruncatedBytes,
    struct timeval presentationTime)
{
  UsageEnvironment &env = envir();

  if (numTruncatedBytes > 0)
  {
    env << "FileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
        << fBufferSize << ").  "
        << numTruncatedBytes
        << " bytes of trailing data was dropped! "
        << "Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
        << fBufferSize + numTruncatedBytes << "\n";
  }

  addData(fBuffer, frameSize, presentationTime);

  if (fOutFid == NULL || fflush(fOutFid) == EOF)
  {
    env << "\tFileSink closing 1\n";
    // The output file has closed.  Handle this the same way as if the input source had closed:
    if (fSource != NULL)
    {
      fSource->stopGettingFrames();
    }
    env << "\tFileSink closing 2\n";

    onSourceClosure();
    return;
  }

  /*
  if (fPerFrameFileNameBuffer != NULL)
  {
    if (fOutFid != NULL)
    {
      fclose(fOutFid);
      fOutFid = NULL;
    }
  }
  */

  continuePlaying();
}

/**
 * @brief Adds received data to output file
 *
 * @param data
 * @param dataSize
 * @param presentationTime
 */
void DenseFileSink::addData(
    unsigned char const *data,
    unsigned dataSize,
    struct timeval presentationTime)
{
  UsageEnvironment &env = envir();

  // Pull the start of the stream that we missed.
  if(fMediaSession->fWritten == 0){
    env << "Pulling chunks 0 to " << fMediaSession->fPacketChunk << "\n";
    pullBeginning(fMediaSession->fPacketChunk);
    return;
  }

  // If we need to write to lookaside
  if (fMediaSession->fPutInLookAsideBuffer)
  {
    memmove(fMediaSession->fLookAside + fMediaSession->fLookAsideSize, data, dataSize);
    fMediaSession->fLookAsideSize += dataSize;

    // Dump lookaside buffer and exit program if lookaside buffer grows too large
    if (fMediaSession->fLookAsideSize > 1000000)
    {
      fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);
      fclose(fOutFid);
      env << "Lookaside buffer grew too large, quitting program!\n";
      exit(EXIT_FAILURE);
    }

    fMediaSession->fPutInLookAsideBuffer = False;
    return;
  }

  // Pull patch if there is too much packet loss
  if (fMediaSession->fLevelDrops >= 3 && fWriteFromPacket)
  {
    pullPatch();
  }

  // If a new chunk has arrived
  if (fMediaSession->fPacketChunk > fMediaSession->fChunk)
  {
    fWriteFromPacket = True;
    fMediaSession->fChunk = fMediaSession->fPacketChunk;
    fMediaSession->fLastOffset = fMediaSession->fWritten;
    env << "Setting new fChunk in FileSink\n"
        << "\tfMediaSession->fPacketChunk: "
        << fMediaSession->fPacketChunk << "\n"
        << "\tfMediaSession->fChunk: "
        << fMediaSession->fChunk << "\n"
        << "\tfMediaSession->fWritten: "
        << fMediaSession->fWritten << "\n"
        << "\tfMediaSession->fLastOffset: "
        << fMediaSession->fLastOffset << "\n";
  }

  // If we need to write from lookaside buffer
  if (fMediaSession->fFinishLookAside)
  {
    finishFromLookAside();
    fMediaSession->fFinishLookAside = False;
  }

  // Write from 'data' to 'fOut'
  if (fOutFid != NULL && data != NULL && fWriteFromPacket)
  {
    fMediaSession->fWritten += fwrite(data, 1, dataSize, fMediaSession->fOut);
  }
}

/**
 * @brief Write contents of lookaside buffer to output file
 */
void DenseFileSink::finishFromLookAside()
{
  UsageEnvironment &env = envir();
  env << "#### Write from lookaside buffer ####\n"
      << "fLookAsideSize: "
      << fMediaSession->fLookAsideSize << "\n"
      << "fWritten: "
      << fMediaSession->fWritten << "\n";

  // Write from lookAside buffer to output file
  fMediaSession->fWritten += fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);

  // Reset Lookaside buffer
  memset(fMediaSession->fLookAside, 0, LOOKASIDE_BUFFER_SIZE);

  // Lookaside is now empty
  fMediaSession->fLookAsideSize = 0;
}

/**
 * @brief Pull a packet after a packet loss
 */
void DenseFileSink::pullPatch()
{
  UsageEnvironment &env = envir();
  env << "#### Pull Patch because of packet loss ####\n"
      << "fWritten: " << fMediaSession->fWritten << "\n";

  long fileSize = pullChunk(fMediaSession->fChunk);
  fMediaSession->fLastOffset += fileSize;
  fWriteFromPacket = False;
  fMediaSession->fWritten = fMediaSession->fLastOffset;
}

/**
 * @brief Pull all chunks from the start up until and including 'chunkCount'
 *
 * @param chunkCount number of chunks to be pulled
 */
void DenseFileSink::pullBeginning(int chunkCount)
{
  for (int i = 0; i <= chunkCount; i++)
  {
    long fileSize = pullChunk(i);
    fWriteFromPacket = False;
    fMediaSession->fLastOffset += fileSize;
    fMediaSession->fWritten += fMediaSession->fLastOffset;
    fMediaSession->fChunk = i;
  }
}

/**
 * @brief Pull 'all' chunks from the server
 *
 */
void DenseFileSink::pullAll()
{
  for (int i = 0; i <= 60; i++)
  {
    pullChunk(i);
  }

  fclose(fOutFid);
  exit(EXIT_SUCCESS);
}

/**
 * @brief Pull a specific chunk from the server and write to file.
 *  
 * Note: There are better ways to do this without wget and extra files.
 * 
 * @param chunkId ID of the chunk to be pulled.
 * @return long size of write to file.
 */
long DenseFileSink::pullChunk(unsigned short chunkId)
{
  UsageEnvironment &env = envir();

  // Compile pull command
  std::string start = "wget ";                  // Command start
  std::string serverAddress = "localhost";      // Server IP
  std::string manStart = "/chunks/first";       // Quality Level
  std::string chunk = std::to_string(chunkId);  // Chunk ID
  std::string manEnd = ".ts";                   // File extension
  std::string reqEnd = " --header \"Host: denseserver.com\"";
  std::string path = start + serverAddress + manStart + chunk + manEnd + reqEnd;

  // Compile file name
  std::string fileName = "first";
  fileName.append(chunk);
  fileName.append(manEnd);

  env << "#### PullChunk ####\n";
  env << "path: " << path.c_str() << "\nfilename: " << fileName.c_str() << "\n";

  // Execute Pull command
  system(path.c_str());

  // Open patch file
  FILE *file = fopen(fileName.c_str(), "rb");
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  env << "fileSize: " << fileSize << "\nfOutfID: " << ftell(fMediaSession->fOut) << "\n";
  fseek(file, 0, SEEK_SET);

  // Read from file to buffer
  char *buffer = new char[fileSize];
  fread(buffer, 1, fileSize, file);

  // Write buffer to output file
  fseek(fMediaSession->fOut, 0, fMediaSession->fLastOffset);
  long written = fwrite(buffer, 1, fileSize, fMediaSession->fOut);
  env << "written: " << written << "\nfOutfID: " << ftell(fMediaSession->fOut) << "\n";
  
  // Cleanup
  fclose(file);
  delete[] buffer;
  remove(fileName.c_str());

  return written;
}
