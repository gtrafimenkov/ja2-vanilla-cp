#include "TestUtils.h"

#include "src/DefaultContentManager.h"


SGPFile* OpenTestResourceForReading(const char *filePath)
{
  std::string extraDataDir = GetExtraDataDir();
  return FileMan::openForReading(FileMan::joinPaths(extraDataDir, filePath));
}

std::string GetExtraDataDir()
{
  return ".";
}
