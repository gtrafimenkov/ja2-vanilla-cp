#include "Build/Directories.h"
#include "Build/Utils/Text.h"
#include "Build/Utils/Text_Utils.h"
#include "sgp/FileMan.h"
#include "Build/GameSettings.h"

#include "Build/GameRes.h"
#include "src/ContentManager.h"
#include "src/GameInstance.h"
#include "sgp/UTF8String.h"

#define ITEMSTRINGFILENAME BINARYDATADIR "/itemdesc.edt"

static void LoadAllItemNames(void)
{
	AutoSGPFile File(GCM->openGameResForReading(ITEMSTRINGFILENAME));
	for (UINT32 i = 0; i < MAXITEMS; i++)
	{
		UINT32 Seek = (SIZE_SHORT_ITEM_NAME + SIZE_ITEM_NAME + SIZE_ITEM_INFO) * i;

    const ItemModel* im = GCM->getItem(i);

    // don't replace already loaded values
    if(im->nameOverride.empty())
    {
      GCM->loadEncryptedString(File, ShortItemNames[i], Seek, SIZE_SHORT_ITEM_NAME);
    }
    if(im->shortNameOverride.empty())
    {
      GCM->loadEncryptedString(File, ItemNames[i], Seek + SIZE_SHORT_ITEM_NAME, SIZE_ITEM_NAME);
    }
  }
}


void LoadAllExternalText( void )
{
	LoadAllItemNames();
}

const wchar_t* GetWeightUnitString( void )
{
	if ( gGameSettings.fOptions[ TOPTION_USE_METRIC_SYSTEM ] ) // metric
	{
		return( pMessageStrings[ MSG_KILOGRAM_ABBREVIATION ] );
	}
	else
	{
		return( pMessageStrings[ MSG_POUND_ABBREVIATION ] );
	}
}

FLOAT GetWeightBasedOnMetricOption( UINT32 uiObjectWeight )
{
	FLOAT fWeight = 0.0f;

	//if the user is smart and wants things displayed in 'metric'
	if ( gGameSettings.fOptions[ TOPTION_USE_METRIC_SYSTEM ] ) // metric
	{
		fWeight = (FLOAT)uiObjectWeight;
	}

	//else the user is a caveman and display it in pounds
	else
	{
		fWeight = uiObjectWeight * 2.2f;
	}

	return( fWeight );
}

#define BOBBYRDESCFILE BINARYDATADIR "/braydesc.edt"

void LoadBobbyRayItemName(uint16_t index, wchar_t *buf, int bufSize)
{
  const ItemModel* im = GCM->getItem(index);
  if(im->bobbyRayNameOverride.empty())
  {
    int startLoc = BOBBYR_ITEM_DESC_FILE_SIZE * index;
    GCM->loadEncryptedString(BOBBYRDESCFILE, buf, startLoc, BOBBYR_ITEM_DESC_NAME_SIZE);
  }
  else
  {
    wcsncpy(buf, &UTF8String(im->bobbyRayNameOverride.c_str()).getWCHAR()[0], BOBBYR_ITEM_DESC_NAME_SIZE);
  }
}

void LoadBobbyRayItemDescription(uint16_t index, wchar_t *buf, int bufSize)
{
  const ItemModel* im = GCM->getItem(index);
  if(im->bobbyRayDescriptionOverride.empty())
  {
    int startLoc = BOBBYR_ITEM_DESC_FILE_SIZE * index;
    GCM->loadEncryptedString(BOBBYRDESCFILE, buf, startLoc, BOBBYR_ITEM_DESC_INFO_SIZE);
  }
  else
  {
    wcsncpy(buf, &UTF8String(im->bobbyRayDescriptionOverride.c_str()).getWCHAR()[0], BOBBYR_ITEM_DESC_NAME_SIZE);
  }
}
