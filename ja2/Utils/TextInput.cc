// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/TextInput.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/MouseSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/SoundControl.h"
#include "Utils/TimerControl.h"

#include "SDL_keyboard.h"
#include "SDL_keycode.h"

static uint16_t *szClipboard;
BOOLEAN gfNoScroll = FALSE;

struct TextInputColors {
  // internal values that contain all of the colors for the text editing fields.
  Font usFont;
  uint16_t usTextFieldColor;
  uint8_t ubForeColor, ubShadowColor;
  uint8_t ubHiForeColor, ubHiShadowColor, ubHiBackColor;
  // optional -- no bevelling by default
  BOOLEAN fBevelling;
  uint16_t usBrighterColor, usDarkerColor;
  // optional -- cursor color defaults to black
  uint16_t usCursorColor;
  // optional colors for disabled fields (defaults to 25% darker shading)
  BOOLEAN fUseDisabledAutoShade;
  uint8_t ubDisabledForeColor;
  uint8_t ubDisabledShadowColor;
  uint16_t usDisabledTextFieldColor;
};

static TextInputColors *pColors = NULL;

// Internal nodes for keeping track of the text and user defined fields.
struct TEXTINPUTNODE {
  InputType usInputType;
  uint8_t ubID;
  uint8_t ubMaxChars;
  wchar_t *szString;
  uint8_t ubStrLen;
  BOOLEAN fEnabled;
  BOOLEAN fUserField;
  MOUSE_REGION region;
  INPUT_CALLBACK InputCallback;
  TEXTINPUTNODE *next;
  TEXTINPUTNODE *prev;
};

// Stack list containing the head nodes of each level.  Only the top level is
// the active level.
struct STACKTEXTINPUTNODE {
  TEXTINPUTNODE *head;
  TextInputColors *pColors;
  STACKTEXTINPUTNODE *next;
};

static STACKTEXTINPUTNODE *pInputStack = NULL;

// Internal list vars.  active always points to the currently edited field.
static TEXTINPUTNODE *gpTextInputHead = NULL;
static TEXTINPUTNODE *gpTextInputTail = NULL;
static TEXTINPUTNODE *gpActive = NULL;

// Saving current mode
static TEXTINPUTNODE *pSavedHead = NULL;
static TextInputColors *pSavedColors = NULL;
static uint16_t gusTextInputCursor = CURSOR_IBEAM;

// Saves the current text input mode by pushing it onto our stack, then starts a
// new one.
static void PushTextInputLevel() {
  STACKTEXTINPUTNODE *const pNewLevel = MALLOC(STACKTEXTINPUTNODE);
  pNewLevel->head = gpTextInputHead;
  pNewLevel->pColors = pColors;
  pNewLevel->next = pInputStack;
  pInputStack = pNewLevel;
  DisableAllTextFields();
}

// After the currently text input mode is removed, we then restore the previous
// one automatically.  Assert failure in this function will expose cases where
// you are trigger happy with killing non-existant text input modes.
static void PopTextInputLevel() {
  STACKTEXTINPUTNODE *pLevel;
  gpTextInputHead = pInputStack->head;
  pColors = pInputStack->pColors;
  pLevel = pInputStack;
  pInputStack = pInputStack->next;
  MemFree(pLevel);
  pLevel = NULL;
  EnableAllTextFields();
}

// flags for determining various editing modes.
static BOOLEAN gfEditingText = FALSE;
static BOOLEAN gfTextInputMode = FALSE;

void SetEditingStatus(bool bIsEditing) {
  if (bIsEditing != gfEditingText) {
    gfEditingText = bIsEditing;
    if (bIsEditing) {
      SDL_StartTextInput();
    } else {
      SDL_StopTextInput();
    }
  }
}

// values that contain the hiliting positions and the cursor position.
static uint8_t gubCursorPos = 0;
static uint8_t gubStartHilite = 0;

// Simply initiates that you wish to begin inputting text.  This should only
// apply to screen initializations that contain fields that edit text.  It also
// verifies and clears any existing fields.  Your input loop must contain the
// function HandleTextInput and processed if the gfTextInputMode flag is set else
// process your regular input handler.  Note that this doesn't mean you are
// necessarily typing, just that there are text fields in your screen and may be
// inactive.  The TAB key cycles through your text fields, and special fields can
// be defined which will call a void functionName( uint16_t usFieldNum )
void InitTextInputMode() {
  if (gpTextInputHead) {
    // Instead of killing all of the currently existing text input fields, they
    // will now (Jan16 '97) be pushed onto a stack, and preserved until we are
    // finished with the new mode when they will automatically be re-instated
    // when the new text input mode is killed.
    PushTextInputLevel();
    // KillTextInputMode();
  }
  gpTextInputHead = NULL;
  pColors = MALLOC(TextInputColors);
  gfTextInputMode = TRUE;
  SetEditingStatus(FALSE);
  pColors->fBevelling = FALSE;
  pColors->fUseDisabledAutoShade = TRUE;
  pColors->usCursorColor = 0;
}

