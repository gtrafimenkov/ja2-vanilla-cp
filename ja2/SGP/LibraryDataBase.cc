// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/LibraryDataBase.h"

#include <cstdlib>
#include <stdexcept>
#include <string.h>

#include "Directories.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Exceptions.h"
#include "SGP/FileMan.h"
#include "SGP/Logger.h"
#include "SGP/MemMan.h"
#include "SGP/StrUtils.h"
#include "SGP/Types.h"

#define FILENAME_SIZE 256

#define FILE_OK 0x00
#define FILE_DELETED 0xFF
#define FILE_OLD 0x01
#define FILE_DOESNT_EXIST 0xFE

// NOTE:  The following structs are also used by the datalib98 utility
struct LIBHEADER {
  char sLibName[FILENAME_SIZE];
  char sPathToLibrary[FILENAME_SIZE];
  int32_t iEntries;
  int32_t iUsed;
  uint16_t iSort;
  uint16_t iVersion;
  BOOLEAN fContainsSubDirectories;
  int32_t iReserved;
};

struct DIRENTRY {
  char sFileName[FILENAME_SIZE];
  uint32_t uiOffset;
  uint32_t uiLength;
  uint8_t ubState;
  uint8_t ubReserved;
  SGP_FILETIME sFileTime;
  uint16_t usReserved2;
};

struct DatabaseManagerHeaderStruct {
  LibraryHeaderStruct *pLibraries;
  uint16_t usNumberOfLibraries;
};

static DatabaseManagerHeaderStruct gFileDataBase;

static BOOLEAN InitializeLibrary(const char *pLibraryName, LibraryHeaderStruct *pLibHeader);

static void ThrowExcOnLibLoadFailure(const char *pLibraryName) {
  std::string message = FormattedString("Library '%s' is not found in folder '%s'.", pLibraryName,
                                        FileMan::getDataDirPath().c_str());
  FastDebugMsg(message.c_str());
  throw LibraryFileNotFoundException(message);
}

void InitializeFileDatabase(const std::vector<std::string> &libraries) {
  // if all the libraries exist, set them up
  gFileDataBase.usNumberOfLibraries = libraries.size();

  // allocate memory for the each of the library headers
  if (libraries.size() > 0) {
    LibraryHeaderStruct *const libs = MALLOCNZ(LibraryHeaderStruct, libraries.size());
    gFileDataBase.pLibraries = libs;

    // Load up each library
    for (int i = 0; i < libraries.size(); i++) {
      if (!InitializeLibrary(libraries[i].c_str(), &libs[i])) {
        ThrowExcOnLibLoadFailure(libraries[i].c_str());
      }
    }
  }
}

static BOOLEAN CloseLibrary(int16_t sLibraryID);

void ShutDownFileDatabase() {
  uint16_t sLoop1;

  // Free up the memory used for each library
  for (sLoop1 = 0; sLoop1 < gFileDataBase.usNumberOfLibraries; sLoop1++) CloseLibrary(sLoop1);

  // Free up the memory used for all the library headers
  if (gFileDataBase.pLibraries) {
    MemFree(gFileDataBase.pLibraries);
    gFileDataBase.pLibraries = NULL;
  }
}

static int CompareFileHeader(const void *a, const void *b) {
  const FileHeaderStruct *fha = (const FileHeaderStruct *)a;
  const FileHeaderStruct *fhb = (const FileHeaderStruct *)b;
  return strcasecmp(fha->pFileName, fhb->pFileName);
}

// Replace all \ in a string by /
static char *Slashify(const char *s) {
  char *const res = MALLOCN(char, strlen(s) + 1);
  char *d = res;
  do {
    *d++ = (*s == '\\' ? '/' : *s);
  } while (*s++ != '\0');
  return res;
}

