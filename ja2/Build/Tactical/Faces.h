#ifndef __TALKING_H_
#define __TALKING_H_

#include "JA2Types.h"

#define NO_EXPRESSION 0
#define BLINKING 1
#define ANGRY 2
#define SURPRISED 3

// Defines
#define NUM_FACE_SLOTS 50

#define FACE_AUTO_DISPLAY_BUFFER 0
#define FACE_AUTO_RESTORE_BUFFER 0

// FLAGS....
#define FACE_BIGFACE 0x00000001  // A BIGFACE instead of small face
#define FACE_POTENTIAL_KEYWAIT \
  0x00000002                           // If the option is set, will not stop face until key pressed
#define FACE_PCTRIGGER_NPC 0x00000004  // This face has to trigger an NPC after being done
#define FACE_INACTIVE_HANDLED_ELSEWHERE \
  0x00000008  // This face has been setup and any disable should be done
              // Externally
#define FACE_TRIGGER_PREBATTLE_INT 0x00000010
#define FACE_SHOW_WHITE_HILIGHT 0x00000020  // Show highlight around face
#define FACE_FORCE_SMALL 0x00000040         // force to small face
#define FACE_MODAL 0x00000080               // make game modal
#define FACE_MAKEACTIVE_ONCE_DONE 0x00000100
#define FACE_SHOW_MOVING_HILIGHT 0x00000200
#define FACE_REDRAW_WHOLE_FACE_NEXT_FRAME 0x00000400  // Redraw the complete face next frame

#define FACE_DRAW_TEXT_OVER 2
#define FACE_ERASE_TEXT_OVER 1
#define FACE_NO_TEXT_OVER 0

struct AUDIO_GAP {
  uint32_t start;
  uint32_t end;
};

/* This is a structure that will contain data about the gaps in a particular
 * wave file */
struct AudioGapList {
  AUDIO_GAP *gaps;       // Pointer to gap array
  const AUDIO_GAP *end;  // Pointer one past the end of the gap array
};

struct FACETYPE {
  uint32_t uiFlags;           // Basic flags
  BOOLEAN fAllocated;         // Allocated or not
  BOOLEAN fTalking;           // Set to true if face is talking ( can be sitting for user
                              // input to esc )
  BOOLEAN fAnimatingTalking;  // Set if the face is animating right now
  BOOLEAN fDisabled;          // Not active
  BOOLEAN fValidSpeech;
  BOOLEAN fInvalidAnim;

  uint32_t uiTalkingDuration;                // A delay based on text length for how long to talk
                                             // if no speech
  uint32_t uiTalkingTimer;                   // A timer to handle delay when no speech file
  uint32_t uiTalkingFromVeryBeginningTimer;  // Timer from very beginning of
                                             // talking...

  BOOLEAN
  fFinishTalking;  // A flag to indicate we want to delay after speech done

  VIDEO_OVERLAY *video_overlay;  // Value for video overlay ( not used too much )

  uint32_t uiSoundID;      // Sound ID if one being played
  SOLDIERTYPE *soldier;    // Soldier if one specified
  uint8_t ubCharacterNum;  // Profile ID num

  uint16_t usFaceX;  // X location to render face
  uint16_t usFaceY;  // Y location to render face
  uint16_t usFaceWidth;
  uint16_t usFaceHeight;
  SGPVSurface *uiAutoDisplayBuffer;  // Display buffer for face
  SGPVSurface *uiAutoRestoreBuffer;  // Restore buffer
  BOOLEAN fAutoRestoreBuffer;        // Flag to indicate our own restorebuffer or not
  BOOLEAN fAutoDisplayBuffer;        // Flag to indicate our own display buffer or not
  BOOLEAN fDisplayTextOver;          // Boolean indicating to display text on face
  BOOLEAN fCanHandleInactiveNow;
  wchar_t zDisplayText[30];  // String of text that can be displayed

  uint16_t usEyesX;
  uint16_t usEyesY;
  uint16_t usEyesOffsetX;
  uint16_t usEyesOffsetY;

  uint16_t usEyesWidth;
  uint16_t usEyesHeight;

  uint16_t usMouthX;
  uint16_t usMouthY;
  uint16_t usMouthOffsetX;
  uint16_t usMouthOffsetY;
  uint16_t usMouthWidth;
  uint16_t usMouthHeight;

  uint16_t sEyeFrame;
  int8_t ubEyeWait;
  uint32_t uiEyelast;
  uint32_t uiEyeDelay;
  uint32_t uiBlinkFrequency;
  uint32_t uiExpressionFrequency;

