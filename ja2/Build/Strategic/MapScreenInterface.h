#ifndef MAP_SCREEN_INTERFACE_H
#define MAP_SCREEN_INTERFACE_H

#include "JA2Types.h"
#include "MessageBoxScreen.h"
#include "SGP/MouseSystem.h"
#include "ScreenIDs.h"
#include "Tactical/ItemTypes.h"

// char breath and life position
#define BAR_INFO_X 66
#define BAR_INFO_Y 61

// merc icon position
#define CHAR_ICON_CONTRACT_Y 64
#define CHAR_ICON_X 187
#define CHAR_ICON_WIDTH 10
#define CHAR_ICON_HEIGHT 10
#define CHAR_ICON_SPACING 13

// max number of characters
// Character List Length
#define MAX_CHARACTER_COUNT 20

// map screen font
#define MAP_SCREEN_FONT BLOCKFONT2

// characterlist regions
#define Y_START 146
#define MAP_START_KEYRING_Y 107
#define Y_SIZE GetFontHeight(MAP_SCREEN_FONT)

// attribute menu defines (must match NUM_TRAINABLE_STATS defines, and
// pAttributeMenuStrings )
enum {
  ATTRIB_MENU_STR = 0,
  ATTRIB_MENU_DEX,
  ATTRIB_MENU_AGI,
  ATTRIB_MENU_HEA,
  ATTRIB_MENU_MARK,
  ATTRIB_MENU_MED,
  ATTRIB_MENU_MECH,
  ATTRIB_MENU_LEAD,
  ATTRIB_MENU_EXPLOS,
  ATTRIB_MENU_CANCEL,
  MAX_ATTRIBUTE_STRING_COUNT,
};

// the epc assignment menu
enum {
  EPC_MENU_ON_DUTY = 0,
  EPC_MENU_PATIENT,
  EPC_MENU_VEHICLE,
  EPC_MENU_REMOVE,
  EPC_MENU_CANCEL,
  MAX_EPC_MENU_STRING_COUNT,
};

// assignment menu defines
enum {
  ASSIGN_MENU_ON_DUTY = 0,
  ASSIGN_MENU_DOCTOR,
  ASSIGN_MENU_PATIENT,
  ASSIGN_MENU_VEHICLE,
  ASSIGN_MENU_REPAIR,
  ASSIGN_MENU_TRAIN,
  ASSIGN_MENU_CANCEL,
  MAX_ASSIGN_STRING_COUNT,
};

// training assignment menu defines
enum {
  TRAIN_MENU_SELF,
  TRAIN_MENU_TOWN,
  TRAIN_MENU_TEAMMATES,
  TRAIN_MENU_TRAIN_BY_OTHER,
  TRAIN_MENU_CANCEL,
  MAX_TRAIN_STRING_COUNT,
};

// the remove merc from team pop up box strings
enum {
  REMOVE_MERC = 0,
  REMOVE_MERC_CANCEL,
  MAX_REMOVE_MERC_COUNT,
};

// squad menu defines
enum {
  SQUAD_MENU_1,
  SQUAD_MENU_2,
  SQUAD_MENU_3,
  SQUAD_MENU_4,
  SQUAD_MENU_5,
  SQUAD_MENU_6,
  SQUAD_MENU_7,
  SQUAD_MENU_8,
  SQUAD_MENU_9,
  SQUAD_MENU_10,
  SQUAD_MENU_11,
  SQUAD_MENU_12,
  SQUAD_MENU_13,
  SQUAD_MENU_14,
  SQUAD_MENU_15,
  SQUAD_MENU_16,
  SQUAD_MENU_17,
  SQUAD_MENU_18,
  SQUAD_MENU_19,
  SQUAD_MENU_20,
  SQUAD_MENU_CANCEL,
  MAX_SQUAD_MENU_STRING_COUNT,
};

// contract menu defines
enum {
  CONTRACT_MENU_CURRENT_FUNDS = 0,
  CONTRACT_MENU_SPACE,
  CONTRACT_MENU_DAY,
  CONTRACT_MENU_WEEK,
  CONTRACT_MENU_TWO_WEEKS,
  CONTRACT_MENU_TERMINATE,
  CONTRACT_MENU_CANCEL,
  MAX_CONTRACT_MENU_STRING_COUNT,
};

enum UpdateBoxReason {
  NO_REASON_FOR_UPDATE = 0,
  CONTRACT_FINISHED_FOR_UPDATE,
  ASSIGNMENT_FINISHED_FOR_UPDATE,
  ASSIGNMENT_RETURNING_FOR_UPDATE,
  ASLEEP_GOING_AUTO_FOR_UPDATE,
  CONTRACT_EXPIRE_WARNING_REASON,
};

