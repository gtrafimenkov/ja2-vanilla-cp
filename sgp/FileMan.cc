#include <algorithm>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Directories.h"
#include "sgp/FileMan.h"
#include "sgp/LibraryDataBase.h"
#include "sgp/MemMan.h"
#include "sgp/PODObj.h"
#include "sgp/Logger.h"

#if _WIN32
#include <shlobj.h>
#else
#include <pwd.h>
#endif

#include "sgp/PlatformIO.h"

#if CASE_SENSITIVE_FS
#include <dirent.h>
#endif

#define BASEDATADIR    "data"
#define LOCAL_CURRENT_DIR "tmp"

enum SGPFileFlags
{
	SGPFILE_NONE = 0U,
	SGPFILE_REAL = 1U << 0
};

struct SGPFile
{
	SGPFileFlags flags;
	union
	{
		FILE*       file;
		LibraryFile lib;
	} u;
};


enum FileOpenFlags
{
	FILE_ACCESS_READ      = 1U << 0,
	FILE_ACCESS_WRITE     = 1U << 1,
	FILE_ACCESS_READWRITE = FILE_ACCESS_READ | FILE_ACCESS_WRITE,
	FILE_ACCESS_APPEND    = 1U << 2
};
ENUM_BITSET(FileOpenFlags)


static std::string s_dataDir;
static std::string s_tileDir;
static std::string s_mapsDir;


static void findDataDirs(const char *exeFolder);
static void SetFileManCurrentDirectory(char const* const pcDirectory);

/** Convert file descriptor to HWFile.
 * Raise runtime_error if not possible. */
static HWFILE getSGPFileFromFD(int fd, const char *filename, const char *fmode);

#if CASE_SENSITIVE_FS
/**
 * Find an object (file or subdirectory) in the given directory in case-independent manner.
 * @return true when found, return the found name using foundName. */
static bool findObjectCaseInsensitive(const char *directory, const char *name, bool lookForFiles, bool lookForSubdirs, std::string &foundName);
#endif

/** Get file open modes from our enumeration.
 * Abort program if conversion is not found.
 * @return file mode for fopen call and posix mode using parameter 'posixMode' */
static const char* GetFileOpenModes(FileOpenFlags flags, int *posixMode);

static std::string s_exeFolderPath;

void InitializeFileManager(const char *exeFolder)
{
  s_exeFolderPath = std::filesystem::canonical(exeFolder).string();

  // Create another directory and set is as the current directory for the process
  // Temporary files will be created in this directory.
  // ----------------------------------------------------------------------------

  std::string tmpPath = FileMan::joinPaths(s_exeFolderPath, LOCAL_CURRENT_DIR);
	if (mkdir(tmpPath.c_str(), 0700) != 0 && errno != EEXIST)
	{
    LOG_ERROR("Unable to create tmp directory '%s'\n", tmpPath.c_str());
		throw std::runtime_error("Unable to create tmp directory");
	}
  else
  {
    SetFileManCurrentDirectory(tmpPath.c_str());
  }

  // Get directory with JA2 resources
  // --------------------------------------------

  findDataDirs(s_exeFolderPath.c_str());

  LOG_INFO("Data directory:                '%s'\n", s_dataDir.c_str());
  LOG_INFO("Tilecache directory:           '%s'\n", s_tileDir.c_str());
  LOG_INFO("------------------------------------------------------------------------------\n");
}


// TODO: need better name?
bool FileExists(char const* const filename)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		char path[512];
		snprintf(path, lengthof(path), "%s/%s", FileMan::getDataDirPath().c_str(), filename);
		file = fopen(path, "rb");
		if (!file) return CheckIfFileExistInLibrary(filename);
	}

	fclose(file);
	return true;
}

/**
 * Open file in the Data directory.
 *
 * Return file descriptor or -1 if file is not found. */
static int OpenFileInDataDirFD(const char *filename, int mode)
{
  std::string path = FileMan::joinPaths(FileMan::getDataDirPath(), filename);
  int d = open(path.c_str(), mode);
  if (d < 0)
  {
#if CASE_SENSITIVE_FS
    // on case-sensitive file system need to try to find another name
    std::string newFileName;
    if(findObjectCaseInsensitive(FileMan::getDataDirPath().c_str(), filename, true, false, newFileName))
    {
      path = FileMan::joinPaths(FileMan::getDataDirPath(), newFileName);
      d = open(path.c_str(), mode);
    }
#endif
  }
  return d;
}

