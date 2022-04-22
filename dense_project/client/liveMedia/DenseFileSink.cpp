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

#include "include/DenseFileSink.hh"

DenseFileSink::DenseFileSink(
    UsageEnvironment &env,
    FILE *fid,
    unsigned bufferSize,
    char const *perFrameFileNamePrefix,
    DenseMediaSession *mediaSession) : FileSink(env, fid, bufferSize, perFrameFileNamePrefix), fMediaSession(mediaSession)
{
}

DenseFileSink::~DenseFileSink()
{
}

DenseFileSink *DenseFileSink::createNew(
    UsageEnvironment &env,
    char const *fileName,
    unsigned bufferSize,
    Boolean oneFilePerFrame)
{
  DenseFileSink *ret = (DenseFileSink *)FileSink::createNew(env, fileName, bufferSize, oneFilePerFrame);
  ret->fWriteFromPacket = True;
  return ret;
}

void DenseFileSink::addData(
    unsigned char const *data,
    unsigned dataSize,
    struct timeval presentationTime)
{
  FileSink::addData(data, dataSize, presentationTime);

  if (fMediaSession->fPutInLookAsideBuffer)
  {
    // FramedSource *chunkSet = source();
    // move the bytes to the lookaside
    memmove(fMediaSession->fLookAside + fMediaSession->fLookAsideSize, data, dataSize);
    fMediaSession->fLookAsideSize += dataSize;

    if (fMediaSession->fLookAsideSize > 1000000)
    {
      fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);
      fclose(fOutFid);
      exit(0);
    }

    fMediaSession->fPutInLookAsideBuffer = False;
    return; // Does not continue
  }

  if (fMediaSession->fPacketChunk > fMediaSession->fChunk)
  { // New chunk arrived at the FileSink and its not to be put in lookaside
    fWriteFromPacket = True;
    fMediaSession->fChunk = fMediaSession->fPacketChunk;
    fMediaSession->fLastOffset = fMediaSession->fWritten;
    /* fprintf(mediaSession->debugFile, "\nSetting new fChunk in FileSink\npacketchunk: %d\nfChunk now: %d\n", mediaSession->packetChunk, mediaSession->fChunk);
    fprintf(mediaSession->debugFile, "\nAlso setting new lastOffset %d\n\n", mediaSession->lastOffset); */
    fprintf(stderr, "\nthe fWritten is: %u\n\nlast offset: %u\n\n\n", fMediaSession->fWritten, fMediaSession->fLastOffset);
    /* fprintf(mediaSession->debugFile, "\nthe fWritten is: %u\n\n", mediaSession->fWritten); */
  }

  if (fMediaSession->fFinishLookAside)
  { // We need to transfer from lookaside to file first
    finishFromLookAside();
    fMediaSession->fFinishLookAside = False;
  }

  if (fOutFid != NULL && data != NULL && fWriteFromPacket)
  {
    fMediaSession->fWritten += fwrite(data, 1, dataSize, fMediaSession->fOut);
  }
}

void DenseFileSink::finishFromLookAside()
{
  Boolean quit = False;

  if (fMediaSession->fLookAsideSize > 0)
  {
    quit = True;
  }

  fMediaSession->fWritten += fwrite(fMediaSession->fLookAside, 1, fMediaSession->fLookAsideSize, fMediaSession->fOut);
  memset(fMediaSession->fLookAside, 0, 2000000); // Zero out the buffer
  fMediaSession->fLookAsideSize = 0;

  if (quit)
  {
    // exit(0);
  }
}

void DenseFileSink::pullPatch()
{
  long fileSize;
  fileSize = pullChunk(fMediaSession->fChunk);
  fWriteFromPacket = False;
  fMediaSession->fWritten = fMediaSession->fLastOffset + fileSize;
}

void DenseFileSink::pullBeginning(int ant)
{
  long fileSize;

  for (int i = 0; i <= ant; i++)
  {
    fileSize = pullChunk(i);
    fWriteFromPacket = False;
    fMediaSession->fWritten += fileSize;
    fMediaSession->fPacketLoss = False;
    fMediaSession->fChunk = i;
  }
}

void DenseFileSink::pullAll()
{
  for (int i = 7; i < 14; i++)
  {
    pullChunk(i);
  }

  fclose(fOutFid);
  exit(0);
}

long DenseFileSink::pullChunk(unsigned short numb)
{
  std::string newpath;

  std::string start = "wget ";
  newpath.append(start);

  newpath.append("10.20.0.16"); // TODO: Do something about this?

  std::string manstart = "/first";
  newpath.append(manstart);

  std::string thechunk = std::to_string(numb);
  newpath.append(thechunk);

  std::string manend = ".ts";
  newpath.append(manend);

  std::string reqend = " --header \"Host: denseserver.com\"";
  newpath.append(reqend);

  std::string namestart = "first";
  namestart.append(thechunk);
  namestart.append(manend);

  system(newpath.c_str());

  FILE *file = fopen(namestart.c_str(), "rb");
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = new char[fileSize];

  fread(buffer, 1, fileSize, file);
  fseek(fMediaSession->fOut, 0, fMediaSession->fWritten);
  long written = fwrite(buffer, 1, fileSize, fMediaSession->fOut);

  fclose(file);

  delete[] buffer;
  return written;
}