// dimensions and offset for merc update box
#define UPDATE_MERC_FACE_X_WIDTH 50
#define UPDATE_MERC_FACE_X_HEIGHT 50
#define UPDATE_MERC_FACE_X_OFFSET 2
#define UPDATE_MERC_FACE_Y_OFFSET 2
#define WIDTH_OF_UPDATE_PANEL_BLOCKS 50
#define HEIGHT_OF_UPDATE_PANEL_BLOCKS 50
#define UPDATE_MERC_Y_OFFSET 4
#define UPDATE_MERC_X_OFFSET 4

// dimensions and offset for merc update box
#define TACT_UPDATE_MERC_FACE_X_WIDTH 70
#define TACT_UPDATE_MERC_FACE_X_HEIGHT 49
#define TACT_UPDATE_MERC_FACE_X_OFFSET 8
#define TACT_UPDATE_MERC_FACE_Y_OFFSET 6
#define TACT_WIDTH_OF_UPDATE_PANEL_BLOCKS 70
#define TACT_HEIGHT_OF_UPDATE_PANEL_BLOCKS 49
#define TACT_UPDATE_MERC_Y_OFFSET 4
#define TACT_UPDATE_MERC_X_OFFSET 4

// the first vehicle slot int he list
#define FIRST_VEHICLE 18

extern BOOLEAN fShowAssignmentMenu;
extern BOOLEAN fShowTrainingMenu;
extern BOOLEAN fShowAttributeMenu;
extern BOOLEAN fShowSquadMenu;
extern BOOLEAN fShowContractMenu;

// The character data structure
struct MapScreenCharacterSt {
  SOLDIERTYPE *merc;
  BOOLEAN selected;
};

// map screen character structure list, contrains soldier ids into menptr
extern MapScreenCharacterSt gCharactersList[];

#define BASE_FOR_EACH_IN_CHAR_LIST(type, iter)                                              \
  for (type *iter = gCharactersList; iter != gCharactersList + MAX_CHARACTER_COUNT; ++iter) \
    if (Assert(iter->merc == NULL || iter->merc->bActive), iter->merc == NULL)              \
      continue;                                                                             \
    else
#define FOR_EACH_IN_CHAR_LIST(iter) BASE_FOR_EACH_IN_CHAR_LIST(MapScreenCharacterSt, iter)
#define CFOR_EACH_IN_CHAR_LIST(iter) BASE_FOR_EACH_IN_CHAR_LIST(const MapScreenCharacterSt, iter)

#define BASE_FOR_EACH_SELECTED_IN_CHAR_LIST(type, iter)                                     \
  for (type *iter = gCharactersList; iter != gCharactersList + MAX_CHARACTER_COUNT; ++iter) \
    if (Assert(!iter->selected || iter->merc != NULL),                                      \
        Assert(iter->merc == NULL || iter->merc->bActive), !iter->selected)                 \
      continue;                                                                             \
    else
#define FOR_EACH_SELECTED_IN_CHAR_LIST(iter) \
  BASE_FOR_EACH_SELECTED_IN_CHAR_LIST(MapScreenCharacterSt, iter)
#define CFOR_EACH_SELECTED_IN_CHAR_LIST(iter) \
  BASE_FOR_EACH_SELECTED_IN_CHAR_LIST(const MapScreenCharacterSt, iter)

// highlighted lines
extern int32_t giHighLine;
extern int32_t giAssignHighLine;
extern int32_t giDestHighLine;
extern int32_t giContractHighLine;
extern int32_t giSleepHighLine;

extern SGPVObject *guiUpdatePanelTactical;
extern BOOLEAN fShowUpdateBox;

extern SGPPoint ContractPosition;
extern SGPPoint AttributePosition;
extern SGPPoint TrainPosition;
extern SGPPoint VehiclePosition;
extern SGPPoint AssignmentPosition;
extern SGPPoint SquadPosition;

extern SGPPoint RepairPosition;

// disble team info panel due to showing of battle roster
extern BOOLEAN fDisableDueToBattleRoster;

extern BOOLEAN gfAtLeastOneMercWasHired;

// curtrent map sector z that is being displayed in the mapscreen
extern int32_t iCurrentMapSectorZ;

// y position of the pop up box
extern int32_t giBoxY;

// pop up box textures
extern SGPVSurface *guiPOPUPTEX;
extern SGPVObject *guiPOPUPBORDERS;

