#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H


enum LoadingScreenID
{
	LOADINGSCREEN_NOTHING,
	LOADINGSCREEN_DAYGENERIC,
	LOADINGSCREEN_DAYTOWN1,
	LOADINGSCREEN_DAYTOWN2,
	LOADINGSCREEN_DAYWILD,
	LOADINGSCREEN_DAYTROPICAL,
	LOADINGSCREEN_DAYFOREST,
	LOADINGSCREEN_DAYDESERT,
	LOADINGSCREEN_DAYPALACE,
	LOADINGSCREEN_NIGHTGENERIC,
	LOADINGSCREEN_NIGHTWILD,
	LOADINGSCREEN_NIGHTTOWN1,
	LOADINGSCREEN_NIGHTTOWN2,
	LOADINGSCREEN_NIGHTFOREST,
	LOADINGSCREEN_NIGHTTROPICAL,
	LOADINGSCREEN_NIGHTDESERT,
	LOADINGSCREEN_NIGHTPALACE,
	LOADINGSCREEN_HELI,
	LOADINGSCREEN_BASEMENT,
	LOADINGSCREEN_MINE,
	LOADINGSCREEN_CAVE,
	LOADINGSCREEN_DAYPINE,
	LOADINGSCREEN_NIGHTPINE,
	LOADINGSCREEN_DAYMILITARY,
	LOADINGSCREEN_NIGHTMILITARY,
	LOADINGSCREEN_DAYSAM,
	LOADINGSCREEN_NIGHTSAM,
	LOADINGSCREEN_DAYPRISON,
	LOADINGSCREEN_NIGHTPRISON,
	LOADINGSCREEN_DAYHOSPITAL,
	LOADINGSCREEN_NIGHTHOSPITAL,
	LOADINGSCREEN_DAYAIRPORT,
	LOADINGSCREEN_NIGHTAIRPORT,
	LOADINGSCREEN_DAYLAB,
	LOADINGSCREEN_NIGHTLAB,
	LOADINGSCREEN_DAYOMERTA,
	LOADINGSCREEN_NIGHTOMERTA,
	LOADINGSCREEN_DAYCHITZENA,
	LOADINGSCREEN_NIGHTCHITZENA,
	LOADINGSCREEN_DAYMINE,
	LOADINGSCREEN_NIGHTMINE,
	LOADINGSCREEN_DAYBALIME,
	LOADINGSCREEN_NIGHTBALIME,
};


// For use by the game loader, before it can possibly know the situation.
extern LoadingScreenID gubLastLoadingScreenID;

// Return the loading screen ID for the specified sector.
LoadingScreenID GetLoadScreenID(uint16_t x, uint16_t y, uint8_t z);

/* Set up the loadscreen with specified ID, draw it to the FRAME_BUFFER, and
 * refresh the screen with it. */
void DisplayLoadScreenWithID(LoadingScreenID);

#endif