void FileDelete(char const* const path)
{
	if (unlink(path) == 0) return;

	switch (errno)
	{
		case ENOENT: return;

#ifdef _WIN32
		/* On WIN32 read-only files cannot be deleted, so try to make the file
		 * writable and unlink() again */
		case EACCES:
			if ((chmod(path, S_IREAD | S_IWRITE) == 0 && unlink(path) == 0) ||
					errno == ENOENT)
			{
				return;
			}
			break;
#endif

		default: break;
	}

	throw std::runtime_error("Deleting file failed");
}


/** Get file open modes from our enumeration.
 * Abort program if conversion is not found.
 * @return file mode for fopen call and posix mode using parameter 'posixMode' */
static const char* GetFileOpenModes(FileOpenFlags flags, int *posixMode)
{
  const char *cMode = NULL;

#ifndef _WIN32
  *posixMode = 0;
#else
	*posixMode = O_BINARY;
#endif

	switch (flags & (FILE_ACCESS_READWRITE | FILE_ACCESS_APPEND))
	{
		case FILE_ACCESS_READ:      cMode = "rb";  *posixMode |= O_RDONLY;            break;
		case FILE_ACCESS_WRITE:     cMode = "wb";  *posixMode |= O_WRONLY;            break;
		case FILE_ACCESS_READWRITE: cMode = "r+b"; *posixMode |= O_RDWR;              break;
		case FILE_ACCESS_APPEND:    cMode = "ab";  *posixMode |= O_WRONLY | O_APPEND; break;

		default: abort();
	}
  return cMode;
}


/** Open file for reading only.
 * When using the smart lookup:
 *  - first try to open file normally.
 *    It will work if the path is absolute and the file is found or path is relative to the current directory
 *    and file is present;
 *  - if file is not found, try to find the file relatively to 'Data' directory;
 *  - if file is not found, try to find the file in libraries located in 'Data' directory; */
HWFILE FileMan::openForReadingSmart(const char* filename, bool useSmartLookup)
{
  int         mode;
  const char* fmode = GetFileOpenModes(FILE_ACCESS_READ, &mode);

  int d;

  {
    d = open(filename, mode);
    if ((d < 0) && useSmartLookup)
    {
      // failed to open file in the local directory
      // let's try Data
      d = OpenFileInDataDirFD(filename, mode);
      if (d < 0)
      {
        LibraryFile libFile;
        memset(&libFile, 0, sizeof(libFile));

        // failed to open in the data dir
        // let's try libraries
        if (OpenFileFromLibrary(filename, &libFile))
        {
#if DEBUG_PRINT_OPENING_FILES
          LOG_INFO("Opened file (from library ): %s\n", filename);
#endif
          SGPFile *file = MALLOCZ(SGPFile);
          file->flags = SGPFILE_NONE;
          file->u.lib = libFile;
          return file;
        }
      }
      else
      {
#if DEBUG_PRINT_OPENING_FILES
        LOG_INFO("Opened file (from data dir): %s\n", filename);
#endif
      }
    }
    else
    {
#if DEBUG_PRINT_OPENING_FILES
      LOG_INFO("Opened file (current dir  ): %s\n", filename);
#endif
    }
  }

  return getSGPFileFromFD(d, filename, fmode);
}


void FileClose(const HWFILE f)
{
	if (f->flags & SGPFILE_REAL)
	{
		fclose(f->u.file);
	}
	else
	{
		CloseLibraryFile(&f->u.lib);
	}
	MemFree(f);
}


#ifdef JA2TESTVERSION
#	include "Timer_Control.h"
extern UINT32 uiTotalFileReadTime;
extern UINT32 uiTotalFileReadCalls;
#endif

void FileRead(HWFILE const f, void* const pDest, size_t const uiBytesToRead)
{
	BOOLEAN ret;
	if (f->flags & SGPFILE_REAL)
	{
		ret = fread(pDest, uiBytesToRead, 1, f->u.file) == 1;
	}
	else
	{
		ret = LoadDataFromLibrary(&f->u.lib, pDest, (UINT32)uiBytesToRead);
	}

	if (!ret) throw std::runtime_error("Reading from file failed");
}