static BOOLEAN InitializeLibrary(const char *const lib_name, LibraryHeaderStruct *const lib) try {
  FILE *hFile = FileMan::openForReadingInDataDir(lib_name);
  if (hFile == NULL) {
    fprintf(stderr, "ERROR: Failed to open library \"%s\"\n", lib_name);
    return FALSE;
  }

  // read in the library header (at the begining of the library)
  LIBHEADER LibFileHeader;
  if (fread(&LibFileHeader, sizeof(LibFileHeader), 1, hFile) != 1) return FALSE;

  const uint32_t count_entries = LibFileHeader.iEntries;

  FileHeaderStruct *fhs = MALLOCN(FileHeaderStruct, count_entries);
#ifdef JA2TESTVERSION
  lib->uiTotalMemoryAllocatedForLibrary = sizeof(*fhs) * count_entries;
#endif

  /* place the file pointer at the begining of the file headers (they are at the
   * end of the file) */
  fseek(hFile, -(int)(count_entries * sizeof(DIRENTRY)), SEEK_END);

  uint32_t used_entries = 0;
  for (uint32_t uiLoop = 0; uiLoop < count_entries; ++uiLoop) {
    DIRENTRY DirEntry;
    if (fread(&DirEntry, sizeof(DirEntry), 1, hFile) != 1) return FALSE;

    if (DirEntry.ubState != FILE_OK) continue;

    // check to see if the file is not longer than it should be
    if (strlen(DirEntry.sFileName) + 1 >= FILENAME_SIZE) {
      FastDebugMsg(String(
          "\n*******InitializeLibrary():  Warning!:  '%s' from the library "
          "'%s' has name whose size (%d) is bigger then it should be (%s)",
          DirEntry.sFileName, lib->sLibraryPath, strlen(DirEntry.sFileName) + 1, FILENAME_SIZE));
    }

    FileHeaderStruct *const fh = &fhs[used_entries++];

    fh->pFileName = Slashify(DirEntry.sFileName);
#ifdef JA2TESTVERSION
    lib->uiTotalMemoryAllocatedForLibrary += strlen(fh->pFileName) + 1;
#endif

    fh->uiFileOffset = DirEntry.uiOffset;
    fh->uiFileLength = DirEntry.uiLength;
  }

  if (used_entries != count_entries) {
    fhs = REALLOC(fhs, FileHeaderStruct, used_entries);
  }

  qsort(fhs, used_entries, sizeof(*fhs), CompareFileHeader);

  lib->pFileHeader = fhs;
  lib->usNumberOfEntries = used_entries;

  lib->sLibraryPath = Slashify(LibFileHeader.sPathToLibrary);
#ifdef JA2TESTVERSION
  lib->uiTotalMemoryAllocatedForLibrary += strlen(lib->sLibraryPath) + 1;
#endif

  lib->hLibraryHandle = hFile;
  lib->iNumFilesOpen = 0;
  return TRUE;
} catch (...) {
  return 0;
}

BOOLEAN LoadDataFromLibrary(LibraryFile *const f, void *const pData, const uint32_t uiBytesToRead) {
  if (f->pFileHeader == NULL) return FALSE;

  uint32_t const uiOffsetInLibrary = f->pFileHeader->uiFileOffset;
  uint32_t const uiLength = f->pFileHeader->uiFileLength;
  FILE *const hLibraryFile = f->lib->hLibraryHandle;
  uint32_t const uiCurPos = f->uiFilePosInFile;

  fseek(hLibraryFile, uiOffsetInLibrary + uiCurPos, SEEK_SET);

  // if we are trying to read more data than the size of the file, return an
  // error
  if (uiBytesToRead + uiCurPos > uiLength) return FALSE;

  if (fread(pData, uiBytesToRead, 1, hLibraryFile) != 1) return FALSE;

  f->uiFilePosInFile += uiBytesToRead;
  return TRUE;
}

static const FileHeaderStruct *GetFileHeaderFromLibrary(const LibraryHeaderStruct *lib,
                                                        const char *filename);
static LibraryHeaderStruct *GetLibraryFromFileName(char const *filename);

bool CheckIfFileExistInLibrary(char const *const filename) {
  LibraryHeaderStruct const *const lib = GetLibraryFromFileName(filename);
  return lib && GetFileHeaderFromLibrary(lib, filename);
}

static BOOLEAN IsLibraryOpened(int16_t sLibraryID);

/* Find out if the file CAN be in a library.  It determines if the library that
 * the file MAY be in is open.  E.g. file is  Laptop/Test.sti, if the Laptop/
 * library is open, it returns true */