// A hybrid version of InitTextInput() which uses a specific scheme.  JA2's
// editor uses scheme 1, so feel free to add new schemes.
void InitTextInputModeWithScheme(uint8_t ubSchemeID) {
  InitTextInputMode();
  switch (ubSchemeID) {
    case DEFAULT_SCHEME:  // yellow boxes with black text, with bluish bevelling
      SetTextInputFont(FONT12POINT1);
      Set16BPPTextFieldColor(Get16BPPColor(FROMRGB(250, 240, 188)));
      SetBevelColors(Get16BPPColor(FROMRGB(136, 138, 135)), Get16BPPColor(FROMRGB(24, 61, 81)));
      SetTextInputRegularColors(FONT_BLACK, FONT_BLACK);
      SetTextInputHilitedColors(FONT_GRAY2, FONT_GRAY2, FONT_METALGRAY);
      break;
  }
}

// Clear any existing fields, and end text input mode.
void KillTextInputMode() {
  TEXTINPUTNODE *i = gpTextInputHead;
  if (!i) return;
  while (i) {
    TEXTINPUTNODE *const del = i;
    i = i->next;
    if (del->szString) {
      MemFree(del->szString);
      del->szString = 0;
      MSYS_RemoveRegion(&del->region);
    }
    MemFree(del);
  }
  MemFree(pColors);
  pColors = 0;
  gpTextInputHead = 0;

  if (pInputStack) {
    PopTextInputLevel();
    SetActiveField(0);
  } else {
    gfTextInputMode = FALSE;
    SetEditingStatus(FALSE);
  }

  if (!gpTextInputHead) gpActive = 0;
}

// Kills all levels of text input modes.  When you init a second consecutive
// text input mode, without first removing them, the existing mode will be
// preserved.  This function removes all of them in one call, though doing so
// "may" reflect poor coding style, though I haven't thought about any really
// just uses for it :(
void KillAllTextInputModes() {
  while (gpTextInputHead) KillTextInputMode();
}

static TEXTINPUTNODE *AllocateTextInputNode(BOOLEAN const start_editing) {
  TEXTINPUTNODE *const n = MALLOCZ(TEXTINPUTNODE);
  n->fEnabled = TRUE;
  if (!gpTextInputHead) {  // First entry, so we start with text input.
    SetEditingStatus(start_editing);
    gpTextInputHead = n;
    gpActive = n;
    n->ubID = 0;
  } else {  // Add to the end of the list.
    TEXTINPUTNODE *const tail = gpTextInputTail;
    tail->next = n;
    n->prev = tail;
    n->ubID = tail->ubID + 1;
  }
  gpTextInputTail = n;
  return n;
}

static void MouseClickedInTextRegionCallback(MOUSE_REGION *reg, int32_t reason);
static void MouseMovedInTextRegionCallback(MOUSE_REGION *reg, int32_t reason);

/* After calling InitTextInputMode, you want to define one or more text input
 * fields.  The order of calls to this function dictate the TAB order from
 * traversing from one field to the next.  This function adds mouse regions and
 * processes them for you, as well as deleting them when you are done. */
void AddTextInputField(int16_t const sLeft, int16_t const sTop, int16_t const sWidth,
                       int16_t const sHeight, int8_t const bPriority,
                       wchar_t const *const szInitText, uint8_t ubMaxChars,
                       InputType const usInputType) {
  TEXTINPUTNODE *const n = AllocateTextInputNode(TRUE);
  // Setup the information for the node
  n->usInputType = usInputType;  // setup the filter type
  // All 24hourclock inputtypes have 6 characters.  01:23 (null terminated)
  if (usInputType == INPUTTYPE_24HOURCLOCK) ubMaxChars = 6;
  // Allocate and copy the string.
  n->ubMaxChars = ubMaxChars;
  n->szString = MALLOCN(wchar_t, ubMaxChars + 1);
  if (szInitText) {
    n->ubStrLen = wcslen(szInitText);
    Assert(n->ubStrLen <= ubMaxChars);
    wcsncpy(n->szString, szInitText, ubMaxChars + 1);
  } else {
    n->ubStrLen = 0;
    n->szString[0] = L'\0';
  }

  // If this is the first field, then hilight it.
  if (gpTextInputHead == n) {
    gubStartHilite = 0;
    gubCursorPos = n->ubStrLen;
  }
  // Setup the region.
  MSYS_DefineRegion(&n->region, sLeft, sTop, sLeft + sWidth, sTop + sHeight, bPriority,
                    gusTextInputCursor, MouseMovedInTextRegionCallback,
                    MouseClickedInTextRegionCallback);
  n->region.SetUserPtr(n);
}

/* This allows you to insert special processing functions and modes that can't
 * be determined here.  An example would be a file dialog where there would be a
 * file list.  This file list would be accessed using the Win95 convention by
 * pressing TAB.  In there, your key presses would be handled differently and by
 * adding a userinput field, you can make this hook into your function to
 * accomplish this.  In a filedialog, alpha characters would be used to jump to
 * the file starting with that letter, and setting the field in the text input
 * field.  Pressing TAB again would place you back in the text input field.
 * All of that stuff would be handled externally, except for the TAB keys. */
void AddUserInputField(INPUT_CALLBACK const userFunction) {
  TEXTINPUTNODE *const n = AllocateTextInputNode(FALSE);
  n->fUserField = TRUE;
  n->InputCallback = userFunction;
}

static TEXTINPUTNODE *GetTextInputField(uint8_t const id) {
  for (TEXTINPUTNODE *i = gpTextInputHead; i; i = i->next) {
    if (i->ubID == id) return i;
  }
  return 0;
}

