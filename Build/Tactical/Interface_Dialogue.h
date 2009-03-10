#ifndef __INTERFACE_DIALOGUE_H
#define __INTERFACE_DIALOGUE_H

#include "Button_System.h"
#include "MouseSystem.h"
#include "NPC.h"


extern UINT8 gubSrcSoldierProfile;


// Structure used in Tactical display of NPC dialogue
struct NPC_DIALOGUE_TYPE
{
	FACETYPE*     face;
	INT16					sX;
	INT16					sY;
	INT16					sPopupX;
	INT16					sPopupY;
	UINT8					ubPopupOrientation;
	UINT8					ubCharNum;
	SGPVObject*   uiPanelVO;
	BUTTON_PICS*  iButtonImages;
	GUIButtonRef  uiCancelButton;
	INT8					bCurSelect;
	INT8					bOldCurSelect;
	UINT16				usWidth;
	UINT16				usHeight;
	MOUSE_REGION	Regions[ 6 ];
	MOUSE_REGION	BackRegion;
	MOUSE_REGION	NameRegion;
	MOUSE_REGION	ScreenRegion;
	MOUSE_REGION	TextRegion;
	BOOLEAN				fTextRegionOn;
	BOOLEAN				fOnName;
	SGPVSurface*  uiSaveBuffer;
	BOOLEAN				fHandled;
	BOOLEAN				fHandledTalkingVal;
	BOOLEAN				fHandledCanDeleteVal;
	BOOLEAN				fRenderSubTitlesNow;
	BOOLEAN				fSetupSubTitles;
	wchar_t				zQuoteStr[ 480 ];			//QIALOGUE_SIZE is in dialog control which includes this file...
};


// GLOBAL NPC STRUCT
extern NPC_DIALOGUE_TYPE gTalkPanel;


BOOLEAN InitiateConversationFull(SOLDIERTYPE* pDestSoldier, SOLDIERTYPE* pSrcSoldier, Approach bApproach, UINT8 approach_record, OBJECTTYPE* approach_object);
BOOLEAN InitiateConversation(SOLDIERTYPE* pDestSoldier, SOLDIERTYPE* pSrcSoldier, Approach);

// Begins quote of NPC Dialogue
void TalkingMenuDialogue(UINT16 usQuoteNum);


// Removes memory allocated for structure, removes face...
void DeleteTalkingMenu(void);

BOOLEAN HandleTalkingMenu(void);

void RenderTalkingMenu(void);

BOOLEAN HandleTalkingMenuEscape( BOOLEAN fCanDelete , BOOLEAN fFromEscKey );

// NPC goto gridno
void NPCGotoGridNo(ProfileID ubTargetNPC, UINT16 usGridNo, UINT8 ubQuoteNum);
// NPC Do action
void NPCDoAction(UINT8 ubTargetNPC, UINT16 usActionCode, UINT8 ubQuoteNum);

void NPCClosePanel(void);

void HandleWaitTimerForNPCTrigger(void);


void HandleNPCItemGiven( UINT8 ubNPC, OBJECTTYPE *pObject, INT8 bInvPos );
void HandleNPCTriggerNPC(UINT8 ubTargetNPC, UINT8 ubTargetRecord, BOOLEAN fShowDialogueMenu, Approach ubTargetApproach);
void HandleNPCDoAction( UINT8 ubTargetNPC, UINT16 usActionCode, UINT8 ubQuoteNum );

bool ProfileCurrentlyTalkingInDialoguePanel(UINT8 ubProfile);

void InternalInitTalkingMenu(UINT8 ubCharacterNum, INT16 sX, INT16 sY);


