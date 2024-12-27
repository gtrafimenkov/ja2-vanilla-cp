// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

// *****************************************************************************
//
// Filename :	MouseSystem.h
//
// Purpose :	Defines and typedefs for the "mousesystem" mouse region handler
//
// Modification history :
//
//		30jan97:Bret	-> Creation
//
// *****************************************************************************

#ifndef _MOUSE_SYSTEM_H_
#define _MOUSE_SYSTEM_H_

#include "JA2Types.h"
#include "SGP/Types.h"

#define _JA2_RENDER_DIRTY  // Undef this if not using the JA2 Dirty Rectangle
                           // System.

// Mouse Region Flags
#define MSYS_NO_FLAGS 0x00000000
#define MSYS_MOUSE_IN_AREA 0x00000001
#define MSYS_REGION_EXISTS 0x00000010
#define MSYS_REGION_ENABLED 0x00000040
#define MSYS_FASTHELP 0x00000080
#define MSYS_GOT_BACKGROUND 0x00000100
#define MSYS_HAS_BACKRECT 0x00000200
#define MSYS_FASTHELP_RESET 0x00000400
#define MSYS_ALLOW_DISABLED_FASTHELP 0x00000800

struct MOUSE_REGION;

typedef void (*MOUSE_CALLBACK)(MOUSE_REGION *, int32_t);

struct MOUSE_REGION {
  void ChangeCursor(uint16_t crsr);

  void Enable() { uiFlags |= MSYS_REGION_ENABLED; }
  void Disable() { uiFlags &= ~MSYS_REGION_ENABLED; }

  void SetFastHelpText(wchar_t const *text);

  void AllowDisabledRegionFastHelp(bool allow);

  void SetUserPtr(void *ptr) { user.ptr = ptr; }

  template <typename T>
  T *GetUserPtr() const {
    return static_cast<T *>(user.ptr);
  }

  int16_t X() const { return RegionTopLeftX; }
  int16_t Y() const { return RegionTopLeftY; }
  int16_t W() const { return RegionBottomRightX - RegionTopLeftX; }
  int16_t H() const { return RegionBottomRightY - RegionTopLeftY; }

  int8_t PriorityLevel;    // Region's Priority, set by system and/or caller
  uint32_t uiFlags;        // Region's state flags
  int16_t RegionTopLeftX;  // Screen area affected by this region (absolute
                           // coordinates)
  int16_t RegionTopLeftY;
  int16_t RegionBottomRightX;
  int16_t RegionBottomRightY;
  int16_t MouseXPos;  // Mouse's Coordinates in absolute screen coordinates
  int16_t MouseYPos;
  int16_t RelativeXPos;  // Mouse's Coordinates relative to the Top-Left corner of
                         // the region
  int16_t RelativeYPos;
  uint16_t ButtonState;             // Current state of the mouse buttons
  uint16_t Cursor;                  // Cursor to use when mouse in this region (see flags)
  MOUSE_CALLBACK MovementCallback;  // Pointer to callback function if movement
                                    // occured in this region
  MOUSE_CALLBACK ButtonCallback;    // Pointer to callback function if button
                                    // action occured in this region
  union                             // User Data, can be set to anything!
  {
    int32_t data[4];
    void *ptr;
  } user;

  // Fast help vars.
  int16_t FastHelpTimer;  // Countdown timer for FastHelp text
  wchar_t *FastHelpText;  // Text string for the FastHelp (describes buttons if
                          // left there a while)
  BACKGROUND_SAVE *FastHelpRect;

  MOUSE_REGION *next;  // List maintenance, do NOT touch these entries
  MOUSE_REGION *prev;
};

// Mouse region priorities
#define MSYS_PRIORITY_LOWEST 0
#define MSYS_PRIORITY_LOW 15
#define MSYS_PRIORITY_NORMAL 31
#define MSYS_PRIORITY_HIGH 63
#define MSYS_PRIORITY_HIGHEST 127

// Mouse system defines used during updates
#define MSYS_NO_ACTION 0
#define MSYS_DO_MOVE 1
#define MSYS_DO_LBUTTON_DWN 2
#define MSYS_DO_LBUTTON_UP 4
#define MSYS_DO_RBUTTON_DWN 8
#define MSYS_DO_RBUTTON_UP 16
#define MSYS_DO_LBUTTON_REPEAT 32
#define MSYS_DO_RBUTTON_REPEAT 64
#define MSYS_DO_WHEEL_UP 0x0080
#define MSYS_DO_WHEEL_DOWN 0x0100

#define MSYS_DO_BUTTONS                                                                  \
  (MSYS_DO_LBUTTON_DWN | MSYS_DO_LBUTTON_UP | MSYS_DO_RBUTTON_DWN | MSYS_DO_RBUTTON_UP | \
   MSYS_DO_RBUTTON_REPEAT | MSYS_DO_LBUTTON_REPEAT | MSYS_DO_WHEEL_UP | MSYS_DO_WHEEL_DOWN)