/* Remove the specified field from the existing fields.  If it doesn't exist,
 * then there will be an assertion failure. */
static void RemoveTextInputField(uint8_t const id) {
  TEXTINPUTNODE *const n = GetTextInputField(id);
  AssertMsg(n,
            "Attempt to remove a text input field that doesn't exist.  "
            "Check your IDs.");
  if (!n) return;

  if (n == gpActive) SelectNextField();
  TEXTINPUTNODE *const prev = n->prev;
  TEXTINPUTNODE *const next = n->next;
  *(next ? &next->prev : &gpTextInputTail) = prev;
  *(prev ? &prev->next : &gpTextInputHead) = next;
  if (n->szString) {
    MemFree(n->szString);
    n->szString = 0;
    MSYS_RemoveRegion(&n->region);
  }
  MemFree(n);
  if (!gpTextInputHead) {
    gfTextInputMode = FALSE;
    gfEditingText = FALSE;
  }
}

// Returns the gpActive field ID number.  It'll return -1 if no field is active.
int16_t GetActiveFieldID() {
  if (gpActive) return gpActive->ubID;
  return -1;
}

// This is a useful call made from an external user input field.  Using the
// previous file dialog example, this call would be made when the user selected a
// different filename in the list via clicking or scrolling with the arrows, or
// even using alpha chars to jump to the appropriate filename.
void SetInputFieldStringWith16BitString(uint8_t ubField, const wchar_t *szNewText) {
  TEXTINPUTNODE *const curr = GetTextInputField(ubField);
  if (!curr) return;

  if (szNewText) {
    curr->ubStrLen = (uint8_t)wcslen(szNewText);
    Assert(curr->ubStrLen <= curr->ubMaxChars);
    wcsncpy(curr->szString, szNewText, curr->ubMaxChars + 1);
  } else if (!curr->fUserField) {
    curr->ubStrLen = 0;
    curr->szString[0] = L'\0';
  } else {
    AssertMsg(0, String("Attempting to illegally set text into user field %d", curr->ubID));
  }
}

void SetInputFieldStringWith8BitString(uint8_t ubField, const char *szNewText) {
  TEXTINPUTNODE *const curr = GetTextInputField(ubField);
  if (!curr) return;

  if (szNewText) {
    curr->ubStrLen = (uint8_t)strlen(szNewText);
    Assert(curr->ubStrLen <= curr->ubMaxChars);
    swprintf(curr->szString, curr->ubMaxChars + 1, L"%hs", szNewText);
  } else if (!curr->fUserField) {
    curr->ubStrLen = 0;
    curr->szString[0] = L'\0';
  } else {
    AssertMsg(0, String("Attempting to illegally set text into user field %d", curr->ubID));
  }
}

wchar_t const *GetStringFromField(uint8_t const ubField) {
  TEXTINPUTNODE const *const n = GetTextInputField(ubField);
  return n ? n->szString : L"";
}

int32_t GetNumericStrictValueFromField(uint8_t const id) {
  wchar_t const *i = GetStringFromField(id);
  if (*i == L'\0') return -1;  // Blank string, so return -1
  /* Convert the string to a number. This ensures that non-numeric values
   * automatically return -1. */
  int32_t total = 0;
  for (; *i != '\0'; ++i) {
    if (*i < L'0' || L'9' < *i) return -1;
    total = total * 10 + (*i - '0');
  }
  return total;
}

// Converts a number to a numeric strict value.  If the number is negative, the
// field will be blank.
void SetInputFieldStringWithNumericStrictValue(uint8_t ubField, int32_t iNumber) {
  TEXTINPUTNODE *const curr = GetTextInputField(ubField);
  if (!curr) return;

  AssertMsg(!curr->fUserField,
            String("Attempting to illegally set text into user field %d", curr->ubID));
  if (iNumber < 0)  // negative number converts to blank string
  {
    curr->szString[0] = L'\0';
  } else {
    int32_t iMax = (int32_t)pow(10.0, curr->ubMaxChars);
    if (iNumber > iMax)  // set string to max value based on number of chars.
      swprintf(curr->szString, curr->ubMaxChars + 1, L"%d", iMax - 1);
    else  // set string to the number given
      swprintf(curr->szString, curr->ubMaxChars + 1, L"%d", iNumber);
  }
  curr->ubStrLen = (uint8_t)wcslen(curr->szString);
}

// Set the active field to the specified ID passed.
void SetActiveField(uint8_t const id) {
  TEXTINPUTNODE *const n = GetTextInputField(id);
  if (!n) return;
  if (n == gpActive) return;
  if (!n->fEnabled) return;

  if (gpActive && gpActive->InputCallback) {
    gpActive->InputCallback(gpActive->ubID, FALSE);
  }

  gpActive = n;
  if (n->szString) {
    gubStartHilite = 0;
    gubCursorPos = n->ubStrLen;
    SetEditingStatus(TRUE);
  } else {
    SetEditingStatus(FALSE);
    if (n->InputCallback) n->InputCallback(n->ubID, TRUE);
  }
}

static void RenderInactiveTextFieldNode(TEXTINPUTNODE const *);

