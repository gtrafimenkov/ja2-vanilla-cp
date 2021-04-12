#include "Strategic/Game_Clock.h"
#include "TileEngine/Render_Dirty.h"
#include "Tactical/Overhead.h"
#include "TileEngine/Environment.h"
#include "TileEngine/WorldDef.h"
#include "Tactical/Rotting_Corpses.h"
#include "Tactical/Soldier_Create.h"
#include "Tactical/Soldier_Add.h"
#include "Strategic/Strategic_Turns.h"
#include "Tactical/Animation_Data.h"
#include "Tactical/Tactical_Turns.h"
#include "Tactical/Points.h"
#include "TileEngine/Smell.h"
#include "Tactical/OppList.h"
#include "Strategic/Queen_Command.h"
#include "Tactical/Dialogue_Control.h"
#include "TileEngine/SmokeEffects.h"
#include "TileEngine/LightEffects.h"
#include "Tactical/Campaign.h"
#include "Tactical/Soldier_Macros.h"
#include "Strategic/StrategicMap.h"
#include "sgp/Random.h"
#include "TileEngine/Explosion_Control.h"
#include "JAScreens.h"
#include "ScreenIDs.h"
#include "Tactical/Items.h"


void HandleRPCDescription()
{
	TacticalStatusType& ts = gTacticalStatus;
	if (!ts.fCountingDownForGuideDescription) return;

	// ATE: postpone if we are not in tactical
	if (guiCurrentScreen != GAME_SCREEN) return;

	if (ts.uiFlags & ENGAGED_IN_CONV) return;

	// Are we a SAM site?
	if (ts.ubGuideDescriptionToUse == 25 ||
			ts.ubGuideDescriptionToUse == 27 ||
			ts.ubGuideDescriptionToUse == 30 ||
			ts.ubGuideDescriptionToUse == 31 ||
			ts.ubGuideDescriptionToUse == 32)
	{
		ts.bGuideDescriptionCountDown = 1;
	}
	else
	{
		// ATE; Don't do in combat
		if (ts.uiFlags & INCOMBAT) return;

		// Don't do if enemy in sector
		if (NumEnemyInSector()) return;
	}

	if (--ts.bGuideDescriptionCountDown != 0) return;
	ts.fCountingDownForGuideDescription = FALSE;

	// Count how many RPC guys we have
	UINT8        n_mercs = 0;
	SOLDIERTYPE* mercs_in_sector[20];
	FOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		// Add guy if he's a candidate
		if (!RPC_RECRUITED(s))                          continue;
		if (s->bLife < OKLIFE)                          continue;
		if (s->sSectorX != ts.bGuideDescriptionSectorX) continue;
		if (s->sSectorY != ts.bGuideDescriptionSectorY) continue;
		if (s->bSectorZ != gbWorldSectorZ)              continue;
		if (s->fBetweenSectors)                         continue;

		if (s->ubProfile == IRA    ||
				s->ubProfile == MIGUEL ||
				s->ubProfile == CARLOS ||
				s->ubProfile == DIMITRI)
		{
			mercs_in_sector[n_mercs++] = s;
		}
	}

	if (n_mercs == 0) return;

	SOLDIERTYPE& chosen = *mercs_in_sector[Random(n_mercs)];
	CharacterDialogueUsingAlternateFile(chosen, ts.ubGuideDescriptionToUse, DIALOGUE_TACTICAL_UI);
}


void HandleTacticalEndTurn()
{
	static UINT32 uiTimeSinceLastStrategicUpdate = 0;

	UINT32 const now = GetWorldTotalSeconds();

	if (uiTimeSinceLastStrategicUpdate - now > 1200)
	{
		HandleRottingCorpses();
		uiTimeSinceLastStrategicUpdate = now;
	}

	DecayBombTimers();
	DecaySmokeEffects(now);
	DecayLightEffects(now);
	DecayBloodAndSmells(now);
	DecayRottingCorpseAIWarnings();

	/* Check for enemy pooling: Add enemies if there happens to be more than the
	 * max in the current battle. If one or more slots have freed up, we can add
	 * them now. */
	AddPossiblePendingEnemiesToBattle();

	// Loop through each active team and decay public opplist
	// May want this done every few times too
	NonCombatDecayPublicOpplist(now);

	// First exit if we are not in realtime combat or realtime noncombat
	if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT))
	{
		BeginLoggingForBleedMeToos(TRUE);

		/* First pass:
		 * Loop through our own mercs:
		 *	Check things like (even if not in our sector)
		 *		1. All updates of breath, shock, bleeding, etc.
		 *    2. Updating First aid, etc.
		 *  (If in our sector:)
		 *		3. Update things like decayed opplist, etc.
		 * Second pass:
		 *  Loop through all mercs in tactical engine
		 *  If not a player merc (ubTeam), do things like 1, 2, 3 above
		 */

		FOR_EACH_IN_TEAM(i, OUR_TEAM)
		{
			SOLDIERTYPE& s = *i;
			if (s.bLife <= 0)    continue;
			if (IsMechanical(s)) continue;

			// Handle everything from getting breath back, to bleeding, etc.
			EVENT_BeginMercTurn(s);

			HandlePlayerServices(s);

			// If time is up, turn off xray
			if (s.uiXRayActivatedTime != 0 && now > s.uiXRayActivatedTime + XRAY_TIME)
			{
				TurnOffXRayEffects(&s);
			}
		}

		BeginLoggingForBleedMeToos(FALSE);

		/* We're looping through only mercs in tactical engine, ignoring our mercs
		 * because they were done earlier */
		FOR_EACH_MERC(i)
		{
			SOLDIERTYPE& s = **i;
			if (s.bTeam == OUR_TEAM) continue;

			// Handle everything from getting breath back, to bleeding, etc.
			EVENT_BeginMercTurn(s);

			HandlePlayerServices(s);
		}
	}

	HandleRPCDescription();
}
