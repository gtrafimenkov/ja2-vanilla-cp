#include <stdexcept>

#include "sgp/HImage.h"
#include "sgp/PODObj.h"
#include "Build/TileEngine/Structure.h"
#include "Build/TileEngine/TileDef.h"
#include "Tile_Surface.h"
#include "sgp/VObject.h"
#include "Build/TileEngine/WorldDef.h"
#include "Build/TileEngine/WorldDat.h"
#include "sgp/Debug.h"
#include "Smooth.h"
#include "sgp/MouseSystem.h"
#include "Build/Sys_Globals.h"
#include "Build/TileEngine/TileDat.h"
#include "sgp/FileMan.h"
#include "sgp/MemMan.h"
#include "Build/TileEngine/Tile_Cache.h"

#include "src/ContentManager.h"
#include "src/GameInstance.h"

#include "slog/slog.h"
#define TAG "Tiles"


TILE_IMAGERY				*gTileSurfaceArray[ NUMBEROFTILETYPES ];


TILE_IMAGERY* LoadTileSurface(const char* cFilename)
try
{
	// Add tile surface
	AutoSGPImage   hImage(CreateImage(cFilename, IMAGE_ALLDATA));
	AutoSGPVObject hVObject(AddVideoObjectFromHImage(hImage));

	// Load structure data, if any.
	// Start by hacking the image filename into that for the structure data
  std::string cStructureFilename(FileMan::replaceExtension(cFilename, ".jsd"));

	AutoStructureFileRef pStructureFileRef;
	if (GCM->doesGameResExists( cStructureFilename ))
	{
    // SLOGD(TAG, "loading tile %s", cStructureFilename.c_str());

		pStructureFileRef = LoadStructureFile( cStructureFilename.c_str() );

		if (hVObject->SubregionCount() != pStructureFileRef->usNumberOfStructures)
		{
			throw std::runtime_error("Structure file error");
		}

		AddZStripInfoToVObject(hVObject, pStructureFileRef, FALSE, 0);
	}

	SGP::PODObj<TILE_IMAGERY> pTileSurf;

	if (pStructureFileRef && pStructureFileRef->pAuxData != NULL)
	{
		pTileSurf->pAuxData = pStructureFileRef->pAuxData;
		pTileSurf->pTileLocData = pStructureFileRef->pTileLocData;
	}
	else if (hImage->uiAppDataSize == hVObject->SubregionCount() * sizeof(AuxObjectData))
	{
		// Valid auxiliary data, so make a copy of it for TileSurf
		pTileSurf->pAuxData = MALLOCN(AuxObjectData, hVObject->SubregionCount());
		memcpy( pTileSurf->pAuxData, hImage->pAppData, hImage->uiAppDataSize );
	}
	else
	{
		pTileSurf->pAuxData = NULL;
	}

	pTileSurf->vo                = hVObject.Release();
	pTileSurf->pStructureFileRef = pStructureFileRef.Release();
	return pTileSurf.Release();
}
catch (...)
{
	SET_ERROR("Could not load tile file: %s", cFilename);
	throw;
}


void DeleteTileSurface(TILE_IMAGERY* const pTileSurf)
{
	if ( pTileSurf->pStructureFileRef != NULL )
	{
		FreeStructureFile( pTileSurf->pStructureFileRef );
	}
	else
	{
		// If a structure file exists, it will free the auxdata.
		// Since there is no structure file in this instance, we
		// free it ourselves.
		if (pTileSurf->pAuxData != NULL)
		{
			MemFree( pTileSurf->pAuxData );
		}
	}

	DeleteVideoObject(pTileSurf->vo);
	MemFree( pTileSurf );
}


void SetRaisedObjectFlag(char const* const filename, TILE_IMAGERY* const t)
{
	static char const RaisedObjectFiles[][9] =
	{
		"bones",
		"bones2",
		"grass2",
		"grass3",
		"l_weed3",
		"litter",
		"miniweed",
		"sblast",
		"sweeds",
		"twigs",
		"wing"
	};

	if (DEBRISWOOD != t->fType && t->fType != DEBRISWEEDS && t->fType != DEBRIS2MISC && t->fType != ANOTHERDEBRIS) return;

	// Loop through array of RAISED objecttype imagery and set global value
  std::string rootfile(FileMan::getFileNameWithoutExt(filename));
	for (char const (*i)[9] = RaisedObjectFiles; i != endof(RaisedObjectFiles); ++i)
	{
		if (strcasecmp(*i, rootfile.c_str()) != 0) continue;
		t->bRaisedObjectType = TRUE;
		return;
	}
}