void SelectNextField() {
  BOOLEAN fDone = FALSE;
  TEXTINPUTNODE *pStart;

  if (!gpActive) return;
  if (gpActive->szString)
    RenderInactiveTextFieldNode(gpActive);
  else if (gpActive->InputCallback)
    (gpActive->InputCallback)(gpActive->ubID, FALSE);
  pStart = gpActive;
  while (!fDone) {
    gpActive = gpActive->next;
    if (!gpActive) gpActive = gpTextInputHead;
    if (gpActive->fEnabled) {
      fDone = TRUE;
      if (gpActive->szString) {
        gubStartHilite = 0;
        gubCursorPos = gpActive->ubStrLen;
        SetEditingStatus(TRUE);
      } else {
        SetEditingStatus(FALSE);
        if (gpActive->InputCallback) (gpActive->InputCallback)(gpActive->ubID, TRUE);
      }
    }
    if (gpActive == pStart) {
      SetEditingStatus(FALSE);
      return;
    }
  }
}

static void SelectPrevField() {
  BOOLEAN fDone = FALSE;
  TEXTINPUTNODE *pStart;

  if (!gpActive) return;
  if (gpActive->szString)
    RenderInactiveTextFieldNode(gpActive);
  else if (gpActive->InputCallback)
    (gpActive->InputCallback)(gpActive->ubID, FALSE);
  pStart = gpActive;
  while (!fDone) {
    gpActive = gpActive->prev;
    if (!gpActive) gpActive = gpTextInputTail;
    if (gpActive->fEnabled) {
      fDone = TRUE;
      if (gpActive->szString) {
        gubStartHilite = 0;
        gubCursorPos = gpActive->ubStrLen;
        SetEditingStatus(TRUE);
      } else {
        SetEditingStatus(FALSE);
        if (gpActive->InputCallback) (gpActive->InputCallback)(gpActive->ubID, TRUE);
      }
    }
    if (gpActive == pStart) {
      SetEditingStatus(FALSE);
      return;
    }
  }
}

// These allow you to customize the general color scheme of your text input
// boxes.  I am assuming that under no circumstances would a user want a
// different color for each field.  It follows the Win95 convention that all text
// input boxes are exactly the same color scheme.  However, these colors can be
// set at anytime, but will effect all of the colors.
void SetTextInputFont(Font const font) { pColors->usFont = font; }

void Set16BPPTextFieldColor(uint16_t usTextFieldColor) {
  pColors->usTextFieldColor = usTextFieldColor;
}

void SetTextInputRegularColors(uint8_t ubForeColor, uint8_t ubShadowColor) {
  pColors->ubForeColor = ubForeColor;
  pColors->ubShadowColor = ubShadowColor;
}

void SetTextInputHilitedColors(uint8_t ubForeColor, uint8_t ubShadowColor, uint8_t ubBackColor) {
  pColors->ubHiForeColor = ubForeColor;
  pColors->ubHiShadowColor = ubShadowColor;
  pColors->ubHiBackColor = ubBackColor;
}

// optional color setups
static void SetDisabledTextFieldColors(uint8_t ubForeColor, uint8_t ubShadowColor,
                                       uint16_t usTextFieldColor) {
  pColors->fUseDisabledAutoShade = FALSE;
  pColors->ubDisabledForeColor = ubForeColor;
  pColors->ubDisabledShadowColor = ubShadowColor;
  pColors->usDisabledTextFieldColor = usTextFieldColor;
}

void SetBevelColors(uint16_t usBrighterColor, uint16_t usDarkerColor) {
  pColors->fBevelling = TRUE;
  pColors->usBrighterColor = usBrighterColor;
  pColors->usDarkerColor = usDarkerColor;
}

void SetCursorColor(uint16_t usCursorColor) { pColors->usCursorColor = usCursorColor; }

static void AddChar(wchar_t);
static void DeleteHilitedText();
static void HandleRegularInput(wchar_t);
static void RemoveChars(size_t pos, size_t n);

