#ifndef __TACTICAL_SAVE_H_
#define __TACTICAL_SAVE_H_

#include "Strategic/CampaignTypes.h"
#include "Tactical/HandleItems.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/SoldierProfileType.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/WorldDef.h"

// Load the Map modifications from the saved game file
void LoadMapTempFilesFromSavedGameFile(HWFILE, uint32_t savegame_version);

// Save the Map Temp files to the saved game file
void SaveMapTempFilesToSavedGameFile(HWFILE);

// Saves the Current Sectors, ( world Items, rotting corpses, ... )  to the
// temporary file used to store the sectors items
void SaveCurrentSectorsInformationToTempItemFile();

// Loads the Currents Sectors information ( world Items, rotting corpses, ... )
// from the temporary file used to store the sectores items
void LoadCurrentSectorsInformationFromTempItemsFile();

// Loads a World Item array from that sectors temp item file
void LoadWorldItemsFromTempItemFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                    uint32_t *item_count, WORLDITEM **items);

//  Adds an array of Item Objects to the specified location on a unloaded map.
//  If you want to overwrite all the items in the array set fReplaceEntireFile
//  to TRUE.
void AddItemsToUnLoadedSector(int16_t sMapX, int16_t sMapY, int8_t bMapZ, int16_t sGridNo,
                              uint32_t uiNumberOfItems, OBJECTTYPE const *pObject, uint8_t ubLevel,
                              uint16_t usFlags, int8_t bRenderZHeightAboveLevel, Visibility);

void AddWorldItemsToUnLoadedSector(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                   uint32_t uiNumberOfItems, const WORLDITEM *pWorldItem);

// Delete all the files in the temp directory.
void InitTacticalSave();

// Call this function to set the new sector a NPC will travel to
void ChangeNpcToDifferentSector(MERCPROFILESTRUCT &, int16_t sSectorX, int16_t sSectorY,
                                int8_t bSectorZ);

// Adds a rotting corpse definition to the end of a sectors rotting corpse temp
// file
void AddRottingCorpseToUnloadedSectorsRottingCorpseFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                                        ROTTING_CORPSE_DEFINITION const *);

// Flags used for the AddDeadSoldierToUnLoadedSector() function
#define ADD_DEAD_SOLDIER_USE_GRIDNO \
  0x00000001  // just place the items and corpse on the gridno location
#define ADD_DEAD_SOLDIER_TO_SWEETSPOT 0x00000002  // Finds the closet free gridno

#define ADD_DEAD_SOLDIER__USE_JFK_HEADSHOT_CORPSE 0x00000040  // Will ue the JFK headshot

// Pass in the sector to add the dead soldier to.
// The gridno if you are passing in either of the flags
// ADD_DEAD_SOLDIER_USE_GRIDNO, or the ADD_DEAD_SOLDIER_TO_SWEETSPOT
//
// This function DOES NOT remove the soldier from the soldier struct.  YOU must
// do it.
void AddDeadSoldierToUnLoadedSector(int16_t sMapX, int16_t sMapY, uint8_t bMapZ,
                                    SOLDIERTYPE *pSoldier, int16_t sGridNo, uint32_t uiFlags);

BOOLEAN GetSectorFlagStatus(int16_t sMapX, int16_t sMapY, uint8_t bMapZ, SectorFlags);
void SetSectorFlag(int16_t sMapX, int16_t sMapY, uint8_t bMapZ, SectorFlags);
void ReSetSectorFlag(int16_t sMapX, int16_t sMapY, uint8_t bMapZ, SectorFlags);

// Saves the NPC temp Quote file to the saved game file
void LoadTempNpcQuoteArrayToSaveGameFile(HWFILE);

// Loads the NPC temp Quote file from the saved game file
void SaveTempNpcQuoteArrayToSaveGameFile(HWFILE);

void JA2EncryptedFileRead(HWFILE, uint8_t *data, uint32_t uiBytesToRead);
void JA2EncryptedFileWrite(HWFILE, uint8_t const *data, uint32_t uiBytesToWrite);

void NewJA2EncryptedFileRead(HWFILE, uint8_t *data, uint32_t uiBytesToRead);
void NewJA2EncryptedFileWrite(HWFILE, uint8_t const *data, uint32_t uiBytesToWrite);

// If hacker's mess with our save/temp files, this is our final line of defence.
void InitExitGameDialogBecauseFileHackDetected();

void HandleAllReachAbleItemsInTheSector(int16_t x, int16_t y, int8_t z);

void GetMapTempFileName(SectorFlags uiType, char *pMapName, int16_t sMapX, int16_t sMapY,
                        int8_t bMapZ);

uint32_t GetNumberOfVisibleWorldItemsFromSectorStructureForSector(int16_t sMapX, int16_t sMapY,
                                                                  int8_t bMapZ);
void SetNumberOfVisibleWorldItemsInSectorStructureForSector(int16_t sMapX, int16_t sMapY,
                                                            int8_t bMapZ, uint32_t uiNumberOfItems);

#define NEW_ROTATION_ARRAY_SIZE 49
#define BASE_NUMBER_OF_ROTATION_ARRAYS 19

void SaveWorldItemsToTempItemFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                  uint32_t uiNumberOfItems, WORLDITEM const *pData);

#endif
