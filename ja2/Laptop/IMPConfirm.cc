// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPConfirm.h"

#include <string.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/Finances.h"
#include "Laptop/History.h"
#include "Laptop/IMPCompileCharacter.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/IMPTextSystem.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "SGP/ButtonSystem.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/Random.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/Strategic.h"
#include "Tactical/LoadSaveMercProfile.h"
#include "Tactical/MercHiring.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierProfileType.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

#define IMP_MERC_FILE "imp.dat"

static BUTTON_PICS *giIMPConfirmButtonImage[2];
GUIButtonRef giIMPConfirmButton[2];

struct FacePosInfo {
  uint8_t eye_x;
  uint8_t eye_y;
  uint8_t mouth_x;
  uint8_t mouth_y;
};

static const FacePosInfo g_face_info[] = {
    {8, 5, 8, 21}, {9, 4, 9, 23},  {8, 5, 7, 24}, {6, 6, 7, 25}, {13, 5, 11, 23}, {11, 5, 10, 24},
    {8, 4, 8, 24}, {8, 4, 8, 24},  {4, 4, 5, 25}, {5, 5, 6, 24}, {7, 5, 7, 24},   {5, 7, 6, 26},
    {7, 6, 7, 24}, {11, 5, 9, 23}, {8, 5, 7, 24}, {5, 6, 5, 26}};

BOOLEAN fLoadingCharacterForPreviousImpProfile = FALSE;

static void BtnIMPConfirmNo(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPConfirmYes(GUI_BUTTON *btn, int32_t reason);

static void CreateConfirmButtons();

void EnterIMPConfirm() {
  // create buttons
  CreateConfirmButtons();
}

void RenderIMPConfirm() {
  // the background
  RenderProfileBackGround();

  // indent
  RenderAvgMercIndentFrame(90, 40);

  // highlight answer
  PrintImpText();
}

static void DestroyConfirmButtons();

void ExitIMPConfirm() {
  // destroy buttons
  DestroyConfirmButtons();
}

void HandleIMPConfirm() {}

static void MakeButton(uint32_t idx, const wchar_t *text, int16_t y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPConfirmButtonImage[idx] = img;
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col,
                              LAPTOP_SCREEN_UL_X + 136, y, MSYS_PRIORITY_HIGH, click);
  giIMPConfirmButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
}

static void CreateConfirmButtons() {
  // create buttons for confirm screen
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;
  MakeButton(0, pImpButtonText[16], dy + 254, BtnIMPConfirmYes);
  MakeButton(1, pImpButtonText[17], dy + 314, BtnIMPConfirmNo);
}

static void DestroyConfirmButtons() {
  // destroy buttons for confirm screen

  RemoveButton(giIMPConfirmButton[0]);
  UnloadButtonImage(giIMPConfirmButtonImage[0]);

  RemoveButton(giIMPConfirmButton[1]);
  UnloadButtonImage(giIMPConfirmButtonImage[1]);
}

static void GiveItemsToPC(uint8_t ubProfileId);

static BOOLEAN AddCharacterToPlayersTeam() {
  MERC_HIRE_STRUCT HireMercStruct;

  // last minute chage to make sure merc with right facehas not only the right
  // body but body specific skills... ie..small mercs have martial arts..but big
  // guys and women don't don't

  HandleMercStatsForChangesInFace();

  memset(&HireMercStruct, 0, sizeof(MERC_HIRE_STRUCT));

  HireMercStruct.ubProfileID = (uint8_t)(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);

  if (!fLoadingCharacterForPreviousImpProfile) {
    // give them items
    GiveItemsToPC(HireMercStruct.ubProfileID);
  }

  HireMercStruct.sSectorX = SECTORX(g_merc_arrive_sector);
  HireMercStruct.sSectorY = SECTORY(g_merc_arrive_sector);
  HireMercStruct.fUseLandingZoneForArrival = TRUE;

  HireMercStruct.fCopyProfileItemsOver = TRUE;

  // indefinite contract length
  HireMercStruct.iTotalContractLength = -1;

  HireMercStruct.ubInsertionCode = INSERTION_CODE_ARRIVING_GAME;
  HireMercStruct.uiTimeTillMercArrives = GetMercArrivalTimeOfDay();

  const FacePosInfo *const fi = &g_face_info[iPortraitNumber];
  SetProfileFaceData(HireMercStruct.ubProfileID, 200 + iPortraitNumber, fi->eye_x, fi->eye_y,
                     fi->mouth_x, fi->mouth_y);

  // if we succesfully hired the merc
  if (!HireMerc(HireMercStruct)) {
    return (FALSE);
  } else {
    return (TRUE);
  }
}

static void WriteOutCurrentImpCharacter(int32_t iProfileId);

