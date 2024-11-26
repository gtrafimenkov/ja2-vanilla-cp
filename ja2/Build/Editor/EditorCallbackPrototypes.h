#ifndef __EDITOR_CALLBACK_PROTOTYPES_H
#define __EDITOR_CALLBACK_PROTOTYPES_H

#include "SGP/ButtonSystem.h"

// Editor Tabs
void TaskTerrainCallback(GUI_BUTTON *btn, int32_t reason);
void TaskBuildingCallback(GUI_BUTTON *btn, int32_t reason);
void TaskItemsCallback(GUI_BUTTON *btn, int32_t reason);
void TaskMercsCallback(GUI_BUTTON *btn, int32_t reason);
void TaskMapInfoCallback(GUI_BUTTON *btn, int32_t reason);
void TaskOptionsCallback(GUI_BUTTON *btn, int32_t reason);
// Options Tab Callbacks
void BtnLoadCallback(GUI_BUTTON *btn, int32_t reason);
void BtnSaveCallback(GUI_BUTTON *btn, int32_t reason);
void BtnCancelCallback(GUI_BUTTON *btn, int32_t reason);
void BtnQuitCallback(GUI_BUTTON *btn, int32_t reason);
void BtnNewMapCallback(GUI_BUTTON *btn, int32_t reason);
void BtnNewBasementCallback(GUI_BUTTON *btn, int32_t reason);
void BtnNewCavesCallback(GUI_BUTTON *btn, int32_t reason);
void BtnChangeTilesetCallback(GUI_BUTTON *btn, int32_t reason);
// Mercs Tab Callbacks
void MercsTogglePlayers(GUI_BUTTON *btn, int32_t reason);
void MercsToggleEnemies(GUI_BUTTON *btn, int32_t reason);
void MercsToggleCreatures(GUI_BUTTON *btn, int32_t reason);
void MercsToggleRebels(GUI_BUTTON *btn, int32_t reason);
void MercsToggleCivilians(GUI_BUTTON *btn, int32_t reason);
void MercsPlayerTeamCallback(GUI_BUTTON *btn, int32_t reason);
void MercsEnemyTeamCallback(GUI_BUTTON *btn, int32_t reason);
void MercsCreatureTeamCallback(GUI_BUTTON *btn, int32_t reason);
void MercsRebelTeamCallback(GUI_BUTTON *btn, int32_t reason);
void MercsCivilianTeamCallback(GUI_BUTTON *btn, int32_t reason);
void MercsDetailedPlacementCallback(GUI_BUTTON *btn, int32_t reason);
void MercsPriorityExistanceCallback(GUI_BUTTON *btn, int32_t reason);
void MercsHasKeysCallback(GUI_BUTTON *btn, int32_t reason);
void MercsNextCallback(GUI_BUTTON *btn, int32_t reason);
void MercsDeleteCallback(GUI_BUTTON *btn, int32_t reason);
void MercsGeneralModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsAttributesModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsInventoryModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsAppearanceModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsProfileModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetEnemyColorCodeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetOrdersCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetAttitudeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsDirectionSetCallback(GUI_BUTTON *btn, int32_t reason);
void MercsFindSelectedMercCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetRelativeEquipmentCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetRelativeAttributesCallback(GUI_BUTTON *btn, int32_t reason);
void MercsToggleColorModeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetColorsCallback(GUI_BUTTON *btn, int32_t reason);
void MercsSetBodyTypeCallback(GUI_BUTTON *btn, int32_t reason);
void MercsInventorySlotCallback(GUI_BUTTON *btn, int32_t reason);
void MouseMovedInMercRegion(MOUSE_REGION *reg, int32_t reason);
void MouseClickedInMercRegion(MOUSE_REGION *reg, int32_t reason);
void MercsCivilianGroupCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleAction1Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleAction2Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleAction3Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleAction4Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData1ACallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData1BCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData2ACallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData2BCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData3ACallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData3BCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData4ACallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleData4BCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleClearCallback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleToggleVariance1Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleToggleVariance2Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleToggleVariance3Callback(GUI_BUTTON *btn, int32_t reason);
void MercsScheduleToggleVariance4Callback(GUI_BUTTON *btn, int32_t reason);
// Terrain Tab Callbacks
void BtnLandRaiseCallback(GUI_BUTTON *btn, int32_t reason);
void BtnLandLowerCallback(GUI_BUTTON *btn, int32_t reason);
void BtnIncBrushDensityCallback(GUI_BUTTON *btn, int32_t reason);
void BtnDecBrushDensityCallback(GUI_BUTTON *btn, int32_t reason);
void BtnFgGrndCallback(GUI_BUTTON *btn, int32_t reason);
void BtnBkGrndCallback(GUI_BUTTON *btn, int32_t reason);
void BtnObjectCallback(GUI_BUTTON *btn, int32_t reason);
void BtnBanksCallback(GUI_BUTTON *btn, int32_t reason);
void BtnRoadsCallback(GUI_BUTTON *btn, int32_t reason);
void BtnDebrisCallback(GUI_BUTTON *btn, int32_t reason);
void BtnBrushCallback(GUI_BUTTON *btn, int32_t reason);
void BtnObject1Callback(GUI_BUTTON *btn, int32_t reason);
void BtnObject2Callback(GUI_BUTTON *btn, int32_t reason);
void BtnFillCallback(GUI_BUTTON *btn, int32_t reason);
void TerrainTileButtonRegionCallback(MOUSE_REGION *reg, int32_t reason);
// Items Tab Callbacks
void ItemsWeaponsCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsAmmoCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsArmourCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsExplosivesCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsEquipment1Callback(GUI_BUTTON *btn, int32_t reason);
void ItemsEquipment2Callback(GUI_BUTTON *btn, int32_t reason);
void ItemsEquipment3Callback(GUI_BUTTON *btn, int32_t reason);
void ItemsTriggersCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsKeysCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsLeftScrollCallback(GUI_BUTTON *btn, int32_t reason);
void ItemsRightScrollCallback(GUI_BUTTON *btn, int32_t reason);
void MouseMovedInItemsRegion(MOUSE_REGION *reg, int32_t reason);
void MouseClickedInItemsRegion(MOUSE_REGION *reg, int32_t reason);
// MapInfo Callbacks
void BtnFakeLightCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoPrimeTimeRadioCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoNightTimeRadioCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfo24HourTimeRadioCallback(GUI_BUTTON *btn, int32_t reason);
void BtnDrawLightsCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoDrawExitGridCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoEntryPointsCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoNormalRadioCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoBasementRadioCallback(GUI_BUTTON *btn, int32_t reason);
void MapInfoCavesRadioCallback(GUI_BUTTON *btn, int32_t reason);
// Building callbacks
void BuildingWallCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingDoorCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingWindowCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingRoofCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingCrackWallCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingFurnitureCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingDecalCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingFloorCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingToiletCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingSmartWallCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingSmartDoorCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingSmartWindowCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingSmartCrackWallCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingDoorKeyCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingNewRoomCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingNewRoofCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingSawRoomCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingKillBuildingCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingCopyBuildingCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingMoveBuildingCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingCaveDrawingCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingDrawRoomNumCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingEraseRoomNumCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingToggleRoofViewCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingToggleWallViewCallback(GUI_BUTTON *btn, int32_t reason);
void BuildingToggleInfoViewCallback(GUI_BUTTON *btn, int32_t reason);

// ItemStats Callbacks
void ItemStatsToggleHideCallback(GUI_BUTTON *btn, int32_t reason);
void ItemStatsDeleteCallback(GUI_BUTTON *btn, int32_t reason);

// Various Callbacks
void BtnUndoCallback(GUI_BUTTON *btn, int32_t reason);
void BtnEraseCallback(GUI_BUTTON *btn, int32_t reason);

#endif