// Mouse system button masks
#define MSYS_LEFT_BUTTON 1
#define MSYS_RIGHT_BUTTON 2

// Mouse system special values
#define MSYS_NO_CALLBACK NULL
#define MSYS_NO_CURSOR 65534

// Mouse system callback reasons
#define MSYS_CALLBACK_REASON_NONE 0
#define MSYS_CALLBACK_REASON_MOVE 2
#define MSYS_CALLBACK_REASON_LBUTTON_DWN 4
#define MSYS_CALLBACK_REASON_LBUTTON_UP 8
#define MSYS_CALLBACK_REASON_RBUTTON_DWN 16
#define MSYS_CALLBACK_REASON_RBUTTON_UP 32
#define MSYS_CALLBACK_REASON_BUTTONS                                    \
  (MSYS_CALLBACK_REASON_LBUTTON_DWN | MSYS_CALLBACK_REASON_LBUTTON_UP | \
   MSYS_CALLBACK_REASON_RBUTTON_DWN | MSYS_CALLBACK_REASON_RBUTTON_UP)
#define MSYS_CALLBACK_REASON_LOST_MOUSE 64
#define MSYS_CALLBACK_REASON_GAIN_MOUSE 128

#define MSYS_CALLBACK_REASON_LBUTTON_REPEAT 256
#define MSYS_CALLBACK_REASON_RBUTTON_REPEAT 512

// Kris:  Nov 31, 1999
// Added support for double clicks.  The DOUBLECLICK event is passed combined
// with the LBUTTON_DWN event if two LBUTTON_DWN events are detected on the same
// button/region within the delay defined by MSYS_DOUBLECLICK_DELAY (in
// milliseconds).  If your button/region supports double clicks and single
// clicks, make sure the DOUBLECLICK event is checked first (rejecting the
// LBUTTON_DWN event if detected)
#define MSYS_CALLBACK_REASON_LBUTTON_DOUBLECLICK 1024

#define MSYS_CALLBACK_REASON_WHEEL_UP 0x0800
#define MSYS_CALLBACK_REASON_WHEEL_DOWN 0x1000

// Internal Functions
void MSYS_SetCurrentCursor(uint16_t Cursor);

// External
void MSYS_Init();
void MSYS_Shutdown();
void MSYS_DefineRegion(MOUSE_REGION *region, uint16_t tlx, uint16_t tly, uint16_t brx, uint16_t bry,
                       int8_t priority, uint16_t crsr, MOUSE_CALLBACK movecallback,
                       MOUSE_CALLBACK buttoncallback);
void MSYS_RemoveRegion(MOUSE_REGION *region);

/* Set one of the user data entries in a mouse region */
void MSYS_SetRegionUserData(MOUSE_REGION *, uint32_t index, int32_t userdata);

/* Retrieve one of the user data entries in a mouse region */
int32_t MSYS_GetRegionUserData(MOUSE_REGION const *, uint32_t index);

// This function will force a re-evaluation of mous regions
// Usually used to force change of mouse cursor if panels switch, etc
void RefreshMouseRegions();

// Now also used by Wizardry -- DB
void RenderFastHelp();

// Hook to the SGP's mouse handler
void MouseSystemHook(uint16_t Type, uint16_t Xcoord, uint16_t Ycoord);

class MouseRegion : private MOUSE_REGION {
 public:
  MouseRegion(uint16_t const x, uint16_t const y, uint16_t const w, uint16_t const h,
              int8_t const priority, uint16_t const cursor, MOUSE_CALLBACK const movecallback,
              MOUSE_CALLBACK const buttoncallback);

  ~MouseRegion() { MSYS_RemoveRegion(this); }

  MOUSE_REGION const &Base() const { return *this; }  // XXX hack

  using MOUSE_REGION::ChangeCursor;
  using MOUSE_REGION::Disable;
  using MOUSE_REGION::Enable;
  using MOUSE_REGION::MouseXPos;
  using MOUSE_REGION::MouseYPos;
  using MOUSE_REGION::PriorityLevel;
  using MOUSE_REGION::RegionBottomRightX;
  using MOUSE_REGION::RegionBottomRightY;
  using MOUSE_REGION::RegionTopLeftX;
  using MOUSE_REGION::RegionTopLeftY;
  using MOUSE_REGION::RelativeXPos;
  using MOUSE_REGION::RelativeYPos;
  using MOUSE_REGION::SetFastHelpText;
  using MOUSE_REGION::SetUserPtr;
  using MOUSE_REGION::uiFlags;
};

#endif