static void BtnIMPConfirmYes(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (LaptopSaveInfo.fIMPCompletedFlag) {
      // already here, leave
      return;
    }

    if (LaptopSaveInfo.iCurrentBalance < COST_OF_PROFILE) {
      // not enough
      return;
    }

    // line moved by CJC Nov 28 2002 to AFTER the check for money
    LaptopSaveInfo.fIMPCompletedFlag = TRUE;

    // charge the player
    AddTransactionToPlayersBook(IMP_PROFILE,
                                (uint8_t)(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId),
                                GetWorldTotalMin(), -COST_OF_PROFILE);
    AddHistoryToPlayersLog(HISTORY_CHARACTER_GENERATED, 0, GetWorldTotalMin(), -1, -1);
    AddCharacterToPlayersTeam();

    // write the created imp merc
    WriteOutCurrentImpCharacter((uint8_t)(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId));

    fButtonPendingFlag = TRUE;
    iCurrentImpPage = IMP_HOME_PAGE;

    // send email notice
    // AddEmail(IMP_EMAIL_PROFILE_RESULTS, IMP_EMAIL_PROFILE_RESULTS_LENGTH,
    // IMP_PROFILE_RESULTS, GetWorldTotalMin());
    AddFutureDayStrategicEvent(EVENT_DAY2_ADD_EMAIL_FROM_IMP, 60 * 7, 0, 2);
    // RenderCharProfile();

    ResetCharacterStats();

    // Display a popup msg box telling the user when and where the merc will
    // arrive DisplayPopUpBoxExplainingMercArrivalLocationAndTime();

    // reset the id of the last merc so we dont get the
    // DisplayPopUpBoxExplainingMercArrivalLocationAndTime() pop up box in
    // another screen by accident
    LaptopSaveInfo.sLastHiredMerc.iIdOfMerc = -1;
  }
}

// fixed? by CJC Nov 28 2002
static void BtnIMPConfirmNo(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_FINISH;

#if 0  // XXX was commented out
		LaptopSaveInfo.fIMPCompletedFlag = FALSE;
		ResetCharacterStats();

		fButtonPendingFlag = TRUE;
		iCurrentImpPage = IMP_HOME_PAGE;
#endif
  }
}

static void MakeProfileInvItemAnySlot(MERCPROFILESTRUCT &, uint16_t usItem, uint8_t ubStatus,
                                      uint8_t ubHowMany);
static void MakeProfileInvItemThisSlot(MERCPROFILESTRUCT &, uint32_t uiPos, uint16_t usItem,
                                       uint8_t ubStatus, uint8_t ubHowMany);

static void GiveItemsToPC(uint8_t ubProfileId) {
  // gives starting items to merc
  // NOTE: Any guns should probably be from those available in regular gun set

  MERCPROFILESTRUCT &p = GetProfile(ubProfileId);

  // STANDARD EQUIPMENT

  // kevlar vest, leggings, & helmet
  MakeProfileInvItemThisSlot(p, VESTPOS, FLAK_JACKET, 100, 1);
  if (PreRandom(100) < (uint32_t)p.bWisdom) {
    MakeProfileInvItemThisSlot(p, HELMETPOS, STEEL_HELMET, 100, 1);
  }

  // canteen
  MakeProfileInvItemThisSlot(p, SMALLPOCK4POS, CANTEEN, 100, 1);

  if (p.bMarksmanship >= 80) {
    // good shooters get a better & matching ammo
    MakeProfileInvItemThisSlot(p, HANDPOS, MP5K, 100, 1);
    MakeProfileInvItemThisSlot(p, SMALLPOCK1POS, CLIP9_30, 100, 2);
  } else {
    // Automatic pistol, with matching ammo
    MakeProfileInvItemThisSlot(p, HANDPOS, BERETTA_93R, 100, 1);
    MakeProfileInvItemThisSlot(p, SMALLPOCK1POS, CLIP9_15, 100, 3);
  }

  // OPTIONAL EQUIPMENT: depends on skills & special skills

  if (p.bMedical >= 60) {
    // strong medics get full medic kit
    MakeProfileInvItemAnySlot(p, MEDICKIT, 100, 1);
  } else if (p.bMedical >= 30) {
    // passable medics get first aid kit
    MakeProfileInvItemAnySlot(p, FIRSTAIDKIT, 100, 1);
  }

  if (p.bMechanical >= 50) {
    // mechanics get toolkit
    MakeProfileInvItemAnySlot(p, TOOLKIT, 100, 1);
  }

  if (p.bExplosive >= 50) {
    // loonies get TNT & Detonator
    MakeProfileInvItemAnySlot(p, TNT, 100, 1);
    MakeProfileInvItemAnySlot(p, DETONATOR, 100, 1);
  }

  // check for special skills
  if (HasSkillTrait(p, LOCKPICKING) && iMechanical) {
    MakeProfileInvItemAnySlot(p, LOCKSMITHKIT, 100, 1);
  }

  if (HasSkillTrait(p, HANDTOHAND)) {
    MakeProfileInvItemAnySlot(p, BRASS_KNUCKLES, 100, 1);
  }

  if (HasSkillTrait(p, ELECTRONICS) && iMechanical) {
    MakeProfileInvItemAnySlot(p, METALDETECTOR, 100, 1);
  }

  if (HasSkillTrait(p, NIGHTOPS)) {
    MakeProfileInvItemAnySlot(p, BREAK_LIGHT, 100, 2);
  }

  if (HasSkillTrait(p, THROWING)) {
    MakeProfileInvItemAnySlot(p, THROWING_KNIFE, 100, 1);
  }

  if (HasSkillTrait(p, STEALTHY)) {
    MakeProfileInvItemAnySlot(p, SILENCER, 100, 1);
  }

  if (HasSkillTrait(p, KNIFING)) {
    MakeProfileInvItemAnySlot(p, COMBAT_KNIFE, 100, 1);
  }

  if (HasSkillTrait(p, CAMOUFLAGED)) {
    MakeProfileInvItemAnySlot(p, CAMOUFLAGEKIT, 100, 1);
  }
}