void FileWrite(HWFILE const f, void const* const pDest, size_t const uiBytesToWrite)
{
	if (!(f->flags & SGPFILE_REAL)) throw std::logic_error("Tried to write to library file");
	if (fwrite(pDest, uiBytesToWrite, 1, f->u.file) != 1) throw std::runtime_error("Writing to file failed");
}


void FileSeek(HWFILE const f, INT32 distance, FileSeekMode const how)
{
	bool success;
	if (f->flags & SGPFILE_REAL)
	{
		int whence;
		switch (how)
		{
			case FILE_SEEK_FROM_START: whence = SEEK_SET; break;
			case FILE_SEEK_FROM_END:   whence = SEEK_END; break;
			default:                   whence = SEEK_CUR; break;
		}

		success = fseek(f->u.file, distance, whence) == 0;
	}
	else
	{
		success = LibraryFileSeek(&f->u.lib, distance, how);
	}
	if (!success) throw std::runtime_error("Seek in file failed");
}


INT32 FileGetPos(const HWFILE f)
{
	return f->flags & SGPFILE_REAL ? (INT32)ftell(f->u.file) : f->u.lib.uiFilePosInFile;
}


UINT32 FileGetSize(const HWFILE f)
{
	if (f->flags & SGPFILE_REAL)
	{
		struct stat sb;
		if (fstat(fileno(f->u.file), &sb) != 0)
		{
			throw std::runtime_error("Getting file size failed");
		}
		return (UINT32)sb.st_size;
	}
	else
	{
		return f->u.lib.pFileHeader->uiFileLength;
	}
}


static void SetFileManCurrentDirectory(char const* const pcDirectory)
{
#if 1 // XXX TODO
	if (chdir(pcDirectory) != 0)
#else
	if (!SetCurrentDirectory(pcDirectory))
#endif
	{
		throw std::runtime_error("Changing directory failed");
	}
}


void FileMan::createDir(char const* const path)
{
	if (mkdir(path, 0755) == 0) return;

	if (errno == EEXIST)
	{
		FileAttributes const attr = FileGetAttributes(path);
		if (attr != FILE_ATTR_ERROR && attr & FILE_ATTR_DIRECTORY) return;
	}

	throw std::runtime_error("Failed to create directory");
}


void EraseDirectory(char const* const dirPath)
{
  std::vector<std::string> paths = FindAllFilesInDir(dirPath);
  for (std::vector<std::string>::const_iterator it(paths.begin()); it != paths.end(); ++it)
  {
		try
		{
			FileDelete(it->c_str());
		}
		catch (...)
		{
			const FileAttributes attr = FileGetAttributes(it->c_str());
			if (attr != FILE_ATTR_ERROR && attr & FILE_ATTR_DIRECTORY) continue;
			throw;
		}
	}
}


const std::string& FileMan::getExeFolderPath()
{
	return s_exeFolderPath;
}

FileAttributes FileGetAttributes(const char* const filename)
{
	FileAttributes attr = FILE_ATTR_NONE;
#ifndef _WIN32 // XXX TODO
	struct stat sb;
	if (stat(filename, &sb) != 0) return FILE_ATTR_ERROR;

	if (S_ISDIR(sb.st_mode))     attr |= FILE_ATTR_DIRECTORY;
	if (!(sb.st_mode & S_IWUSR)) attr |= FILE_ATTR_READONLY;
#else
	const UINT32 w32attr = GetFileAttributes(filename);
	if (w32attr == INVALID_FILE_ATTRIBUTES) return FILE_ATTR_ERROR;

	if (w32attr & FILE_ATTRIBUTE_READONLY)  attr |= FILE_ATTR_READONLY;
	if (w32attr & FILE_ATTRIBUTE_DIRECTORY) attr |= FILE_ATTR_DIRECTORY;
#endif
	return attr;
}


BOOLEAN FileClearAttributes(const char* const filename)
{
#if 1 // XXX TODO
#	if defined WITH_FIXMES
	fprintf(stderr, "===> %s:%d: IGNORING %s(\"%s\")\n", __FILE__, __LINE__, __func__, filename);
#	endif
	return FALSE;
	UNIMPLEMENTED
#else
	return SetFileAttributes(filename, FILE_ATTRIBUTE_NORMAL);
#endif
}