BOOLEAN HandleTextInput(InputAtom const *const a) {
  gfNoScroll = FALSE;
  // Not in text input mode
  if (!gfTextInputMode) return FALSE;
  // Unless we are psycho typers, we only want to process these key events.
  if (a->usEvent != TEXT_INPUT && a->usEvent != KEY_DOWN && a->usEvent != KEY_REPEAT) return FALSE;
  // Currently in a user field, so return unless TAB is pressed.
  if (!gfEditingText && a->usParam != SDLK_TAB) return FALSE;

  if (a->usEvent == TEXT_INPUT) {
    wchar_t const c = a->Char;
    /* If the key has no character associated, bail out */
    AssertMsg(c != L'\0', "TEXT_INPUT event sent null character");
    DeleteHilitedText();
    HandleRegularInput(c);
    return TRUE;
  }

  switch (a->usKeyState) {
    case 0:
      switch (a->usParam) {
        /* ESC and ENTER must be handled externally, due to the infinite uses
         * for them. */
        case SDLK_ESCAPE:
          return FALSE;  // ESC is equivalent to cancel

        case SDLK_RETURN:  // ENTER is to confirm.
          PlayJA2Sample(REMOVING_TEXT, BTNVOLUME, 1, MIDDLEPAN);
          return FALSE;

        case SDLK_TAB:
          /* Always select the next field, even when a user defined field is
           * currently selected. The order in which you add your text and user
           * fields dictates the cycling order when TAB is pressed. */
          SelectNextField();
          return TRUE;

        case SDLK_LEFT:
          gfNoScroll = TRUE;
          if (gubCursorPos != 0) --gubCursorPos;
          gubStartHilite = gubCursorPos;
          return TRUE;

        case SDLK_RIGHT:
          if (gubCursorPos < gpActive->ubStrLen) ++gubCursorPos;
          gubStartHilite = gubCursorPos;
          return TRUE;

        case SDLK_END:
          gubCursorPos = gpActive->ubStrLen;
          gubStartHilite = gubCursorPos;
          return TRUE;

        case SDLK_HOME:
          gubCursorPos = 0;
          gubStartHilite = gubCursorPos;
          return TRUE;

        case SDLK_DELETE:
          /* DEL either deletes the selected text, or the character to the right
           * of the cursor if applicable. */
          if (gubStartHilite != gubCursorPos) {
            DeleteHilitedText();
          } else if (gubCursorPos < gpActive->ubStrLen) {
            RemoveChars(gubCursorPos, 1);
          } else {
            return TRUE;
          }
          break;

        case SDLK_BACKSPACE:
          /* Delete the selected text, or the character to the left of the
           * cursor if applicable. */
          if (gubStartHilite != gubCursorPos) {
            DeleteHilitedText();
          } else if (gubCursorPos > 0) {
            gubStartHilite = --gubCursorPos;
            RemoveChars(gubCursorPos, 1);
          } else {
            return TRUE;
          }
          break;

        default:
          return TRUE;
      }
      break;

    case SHIFT_DOWN:
      switch (a->usParam) {
        case SDLK_TAB:  // See comment for non-shifted TAB above
          SelectPrevField();
          return TRUE;

        case SDLK_LEFT:
          gfNoScroll = TRUE;
          if (gubCursorPos != 0) --gubCursorPos;
          return TRUE;

        case SDLK_RIGHT:
          if (gubCursorPos < gpActive->ubStrLen) ++gubCursorPos;
          return TRUE;

        case SDLK_END:
          gubCursorPos = gpActive->ubStrLen;
          return TRUE;

        case SDLK_HOME:
          gubCursorPos = 0;
          return TRUE;

        default:
          return TRUE;
      }

    case CTRL_DOWN:
      switch (a->usParam) {
#if 0
				case SDLK_c: ExecuteCopyCommand();  return TRUE;
				case SDLK_x: ExecuteCutCommand();   return TRUE;
				case SDLK_v: ExecutePasteCommand(); return TRUE;
#endif

        case SDLK_DELETE:
          // Delete the entire text field, regardless of hilighting.
          gubStartHilite = 0;
          gubCursorPos = gpActive->ubStrLen;
          DeleteHilitedText();
          break;

        default:
          return FALSE;
      }
      break;

    default:
      return FALSE;
  }

  PlayJA2Sample(ENTERING_TEXT, BTNVOLUME, 1, MIDDLEPAN);
  return TRUE;
}

// All input types are handled in this function.
static void HandleRegularInput(wchar_t const c) {
  TEXTINPUTNODE const &n = *gpActive;
  switch (n.usInputType) {
    case INPUTTYPE_NUMERICSTRICT:
      if (L'0' <= c && c <= L'9') AddChar(c);
      break;

    case INPUTTYPE_FULL_TEXT:
      if (IsPrintableChar(c)) AddChar(c);
      break;

    case INPUTTYPE_DOSFILENAME:  // DOS file names
      if ((L'A' <= c && c <= L'Z') || (L'a' <= c && c <= L'z') ||
          /* Cannot begin a new filename with a number */
          (L'0' <= c && c <= L'9' && gubCursorPos != 0) || c == L'_' || c == L'.') {
        AddChar(c);
      }
      break;

    case INPUTTYPE_COORDINATE:  // coordinates such as a9, z78, etc.
      // First char is an lower case alpha, subsequent chars are numeric
      if (gubCursorPos == 0) {
        if (L'a' <= c && c <= L'z') {
          AddChar(c);
        } else if (L'A' <= c && c <= L'Z') {
          AddChar(c + 32);  // Convert to lowercase
        }
      } else {
        if (L'0' <= c && c <= L'9') AddChar(c);
      }
      break;

    case INPUTTYPE_24HOURCLOCK:
      switch (gubCursorPos) {
        case 0:
          if (L'0' <= c && c <= L'2') AddChar(c);
          break;

        case 1:
          if (L'0' <= c && c <= L'9') {
            if (n.szString[0] == L'2' && c > L'3') break;
            AddChar(c);
          }
          if (n.szString[2] == L'\0') {
            AddChar(L':');
          } else {
            gubStartHilite = ++gubCursorPos;
          }
          break;

        case 2:
          if (c == L':') {
            AddChar(c);
          } else if (L'0' <= c && c <= L'9') {
            AddChar(L':');
            AddChar(c);
          }
          break;

        case 3:
          if (L'0' <= c && c <= L'5') AddChar(c);
          break;

        case 4:
          if (L'0' <= c && c <= L'9') AddChar(c);
          break;
      }
      break;
  }
}