static int32_t FirstFreeBigEnoughPocket(MERCPROFILESTRUCT const &, uint16_t usItem);

static void MakeProfileInvItemAnySlot(MERCPROFILESTRUCT &p, uint16_t const usItem,
                                      uint8_t const ubStatus, uint8_t const ubHowMany) {
  int32_t const iSlot = FirstFreeBigEnoughPocket(p, usItem);
  if (iSlot == -1) {
    // no room, item not received
    return;
  }

  // put the item into that slot
  MakeProfileInvItemThisSlot(p, iSlot, usItem, ubStatus, ubHowMany);
}

static void MakeProfileInvItemThisSlot(MERCPROFILESTRUCT &p, uint32_t const uiPos,
                                       uint16_t const usItem, uint8_t const ubStatus,
                                       uint8_t const ubHowMany) {
  p.inv[uiPos] = usItem;
  p.bInvStatus[uiPos] = ubStatus;
  p.bInvNumber[uiPos] = ubHowMany;
}

static int32_t FirstFreeBigEnoughPocket(MERCPROFILESTRUCT const &p, uint16_t const usItem) {
  uint32_t uiPos;

  // if it fits into a small pocket
  if (Item[usItem].ubPerPocket != 0) {
    // check small pockets first
    for (uiPos = SMALLPOCK1POS; uiPos <= SMALLPOCK8POS; uiPos++) {
      if (p.inv[uiPos] == NONE) {
        return (uiPos);
      }
    }
  }

  // check large pockets
  for (uiPos = BIGPOCK1POS; uiPos <= BIGPOCK4POS; uiPos++) {
    if (p.inv[uiPos] == NONE) {
      return (uiPos);
    }
  }

  return (-1);
}

static void WriteOutCurrentImpCharacter(int32_t iProfileId) {
  // grab the profile number and write out what is contained there in
  AutoSGPFile hFile(FileMan::openForWriting(IMP_MERC_FILE));

  // Write the profile id, portrait id and the profile itself. Abort on error
  FileWrite(hFile, &iProfileId, sizeof(int32_t));
  FileWrite(hFile, &iPortraitNumber, sizeof(int32_t));
  InjectMercProfileIntoFile(hFile, gMercProfiles[iProfileId]);
}

static void LoadInCurrentImpCharacter() {
  int32_t iProfileId = 0;

  MERCPROFILESTRUCT p;
  ExtractImpProfileFromFile(IMP_MERC_FILE, &iProfileId, &iPortraitNumber, p);
  gMercProfiles[iProfileId] = p;

  if (LaptopSaveInfo.iCurrentBalance < COST_OF_PROFILE) {
    // not enough
    return;
  }

  // charge the player
  // is the character male?
  fCharacterIsMale = (gMercProfiles[iProfileId].bSex == MALE);
  fLoadingCharacterForPreviousImpProfile = TRUE;
  AddTransactionToPlayersBook(IMP_PROFILE, 0, GetWorldTotalMin(), -(COST_OF_PROFILE));
  AddHistoryToPlayersLog(HISTORY_CHARACTER_GENERATED, 0, GetWorldTotalMin(), -1, -1);
  LaptopSaveInfo.iVoiceId = iProfileId - PLAYER_GENERATED_CHARACTER_ID;
  AddCharacterToPlayersTeam();
  AddFutureDayStrategicEvent(EVENT_DAY2_ADD_EMAIL_FROM_IMP, 60 * 7, 0, 2);
  LaptopSaveInfo.fIMPCompletedFlag = TRUE;
  fPausedReDrawScreenFlag = TRUE;
  fLoadingCharacterForPreviousImpProfile = FALSE;
}

void ResetIMPCharactersEyesAndMouthOffsets(const uint8_t ubMercProfileID) {
  // ATE: Check boundary conditions!
  MERCPROFILESTRUCT &p = GetProfile(ubMercProfileID);
  if (p.ubFaceIndex - 200 > 16 || ubMercProfileID >= PROF_HUMMER) return;

  const FacePosInfo *const fi = &g_face_info[p.ubFaceIndex - 200];
  p.usEyesX = fi->eye_x;
  p.usEyesY = fi->eye_y;
  p.usMouthX = fi->mouth_x;
  p.usMouthY = fi->mouth_y;
}