enum
{
	NPC_ACTION_NONE = 0,
	NPC_ACTION_DONT_ACCEPT_ITEM,
	NPC_ACTION_FACE_CLOSEST_PLAYER,
	NPC_ACTION_OPEN_CLOSEST_DOOR,
	NPC_ACTION_RECRUIT,
	NPC_ACTION_THREATENINGLY_RAISE_GUN,
	NPC_ACTION_LOWER_GUN,
	NPC_ACTION_READY_GUN,
	NPC_ACTION_START_RUNNING,
	NPC_ACTION_STOP_RUNNING,
	NPC_ACTION_BOOST_TOWN_LOYALTY, // 10
	NPC_ACTION_PENALIZE_TOWN_LOYALTY,
	NPC_ACTION_STOP_PLAYER_GIVING_FIRST_AID,
	NPC_ACTION_FACE_NORTH,
	NPC_ACTION_FACE_NORTH_EAST,
	NPC_ACTION_FACE_EAST,
	NPC_ACTION_FACE_SOUTH_EAST,
	NPC_ACTION_FACE_SOUTH,
	NPC_ACTION_FACE_SOUTH_WEST,
	NPC_ACTION_FACE_WEST,
	NPC_ACTION_FACE_NORTH_WEST, // 20
	NPC_ACTION_TRIGGER_FRIEND_WITH_HOSTILE_QUOTE,
	NPC_ACTION_BECOME_ENEMY,
	NPC_ACTION_RECRUIT_WITH_SALARY,
	NPC_ACTION_CLOSE_DIALOGUE_PANEL,
	NPC_ACTION_ENTER_COMBAT, // 25
	NPC_ACTION_TERRORIST_REVEALS_SELF,
	NPC_ACTION_OPEN_CLOSEST_CABINET,
	NPC_ACTION_SLAP,
	NPC_ACTION_TRIGGER_QUEEN_BY_CITIES_CONTROLLED,
	NPC_ACTION_SEND_SOLDIERS_TO_DRASSEN, // 30
	NPC_ACTION_SEND_SOLDIERS_TO_BATTLE_LOCATION,
	NPC_ACTION_TRIGGER_QUEEN_BY_SAM_SITES_CONTROLLED,
	NPC_ACTION_PUNCH_PC_SLOT_0,
	NPC_ACTION_PUNCH_PC_SLOT_1,
	NPC_ACTION_PUNCH_PC_SLOT_2,				// 35
	NPC_ACTION_FRUSTRATED_SLAP,
	NPC_ACTION_PUNCH_FIRST_LIVING_PC,
	NPC_ACTION_SHOOT_ELLIOT,
	NPC_ACTION_PLAYER_SAYS_NICE_LATER,
	NPC_ACTION_GET_ITEMS_FROM_CLOSEST_CABINET, // 40
	NPC_ACTION_INITIATE_SHOPKEEPER_INTERFACE,
	NPC_ACTION_GET_OUT_OF_WHEELCHAIR,
	NPC_ACTION_GET_OUT_OF_WHEELCHAIR_AND_BECOME_HOSTILE,
	NPC_ACTION_PLAYER_SAYS_NASTY_LATER, // 44

	NPC_ACTION_GRANT_EXPERIENCE_1 = 50,
	NPC_ACTION_GRANT_EXPERIENCE_2,
	NPC_ACTION_GRANT_EXPERIENCE_3,
	NPC_ACTION_GRANT_EXPERIENCE_4,
	NPC_ACTION_GRANT_EXPERIENCE_5,