static void AddChar(wchar_t const c) {
  PlayJA2Sample(ENTERING_TEXT, BTNVOLUME, 1, MIDDLEPAN);
  TEXTINPUTNODE &n = *gpActive;
  uint8_t &len = n.ubStrLen;
  if (len >= n.ubMaxChars) return;
  // Insert character after cursor
  wchar_t *const str = n.szString;
  wchar_t *i = str + ++len;
  wchar_t const *const end = str + gubCursorPos;
  gubStartHilite = ++gubCursorPos;
  for (; i != end; --i) *i = i[-1];
  *i = c;
}

static void DeleteHilitedText() {
  uint8_t start = gubStartHilite;
  uint8_t end = gubCursorPos;
  if (start == end) return;
  if (start > end) Swap(start, end);
  gubStartHilite = start;
  gubCursorPos = start;
  RemoveChars(start, end - start);
}

static void RemoveChars(size_t const pos, size_t const n) {
  TEXTINPUTNODE &t = *gpActive;
  Assert(pos + n <= t.ubStrLen);
  t.ubStrLen -= n;
  for (wchar_t *str = t.szString + pos; (*str = str[n]) != L'\0'; ++str) {
  }
}

static void SetActiveFieldMouse(MOUSE_REGION const *const r) {
  TEXTINPUTNODE *const n = r->GetUserPtr<TEXTINPUTNODE>();
  if (n == gpActive) return;
  // Deselect the current text edit region if applicable, then set the new one.
  if (gpActive && gpActive->InputCallback) {
    gpActive->InputCallback(gpActive->ubID, FALSE);
  }

  RenderInactiveTextFieldNode(gpActive);
  gpActive = n;
}

static size_t CalculateCursorPos(int32_t const click_x) {
  Font const font = pColors->usFont;
  wchar_t const *const str = gpActive->szString;
  int32_t char_pos = 0;
  size_t i;
  for (i = 0; str[i] != L'\0'; ++i) {
    char_pos += GetCharWidth(font, str[i]);
    if (char_pos >= click_x) break;
  }
  return i;
}

// Internally used to continue highlighting text
static void MouseMovedInTextRegionCallback(MOUSE_REGION *const reg, int32_t const reason) {
  if (!gfLeftButtonState) return;

  if (reason & MSYS_CALLBACK_REASON_MOVE || reason & MSYS_CALLBACK_REASON_LOST_MOUSE ||
      reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    SetActiveFieldMouse(reg);
    if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
      if (gusMouseYPos < reg->RegionTopLeftY) {
        gubCursorPos = 0;
      } else if (gusMouseYPos > reg->RegionBottomRightY) {
        gubCursorPos = gpActive->ubStrLen;
      }
    } else {
      gubCursorPos = CalculateCursorPos(gusMouseXPos - reg->RegionTopLeftX);
    }
  }
}

// Internally used to calculate where to place the cursor.
static void MouseClickedInTextRegionCallback(MOUSE_REGION *const reg, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    SetActiveFieldMouse(reg);
    // Signifies that we are typing text now.
    SetEditingStatus(TRUE);
    size_t const pos = CalculateCursorPos(gusMouseXPos - reg->RegionTopLeftX);
    gubCursorPos = pos;
    gubStartHilite = pos;
  }
}

static void RenderBackgroundField(TEXTINPUTNODE const *const n) {
  int16_t const tlx = n->region.RegionTopLeftX;
  int16_t const tly = n->region.RegionTopLeftY;
  int16_t const brx = n->region.RegionBottomRightX;
  int16_t const bry = n->region.RegionBottomRightY;
  TextInputColors const &clrs = *pColors;

  if (clrs.fBevelling) {
    ColorFillVideoSurfaceArea(FRAME_BUFFER, tlx, tly, brx, bry, clrs.usDarkerColor);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, tlx + 1, tly + 1, brx, bry, clrs.usBrighterColor);
  }

  uint16_t const colour = n->fEnabled || clrs.fUseDisabledAutoShade ? clrs.usTextFieldColor
                                                                    : clrs.usDisabledTextFieldColor;
  ColorFillVideoSurfaceArea(FRAME_BUFFER, tlx + 1, tly + 1, brx - 1, bry - 1, colour);

  InvalidateRegion(tlx, tly, brx, bry);
}

/* Required in your screen loop to update the values, as well as blinking the
 * cursor. */
static void RenderActiveTextField() {
  TEXTINPUTNODE const *const n = gpActive;
  if (!n || !n->szString) return;

  SaveFontSettings();
  RenderBackgroundField(n);

  TextInputColors const &clrs = *pColors;
  Font const font = clrs.usFont;
  uint16_t const h = GetFontHeight(font);
  int32_t const y =
      n->region.RegionTopLeftY + (n->region.RegionBottomRightY - n->region.RegionTopLeftY - h) / 2;
  wchar_t const *const str = n->szString;
  wchar_t const *start = str + gubStartHilite;
  wchar_t const *end = str + gubCursorPos;
  if (start != end) {  // Some or all of the text is hilighted, so we will use a
                       // different method.
    // Sort the hilite order.
    if (start > end) Swap(start, end);
    // Traverse the string one character at a time, and draw the highlited part
    // differently.
    uint32_t x = n->region.RegionTopLeftX + 3;
    for (wchar_t const *i = str; *i != L'\0'; ++i) {
      if (start <= i && i < end) {  // In highlighted part of text
        SetFontAttributes(font, clrs.ubHiForeColor, clrs.ubHiShadowColor, clrs.ubHiBackColor);
      } else {  // In regular part of text
        SetFontAttributes(font, clrs.ubForeColor, clrs.ubShadowColor, 0);
      }
      x += MPrintChar(x, y, *i);
    }
  } else {
    SetFontAttributes(font, clrs.ubForeColor, clrs.ubShadowColor, 0);
    MPrint(n->region.RegionTopLeftX + 3, y, str);
  }

  // Draw the blinking ibeam cursor during the on blink period.
  if (gfEditingText && str && GetJA2Clock() % 1000 < TEXT_CURSOR_BLINK_INTERVAL) {
    int32_t x = n->region.RegionTopLeftX + 2;
    wchar_t const *c = str;
    for (size_t i = gubCursorPos; i != 0; --i) {
      Assert(*c != L'\0');
      x += GetCharWidth(font, *c++);
    }
    ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + 1, y + h, clrs.usCursorColor);
  }

  RestoreFontSettings();
}

