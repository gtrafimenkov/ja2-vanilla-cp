#include "DefaultContentManagerUT.h"

#include "Build/GameRes.h"
#include "sgp/FileMan.h"

#include "src/DefaultContentManager.h"
#include "src/TestUtils.h"

/** Create DefaultContentManager for usage in unit testing. */
DefaultContentManager * createDefaultCMForTesting()
{
  std::string extraDataDir = GetExtraDataDir();
  std::string gameResRootPath = FileMan::joinPaths(extraDataDir, "_unittests");
  std::string externalizedDataPath = FileMan::joinPaths(extraDataDir, "externalized");

  DefaultContentManager *cm;

  cm = new DefaultContentManager(GV_ENGLISH,
                                 gameResRootPath, externalizedDataPath);

  // we don't load game resources
  // bacause we don't need them at the moment

  return cm;
}