	NPC_ACTION_GOTO_HIDEOUT = 100,
	NPC_ACTION_FATIMA_GIVE_LETTER,
	NPC_ACTION_LEAVE_HIDEOUT,
	NPC_ACTION_TRAVERSE_MAP_EAST,
	NPC_ACTION_TRAVERSE_MAP_SOUTH,
	NPC_ACTION_TRAVERSE_MAP_WEST,
	NPC_ACTION_TRAVERSE_MAP_NORTH,
	NPC_ACTION_REPORT_SHIPMENT_SIZE,
	NPC_ACTION_RETURN_STOLEN_SHIPMENT_ITEMS,
	NPC_ACTION_SET_PABLO_BRIBE_DELAY,
	NPC_ACTION_ASK_ABOUT_ESCORTING_EPC, // 110
	NPC_ACTION_DRINK_DRINK_DRINK,
	NPC_ACTION_TRIGGER_END_OF_FOOD_QUEST,
	NPC_ACTION_SEND_PACOS_INTO_HIDING,
	NPC_ACTION_HAVE_PACOS_FOLLOW,
	NPC_ACTION_SET_DELAYED_PACKAGE_TIMER, // 115
	NPC_ACTION_SET_RANDOM_PACKAGE_DAMAGE_TIMER,
	NPC_ACTION_FREE_KIDS,
	NPC_ACTION_CHOOSE_DOCTOR,
	NPC_ACTION_REPORT_BALANCE,
	NPC_ACTION_ASK_ABOUT_PAYING_RPC,
	NPC_ACTION_DELAYED_MAKE_BRENDA_LEAVE,
	NPC_ACTION_SEX,
	NPC_ACTION_KYLE_GETS_MONEY,
	NPC_ACTION_LAYLA_GIVEN_WRONG_AMOUNT_OF_CASH, // 124
	NPC_ACTION_SET_GIRLS_AVAILABLE,
	NPC_ACTION_SET_DELAY_TILL_GIRLS_AVAILABLE,
	NPC_ACTION_SET_WAITED_FOR_GIRL_FALSE,
	NPC_ACTION_TRIGGER_LAYLA_13_14_OR_15,
	NPC_ACTION_OPEN_CARLAS_DOOR,
	NPC_ACTION_OPEN_CINDYS_DOOR, // 130
	NPC_ACTION_OPEN_BAMBIS_DOOR,
	NPC_ACTION_OPEN_MARIAS_DOOR,
	NPC_ACTION_POSSIBLY_ADVERTISE_CINDY,
	NPC_ACTION_POSSIBLY_ADVERTISE_BAMBI,
	NPC_ACTION_DARREN_REQUESTOR, // 135
	NPC_ACTION_ADD_JOEY_TO_WORLD,
	NPC_ACTION_MARK_KINGPIN_QUOTE_0_USED,
	NPC_ACTION_START_BOXING_MATCH,
	NPC_ACTION_ENABLE_CAMBRIA_DOCTOR_BONUS,// OBSOLETE, NO LONGER DELAYED
	NPC_ACTION_MARTHA_DIES, // 140
	NPC_ACTION_DARREN_GIVEN_CASH,
	NPC_ACTION_ANGEL_GIVEN_CASH,
	NPC_ACTION_TRIGGER_ANGEL_17_OR_18,
	NPC_ACTION_BUY_LEATHER_KEVLAR_VEST,
	NPC_ACTION_TRIGGER_MARIA, // 145
	NPC_ACTION_TRIGGER_ANGEL_16_OR_19,
	NPC_ACTION_ANGEL_LEAVES_DEED,
	NPC_ACTION_TRIGGER_ANGEL_21_OR_22,
	NPC_ACTION_UN_RECRUIT_EPC,
	NPC_ACTION_TELEPORT_NPC, // 150
	NPC_ACTION_REMOVE_DOREEN,
	NPC_ACTION_RESET_SHIPMENT_ARRIVAL_STUFF,
	// 153 Fix helicopter by next morning?
	NPC_ACTION_DECIDE_ACTIVE_TERRORISTS = 154,
	NPC_ACTION_TRIGGER_FATHER_18_20_OR_15,
	NPC_ACTION_CHECK_LAST_TERRORIST_HEAD,
	NPC_ACTION_CARMEN_LEAVES_FOR_C13,
	NPC_ACTION_CARMEN_LEAVES_FOR_GOOD,
	NPC_ACTION_CARMEN_LEAVES_ON_NEXT_SECTOR_LOAD,
	NPC_ACTION_TRIGGER_VINCE_BY_LOYALTY, // 160
	NPC_ACTION_MEDICAL_REQUESTOR,
	NPC_ACTION_MEDICAL_REQUESTOR_2,
	NPC_ACTION_CHECK_DOCTORING_MONEY_GIVEN, // handled in NPC.c
	NPC_ACTION_START_DOCTORING,
	NPC_ACTION_VINCE_UNRECRUITABLE, // 165
	NPC_ACTION_END_COMBAT,
	NPC_ACTION_BECOME_FRIENDLY_END_COMBAT,
	NPC_ACTION_SET_EPC_TO_NPC,
	NPC_ACTION_BUY_VEHICLE_REQUESTOR,
	NPC_ACTION_END_MEANWHILE,// 170
	NPC_ACTION_START_BLOODCAT_QUEST,
	NPC_ACTION_START_MINE,
	NPC_ACTION_STOP_MINE,
	NPC_ACTION_RESET_MINE_CAPTURED,
	NPC_ACTION_SET_OSWALD_RECORD_13_USED, // 175
	NPC_ACTION_SET_CALVIN_RECORD_13_USED,
	NPC_ACTION_SET_CARL_RECORD_13_USED,
	NPC_ACTION_SET_FRED_RECORD_13_USED,
	NPC_ACTION_SET_MATT_RECORD_13_USED,
	NPC_ACTION_TRIGGER_MATT, // 180
	NPC_ACTION_REDUCE_CONRAD_SALARY_CONDITIONS,
	NPC_ACTION_REMOVE_CONRAD,
	NPC_ACTION_KROTT_REQUESTOR,
	NPC_ACTION_KROTT_ALIVE_LOYALTY_BOOST,		/* Delayed loyalty effects elimininated.  Sep.12/98.  ARM */
	NPC_ACTION_TRIGGER_YANNI, // 185
	NPC_ACTION_TRIGGER_MARY_OR_JOHN_RECORD_9,
	NPC_ACTION_TRIGGER_MARY_OR_JOHN_RECORD_10,
	NPC_ACTION_ADD_JOHNS_GUN_SHIPMENT,
	// 189 ??
	NPC_ACTION_TRIGGER_KROTT_11_OR_12 = 190, // 190 Trigger record 11 or 12 for Krott
	NPC_ACTION_MADLAB_GIVEN_GUN = 191,
	NPC_ACTION_MADLAB_GIVEN_CAMERA,
	NPC_ACTION_MADLAB_ATTACHES_GOOD_CAMERA,
	NPC_ACTION_READY_ROBOT,
	NPC_ACTION_WALTER_GIVEN_MONEY_INITIALLY,
	NPC_ACTION_WALTER_GIVEN_MONEY,
	NPC_ACTION_MAKE_NPC_FIRST_BARTENDER = 197,
	NPC_ACTION_MAKE_NPC_SECOND_BARTENDER,
	NPC_ACTION_MAKE_NPC_THIRD_BARTENDER,
	NPC_ACTION_MAKE_NPC_FOURTH_BARTENDER,//200
	NPC_ACTION_GERARD_GIVEN_CASH,
	NPC_ACTION_FILL_UP_CAR, // obsolete?
	NPC_ACTION_JOE_GIVEN_CASH,
	NPC_ACTION_TRIGGER_ELLIOT_9_OR_10 = 204, // obsolete?
	NPC_ACTION_HANDLE_END_OF_FIGHT,// 205
	NPC_ACTION_DARREN_PAYS_PLAYER,
	NPC_ACTION_FIGHT_AGAIN_REQUESTOR,
	NPC_ACTION_TRIGGER_SPIKE_OR_DARREN,
	// 209 is blank
	NPC_ACTION_CHANGE_MANNY_POSITION = 210,
	NPC_ACTION_TIMER_FOR_VEHICLE, // 211
	NPC_ACTION_ASK_ABOUT_PAYING_RPC_WITH_DAILY_SALARY,//212
  NPC_ACTION_TRIGGER_MICKY_BY_SCI_FI, // 213
	// 214 is blank
	NPC_ACTION_TRIGGER_ELLIOT_BY_BATTLE_RESULT = 215,
	NPC_ACTION_TRIGGER_ELLIOT_BY_SAM_DISABLED,
	NPC_ACTION_LAYLAS_NEXT_LINE_AFTER_CARLA,
	NPC_ACTION_LAYLAS_NEXT_LINE_AFTER_CINDY,
	NPC_ACTION_LAYLAS_NEXT_LINE_AFTER_BAMBI,
	NPC_ACTION_LAYLAS_NEXT_LINE_AFTER_MARIA,
	NPC_ACTION_PROMPT_PLAYER_TO_LIE,						// 221
	NPC_ACTION_REMOVE_JOE_QUEEN,							// 222
	NPC_ACTION_REMOVE_ELLIOT_END_MEANWHILE,		// 223
	NPC_ACTION_NO_SCI_FI_END_MEANWHILE,				//224
	// 225 is obsolete
	NPC_ACTION_TRIGGER_MARRY_DARYL_PROMPT = 226,
	NPC_ACTION_HAVE_MARRIED_NPC_LEAVE_TEAM,
	NPC_ACTION_KINGPIN_GIVEN_MONEY, // actually handled in item-acceptance code, NPC.c
	NPC_ACTION_KINGPIN_TRIGGER_25_OR_14,
	NPC_ACTION_SEND_ENRICO_MIGUEL_EMAIL,
	NPC_ACTION_END_DEMO,// 231,
	NPC_ACTION_INVOKE_CONVERSATION_MODE, // 232
	// 233 is obsolete
	NPC_ACTION_START_TIMER_ON_KEITH_GOING_OUT_OF_BUSINESS = 234,
	NPC_ACTION_KEITH_GOING_BACK_IN_BUSINESS,
	NPC_ACTION_MAKE_RAT_DISAPPEAR,
	NPC_ACTION_DOCTOR_ESCORT_PATIENTS,
	NPC_ACTION_ELLIOT_DECIDE_WHICH_QUOTE_FOR_PLAYER_ATTACK,
	NPC_ACTION_QUEEN_DECIDE_WHICH_QUOTE_FOR_PLAYER_ATTACK,
	NPC_ACTION_CANCEL_WAYPOINTS, // 240
	// 241 currently obsolete, CJC Oct 14
	NPC_ACTION_SHOW_TIXA = 242,
	NPC_ACTION_SHOW_ORTA,
	NPC_ACTION_DRINK_WINE,
	NPC_ACTION_DRINK_BOOZE,
	NPC_ACTION_TRIGGER_ANGEL_22_OR_24,
	NPC_ACTION_SET_FACT_105_FALSE, // 247
	NPC_ACTION_MAKE_BRENDA_STATIONARY, // 248
	NPC_ACTION_TRIGGER_DARREN_OR_KINGPIN_IMPRESSED = 249,
	NPC_ACTION_TRIGGER_KINGPIN_IMPRESSED, // 250
	NPC_ACTION_ADD_RAT,

