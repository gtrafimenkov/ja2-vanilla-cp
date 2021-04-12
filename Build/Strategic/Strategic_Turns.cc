#include "Strategic/Game_Clock.h"
#include "TileEngine/Render_Dirty.h"
#include "Utils/Timer_Control.h"
#include "Tactical/Overhead.h"
#include "TileEngine/Environment.h"
#include "TileEngine/WorldDef.h"
#include "Tactical/Rotting_Corpses.h"
#include "Tactical/Soldier_Create.h"
#include "Tactical/Soldier_Add.h"
#include "Strategic/Strategic_Turns.h"
#include "Tactical/Animation_Data.h"
#include "Tactical/Tactical_Turns.h"
#include "Tactical/RT_Time_Defines.h"
#include "Strategic/Assignments.h"
#include "JAScreens.h"
#include "ScreenIDs.h"


#define	NUM_SEC_PER_STRATEGIC_TURN					( NUM_SEC_IN_MIN * 15 )	// Every fifteen minutes


static UINT32 guiLastTacticalRealTime = 0;


void StrategicTurnsNewGame(void)
{
	// Sync game start time
	SyncStrategicTurnTimes( );
}


void SyncStrategicTurnTimes(void)
{
	guiLastTacticalRealTime =  GetJA2Clock( );
}


void HandleStrategicTurn(void)
{
	UINT32	uiTime;
	UINT32	uiCheckTime; // XXX HACK000E

	// OK, DO THIS CHECK EVERY ONCE AND A WHILE...
	if ( COUNTERDONE( STRATEGIC_OVERHEAD ) )
	{
		RESETCOUNTER( STRATEGIC_OVERHEAD );

		// if the game is paused, or we're in mapscreen and time is not being compressed
		if (GamePaused() ||
				( ( guiCurrentScreen == MAP_SCREEN ) && !IsTimeBeingCompressed() ) )
		{
			// don't do any of this
			return;
		}

		//Kris -- What to do?
		if( giTimeCompressMode == NOT_USING_TIME_COMPRESSION )
		{
			SetGameTimeCompressionLevel( TIME_COMPRESS_X1 );
		}


		uiTime = GetJA2Clock( );

		// Do not handle turns update if in turnbased combat
		if ( ( gTacticalStatus.uiFlags & TURNBASED ) && ( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			guiLastTacticalRealTime = uiTime;
		}
		else
		{
			if ( giTimeCompressMode == TIME_COMPRESS_X1 || giTimeCompressMode == 0 )
			{
				uiCheckTime = NUM_REAL_SEC_PER_TACTICAL_TURN;
			}
			else
			{
				// OK, if we have compressed time...., adjust our check value to be faster....
				if( giTimeCompressSpeeds[ giTimeCompressMode ] > 0 )
				{
				  uiCheckTime = NUM_REAL_SEC_PER_TACTICAL_TURN / ( giTimeCompressSpeeds[ giTimeCompressMode ] * RT_COMPRESSION_TACTICAL_TURN_MODIFIER );
				}
				else
				{
					abort(); // XXX HACK000E
				}
			}

			if ( ( uiTime - guiLastTacticalRealTime ) > uiCheckTime )
			{
				HandleTacticalEndTurn( );

				guiLastTacticalRealTime = uiTime;
			}
		}

	}
}


void HandleStrategicTurnImplicationsOfExitingCombatMode( void )
{
	SyncStrategicTurnTimes();
	HandleTacticalEndTurn();
}