BOOLEAN GetFileManFileTime(const HWFILE f, SGP_FILETIME* const pCreationTime, SGP_FILETIME* const pLastAccessedTime, SGP_FILETIME* const pLastWriteTime)
{
#if 1 // XXX TODO
	UNIMPLEMENTED;
  return FALSE;
#else
	//Initialize the passed in variables
	memset(pCreationTime,     0, sizeof(*pCreationTime));
	memset(pLastAccessedTime, 0, sizeof(*pLastAccessedTime));
	memset(pLastWriteTime,    0, sizeof(*pLastWriteTime));

	if (f->flags & SGPFILE_REAL)
	{
		const HANDLE hRealFile = f->u.file;

		//Gets the UTC file time for the 'real' file
		SGP_FILETIME sCreationUtcFileTime;
		SGP_FILETIME sLastAccessedUtcFileTime;
		SGP_FILETIME sLastWriteUtcFileTime;
		GetFileTime(hRealFile, &sCreationUtcFileTime, &sLastAccessedUtcFileTime, &sLastWriteUtcFileTime);

		//converts the creation UTC file time to the current time used for the file
		FileTimeToLocalFileTime(&sCreationUtcFileTime, pCreationTime);

		//converts the accessed UTC file time to the current time used for the file
		FileTimeToLocalFileTime(&sLastAccessedUtcFileTime, pLastAccessedTime);

		//converts the write UTC file time to the current time used for the file
		FileTimeToLocalFileTime(&sLastWriteUtcFileTime, pLastWriteTime);
		return TRUE;
	}
	else
	{
		return GetLibraryFileTime(&f->u.lib, pLastWriteTime);
	}
#endif
}


INT32	CompareSGPFileTimes(const SGP_FILETIME* const pFirstFileTime, const SGP_FILETIME* const pSecondFileTime)
{
#if 1 // XXX TODO
	UNIMPLEMENTED;
  return 0;
#else
	return CompareFileTime(pFirstFileTime, pSecondFileTime);
#endif
}


FILE* GetRealFileHandleFromFileManFileHandle(const HWFILE f)
{
	return f->flags & SGPFILE_REAL ? f->u.file : f->u.lib.lib->hLibraryHandle;
}


static UINT32 GetFreeSpaceOnHardDrive(const char* pzDriveLetter);


UINT32 GetFreeSpaceOnHardDriveWhereGameIsRunningFrom(void)
{
#if 1 // XXX TODO
	FIXME
	return 1024 * 1024 * 1024; // XXX TODO return an arbitrary number for now
#else
	//get the drive letter from the exec dir
  STRING512 zDrive;
	_splitpath(GetExecutableDirectory(), zDrive, NULL, NULL, NULL);

	sprintf(zDrive, "%s\\", zDrive);
	return GetFreeSpaceOnHardDrive(zDrive);
#endif
}


static UINT32 GetFreeSpaceOnHardDrive(const char* const pzDriveLetter)
{
#if 1 // XXX TODO
	UNIMPLEMENTED
#else
	UINT32 uiSectorsPerCluster     = 0;
	UINT32 uiBytesPerSector        = 0;
	UINT32 uiNumberOfFreeClusters  = 0;
	UINT32 uiTotalNumberOfClusters = 0;
	if (!GetDiskFreeSpace(pzDriveLetter, &uiSectorsPerCluster, &uiBytesPerSector, &uiNumberOfFreeClusters, &uiTotalNumberOfClusters))
	{
		const UINT32 uiLastError = GetLastError();
		char zString[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, uiLastError, 0, zString, 1024, NULL);
		return TRUE;
	}

	return uiBytesPerSector * uiNumberOfFreeClusters * uiSectorsPerCluster;
#endif
}


/** Join two path components. */
std::string FileMan::joinPaths(const std::string &first, const char *second)
{
  std::string result = first;
  if((result.length() == 0) || (result[result.length()-1] != PATH_SEPARATOR))
  {
    if(second[0] != PATH_SEPARATOR)
    {
      result += PATH_SEPARATOR;
    }
  }
  result += second;
  return result;
}