	NPC_ACTION_ENDGAME_STATE_1 = 253,
	NPC_ACTION_ENDGAME_STATE_2,
	NPC_ACTION_MAKE_MIGUEL_STATIONARY,
	NPC_ACTION_MAKE_ESTONI_A_FUEL_SITE, // 256
	NPC_ACTION_24_HOURS_SINCE_JOEY_RESCUED,// 257
	NPC_ACTION_24_HOURS_SINCE_DOCTORS_TALKED_TO,// 258
	NPC_ACTION_SEND_SOLDIERS_TO_OMERTA,// 259
	NPC_ACTION_ADD_MORE_ELITES,// 260
	NPC_ACTION_GIVE_KNOWLEDGE_OF_ALL_MERCS,// 261
	NPC_ACTION_REMOVE_MERC_FOR_MARRIAGE,
	NPC_ACTION_TRIGGER_JOE_32_OR_33,
	NPC_ACTION_REMOVE_NPC,
	NPC_ACTION_HISTORY_GOT_ROCKET_RIFLES,
	NPC_ACTION_HISTORY_DEIDRANNA_DEAD_BODIES,
	NPC_ACTION_HISTORY_BOXING_MATCHES,
	NPC_ACTION_HISTORY_SOMETHING_IN_MINES,
	NPC_ACTION_HISTORY_DEVIN,
	NPC_ACTION_HISTORY_MIKE,// 270
	NPC_ACTION_HISTORY_TONY,
	NPC_ACTION_HISTORY_KROTT,
	NPC_ACTION_HISTORY_KYLE,
	NPC_ACTION_HISTORY_MADLAB,
	NPC_ACTION_HISTORY_GABBY,
	NPC_ACTION_HISTORY_KEITH_OUT_OF_BUSINESS,
	NPC_ACTION_HISTORY_HOWARD_CYANIDE,
	NPC_ACTION_HISTORY_KEITH,
	NPC_ACTION_HISTORY_HOWARD,
	NPC_ACTION_HISTORY_PERKO,// 280
	NPC_ACTION_HISTORY_SAM,
	NPC_ACTION_HISTORY_FRANZ,
	NPC_ACTION_HISTORY_ARNOLD,
	NPC_ACTION_HISTORY_FREDO,
	NPC_ACTION_HISTORY_RICHGUY_BALIME,// 285
	NPC_ACTION_HISTORY_JAKE,
	NPC_ACTION_HISTORY_BUM_KEYCARD,
	NPC_ACTION_HISTORY_WALTER,
	NPC_ACTION_HISTORY_DAVE,
	NPC_ACTION_HISTORY_PABLO,// 290
	NPC_ACTION_HISTORY_KINGPIN_MONEY,
	NPC_ACTION_SEND_TROOPS_TO_SAM,
	NPC_ACTION_PUT_PACOS_IN_BASEMENT,
	NPC_ACTION_HISTORY_ASSASSIN,
	NPC_ACTION_TRIGGER_HANS_BY_ROOM, // 295
	NPC_ACTION_TRIGGER_MADLAB_31,
	NPC_ACTION_TRIGGER_MADLAB_32,
	NPC_ACTION_TRIGGER_BREWSTER_BY_WARDEN_PROXIMITY, // 298

	NPC_ACTION_TURN_TO_FACE_NEAREST_MERC = 500,
	NPC_ACTION_TURN_TO_FACE_PROFILE_ID_0,

	NPC_ACTION_LAST_TURN_TO_FACE_PROFILE = 650
};

#define HOSPITAL_PATIENT_DISTANCE 9

extern INT32 giHospitalTempBalance;
extern INT32 giHospitalRefund;
extern INT8 gbHospitalPriceModifier;

UINT32 CalcPatientMedicalCost(const SOLDIERTYPE* s);
extern UINT32 CalcMedicalCost( UINT8 ubId );

extern BOOLEAN gfInTalkPanel;

void HandleTalkingMenuBackspace(void);

void HandlePendingInitConv(void);

extern BOOLEAN       gfWaitingForTriggerTimer;
extern MercPopUpBox* g_interface_dialogue_box;

#endif
