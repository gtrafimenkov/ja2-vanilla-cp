#ifndef NPC_H
#define NPC_H

#include "JA2Types.h"
#include "Strategic/Facts.h"

#define NPC_TALK_RADIUS 4

enum Approach {
  APPROACH_NONE = 0,

  APPROACH_FRIENDLY = 1,
  APPROACH_DIRECT,
  APPROACH_THREATEN,
  APPROACH_RECRUIT,
  APPROACH_REPEAT,

  APPROACH_GIVINGITEM,
  NPC_INITIATING_CONV,
  NPC_INITIAL_QUOTE,
  NPC_WHOAREYOU,
  TRIGGER_NPC,

  APPROACH_GIVEFIRSTAID,
  APPROACH_SPECIAL_INITIAL_QUOTE,
  APPROACH_ENEMY_NPC_QUOTE,
  APPROACH_DECLARATION_OF_HOSTILITY,
  APPROACH_EPC_IN_WRONG_SECTOR,

  APPROACH_EPC_WHO_IS_RECRUITED,
  APPROACH_INITIAL_QUOTE,
  APPROACH_CLOSING_SHOP,
  APPROACH_SECTOR_NOT_SAFE,
  APPROACH_DONE_SLAPPED,  // 20

  APPROACH_DONE_PUNCH_0,
  APPROACH_DONE_PUNCH_1,
  APPROACH_DONE_PUNCH_2,
  APPROACH_DONE_OPEN_STRUCTURE,
  APPROACH_DONE_GET_ITEM,  // 25

  APPROACH_DONE_GIVING_ITEM,
  APPROACH_DONE_TRAVERSAL,
  APPROACH_BUYSELL,
  APPROACH_ONE_OF_FOUR_STANDARD,
  APPROACH_FRIENDLY_DIRECT_OR_RECRUIT,  // 30
};

extern int8_t const gbFirstApproachFlags[4];

void ShutdownNPCQuotes();

extern void SetQuoteRecordAsUsed(uint8_t ubNPC, uint8_t ubRecord);

// uiApproachData is used for approach things like giving items, etc.
uint8_t CalcDesireToTalk(uint8_t ubNPC, uint8_t ubMerc, Approach);
void ConverseFull(uint8_t ubNPC, uint8_t ubMerc, Approach, uint8_t approach_record,
                  OBJECTTYPE *approach_object);
void Converse(uint8_t ubNPC, uint8_t ubMerc, Approach);

extern void NPCReachedDestination(SOLDIERTYPE *pNPC, BOOLEAN fAlreadyThere);
extern void PCsNearNPC(uint8_t ubNPC);
extern BOOLEAN PCDoesFirstAidOnNPC(uint8_t ubNPC);
extern void TriggerNPCRecord(uint8_t ubTriggerNPC, uint8_t ubTriggerNPCRec);
extern BOOLEAN TriggerNPCWithIHateYouQuote(uint8_t ubTriggerNPC);

extern void TriggerNPCRecordImmediately(uint8_t ubTriggerNPC, uint8_t ubTriggerNPCRec);

BOOLEAN TriggerNPCWithGivenApproach(uint8_t ubTriggerNPC, Approach);

bool ReloadQuoteFile(uint8_t ubNPC);
void ReloadAllQuoteFiles();

// Save and loads the npc info to a saved game file
void SaveNPCInfoToSaveGameFile(HWFILE);
void LoadNPCInfoFromSavedGameFile(HWFILE, uint32_t uiSaveGameVersion);

extern void TriggerFriendWithHostileQuote(uint8_t ubNPC);

extern void ReplaceLocationInNPCDataFromProfileID(uint8_t ubNPC, int16_t sOldGridNo,
                                                  int16_t sNewGridNo);

extern uint8_t ActionIDForMovementRecord(uint8_t ubNPC, uint8_t ubRecord);

// given a victory in this sector, handle specific facts
void HandleVictoryInNPCSector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ);

// check if this shopkeep has been shutdown, if so do soething and return the
// fact
BOOLEAN HandleShopKeepHasBeenShutDown(uint8_t ubCharNum);

BOOLEAN NPCHasUnusedRecordWithGivenApproach(uint8_t ubNPC, Approach);
BOOLEAN NPCWillingToAcceptItem(uint8_t ubNPC, uint8_t ubMerc, OBJECTTYPE *pObj);

void SaveBackupNPCInfoToSaveGameFile(HWFILE);
void LoadBackupNPCInfoFromSavedGameFile(HWFILE);

void UpdateDarrelScriptToGoTo(SOLDIERTYPE *pSoldier);

#define WALTER_BRIBE_AMOUNT 20000

BOOLEAN GetInfoForAbandoningEPC(uint8_t ubNPC, uint16_t *pusQuoteNum, Fact *fact_to_set_true);

BOOLEAN RecordHasDialogue(uint8_t ubNPC, uint8_t ubRecord);

int8_t ConsiderCivilianQuotes(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ,
                              BOOLEAN fSetAsUsed);

void ResetOncePerConvoRecordsForNPC(uint8_t ubNPC);

void HandleNPCChangesForTacticalTraversal(const SOLDIERTYPE *s);

BOOLEAN NPCHasUnusedHostileRecord(uint8_t ubNPC, Approach);

void ResetOncePerConvoRecordsForAllNPCsInLoadedSector();

int16_t NPCConsiderInitiatingConv(const SOLDIERTYPE *pNPC);

#endif