/** Join two path components. */
std::string FileMan::joinPaths(const std::string &first, const std::string &second)
{
  return joinPaths(first, second.c_str());
}

/** Join two path components. */
std::string FileMan::joinPaths(const char *first, const char *second)
{
  return joinPaths(std::string(first), second);
}

#if CASE_SENSITIVE_FS

/**
 * Find an object (file or subdirectory) in the given directory in case-independent manner.
 * @return true when found, return the found name using foundName. */
static bool findObjectCaseInsensitive(const char *directory, const char *name, bool lookForFiles, bool lookForSubdirs, std::string &foundName)
{
  bool result = false;

  // if name contains directories, than we have to find actual case-sensitive name of the directory
  // and only then look for a file
  const char *splitter = strstr(name, "/");
  int dirNameLen = (int)(splitter - name);
  if(splitter && (dirNameLen > 0) && splitter[1] != 0)
  {
    // we have directory in the name
    // let's find its correct name first
    char newDirectory[128];
    std::string actualSubdirName;
    strncpy(newDirectory, name, sizeof(newDirectory));
    newDirectory[dirNameLen] = 0;

    if(findObjectCaseInsensitive(directory, newDirectory, false, true, actualSubdirName))
    {
      // found subdirectory; let's continue the full search
      std::string pathInSubdir;
      std::string newDirectory = FileMan::joinPaths(directory, actualSubdirName.c_str());
      if(findObjectCaseInsensitive(newDirectory.c_str(), splitter + 1, lookForFiles, lookForSubdirs, pathInSubdir))
      {
        // found name in subdir
        foundName = FileMan::joinPaths(actualSubdirName, pathInSubdir);
        result = true;
      }
    }
  }
  else
  {
    // name contains only file, no directories
    DIR *d;
    struct dirent *entry;
    uint8_t objectTypes = (lookForFiles ? DT_REG : 0) | (lookForSubdirs ? DT_DIR : 0);

    d = opendir(directory);
    if (d)
    {
      while ((entry = readdir(d)) != NULL)
      {
        if((entry->d_type & objectTypes)
           && !strcasecmp(name, entry->d_name))
        {
          foundName = entry->d_name;
          result = true;
        }
      }
      closedir(d);
    }
  }

  // LOG_INFO("XXXXX Looking for %s/[ %s ] : %s\n", directory, name, result ? "success" : "failure");
  return result;
}
#endif


/**
 * Find actual paths to directories 'Data' and 'Data/Tilecache', 'Data/Maps'
 * On case-sensitive filesystems that might be tricky: if such directories
 * exist we should use them.  If doesn't exist, then use lowercased names.
 */
static void findDataDirs(const char *exeFolder)
{
    s_dataDir = FileMan::joinPaths(exeFolder, BASEDATADIR);
    s_tileDir = FileMan::joinPaths(s_dataDir, TILECACHEDIR);
    s_mapsDir = FileMan::joinPaths(s_dataDir, MAPSDIR);

#if CASE_SENSITIVE_FS

    // need to find precise names of the directories

    std::string name;
    if(findObjectCaseInsensitive(exeFolder, BASEDATADIR, false, true, name))
    {
      s_dataDir = FileMan::joinPaths(exeFolder, name);
    }

    if(findObjectCaseInsensitive(s_dataDir.c_str(), TILECACHEDIR, false, true, name))
    {
      s_tileDir = FileMan::joinPaths(s_dataDir, name);
    }

    if(findObjectCaseInsensitive(s_dataDir.c_str(), MAPSDIR, false, true, name))
    {
      s_mapsDir = FileMan::joinPaths(s_dataDir, name);
    }
#endif
}

/** Get path to the 'Data' directory of the game. */
const std::string& FileMan::getDataDirPath()
{
  return s_dataDir;
}

/** Get path to the 'Data/Tilecache' directory of the game. */
const std::string& FileMan::getTilecacheDirPath()
{
  return s_tileDir;
}


/** Get path to the 'Data/Maps' directory of the game. */
const std::string& FileMan::getMapsDirPath()
{
  return s_mapsDir;
}

/** Convert file descriptor to HWFile.
 * Raise runtime_error if not possible. */
