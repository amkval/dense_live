#include "OutputFile.hh"

#include "include/DenseFileSink.hh"

#include <string>
#include <iostream>

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
    unsigned bufferSize) : FileSink(env, fileName, bufferSize, NULL),
                           fMediaSession(mediaSession), fWriteFromPacket(True)
{
}

DenseFileSink::~DenseFileSink()
{
}

Boolean DenseFileSink::continuePlaying()
{
  UsageEnvironment &env = envir();
  //env << "####:\t\tDenseFileSink::continuePlaying()\n";

  if (fSource == NULL)
  {
    env << "\tthere is no fSource (FramedSource) here!\n";
    return False;
  }

  fSource->getNextFrame(
      fBuffer, fBufferSize,
      afterGettingFrame, this,
      onSourceClosure, this);
  //env << "<<<<:\t\tDenseFileSink::continuePlaying()\n";
  return True;
}

// Note: Do we need both this one and the next?
// Check it both are used!
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
  //env << "####:\t\tDenseFileSink::afterGettingFrame()\n";

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

  if (fPerFrameFileNameBuffer != NULL)
  {
    if (fOutFid != NULL)
    {
      fclose(fOutFid);
      fOutFid = NULL;
    }
  }

  continuePlaying();
  //env << "<<<<:\t\tDenseFileSink::afterGettingFrame()\n";
}

/**
 * @brief TODO: Fill in this info
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
  FileSink::addData(data, dataSize, presentationTime);

  UsageEnvironment &env = envir();
  //env << "DenseFileSink::addData()\n";

  if (fMediaSession->fPutInLookAsideBuffer)
  {
    // Move the bytes to Lookaside Buffer
    env << "DenseFileSink::addData():\n"
        << "\tfMediaSession->fPacketChunk" << fMediaSession->fPacketChunk << "\n"
        << "\tfMediaSession->fLookAsideSize" << fMediaSession->fLookAsideSize << "\n";

    memmove(fMediaSession->fLookAside + fMediaSession->fLookAsideSize, data, dataSize);
    fMediaSession->fLookAsideSize += dataSize;

    if (fMediaSession->fLookAsideSize > 1000000)
    {
      fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);
      fclose(fOutFid);
      exit(0);
    }

    fMediaSession->fPutInLookAsideBuffer = False;
  }
  else
  {
    // New chunk arrived at the FileSink and its not to be put in lookaside
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

    // We need to transfer from lookaside to file first
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
}

/**
 * @brief TODO:
 *
 */
void DenseFileSink::finishFromLookAside()
{
  UsageEnvironment &env = envir();
  env << "FileSink::finishFromLookAside()\n"
      << "\tBytes int the lookAside buffer: "
      << fMediaSession->fLookAsideSize << "\n"
      << "\tBytes in the outFile: "
      << fMediaSession->fWritten << "\n";
  
  // Write from lookAside buffer to output file
  fMediaSession->fWritten += fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);

  // Reset Lookaside buffer  TODO: centralize buffer size!
  memset(fMediaSession->fLookAside, 0, 2000000);

  // Lookaside is now empty
  fMediaSession->fLookAsideSize = 0;

  env << "FileSink::finishFromLookAside()\n"
      << "\tBytes int the lookAside buffer: "
      << fMediaSession->fLookAsideSize << "\n"
      << "\tBytes in the outFile: "
      << fMediaSession->fWritten << "\n";
}

/**
 * @brief Pull a packet after a packet loss
 * Note: Not used? Does pull patching work at all?
 */
void DenseFileSink::pullPatch()
{
  UsageEnvironment &env = envir();
  env << "addData PACKET LOSS PRECEEDED THIS\n"
      << "\tfWritten: " << fMediaSession->fWritten << "\n";

  long fileSize = pullChunk(fMediaSession->fChunk);
  fWriteFromPacket = False;
  fMediaSession->fWritten = fMediaSession->fLastOffset + fileSize;
}

/**
 * @brief Pull all chunks from the start up until and including 'chunkCount'
 *
 * @param chunkCount number of chunks to be pulled
 */
void DenseFileSink::pullBeginning(int chunkCount)
{
  long fileSize;

  for (int i = 0; i <= chunkCount; i++)
  {
    fileSize = pullChunk(i);
    fWriteFromPacket = False;
    fMediaSession->fWritten += fileSize;
    fMediaSession->fPacketLoss = False;
    fMediaSession->fChunk = i;
  }
}

/**
 * @brief Pull 'all' chunks from the server
 *
 * Note:  Used while testing
 *        Uses hard coded values, which is not optimal
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
 * @param chunkId ID of the chunk to be pulled.
 * @return long size of write to file.
 */
long DenseFileSink::pullChunk(unsigned short chunkId)
{
  UsageEnvironment &env = envir();

  // Compile pull command
  std::string start = "wget ";                 // Command start
  std::string serverAddress = "10.20.0.16";    // Server IP
  std::string manStart = "/first";             // Quality Level
  std::string chunk = std::to_string(chunkId); // Chunk ID
  std::string manEnd = ".ts";                  // File extension
  std::string reqEnd = " --header \"Host: denseserver.com\"";
  std::string path = start + serverAddress + manStart + chunk + manEnd + reqEnd;

  std::string fileName = "first";
  fileName.append(chunk);
  fileName.append(manEnd);

  env << "DenseFileSink::pullChunk(): path: " << path.c_str() << " and filename: " << fileName.c_str() << "\n";

  // Execute Pull command
  system(path.c_str());

  FILE *file = fopen(fileName.c_str(), "rb");
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  env << "fileSize: " << fileSize << " fOutfID: " << ftell(fMediaSession->fOut) << "\n";
  fseek(file, 0, SEEK_SET);

  char *buffer = new char[fileSize];

  fread(buffer, 1, fileSize, file);
  fseek(fMediaSession->fOut, 0, fMediaSession->fWritten);
  long written = fwrite(buffer, 1, fileSize, fMediaSession->fOut);
  env << "written: " << written << " fOutfID: " << ftell(fMediaSession->fOut) << "\n";
  fclose(file);

  delete[] buffer;
  return written;
}
