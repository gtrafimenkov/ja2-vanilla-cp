#include "src/CalibreModel.h"

#include <algorithm>
#include <stdexcept>

#include "Build/Utils/Text.h"
#include "sgp/StrUtils.h"
#include "sgp/UTF8String.h"

#include "src/JsonObject.h"

#include "src/ContentManager.h"
#include "src/GameInstance.h"

CalibreModel::CalibreModel(uint16_t index_,
                           const char* burstSoundString_,
                           bool showInHelpText_,
                           bool monsterWeapon_,
                           int silencerSound_
  )
  :index(index_), burstSoundString(burstSoundString_),
   showInHelpText(showInHelpText_),
   monsterWeapon(monsterWeapon_),
   silencerSound(silencerSound_)
{
}

void CalibreModel::serializeTo(JsonObject &obj) const
{
  obj.AddMember("index",                index);
  obj.AddMember("burstSoundString",     burstSoundString.c_str());
  obj.AddMember("showInHelpText",       showInHelpText);
  obj.AddMember("monsterWeapon",        monsterWeapon);
  obj.AddMember("silencerSound",        silencerSound);
}

CalibreModel* CalibreModel::deserialize(JsonObjectReader &obj)
{
  int index = obj.GetInt("index");
  const char *burstSoundString = obj.GetString("burstSoundString");
  return new CalibreModel(index, burstSoundString,
                          obj.GetBool("showInHelpText"),
                          obj.GetBool("monsterWeapon"),
                          obj.GetInt("silencerSound")
    );
}

const wchar_t* CalibreModel::getName() const
{
  return &GCM->getCalibreName(index)->getWCHAR()[0];
}

const CalibreModel* CalibreModel::getNoCalibreObject()
{
  static CalibreModel noCalibre(NOAMMO, "", false, false, -1);
  return &noCalibre;
}