static HWFILE getSGPFileFromFD(int fd, const char *filename, const char *fmode)
{
	if (fd < 0)
  {
    char buf[128];
    snprintf(buf, sizeof(buf), "Opening file '%s' failed", filename);
    throw std::runtime_error(buf);
  }

	FILE* const f = fdopen(fd, fmode);
	if (!f)
	{
    char buf[128];
    snprintf(buf, sizeof(buf), "Opening file '%s' failed", filename);
    throw std::runtime_error(buf);
	}

  SGPFile *file = MALLOCZ(SGPFile);
	file->flags  = SGPFILE_REAL;
	file->u.file = f;
	return file;
}


/** Open file for writing.
 * If file is missing it will be created.
 * If file exists, it's content will be removed. */
HWFILE FileMan::openForWriting(const char *filename)
{
	int mode;
	const char* fmode = GetFileOpenModes(FILE_ACCESS_WRITE, &mode);

  if(truncate)
  {
    mode |= O_TRUNC;
  }

	int d = open3(filename, mode | O_CREAT, 0600);
  return getSGPFileFromFD(d, filename, fmode);
}


/** Open file for appending data.
 * If file doesn't exist, it will be created. */
HWFILE FileMan::openForAppend(const char *filename)
{
	int         mode;
	const char* fmode = GetFileOpenModes(FILE_ACCESS_APPEND, &mode);

  int d = open3(filename, mode | O_CREAT, 0600);
  return getSGPFileFromFD(d, filename, fmode);
}


/** Open file for reading and writing.
 * If file doesn't exist, it will be created. */
HWFILE FileMan::openForReadWrite(const char *filename)
{
	int         mode;
	const char* fmode = GetFileOpenModes(FILE_ACCESS_READWRITE, &mode);

  int d = open3(filename, mode | O_CREAT, 0600);
  return getSGPFileFromFD(d, filename, fmode);
}


/** Open file in the 'Data' directory in case-insensitive manner. */
FILE* FileMan::openForReadingInDataDir(const char *filename)
{
	int mode;
	const char* fmode = GetFileOpenModes(FILE_ACCESS_READ, &mode);

  int d = OpenFileInDataDirFD(filename, mode);
  if(d >= 0)
  {
    FILE* hFile = fdopen(d, fmode);
    if (hFile == NULL)
    {
      close(d);
    }
    else
    {
      return hFile;
    }
  }
  return NULL;
}

/**
 * Find all files with the given extension in the given directory.
 * @param dirPath Path to the directory
 * @param extension Extension with dot (e.g. ".txt")
 * @param caseIncensitive When True, do case-insensitive search even of case-sensitive file-systems. * * @return List of paths (dir + filename). */
std::vector<std::string>
FindFilesInDir(const std::string &dirPath,
               const std::string &ext,
               bool caseIncensitive,
               bool returnOnlyNames,
               bool sortResults)
{
  std::string ext_copy = ext;
  if(caseIncensitive)
  {
    std::transform(ext_copy.begin(), ext_copy.end(), ext_copy.begin(), ::tolower);
  }

  std::vector<std::string> paths;
  for (const auto & entry : std::filesystem::directory_iterator(dirPath))
  {
    if(std::filesystem::is_regular_file(entry))
    {
      auto p = entry.path();
      std::string file_ext = p.extension().string();
      if(caseIncensitive)
      {
        std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
      }
      if(file_ext.compare(ext_copy) == 0)
      {
        if(returnOnlyNames)
        {
          paths.push_back(p.filename().string());
        }
        else
        {
          paths.push_back(p.string());
        }
      }
    }
  }
  if(sortResults)
  {
    std::sort(paths.begin(), paths.end());
  }
  return paths;
}

/**
 * Find all files in a directory.
 * @return List of paths (dir + filename). */
std::vector<std::string>
FindAllFilesInDir(const std::string &dirPath, bool sortResults)
{
  std::vector<std::string> paths;
  for (const auto & entry : std::filesystem::directory_iterator(dirPath)) {
    if(std::filesystem::is_regular_file(entry)) {
      paths.push_back(entry.path().string());
    }
  }
  if(sortResults)
  {
    std::sort(paths.begin(), paths.end());
  }
  return paths;
}

/** Get parent path (e.g. directory path from the full path). */
std::string FileMan::getParentPath(const std::string &_path, bool absolute)
{
  std::filesystem::path p(_path);
  return p.parent_path().string();
}
