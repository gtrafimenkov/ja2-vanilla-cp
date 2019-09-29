#include "Build/Strategic/Strategic.h"
#include "Build/Strategic/MapScreen.h"
#include "sgp/Types.h"
#include "Build/Tactical/Squads.h"
#include "Build/Strategic/Assignments.h"
#include "Build/Tactical/Overhead.h"
#include "Build/Tactical/Soldier_Profile.h"
#include "Build/Tactical/Dialogue_Control.h"
#include "Build/Laptop/Personnel.h"
#include "Build/Tactical/Tactical_Save.h"
#include "Build/TileEngine/Isometric_Utils.h"
#include "Build/Tactical/Vehicles.h"
#include "Build/Strategic/Game_Clock.h"


static void HandleSoldierDeadComments(SOLDIERTYPE const*);


void HandleStrategicDeath(SOLDIERTYPE& s)
{
	if (s.bAssignment == VEHICLE && s.iVehicleId != -1)
	{
		TakeSoldierOutOfVehicle(&s);
  }

	RemoveCharacterFromSquads(&s);
	if (s.bAssignment != ASSIGNMENT_DEAD)
	{
		SetTimeOfAssignmentChangeForMerc(&s);
	}
	ChangeSoldiersAssignment(&s, ASSIGNMENT_DEAD);

	if (fInMapMode)
	{
		ReBuildCharactersList();

		HandleSoldierDeadComments(&s);
		AddDeadSoldierToUnLoadedSector(s.sSectorX, s.sSectorY, s.bSectorZ, &s, RandomGridNo(), ADD_DEAD_SOLDIER_TO_SWEETSPOT);

		fReDrawFace = TRUE;

		StopTimeCompression();
	}
}


static void HandleSoldierDeadComments(SOLDIERTYPE const* const dead)
{
	FOR_EACH_IN_TEAM(s, dead->bTeam)
	{
		if (s->bLife < OKLIFE) continue;

		UINT16     quote_num;
		INT8 const buddy_idx = WhichBuddy(s->ubProfile, dead->ubProfile);
		switch (buddy_idx)
		{
			case 0: quote_num = QUOTE_BUDDY_ONE_KILLED;            break;
			case 1: quote_num = QUOTE_BUDDY_TWO_KILLED;            break;
			case 2: quote_num = QUOTE_LEARNED_TO_LIKE_MERC_KILLED; break;

			default: continue;
		}
		TacticalCharacterDialogue(s, quote_num);
	}
}
