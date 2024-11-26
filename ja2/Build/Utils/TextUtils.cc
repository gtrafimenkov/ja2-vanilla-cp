#include "Utils/TextUtils.h"

#include "Directories.h"
#include "GameSettings.h"
#include "SGP/FileMan.h"
#include "Utils/EncryptedFile.h"
#include "Utils/Text.h"

#define ITEMSTRINGFILENAME BINARYDATADIR "/itemdesc.edt"

void LoadItemInfo(uint16_t const ubIndex, wchar_t Info[]) {
  uint32_t Seek = (SIZE_SHORT_ITEM_NAME + SIZE_ITEM_NAME + SIZE_ITEM_INFO) * ubIndex;
  LoadEncryptedDataFromFile(ITEMSTRINGFILENAME, Info, Seek + SIZE_ITEM_NAME + SIZE_SHORT_ITEM_NAME,
                            SIZE_ITEM_INFO);
}

static void LoadAllItemNames() {
  AutoSGPFile File(FileMan::openForReadingSmart(ITEMSTRINGFILENAME, true));
  for (uint32_t i = 0; i < MAXITEMS; i++) {
    uint32_t Seek = (SIZE_SHORT_ITEM_NAME + SIZE_ITEM_NAME + SIZE_ITEM_INFO) * i;
    LoadEncryptedData(File, ShortItemNames[i], Seek, SIZE_SHORT_ITEM_NAME);
    LoadEncryptedData(File, ItemNames[i], Seek + SIZE_SHORT_ITEM_NAME, SIZE_ITEM_NAME);
  }
}

void LoadAllExternalText() { LoadAllItemNames(); }

const wchar_t *GetWeightUnitString() {
  if (gGameSettings.fOptions[TOPTION_USE_METRIC_SYSTEM])  // metric
  {
    return (pMessageStrings[MSG_KILOGRAM_ABBREVIATION]);
  } else {
    return (pMessageStrings[MSG_POUND_ABBREVIATION]);
  }
}

float GetWeightBasedOnMetricOption(uint32_t uiObjectWeight) {
  float fWeight = 0.0f;

  // if the user is smart and wants things displayed in 'metric'
  if (gGameSettings.fOptions[TOPTION_USE_METRIC_SYSTEM])  // metric
  {
    fWeight = (float)uiObjectWeight;
  }

  // else the user is a caveman and display it in pounds
  else {
    fWeight = uiObjectWeight * 2.2f;
  }

  return (fWeight);
}
