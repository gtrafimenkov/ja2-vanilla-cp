#ifndef _LIBRARY_DATABASE_H
#define _LIBRARY_DATABASE_H

#include <string>
#include <vector>

#include "SGP/FileMan.h"
#include "SGP/Types.h"

#define REAL_FILE_LIBRARY_ID 1022

#define DB_BITS_FOR_LIBRARY 10
#define DB_BITS_FOR_FILE_ID 22

#define DB_EXTRACT_LIBRARY(exp) (exp >> DB_BITS_FOR_FILE_ID)
#define DB_EXTRACT_FILE_ID(exp) (exp & 0x3FFFFF)

#define DB_ADD_LIBRARY_ID(exp) (exp << DB_BITS_FOR_FILE_ID)
#define DB_ADD_FILE_ID(exp) (exp & 0xC00000)

struct FileHeaderStruct {
  char *pFileName;
  uint32_t uiFileLength;
  uint32_t uiFileOffset;
};

struct LibraryHeaderStruct {
  char *sLibraryPath;
  FILE *hLibraryHandle;
  uint16_t usNumberOfEntries;
  int32_t iNumFilesOpen;
  FileHeaderStruct *pFileHeader;

  //
  //	Temp:	Total memory used for each library ( all memory allocated
  //
#ifdef JA2TESTVERSION
  uint32_t uiTotalMemoryAllocatedForLibrary;
#endif
};

struct LibraryFile {
  uint32_t uiFilePosInFile;  // current position in the file
  LibraryHeaderStruct *lib;
  const FileHeaderStruct *pFileHeader;
};

void InitializeFileDatabase(const std::vector<std::string> &libraries);
void ShutDownFileDatabase();
bool CheckIfFileExistInLibrary(char const *filename);

BOOLEAN OpenFileFromLibrary(const char *filename, LibraryFile *);
/* Close an individual file that is contained in the library */
void CloseLibraryFile(LibraryFile *);
BOOLEAN LoadDataFromLibrary(LibraryFile *, void *pData, uint32_t uiBytesToRead);
BOOLEAN LibraryFileSeek(LibraryFile *, int32_t distance, FileSeekMode);

#if 0
BOOLEAN GetLibraryFileTime(LibraryFile const*, SGP_FILETIME* pLastWriteTime);
#endif

#endif
