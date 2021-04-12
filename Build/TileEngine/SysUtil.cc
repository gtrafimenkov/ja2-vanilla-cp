#include "Local.h"
#include "TileEngine/SysUtil.h"
#include "sgp/VSurface.h"


SGPVSurface* guiSAVEBUFFER;
SGPVSurface* guiEXTRABUFFER;


void InitializeGameVideoObjects()
{
	guiSAVEBUFFER  = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
	guiEXTRABUFFER = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
}