static LibraryHeaderStruct *GetLibraryFromFileName(char const *const filename) {
  // Loop through all the libraries to check which library the file is in
  LibraryHeaderStruct *best_match = 0;
  for (int16_t i = 0; i != gFileDataBase.usNumberOfLibraries; ++i) {
    if (!IsLibraryOpened(i)) continue;

    LibraryHeaderStruct *const lib = &gFileDataBase.pLibraries[i];
    char const *const lib_path = lib->sLibraryPath;
    if (lib_path[0] == '\0') {  // The library is for the default path
      if (strchr(filename, '/')) continue;
      // There is no directory in the file name
      return lib;
    } else {  // Compare the library name to the file name that is passed in
      size_t const lib_path_len = strlen(lib_path);
      if (strncasecmp(lib_path, filename, lib_path_len) != 0) continue;
      // The directory paths are the same to the length of the lib's path
      if (best_match && strlen(best_match->sLibraryPath) >= lib_path_len) continue;
      // We've never matched or this match's path is longer than the previous
      // match (meaning it's more exact)
      best_match = lib;
    }
  }
  return best_match;
}

static int CompareFileNames(const void *key, const void *member);

static const char *g_current_lib_path;

/* Performsperforms a binary search of the library.  It adds the libraries path
 * to the file in the library and then string compared that to the name that we
 * are searching for. */
static const FileHeaderStruct *GetFileHeaderFromLibrary(const LibraryHeaderStruct *const lib,
                                                        const char *const filename) {
  g_current_lib_path = lib->sLibraryPath;
  return (const FileHeaderStruct *)bsearch(filename, lib->pFileHeader, lib->usNumberOfEntries,
                                           sizeof(*lib->pFileHeader), CompareFileNames);
}

static int CompareFileNames(const void *key, const void *member) {
  const char *const sSearchKey = (const char *)key;
  const FileHeaderStruct *const TempFileHeader = (const FileHeaderStruct *)member;
  char sFileNameWithPath[FILENAME_SIZE];

  sprintf(sFileNameWithPath, "%s%s", g_current_lib_path, TempFileHeader->pFileName);

  return strcasecmp(sSearchKey, sFileNameWithPath);
}

/* This function will see if a file is in a library.  If it is, the file will be
 * opened and a file handle will be created for it. */
BOOLEAN OpenFileFromLibrary(const char *const pName, LibraryFile *const f) {
  // Check if the file can be contained from an open library ( the path to the
  // file a library path )
  LibraryHeaderStruct *const lib = GetLibraryFromFileName(pName);
  if (!lib) return FALSE;

  // if the file is in a library, get the file
  const FileHeaderStruct *const pFileHeader = GetFileHeaderFromLibrary(lib, pName);
  if (pFileHeader == NULL) return FALSE;

  // increment the number of open files
  lib->iNumFilesOpen++;

  f->lib = lib;
  f->pFileHeader = pFileHeader;
  return TRUE;
}

void CloseLibraryFile(LibraryFile *const f) { --f->lib->iNumFilesOpen; }

BOOLEAN LibraryFileSeek(LibraryFile *const f, int32_t distance, const FileSeekMode how) {
  uint32_t pos;
  switch (how) {
    case FILE_SEEK_FROM_START:
      pos = 0;
      break;
    case FILE_SEEK_FROM_END:
      pos = f->pFileHeader->uiFileLength;
      break;
    case FILE_SEEK_FROM_CURRENT:
      pos = f->uiFilePosInFile;
      break;
    default:
      return FALSE;
  }
  f->uiFilePosInFile = pos + distance;
  return TRUE;
}

