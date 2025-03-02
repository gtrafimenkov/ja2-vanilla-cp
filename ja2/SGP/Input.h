// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INPUT_
#define __INPUT_

#include "SGP/Types.h"

struct SDL_MouseButtonEvent;
struct SDL_MouseWheelEvent;
struct SDL_Keysym;
struct SDL_TextInputEvent;

#define KEY_DOWN 0x0001
#define KEY_UP 0x0002
#define KEY_REPEAT 0x0004
#define TEXT_INPUT 0x0006
#define LEFT_BUTTON_DOWN 0x0008
#define LEFT_BUTTON_UP 0x0010
#define LEFT_BUTTON_DBL_CLK 0x0020
#define LEFT_BUTTON_REPEAT 0x0040
#define RIGHT_BUTTON_DOWN 0x0080
#define RIGHT_BUTTON_UP 0x0100
#define RIGHT_BUTTON_REPEAT 0x0200
#define MOUSE_POS 0x0400
#define MOUSE_WHEEL_UP 0x0800
#define MOUSE_WHEEL_DOWN 0x1000
#define MOUSE_EVENTS 0x1FF8

#define SHIFT_DOWN 0x01
#define CTRL_DOWN 0x02
#define ALT_DOWN 0x04

#define DBL_CLK_TIME 300  // Increased by Alex, Jun-10-97, 200 felt too short
#define BUTTON_REPEAT_TIMEOUT 250
#define BUTTON_REPEAT_TIME 50

struct InputAtom {
  uint16_t usKeyState;
  uint16_t usEvent;
  uint32_t usParam;
  wchar_t Char;
};

extern BOOLEAN DequeueEvent(InputAtom *Event);

void MouseButtonDown(const SDL_MouseButtonEvent *);
void MouseButtonUp(const SDL_MouseButtonEvent *);
void MouseWheelScroll(const SDL_MouseWheelEvent *);

void KeyDown(const SDL_Keysym *);
void KeyUp(const SDL_Keysym *);
void TextInput(const SDL_TextInputEvent *);

extern void GetMousePos(SGPPoint *Point);

extern BOOLEAN DequeueSpecificEvent(InputAtom *Event, uint32_t uiMaskFlags);

extern void RestrictMouseToXYXY(uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2);
void RestrictMouseCursor(const SGPRect *pRectangle);
extern void SetSafeMousePosition(int x, int y);
extern void FreeMouseCursor();
extern BOOLEAN IsCursorRestricted();
extern void GetRestrictedClipCursor(SGPRect *pRectangle);
extern void RestoreCursorClipRect();

void SimulateMouseMovement(uint32_t uiNewXPos, uint32_t uiNewYPos);

void DequeueAllKeyBoardEvents();

extern uint16_t gusMouseXPos;       // X position of the mouse on screen
extern uint16_t gusMouseYPos;       // y position of the mouse on screen
extern BOOLEAN gfLeftButtonState;   // TRUE = Pressed, FALSE = Not Pressed
extern BOOLEAN gfRightButtonState;  // TRUE = Pressed, FALSE = Not Pressed

BOOLEAN IsKeyDown(int a);
#define _LeftButtonDown gfLeftButtonState
#define _RightButtonDown gfRightButtonState

#endif