extern BOOLEAN fShowMapScreenMovementList;

// do we need to rebuild the mapscreen characterlist?
extern BOOLEAN fReBuildCharacterList;

// restore glow rotation in contract region glow boxes
extern BOOLEAN fResetContractGlow;

// init vehicle and characters list
void InitalizeVehicleAndCharacterList();

// set this entry to as selected
void SetEntryInSelectedCharacterList(int8_t bEntry);
// set this entry to as unselected
void ResetEntryForSelectedList(int8_t bEntry);

// reset selected list
void ResetSelectedListForMapScreen();

// build a selected list from a to b, inclusive
void BuildSelectedListFromAToB(int8_t bA, int8_t bB);

// isa this entry int he selected character list set?
BOOLEAN IsEntryInSelectedListSet(int8_t bEntry);

// is there more than one person selected?
BOOLEAN MultipleCharacterListEntriesSelected();

// toggle this entry on or off
void ToggleEntryInSelectedList(int8_t bEntry);

void ResetAssignmentsForMercsTrainingUnpaidSectorsInSelectedList();

void RestoreBackgroundForAssignmentGlowRegionList();
void RestoreBackgroundForDestinationGlowRegionList();
void RestoreBackgroundForContractGlowRegionList();
void RestoreBackgroundForSleepGlowRegionList();

// play click when we are entering a glow region
void PlayGlowRegionSound();

// is this character in the action of plotting a path?
BOOLEAN CharacterIsGettingPathPlotted(int16_t sCharNumber);

// disable team info panels
void DisableTeamInfoPanels();

// enable team info panels
void EnableTeamInfoPanels();

// do mapscreen message box
void DoMapMessageBox(MessageBoxStyleID, wchar_t const *zString, ScreenID uiExitScreen,
                     MessageBoxFlags, MSGBOX_CALLBACK ReturnCallback);

// hop up one leve,l int he map screen level interface
void GoUpOneLevelInMap();

// go down one level in the mapscreen map interface
void GoDownOneLevelInMap();

// jump to this level on the map
void JumpToLevel(int32_t iLevel);

// check to see if we need to update the screen
void CheckAndUpdateBasedOnContractTimes();

// Display a red arrow by the name of each selected merc
void HandleDisplayOfSelectedMercArrows();

// check which guys can move with this guy
void DeselectSelectedListMercsWhoCantMoveWithThisGuy(const SOLDIERTYPE *s);

// get morale string for this grunt given this morale level
wchar_t const *GetMoraleString(SOLDIERTYPE const &);

// handle leaving of equipment in sector
void HandleLeavingOfEquipmentInCurrentSector(SOLDIERTYPE &);

// set up a linked list of items being dropped and post an event to later drop
// them
void HandleMercLeavingEquipment(SOLDIERTYPE &, bool in_drassen);

// actually drop the stored list of items
void HandleEquipmentLeftInOmerta(uint32_t uiSlotIndex);
void HandleEquipmentLeftInDrassen(uint32_t uiSlotIndex);

// init/shutdown leave item lists
void InitLeaveList();
void ShutDownLeaveList();

// add item to leave equip index
void AddItemToLeaveIndex(const OBJECTTYPE *o, uint32_t uiIndex);

// handle a group about to arrive in a sector
void HandleGroupAboutToArrive();

// create and destroy the status bars mouse region
void CreateMapStatusBarsRegion();
void RemoveMapStatusBarsRegion();
void UpdateCharRegionHelpText();

// find this soldier in mapscreen character list and set as contract
void FindAndSetThisContractSoldier(SOLDIERTYPE *pSoldier);

// lose the cursor, re-render
void HandleMAPUILoseCursorFromOtherScreen();

void RenderMapRegionBackground();

// update mapscreen assignment positions
void UpdateMapScreenAssignmentPositions();

// get the umber of valid mercs in the mapscreen character list
int32_t GetNumberOfPeopleInCharacterList();

// the next and previous people in the mapscreen
void GoToPrevCharacterInList();
void GoToNextCharacterInList();

// this does the whole miner giving player info speil
void HandleMinerEvent(uint8_t bMinerNumber, int16_t sQuoteNumber, BOOLEAN fForceMapscreen);

void TurnOnSectorLocator(uint8_t ubProfileID);
void TurnOffSectorLocator();

extern int16_t gsSectorLocatorX;
extern int16_t gsSectorLocatorY;
extern uint8_t gubBlitSectorLocatorCode;

enum { LOCATOR_COLOR_NONE, LOCATOR_COLOR_RED, LOCATOR_COLOR_YELLOW };