static BOOLEAN CloseLibrary(int16_t sLibraryID) {
  uint32_t uiLoop1;

  // if the library isnt loaded, dont close it
  if (!IsLibraryOpened(sLibraryID)) return (FALSE);
  LibraryHeaderStruct *const lib = &gFileDataBase.pLibraries[sLibraryID];

#ifdef JA2TESTVERSION
  FastDebugMsg(
      String("ShutDownFileDatabase( ): %d bytes of ram used for the "
             "Library #%3d, path '%s',  in the File Database System\n",
             lib->uiTotalMemoryAllocatedForLibrary, sLibraryID, lib->sLibraryPath));
  lib->uiTotalMemoryAllocatedForLibrary = 0;
#endif

  // if there are any open files, loop through the library and close down
  // whatever file is still open
  if (lib->iNumFilesOpen) {
    FastDebugMsg(String("CloseLibrary():  ERROR:  %s library still has %d open files.",
                        lib->sLibraryPath, lib->iNumFilesOpen));
  }

  // Free up the memory used for each file name
  for (uiLoop1 = 0; uiLoop1 < lib->usNumberOfEntries; uiLoop1++) {
    MemFree(lib->pFileHeader[uiLoop1].pFileName);
    lib->pFileHeader[uiLoop1].pFileName = NULL;
  }

  // Free up the memory needed for the Library File Headers
  if (lib->pFileHeader) {
    MemFree(lib->pFileHeader);
    lib->pFileHeader = NULL;
  }

  // Free up the memory used for the library name
  if (lib->sLibraryPath) {
    MemFree(lib->sLibraryPath);
    lib->sLibraryPath = NULL;
  }

  fclose(lib->hLibraryHandle);
  lib->hLibraryHandle = NULL;

  return (TRUE);
}

static BOOLEAN IsLibraryOpened(int16_t const sLibraryID) {
  return sLibraryID < gFileDataBase.usNumberOfLibraries &&
         gFileDataBase.pLibraries[sLibraryID].hLibraryHandle != NULL;
}

static int CompareDirEntryFileNames(const void *key, const void *member);

#if 1  // XXX TODO UNIMPLEMENTED
#else
BOOLEAN GetLibraryFileTime(LibraryFile const *const f, SGP_FILETIME *const pLastWriteTime) try {
  LibraryHeaderStruct const *const lib = f->lib;
  if (!lib) return FALSE;

  LibFileHeader const *const file = lib->pFileHeader;
  if (!file) return FALSE;

  uint16_t usNumEntries = 0;
  uint32_t uiNumBytesRead;
  LIBHEADER LibFileHeader;
  BOOLEAN fDone = FALSE;
  //	uint32_t	cnt;
  int32_t iFilePos = 0;

  memset(pLastWriteTime, 0, sizeof(SGP_FILETIME));

  SetFilePointer(lib->hLibraryHandle, 0, NULL, FILE_BEGIN);

  // Read in the library header ( at the begining of the library )
  if (!ReadFile(lib->hLibraryHandle, &LibFileHeader, sizeof(LIBHEADER), &uiNumBytesRead, NULL))
    return (FALSE);
  if (uiNumBytesRead != sizeof(LIBHEADER)) {
    // Error Reading the file database header.
    return (FALSE);
  }

  DIRENTRY *const pAllEntries = MALLOCN(DIRENTRY, LibFileHeader.iEntries);
  memset(pAllEntries, 0, sizeof(DIRENTRY));

  iFilePos = -(LibFileHeader.iEntries * (int32_t)sizeof(DIRENTRY));

  // set the file pointer to the right location
  SetFilePointer(lib->hLibraryHandle, iFilePos, NULL, FILE_END);

  // Read in the library header ( at the begining of the library )
  if (!ReadFile(lib->hLibraryHandle, pAllEntries, sizeof(DIRENTRY) * LibFileHeader.iEntries,
                &uiNumBytesRead, NULL))
    return (FALSE);
  if (uiNumBytesRead != (sizeof(DIRENTRY) * LibFileHeader.iEntries)) {
    // Error Reading the file database header.
    return (FALSE);
  }

  DIRENTRY *pDirEntry = bsearch(file->pFileName, pAllEntries, LibFileHeader.iEntries,
                                sizeof(*pAllEntries), CompareDirEntryFileNames);

  if (pDirEntry == NULL) return FALSE;

  // Copy the dir entry time over to the passed in time
  *pLastWriteTime = pDirEntry->sFileTime;

  MemFree(pAllEntries);
  pAllEntries = NULL;

  return (TRUE);
} catch (...) {
  return FALSE;
}
#endif

static int CompareDirEntryFileNames(const void *key, const void *member) {
  const char *const sSearchKey = (const char *)key;
  const DIRENTRY *const TempDirEntry = (const DIRENTRY *)member;
  return strcasecmp(sSearchKey, TempDirEntry->sFileName);
}

#include "gtest/gtest.h"

TEST(LibraryDatabase, asserts) {
  EXPECT_EQ(sizeof(LIBHEADER), 532);
  EXPECT_EQ(sizeof(DIRENTRY), 280);
}
