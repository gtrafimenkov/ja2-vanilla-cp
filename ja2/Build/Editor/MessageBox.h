#ifndef __MESSAGEBOX_H
#define __MESSAGEBOX_H

#include "SGP/Types.h"

enum { MESSAGEBOX_NONE, MESSAGEBOX_DONE, MESSAGEBOX_WAIT };

extern uint8_t gubMessageBoxStatus;
extern BOOLEAN gfMessageBoxResult;

void CreateMessageBox(wchar_t const *msg);
BOOLEAN MessageBoxHandled();
void RemoveMessageBox();

// NOTES:
// Rewrote the damn thing.  It is better now because you have less management
// issues. In your screen's main loop, instead of using the previous method, you
// would use the following syntax:
//
//	if( gubMessageBoxState )
//	{
//		if( MessageBoxHandled() )
//			return ProcessMyOwnMessageBoxResultHandler();
//		return MYCURRENT_SCREEN;
//	}
//
//  uint32_t ProcessMyOwnMessageBoxResultHandlerFunction()
//	{
//		RemoveMessageBox();  //MUST BE HERE
//		if( gfMessageBoxResult ) //user selected yes
//		{
//			...
//			return DESIRED_SCREEN;
//		}
//		else //user selected no
//		...
//	}

#endif