void RenderInactiveTextField(uint8_t const id) {
  TEXTINPUTNODE const *const n = GetTextInputField(id);
  if (!n || !n->szString) return;
  SaveFontSettings();
  SetFontAttributes(pColors->usFont, pColors->ubForeColor, pColors->ubShadowColor);
  RenderBackgroundField(n);
  uint16_t const offset =
      (n->region.RegionBottomRightY - n->region.RegionTopLeftY - GetFontHeight(pColors->usFont)) /
      2;
  MPrint(n->region.RegionTopLeftX + 3, n->region.RegionTopLeftY + offset, n->szString);
  RestoreFontSettings();
}

static void RenderInactiveTextFieldNode(TEXTINPUTNODE const *const n) {
  if (!n || !n->szString) return;

  SaveFontSettings();
  TextInputColors const &clrs = *pColors;
  bool const disabled = !n->fEnabled && clrs.fUseDisabledAutoShade;
  if (disabled) {  // Use the color scheme specified by the user
    SetFontAttributes(clrs.usFont, clrs.ubDisabledForeColor, clrs.ubDisabledShadowColor);
  } else {
    SetFontAttributes(clrs.usFont, clrs.ubForeColor, clrs.ubShadowColor);
  }
  RenderBackgroundField(n);
  MOUSE_REGION const &r = n->region;
  uint16_t const y =
      r.RegionTopLeftY + (r.RegionBottomRightY - r.RegionTopLeftY - GetFontHeight(clrs.usFont)) / 2;
  MPrint(r.RegionTopLeftX + 3, y, n->szString);
  RestoreFontSettings();

  if (disabled) {
    FRAME_BUFFER->ShadowRect(r.RegionTopLeftX, r.RegionTopLeftY, r.RegionBottomRightX,
                             r.RegionBottomRightY);
  }
}

// Use when you do a full interface update.
void RenderAllTextFields() {
  // Render all of the other text input levels first
  for (STACKTEXTINPUTNODE const *stack = pInputStack; stack; stack = stack->next) {
    for (TEXTINPUTNODE const *i = stack->head; i; i = i->next) {
      RenderInactiveTextFieldNode(i);
    }
  }

  // Render the current text input level
  for (TEXTINPUTNODE const *i = gpTextInputHead; i; i = i->next) {
    if (i != gpActive) {
      RenderInactiveTextFieldNode(i);
    } else {
      RenderActiveTextField();
    }
  }
}

static void EnableTextField(uint8_t const id) {
  TEXTINPUTNODE *const n = GetTextInputField(id);
  if (!n || n->fEnabled) return;
  if (!gpActive) gpActive = n;
  n->region.Enable();
  n->fEnabled = TRUE;
}

void DisableTextField(uint8_t const id) {
  TEXTINPUTNODE *const n = GetTextInputField(id);
  if (!n) return;
  if (gpActive == n) SelectNextField();
  if (!n->fEnabled) return;
  n->region.Disable();
  n->fEnabled = FALSE;
}

void EnableTextFields(uint8_t const first_id, uint8_t const last_id) {
  for (TEXTINPUTNODE *i = gpTextInputHead; i; i = i->next) {
    if (i->ubID < first_id || last_id < i->ubID) continue;
    if (i->fEnabled) continue;
    i->region.Enable();
    i->fEnabled = TRUE;
  }
}

void DisableTextFields(uint8_t const first_id, uint8_t const last_id) {
  for (TEXTINPUTNODE *i = gpTextInputHead; i; i = i->next) {
    if (i->ubID < first_id || last_id < i->ubID) continue;
    if (!i->fEnabled) continue;
    if (gpActive == i) SelectNextField();
    i->region.Disable();
    i->fEnabled = FALSE;
  }
}

void EnableAllTextFields() {
  for (TEXTINPUTNODE *i = gpTextInputHead; i; i = i->next) {
    if (i->fEnabled) continue;
    i->region.Enable();
    i->fEnabled = TRUE;
  }
  if (!gpActive) gpActive = gpTextInputHead;
}

void DisableAllTextFields() {
  gpActive = 0;
  for (TEXTINPUTNODE *i = gpTextInputHead; i; i = i->next) {
    if (!i->fEnabled) continue;
    i->region.Disable();
    i->fEnabled = FALSE;
  }
}

