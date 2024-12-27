// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SAVELOADMAP__H_
#define _SAVELOADMAP__H_

#include "TileEngine/ExitGrids.h"
#include "TileEngine/WorldDef.h"

// Used for the ubType in the MODIFY_MAP struct
enum {
  SLM_NONE,

  // Adding a map graphic
  SLM_LAND,
  SLM_OBJECT,
  SLM_STRUCT,
  SLM_SHADOW,
  SLM_MERC,  // Should never be used
  SLM_ROOF,
  SLM_ONROOF,
  SLM_TOPMOST,  // Should never be used

  // For Removing
  SLM_REMOVE_LAND,
  SLM_REMOVE_OBJECT,
  SLM_REMOVE_STRUCT,
  SLM_REMOVE_SHADOW,
  SLM_REMOVE_MERC,  // Should never be used
  SLM_REMOVE_ROOF,
  SLM_REMOVE_ONROOF,
  SLM_REMOVE_TOPMOST,  // Should never be used

  // Smell, or Blood is used
  SLM_BLOOD_SMELL,

  // Damage a particular struct
  SLM_DAMAGED_STRUCT,

  // Exit Grids
  SLM_EXIT_GRIDS,

  // State of Openable structs
  SLM_OPENABLE_STRUCT,

  // Modify window graphic & structure
  SLM_WINDOW_HIT,
};

struct MODIFY_MAP {
  uint16_t usGridNo;         // The gridno the graphic will be applied to
  uint16_t usImageType;      // graphic index
  uint16_t usSubImageIndex;  //
                             //	uint16_t	usIndex;
  uint8_t ubType;            // the layer it will be applied to

  uint8_t ubExtra;  // Misc. variable used to strore arbritary values
};

// Use this as sentry, so map changes are added to the map temp file
class ApplyMapChangesToMapTempFile {
 public:
  ApplyMapChangesToMapTempFile(bool const active = true) { active_ = active; }

  ~ApplyMapChangesToMapTempFile() { active_ = false; }

  static bool IsActive() { return active_; }

 private:
  static bool active_;
};

// Applies a change TO THE MAP TEMP file
void AddStructToMapTempFile(uint32_t iMapIndex, uint16_t usIndex);

void AddObjectToMapTempFile(uint32_t uiMapIndex, uint16_t usIndex);

void LoadAllMapChangesFromMapTempFileAndApplyThem();

void RemoveStructFromMapTempFile(uint32_t uiMapIndex, uint16_t usIndex);

void AddRemoveObjectToMapTempFile(uint32_t uiMapIndex, uint16_t usIndex);

void SaveBloodSmellAndRevealedStatesFromMapToTempFile();

void SaveRevealedStatusArrayToRevealedTempFile(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

void LoadRevealedStatusArrayFromRevealedTempFile();

void RemoveStructFromUnLoadedMapTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                         int16_t sSectorY, uint8_t ubSectorZ);
void AddStructToUnLoadedMapTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                    int16_t sSectorY, uint8_t ubSectorZ);

// Adds the exit grid to
void AddExitGridToMapTempFile(uint16_t usGridNo, EXITGRID *pExitGrid, int16_t sSectorX,
                              int16_t sSectorY, uint8_t ubSectorZ);

// This function removes a struct with the same MapIndex and graphic index from
// the given sectors temp file
BOOLEAN RemoveGraphicFromTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                  int16_t sSectorY, uint8_t ubSectorZ);

void AddWindowHitToMapTempFile(uint32_t uiMapIndex);

void ChangeStatusOfOpenableStructInUnloadedSector(uint16_t usSectorX, uint16_t usSectorY,
                                                  int8_t bSectorZ, uint16_t usGridNo,
                                                  BOOLEAN fChangeToOpen);

#endif