void HandleBlitOfSectorLocatorIcon(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ,
                                   uint8_t ubLocatorID);

// the tactical version

// handle the actual showingof the list
void HandleShowingOfTacticalInterfaceFastHelpText();

// start showing the list
void StartShowingInterfaceFastHelpText();

// is the list active?
BOOLEAN IsTheInterfaceFastHelpTextActive();

/* This will setup a fast help text region that is unrelated to mouse regions.
 * The user is to pass in the x,y position of the box, the width to wrap the
 * string and the string itself */
void SetUpFastHelpRegion(int32_t x, int32_t y, int32_t width, const wchar_t *text);

// reset assignment for mercs trainign militia in this sector
void ResetAssignmentOfMercsThatWereTrainingMilitiaInThisSector(int16_t sSectorX, int16_t sSectorY);

// the sector move box
void CreateDestroyMovementBox(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ);
void SetUpMovingListsForSector(int16_t x, int16_t y, int16_t z);
void ReBuildMoveBox();
BOOLEAN IsCharacterSelectedForAssignment(int16_t sCharNumber);
BOOLEAN IsCharacterSelectedForSleep(int16_t sCharNumber);

// the update box
void CreateDestroyTheUpdateBox();
void DisplaySoldierUpdateBox();

/// set the town of Tixa as found by the player
void SetTixaAsFound();

// set the town of Orta as found by the player
void SetOrtaAsFound();

// set this SAM site as being found by the player
void SetSAMSiteAsFound(uint8_t uiSamIndex);

// Set up the timers for the move menu in mapscreen for double click detection.
void InitTimersForMoveMenuMouseRegions();

// the screen mask
void CreateScreenMaskForMoveBox();
void RemoveScreenMaskForMoveBox();

// help text to show user merc has insurance
void UpdateHelpTextForMapScreenMercIcons();
void CreateDestroyInsuranceMouseRegionForMercs(BOOLEAN fCreate);

// stuff to deal with player just starting the game
BOOLEAN HandleTimeCompressWithTeamJackedInAndGearedToGo();

// handle sector being taken over uncontested
void NotifyPlayerWhenEnemyTakesControlOfImportantSector(int16_t x, int16_t y, int8_t z);

// handle notifying player of invasion by enemy
void NotifyPlayerOfInvasionByEnemyForces(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ,
                                         MSGBOX_CALLBACK ReturnCallback);

void ShutDownUserDefineHelpTextRegions();

// add special events
void AddSoldierToWaitingListQueue(SOLDIERTYPE &);
void AddReasonToWaitingListQueue(UpdateBoxReason);
void AddDisplayBoxToWaitingQueue();

enum MoveError {
  ME_CUSTOM = -99,
  ME_OK = 0,
  ME_UNDERGROUND = 1,
  ME_ENEMY = 2,
  ME_BUSY = 3,
  ME_POW = 5,
  ME_TRANSIT = 8,
  ME_AIR_RAID = 10,
  ME_COMBAT = 11,
  ME_VEHICLE_EMPTY = 32,
  ME_MUSEUM = 34,
  ME_VEHICLE_NO_GAS = 42,
  ME_VEHICLE_DAMAGED = 47,
  ME_ROBOT_ALONE = 49
};

// can this group move it out
MoveError CanEntireMovementGroupMercIsInMove(SOLDIERTYPE &);
void ReportMapScreenMovementError(int8_t bErrorNumber);

void HandleRebuildingOfMapScreenCharacterList();

void RequestToggleTimeCompression();
void RequestIncreaseInTimeCompression();
void RequestDecreaseInTimeCompression();

void SelectUnselectedMercsWhoMustMoveWithThisGuy();

void LoadLeaveItemList(HWFILE);
void SaveLeaveItemList(HWFILE);

BOOLEAN CheckIfSalaryIncreasedAndSayQuote(SOLDIERTYPE *pSoldier, BOOLEAN fTriggerContractMenu);

void EndUpdateBox(BOOLEAN fContinueTimeCompression);

BOOLEAN MapscreenCanPassItemToChar(const SOLDIERTYPE *);

int32_t GetNumberOfMercsInUpdateList();

extern MOUSE_REGION gMapStatusBarsRegion;

void RandomMercInGroupSaysQuote(GROUP const &, uint16_t quote_num);

void MakeDialogueEventShowContractMenu(SOLDIERTYPE &);

void LoadMapScreenInterfaceGraphics();
void DeleteMapScreenInterfaceGraphics();

#endif
