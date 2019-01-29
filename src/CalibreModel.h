#pragma once

#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include "sgp/StrUtils.h"

class JsonObject;
class JsonObjectReader;

#define NOAMMO (0)

struct CalibreModel
{
  CalibreModel(uint16_t index,
               const char* burstSoundString,
               bool showInHelpText,
               bool monsterWeapon,
               int silencerSound
    );

  const wchar_t* getName() const;

  virtual void serializeTo(JsonObject &obj) const;
  static CalibreModel* deserialize(JsonObjectReader &obj);

  static const CalibreModel* getNoCalibreObject();

  uint16_t index;
  std::string burstSoundString;
  bool showInHelpText;
  bool monsterWeapon;
  int silencerSound;
};

const CalibreModel* getCalibre(uint16_t index, const std::vector<const CalibreModel*> &calibreMap);