  uint8_t ubExpression;

  int8_t bOldSoldierLife;
  int8_t bOldActionPoints;
  int8_t bOldAssignment;
  int8_t ubOldServiceCount;
  const SOLDIERTYPE *old_service_partner;

  uint16_t sMouthFrame;
  uint32_t uiMouthlast;
  uint32_t uiMouthDelay;

  uint32_t uiLastBlink;
  uint32_t uiLastExpression;

  SGPVObject *uiVideoObject;

  BOOLEAN fCompatibleItems;
  BOOLEAN fOldCompatibleItems;
  BOOLEAN bOldStealthMode;
  int8_t bOldOppCnt;

  AudioGapList GapList;

  union  // XXX TODO001E ugly
  {
    struct  // Used for FACE_PCTRIGGER_NPC
    {
      ProfileID npc;
      uint8_t record;
    } trigger;
    struct  // Used for FACE_TRIGGER_PREBATTLE_INT
    {
      GROUP *group;
    } initiating_battle;
  } u;
};

// FACE HANDLING
//
// Faces are done like this: Call
FACETYPE &InitFace(ProfileID id, SOLDIERTYPE *s, uint32_t uiInitFlags);
/* The first parameter is the profile ID and the second is the soldier (which
 * for most cases will be NULL if the face is not created from a SOLDIERTYPE).
 * This function allocates a slot in the table for the face, loads its STI file,
 * sets some values for X,Y locations of eyes from the profile. Does not make
 * the face visible or anything like that */

// Removes the face from the internal table, deletes any memory allocated if
// any.
void DeleteFace(FACETYPE *);

// IF you want to setup the face for automatic eye blinking, mouth movement, you
// need to call
void SetAutoFaceActive(SGPVSurface *display, SGPVSurface *restore, FACETYPE &, uint16_t usFaceX,
                       uint16_t usFaceY);
// The first paramter is the display buffer you wish the face to be rendered on.
// The second is the Internal savebuffer which is used to facilitate the
// rendering of only things which have changed when blinking. IF the value of
// FACE_AUTO_RESTORE_BUFFER is given, the system will allocate it's own memory
// for a saved buffer and will delete it when finished with it. This function
// also takes an XY location

// To begin rendering of the face sprite, call this function once:
void RenderAutoFace(FACETYPE &);
// This will draw the face into it's saved buffer and then display it on the
// display buffer. If the display buffer given is FRAME_BUFFER, the regions will
// automatically be dirtied, so no calls to InvalidateRegion() should be
// nessesary.

// If you want to setup the face to talking, ( most times this call is done in
// JA2 by other functions, not
// directly), you call
void SetFaceTalking(FACETYPE &, char const *zSoundFile, wchar_t const *zTextString);
// This function will setup appropriate face data and begin the speech process.
// It can fail if the sound
// cannot be played for any reason.

// Set some face talking flags without need to play sound
void ExternSetFaceTalking(FACETYPE &, uint32_t sound_id);

// Once this is done, this function must be called overy gameloop that you want
// to handle the sprite:
void HandleAutoFaces();
// This will handle all faces set to be auto mamaged by SetAutoFaceActive().
// What is does is determines the best mouth and eye graphic to use. It then
// renders only the rects nessessary into the display buffer.

// If you need to shutoff the face from talking, use the function
void ShutupaYoFace(FACETYPE *);
void InternalShutupaYoFace(FACETYPE *, BOOLEAN fForce);

// This can be used to times when you need process the user hitting <ESC> to
// cancel the speech, etc. It will shutoff any playing sound sample

/* If you still want the face in memory but want to stop if from being
 * displayed, or handled call */
void SetAutoFaceInActive(FACETYPE &);

// To set all currently allocated faces to either active or incactive, call
// these
void SetAllAutoFacesInactive();

// FUnctions usually not needed for most uses, but give a finer control over
// rendering if needed
void HandleTalkingAutoFaces();

// Same Functions but taking soldier first to get profile
void InitSoldierFace(SOLDIERTYPE &);
void DeleteSoldierFace(SOLDIERTYPE *pSoldier);

/* To render an allocated face, but one that is independent of its active
 * status and does not require eye blinking or mouth movements, call */
void ExternRenderFace(SGPVSurface *buffer, FACETYPE &, int16_t x, int16_t y);

void LoadFacesGraphics();
void DeleteFacesGraphics();

#endif