BOOLEAN EditingText() { return gfEditingText; }

BOOLEAN TextInputMode() { return gfTextInputMode; }

// copy, cut, and paste hilighted text code
void InitClipboard() { szClipboard = NULL; }

void KillClipboard() {
  if (szClipboard) {
    MemFree(szClipboard);
    szClipboard = NULL;
  }
}

static void ExecuteCopyCommand() {
  uint8_t ubCount;
  uint8_t ubStart, ubEnd;
  if (!gpActive || !gpActive->szString) return;
  // Delete the current contents in the clipboard
  KillClipboard();
  // Calculate the start and end of the hilight
  if (gubStartHilite != gubCursorPos) {
    if (gubStartHilite < gubCursorPos) {
      ubStart = gubStartHilite;
      ubEnd = gubCursorPos;
    } else {
      ubStart = gubCursorPos;
      ubEnd = gubStartHilite;
    }
    ubCount = (uint8_t)(ubEnd - ubStart);
    szClipboard = MALLOCN(uint16_t, ubCount + 1);
    for (ubCount = ubStart; ubCount < ubEnd; ubCount++) {
      szClipboard[ubCount - ubStart] = gpActive->szString[ubCount];
    }
    szClipboard[ubCount - ubStart] = L'\0';
  }
}

static void ExecutePasteCommand() {
  uint8_t ubCount;
  if (!gpActive || !szClipboard) return;
  DeleteHilitedText();

  ubCount = 0;
  while (szClipboard[ubCount]) {
    AddChar(szClipboard[ubCount]);
    ubCount++;
  }
}

static void ExecuteCutCommand() {
  ExecuteCopyCommand();
  DeleteHilitedText();
}

// Saves the current text input mode, then removes it and activates the previous
// text input mode, if applicable.  The second function restores the settings.
// Doesn't currently support nested calls.
void SaveAndRemoveCurrentTextInputMode() {
  AssertMsg(pSavedHead == NULL,
            "Attempting to save text input stack head, when one already exists.");
  pSavedHead = gpTextInputHead;
  pSavedColors = pColors;
  if (pInputStack) {
    gpTextInputHead = pInputStack->head;
    pColors = pInputStack->pColors;
  } else {
    gpTextInputHead = NULL;
    pColors = NULL;
  }
}

void RestoreSavedTextInputMode() {
  AssertMsg(pSavedHead != NULL,
            "Attempting to restore saved text input stack "
            "head, when one doesn't exist.");
  gpTextInputHead = pSavedHead;
  pColors = pSavedColors;
  pSavedHead = NULL;
  pSavedColors = NULL;
}

void SetTextInputCursor(uint16_t const new_cursor) { gusTextInputCursor = new_cursor; }

// Utility functions for the INPUTTYPE_24HOURCLOCK input type.
uint16_t GetExclusive24HourTimeValueFromField(uint8_t ubField) {
  TEXTINPUTNODE const *const curr = GetTextInputField(ubField);
  AssertMsg(curr, String("GetExclusive24HourTimeValueFromField: Invalid field %d", ubField));
  if (!curr) return 0xffff;

  uint16_t usTime;
  if (curr->usInputType != INPUTTYPE_24HOURCLOCK) return 0xffff;  // illegal!
  // First validate the hours 00-23
  if ((curr->szString[0] == '2' && curr->szString[1] >= '0' &&  // 20-23
       curr->szString[1] <= '3') ||
      (curr->szString[0] >= '0' && curr->szString[0] <= '1' &&  // 00-19
       curr->szString[1] >= '0' &&
       curr->szString[1] <= '9')) {  // Next, validate the colon, and the minutes 00-59
    if (curr->szString[2] == ':' && curr->szString[5] == 0 &&    //	:
        curr->szString[3] >= '0' && curr->szString[3] <= '5' &&  // 0-5
        curr->szString[4] >= '0' && curr->szString[4] <= '9')    // 0-9
    {
      // Hours
      usTime = ((curr->szString[0] - 0x30) * 10 + curr->szString[1] - 0x30) * 60;
      // Minutes
      usTime += (curr->szString[3] - 0x30) * 10 + curr->szString[4] - 0x30;
      return usTime;
    }
  }
  // invalid
  return 0xffff;
}

// Utility functions for the INPUTTYPE_24HOURCLOCK input type.
void SetExclusive24HourTimeValue(uint8_t ubField, uint16_t usTime) {
  // First make sure the time is a valid time.  If not, then use 23:59
  if (usTime == 0xffff) {
    SetInputFieldStringWith16BitString(ubField, L"");
    return;
  }
  usTime = std::min((uint16_t)1439, usTime);

  TEXTINPUTNODE *const curr = GetTextInputField(ubField);
  if (!curr) return;

  AssertMsg(!curr->fUserField,
            String("Attempting to illegally set text into user field %d", curr->ubID));
  curr->szString[0] = (usTime / 600) + 0x30;      // 10 hours
  curr->szString[1] = (usTime / 60 % 10) + 0x30;  // 1 hour
  usTime %= 60;                                   // truncate the hours
  curr->szString[2] = ':';
  curr->szString[3] = (usTime / 10) + 0x30;  // 10 minutes
  curr->szString[4] = (usTime % 10) + 0x30;  // 1 minute;
  curr->szString[5] = 0;
}
