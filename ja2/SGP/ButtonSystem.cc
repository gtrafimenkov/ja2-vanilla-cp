// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/ButtonSystem.h"

#include <stdexcept>

#include "Macro.h"
#include "SGP/ButtonSoundControl.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "SGP/WCheck.h"
#include "Utils/WordWrap.h"

#ifdef _JA2_RENDER_DIRTY
#include "Utils/FontControl.h"
#endif

// Names of the default generic button image files.
#define DEFAULT_GENERIC_BUTTON_OFF "genbutn.sti"
#define DEFAULT_GENERIC_BUTTON_ON "genbutn2.sti"
#define DEFAULT_GENERIC_BUTTON_OFF_HI "genbutn3.sti"
#define DEFAULT_GENERIC_BUTTON_ON_HI "genbutn4.sti"

#define MSYS_STARTING_CURSORVAL 0

#define MAX_BUTTON_ICONS 40

#define GUI_BTN_NONE 0
#define GUI_BTN_DUPLICATE_VOBJ 1

#define FOR_EACH_BUTTON(iter)              \
  FOR_EACH(GUI_BUTTON *, iter, ButtonList) \
  if (!*iter)                              \
    continue;                              \
  else

#ifdef BUTTONSYSTEM_DEBUGGING

// Called immediately before assigning the button to the button list.
static void AssertFailIfIdenticalButtonAttributesFound(const GUI_BUTTON *b) {
  FOR_EACH_BUTTON(i) {
    GUI_BUTTON const *const c = *i;
    if (c->uiFlags & BUTTON_DELETION_PENDING) continue;
    if (c->uiFlags & BUTTON_NO_DUPLICATE) continue;
    if (b->Area.PriorityLevel != c->Area.PriorityLevel) continue;
    if (b->X() != c->X()) continue;
    if (b->Y() != c->Y()) continue;
    if (b->BottomRightX() != c->BottomRightX()) continue;
    if (b->BottomRightY() != c->BottomRightY()) continue;
    if (b->ClickCallback != c->ClickCallback) continue;
    if (b->MoveCallback != c->MoveCallback) continue;
    /* if we get this far, it is reasonably safe to assume that the newly
     * created button already exists.  Placing a break point on the following
     * assert will allow the coder to easily isolate the case!
     */
    AssertMsg(0, String("Attempting to create a button that has already been "
                        "created (existing buttonID %d).",
                        c->IDNum));
  }
}

#endif

/* Kris:
 * These are the variables used for the anchoring of a particular button.  When
 * you click on a button, it get's anchored, until you release the mouse button.
 * When you move around, you don't want to select other buttons, even when you
 * release it.  This follows the Windows 95 convention.
 */
static GUI_BUTTON *gpAnchoredButton;
static GUI_BUTTON *gpPrevAnchoredButton;
static BOOLEAN gfAnchoredState;

static int8_t gbDisabledButtonStyle;

BOOLEAN gfRenderHilights = TRUE;

// Struct definition for the QuickButton pictures.
struct BUTTON_PICS {
  HVOBJECT vobj;         // The Image itself
  int32_t Grayed;        // Index to use for a "Grayed-out" button
  int32_t OffNormal;     // Index to use when button is OFF
  int32_t OffHilite;     // Index to use when button is OFF w/ hilite on it
  int32_t OnNormal;      // Index to use when button is ON
  int32_t OnHilite;      // Index to use when button is ON w/ hilite on it
  ButtonDimensions max;  // width/height of largest image in use
  uint32_t fFlags;       // Special image flags
};

static BUTTON_PICS ButtonPictures[MAX_BUTTON_PICS];

SGPVSurface *ButtonDestBuffer;

GUI_BUTTON *ButtonList[MAX_BUTTONS];

const ButtonDimensions *GetDimensionsOfButtonPic(const BUTTON_PICS *const pics) {
  return &pics->max;
}

static HVOBJECT GenericButtonOffNormal;
static HVOBJECT GenericButtonOffHilite;
static HVOBJECT GenericButtonOnNormal;
static HVOBJECT GenericButtonOnHilite;
static uint16_t GenericButtonFillColors;

static HVOBJECT GenericButtonIcons[MAX_BUTTON_ICONS];

static BOOLEAN gfDelayButtonDeletion = FALSE;
static BOOLEAN gfPendingButtonDeletion = FALSE;

extern MOUSE_REGION *MSYS_PrevRegion;

// Finds an available slot for loading button pictures
static BUTTON_PICS *FindFreeButtonSlot() {
  // Search for a slot
  FOR_EACH(BUTTON_PICS, i, ButtonPictures) {
    if (i->vobj == NULL) return i;
  }
  throw std::runtime_error("Out of button image slots");
}

static void SetMaxSize(BUTTON_PICS *const pics, const int32_t img_idx) {
  if (img_idx == BUTTON_NO_IMAGE) return;
  ETRLEObject const &e = pics->vobj->SubregionProperties(img_idx);
  uint32_t const w = e.sOffsetX + e.usWidth;
  uint32_t const h = e.sOffsetY + e.usHeight;
  if (pics->max.w < w) pics->max.w = w;
  if (pics->max.h < h) pics->max.h = h;
}

static void InitButtonImage(BUTTON_PICS *const pics, const HVOBJECT VObj, const uint32_t Flags,
                            const int32_t Grayed, const int32_t OffNormal, const int32_t OffHilite,
                            const int32_t OnNormal, const int32_t OnHilite) {
  pics->vobj = VObj;

  // Init the QuickButton image structure with indexes to use
  pics->Grayed = Grayed;
  pics->OffNormal = OffNormal;
  pics->OffHilite = OffHilite;
  pics->OnNormal = OnNormal;
  pics->OnHilite = OnHilite;
  pics->fFlags = Flags;

  // Fit the button size to the largest image in the set
  pics->max.w = 0;
  pics->max.h = 0;
  SetMaxSize(pics, Grayed);
  SetMaxSize(pics, OffNormal);
  SetMaxSize(pics, OffHilite);
  SetMaxSize(pics, OnNormal);
  SetMaxSize(pics, OnHilite);
}

BUTTON_PICS *LoadButtonImage(const char *filename, int32_t Grayed, int32_t OffNormal,
                             int32_t OffHilite, int32_t OnNormal, int32_t OnHilite) {
  AssertMsg(filename != NULL, "Attempting to LoadButtonImage() with null filename.");

  if (Grayed == BUTTON_NO_IMAGE && OffNormal == BUTTON_NO_IMAGE && OffHilite == BUTTON_NO_IMAGE &&
      OnNormal == BUTTON_NO_IMAGE && OnHilite == BUTTON_NO_IMAGE) {
    throw std::logic_error("No button pictures selected");
  }

  BUTTON_PICS *const UseSlot = FindFreeButtonSlot();
  SGPVObject *const VObj = AddVideoObjectFromFile(filename);
  InitButtonImage(UseSlot, VObj, GUI_BTN_NONE, Grayed, OffNormal, OffHilite, OnNormal, OnHilite);
  return UseSlot;
}

BUTTON_PICS *LoadButtonImage(char const *const filename, int32_t const off_normal,
                             int32_t const on_normal) {
  return LoadButtonImage(filename, -1, off_normal, -1, on_normal, -1);
}

BUTTON_PICS *UseLoadedButtonImage(BUTTON_PICS *const LoadedImg, const int32_t Grayed,
                                  const int32_t OffNormal, const int32_t OffHilite,
                                  const int32_t OnNormal, const int32_t OnHilite) {
  if (Grayed == BUTTON_NO_IMAGE && OffNormal == BUTTON_NO_IMAGE && OffHilite == BUTTON_NO_IMAGE &&
      OnNormal == BUTTON_NO_IMAGE && OnHilite == BUTTON_NO_IMAGE) {
    throw std::logic_error("No button pictures selected for pre-loaded button image");
  }

  // Is button image index given valid?
  const HVOBJECT vobj = LoadedImg->vobj;
  if (!vobj) {
    throw std::logic_error("Invalid button picture handle given for pre-loaded button image");
  }

  BUTTON_PICS *const UseSlot = FindFreeButtonSlot();
  InitButtonImage(UseSlot, vobj, GUI_BTN_DUPLICATE_VOBJ, Grayed, OffNormal, OffHilite, OnNormal,
                  OnHilite);
  return UseSlot;
}

BUTTON_PICS *UseLoadedButtonImage(BUTTON_PICS *const img, int32_t const off_normal,
                                  int32_t const on_normal) {
  return UseLoadedButtonImage(img, -1, off_normal, -1, on_normal, -1);
}

void UnloadButtonImage(BUTTON_PICS *const pics) {
#if defined BUTTONSYSTEM_DEBUGGING
  AssertMsg(pics->vobj != NULL,
            "Attempting to UnloadButtonImage that has a "
            "null vobj (already deleted).");
#endif
  if (pics->vobj == NULL) return;

  // If this is a duplicated button image, then don't trash the vobject
  if (!(pics->fFlags & GUI_BTN_DUPLICATE_VOBJ)) {
    /* Deleting a non-duplicate, so see if any dups present. if so, then convert
     * one of them to an original!
     */
    FOR_EACH(BUTTON_PICS, other, ButtonPictures) {
      if (other == pics) continue;
      if (other->vobj != pics->vobj) continue;
      if (!(other->fFlags & GUI_BTN_DUPLICATE_VOBJ)) continue;

      /* If we got here, then we got a duplicate object of the one we want to
       * delete, so convert it to an original!
       */
      other->fFlags &= ~GUI_BTN_DUPLICATE_VOBJ;

      // Now remove this button, but not its vobject
      goto remove_pic;
    }

    DeleteVideoObject(pics->vobj);
  }

remove_pic:
  pics->vobj = NULL;
}

void EnableButton(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->uiFlags |= BUTTON_ENABLED | BUTTON_DIRTY;
}

void DisableButton(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->uiFlags &= ~BUTTON_ENABLED;
  b->uiFlags |= BUTTON_DIRTY;
}

void EnableButton(GUIButtonRef const b, bool const enable) {
  enable ? EnableButton(b) : DisableButton(b);
}

/* Initializes the button image sub-system. This function is called by
 * InitButtonSystem.
 */
static void InitializeButtonImageManager() {
  // Blank out all QuickButton images
  for (int x = 0; x < MAX_BUTTON_PICS; ++x) {
    BUTTON_PICS *const pics = &ButtonPictures[x];
    pics->vobj = NULL;
    pics->Grayed = -1;
    pics->OffNormal = -1;
    pics->OffHilite = -1;
    pics->OnNormal = -1;
    pics->OnHilite = -1;
  }

  // Blank out all Generic button data
  GenericButtonOffNormal = NULL;
  GenericButtonOffHilite = NULL;
  GenericButtonOnNormal = NULL;
  GenericButtonOnHilite = NULL;
  GenericButtonFillColors = 0;

  // Blank out all icon images
  for (int x = 0; x < MAX_BUTTON_ICONS; ++x) GenericButtonIcons[x] = NULL;

  // Load the default generic button images
  GenericButtonOffNormal = AddVideoObjectFromFile(DEFAULT_GENERIC_BUTTON_OFF);
  GenericButtonOnNormal = AddVideoObjectFromFile(DEFAULT_GENERIC_BUTTON_ON);

  /* Load up the off hilite and on hilite images. We won't check for errors
   * because if the file doesn't exists, the system simply ignores that file.
   * These are only here as extra images, they aren't required for operation
   * (only OFF Normal and ON Normal are required).
   */
  try {
    GenericButtonOffHilite = AddVideoObjectFromFile(DEFAULT_GENERIC_BUTTON_OFF_HI);
  } catch (...) { /* see comment above */
  }
  try {
    GenericButtonOnHilite = AddVideoObjectFromFile(DEFAULT_GENERIC_BUTTON_ON_HI);
  } catch (...) { /* see comment above */
  }

  uint8_t const Pix = GenericButtonOffNormal->GetETRLEPixelValue(8, 0, 0);
  GenericButtonFillColors = GenericButtonOffNormal->Palette16()[Pix];
}

// Finds the next available slot for button icon images.
static int16_t FindFreeIconSlot() {
  for (int16_t x = 0; x < MAX_BUTTON_ICONS; ++x) {
    if (GenericButtonIcons[x] == NULL) return x;
  }
  throw std::runtime_error("Out of generic button icon slots");
}

int16_t LoadGenericButtonIcon(const char *filename) {
  AssertMsg(filename != NULL, "Attempting to LoadGenericButtonIcon() with null filename.");

  // Get slot for icon image
  int16_t const ImgSlot = FindFreeIconSlot();

  // Load the icon
  GenericButtonIcons[ImgSlot] = AddVideoObjectFromFile(filename);

  // Return the slot number
  return ImgSlot;
}

void UnloadGenericButtonIcon(int16_t GenImg) {
  AssertMsg(0 <= GenImg && GenImg < MAX_BUTTON_ICONS,
            String("Attempting to UnloadGenericButtonIcon with out of range index %d.", GenImg));

#if defined BUTTONSYSTEM_DEBUGGING
  AssertMsg(GenericButtonIcons[GenImg],
            "Attempting to UnloadGenericButtonIcon "
            "that has no icon (already deleted).");
#endif
  if (!GenericButtonIcons[GenImg]) return;
  // If an icon is present in the slot, remove it.
  DeleteVideoObject(GenericButtonIcons[GenImg]);
  GenericButtonIcons[GenImg] = NULL;
}

// Cleans up, and shuts down the button image manager sub-system.
static void ShutdownButtonImageManager() {
  // Remove all QuickButton images
  FOR_EACH(BUTTON_PICS, i, ButtonPictures) {
    if (i->vobj != NULL) UnloadButtonImage(i);
  }

  // Remove all GenericButton images
  if (GenericButtonOffNormal != NULL) {
    DeleteVideoObject(GenericButtonOffNormal);
    GenericButtonOffNormal = NULL;
  }

  if (GenericButtonOffHilite != NULL) {
    DeleteVideoObject(GenericButtonOffHilite);
    GenericButtonOffHilite = NULL;
  }

  if (GenericButtonOnNormal != NULL) {
    DeleteVideoObject(GenericButtonOnNormal);
    GenericButtonOnNormal = NULL;
  }

  if (GenericButtonOnHilite != NULL) {
    DeleteVideoObject(GenericButtonOnHilite);
    GenericButtonOnHilite = NULL;
  }

  GenericButtonFillColors = 0;

  // Remove all button icons
  for (int x = 0; x < MAX_BUTTON_ICONS; ++x) {
    if (GenericButtonIcons[x] != NULL) UnloadGenericButtonIcon(x);
  }
}

void InitButtonSystem() {
  ButtonDestBuffer = FRAME_BUFFER;

  // Initialize the button image manager sub-system
  InitializeButtonImageManager();
}

void ShutdownButtonSystem() {
  // Kill off all buttons in the system
  FOR_EACH_BUTTON(i) { delete *i; }
  ShutdownButtonImageManager();
}

static void RemoveButtonsMarkedForDeletion() {
  FOR_EACH_BUTTON(i) {
    if ((*i)->uiFlags & BUTTON_DELETION_PENDING) delete *i;
  }
}

void RemoveButton(GUIButtonRef &btn) {
  int32_t const btn_id = btn.ID();
  btn.Reset();

  CHECKV(0 < btn_id && btn_id < MAX_BUTTONS);  // XXX HACK000C
  AssertMsg(0 < btn_id && btn_id < MAX_BUTTONS, String("ButtonID %d is out of range.", btn_id));
  GUI_BUTTON *const b = ButtonList[btn_id];
  CHECKV(b);  // XXX HACK000C
  AssertMsg(b, String("Accessing non-existent button %d.", btn_id));

  /* If we happen to be in the middle of a callback, and attempt to delete a
   * button, like deleting a node during list processing, then we delay it till
   * after the callback is completed.
   */
  if (gfDelayButtonDeletion) {
    b->uiFlags |= BUTTON_DELETION_PENDING;
    gfPendingButtonDeletion = TRUE;
    return;
  }

  delete b;
}

// Finds the next available button slot.
static int32_t GetNextButtonNumber() {
  /* Never hand out ID 0.  Slot 0 is always a null pointer */
  for (int32_t x = 1; x < MAX_BUTTONS; x++) {
    if (ButtonList[x] == NULL) return x;
  }
  throw std::runtime_error("No more button slots");
}

static void QuickButtonCallbackMButn(MOUSE_REGION *reg, int32_t reason);
static void QuickButtonCallbackMMove(MOUSE_REGION *reg, int32_t reason);

GUI_BUTTON::GUI_BUTTON(uint32_t const flags, int16_t const left, int16_t const top,
                       int16_t const width, int16_t const height, int8_t const priority,
                       GUI_CALLBACK const click, GUI_CALLBACK const move)
    : IDNum(GetNextButtonNumber()),
      image(0),
      Area(left, top, width, height, priority, MSYS_STARTING_CURSORVAL, QuickButtonCallbackMMove,
           QuickButtonCallbackMButn),
      ClickCallback(click),
      MoveCallback(move),
      uiFlags(BUTTON_DIRTY | BUTTON_ENABLED | flags),
      uiOldFlags(0),
      bDisabledStyle(GUI_BUTTON::DISABLED_STYLE_DEFAULT),
      string(0),
      usFont(0),
      sForeColor(0),
      sShadowColor(-1),
      sForeColorDown(-1),
      sShadowColorDown(-1),
      sForeColorHilited(-1),
      sShadowColorHilited(-1),
      bJustification(GUI_BUTTON::TEXT_CENTER),
      bTextXOffset(-1),
      bTextYOffset(-1),
      bTextXSubOffSet(-1),
      bTextYSubOffSet(-1),
      fShiftText(TRUE),
      sWrappedWidth(-1),
      icon(0),
      usIconIndex(-1),
      bIconXOffset(-1),
      bIconYOffset(-1),
      fShiftImage(TRUE),
      ubToggleButtonActivated(FALSE) {
  AssertMsg(left >= 0 && top >= 0 && width >= 0 && height >= 0,
            String("Attempting to create button with invalid coordinates %dx%d+%dx%d", left, top,
                   width, height));

  Area.SetUserPtr(this);

#ifdef BUTTONSYSTEM_DEBUGGING
  AssertFailIfIdenticalButtonAttributesFound(this);
#endif

  ButtonList[IDNum] = this;

  SpecifyButtonSoundScheme(this, BUTTON_SOUND_SCHEME_GENERIC);
}

GUI_BUTTON::~GUI_BUTTON() {
  if (this == gpAnchoredButton) gpAnchoredButton = 0;
  if (this == gpPrevAnchoredButton) gpPrevAnchoredButton = 0;

  ButtonList[IDNum] = 0;

  if (uiFlags & BUTTON_SELFDELETE_IMAGE) {
    /* Checkboxes and simple create buttons have their own graphics associated
     * with them, and it is handled internally.  We delete it here.  This
     * provides the advantage of less micromanagement, but with the
     * disadvantage of wasting more memory if you have lots of buttons using the
     * same graphics.
     */
    UnloadButtonImage(image);
  }

  if (string) MemFree(string);
}

static void CopyButtonText(GUI_BUTTON *b, const wchar_t *text) {
  if (text == NULL || text[0] == L'\0') return;

  wchar_t *const Buf = MALLOCN(wchar_t, wcslen(text) + 1);
  wcscpy(Buf, text);
  b->string = Buf;
}

static void DefaultMoveCallback(GUI_BUTTON *btn, int32_t reason);

GUIButtonRef CreateIconButton(int16_t Icon, int16_t IconIndex, int16_t xloc, int16_t yloc,
                              int16_t w, int16_t h, int16_t Priority, GUI_CALLBACK ClickCallback) {
  // if button size is too small, adjust it.
  if (w < 4) w = 4;
  if (h < 3) h = 3;

  GUI_BUTTON *const b = new GUI_BUTTON(BUTTON_GENERIC, xloc, yloc, w, h, Priority, ClickCallback,
                                       DefaultMoveCallback);
  b->icon = GenericButtonIcons[Icon];
  b->usIconIndex = IconIndex;
  return b;
}

GUIButtonRef CreateTextButton(const wchar_t *string, Font const font, int16_t sForeColor,
                              int16_t sShadowColor, int16_t xloc, int16_t yloc, int16_t w,
                              int16_t h, int16_t Priority, GUI_CALLBACK ClickCallback) {
  // if button size is too small, adjust it.
  if (w < 4) w = 4;
  if (h < 3) h = 3;

  GUI_BUTTON *const b = new GUI_BUTTON(BUTTON_GENERIC, xloc, yloc, w, h, Priority, ClickCallback,
                                       DefaultMoveCallback);
  CopyButtonText(b, string);
  b->usFont = font;
  b->sForeColor = sForeColor;
  b->sShadowColor = sShadowColor;
  return b;
}

GUIButtonRef CreateHotSpot(int16_t xloc, int16_t yloc, int16_t Width, int16_t Height,
                           int16_t Priority, GUI_CALLBACK ClickCallback) {
  return new GUI_BUTTON(BUTTON_HOT_SPOT, xloc, yloc, Width, Height, Priority, ClickCallback,
                        DefaultMoveCallback);
}

static GUIButtonRef QuickCreateButtonInternal(BUTTON_PICS *const pics, const int16_t xloc,
                                              const int16_t yloc, const int32_t Type,
                                              const int16_t Priority,
                                              const GUI_CALLBACK MoveCallback,
                                              const GUI_CALLBACK ClickCallback) {
  // Is there a QuickButton image in the given image slot?
  if (!pics->vobj) {
    throw std::runtime_error("QuickCreateButton: Invalid button image");
  }

  GUI_BUTTON *const b =
      new GUI_BUTTON((Type & (BUTTON_CHECKBOX | BUTTON_NEWTOGGLE)) | BUTTON_QUICK, xloc, yloc,
                     pics->max.w, pics->max.h, Priority, ClickCallback, MoveCallback);
  b->image = pics;
  return b;
}

GUIButtonRef QuickCreateButton(BUTTON_PICS *const image, const int16_t x, const int16_t y,
                               const int16_t priority, const GUI_CALLBACK click) {
  return QuickCreateButtonInternal(image, x, y, BUTTON_TOGGLE, priority, DefaultMoveCallback,
                                   click);
}

GUIButtonRef QuickCreateButtonNoMove(BUTTON_PICS *const image, const int16_t x, const int16_t y,
                                     const int16_t priority, const GUI_CALLBACK click) {
  return QuickCreateButtonInternal(image, x, y, BUTTON_TOGGLE, priority, MSYS_NO_CALLBACK, click);
}

GUIButtonRef QuickCreateButtonToggle(BUTTON_PICS *const image, const int16_t x, const int16_t y,
                                     const int16_t priority, const GUI_CALLBACK click) {
  return QuickCreateButtonInternal(image, x, y, BUTTON_NEWTOGGLE, priority, MSYS_NO_CALLBACK,
                                   click);
}

GUIButtonRef QuickCreateButtonImg(const char *gfx, int32_t grayed, int32_t off_normal,
                                  int32_t off_hilite, int32_t on_normal, int32_t on_hilite,
                                  int16_t x, int16_t y, int16_t priority, GUI_CALLBACK click) {
  BUTTON_PICS *const img =
      LoadButtonImage(gfx, grayed, off_normal, off_hilite, on_normal, on_hilite);
  GUIButtonRef const btn = QuickCreateButton(img, x, y, priority, click);
  btn->uiFlags |= BUTTON_SELFDELETE_IMAGE;
  return btn;
}

GUIButtonRef QuickCreateButtonImg(char const *const gfx, int32_t const off_normal,
                                  int32_t const on_normal, int16_t const x, int16_t const y,
                                  int16_t const priority, GUI_CALLBACK const click) {
  return QuickCreateButtonImg(gfx, -1, off_normal, -1, on_normal, -1, x, y, priority, click);
}

GUIButtonRef CreateIconAndTextButton(BUTTON_PICS *const Image, const wchar_t *const string,
                                     Font const font, const int16_t sForeColor,
                                     const int16_t sShadowColor, const int16_t sForeColorDown,
                                     const int16_t sShadowColorDown, const int16_t xloc,
                                     const int16_t yloc, const int16_t Priority,
                                     const GUI_CALLBACK ClickCallback) {
  GUIButtonRef const b = QuickCreateButton(Image, xloc, yloc, Priority, ClickCallback);
  CopyButtonText(b, string);
  b->usFont = font;
  b->sForeColor = sForeColor;
  b->sShadowColor = sShadowColor;
  b->sForeColorDown = sForeColorDown;
  b->sShadowColorDown = sShadowColorDown;
  return b;
}

GUIButtonRef CreateLabel(const wchar_t *text, Font const font, int16_t forecolor,
                         int16_t shadowcolor, int16_t x, int16_t y, int16_t w, int16_t h,
                         int16_t priority) {
  GUIButtonRef const btn =
      CreateTextButton(text, font, forecolor, shadowcolor, x, y, w, h, priority, NULL);
  btn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_NONE);
  DisableButton(btn);
  return btn;
}

void GUI_BUTTON::SpecifyText(wchar_t const *const text) {
  // free the previous strings memory if applicable
  if (string) MemFree(string);
  string = NULL;

  CopyButtonText(this, text);
  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::SpecifyDownTextColors(int16_t const fore_colour_down,
                                       int16_t const shadow_colour_down) {
  sForeColorDown = fore_colour_down;
  sShadowColorDown = shadow_colour_down;
  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::SpecifyHilitedTextColors(int16_t const fore_colour_highlighted,
                                          int16_t const shadow_colour_highlighted) {
  sForeColorHilited = fore_colour_highlighted;
  sShadowColorHilited = shadow_colour_highlighted;
  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::SpecifyTextJustification(Justification const j) {
  bJustification = j;
  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::SpecifyGeneralTextAttributes(wchar_t const *const string, Font const font,
                                              int16_t const fore_colour,
                                              int16_t const shadow_colour) {
  SpecifyText(string);
  usFont = font;
  sForeColor = fore_colour;
  sShadowColor = shadow_colour;
  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::SpecifyTextOffsets(int8_t const text_x_offset, int8_t const text_y_offset,
                                    BOOLEAN const shift_text) {
  bTextXOffset = text_x_offset;
  bTextYOffset = text_y_offset;
  fShiftText = shift_text;
}

void GUI_BUTTON::SpecifyTextSubOffsets(int8_t const text_x_offset, int8_t const text_y_offset,
                                       BOOLEAN const shift_text) {
  bTextXSubOffSet = text_x_offset;
  bTextYSubOffSet = text_y_offset;
  fShiftText = shift_text;
}

void GUI_BUTTON::SpecifyTextWrappedWidth(int16_t const wrapped_width) {
  sWrappedWidth = wrapped_width;
}

void GUI_BUTTON::SpecifyDisabledStyle(DisabledStyle const style) { bDisabledStyle = style; }

void GUI_BUTTON::SpecifyIcon(SGPVObject const *const icon_, uint16_t const usVideoObjectIndex,
                             int8_t const bXOffset, int8_t const bYOffset, BOOLEAN const) {
  icon = icon_;
  usIconIndex = usVideoObjectIndex;

  if (!icon_) return;

  bIconXOffset = bXOffset;
  bIconYOffset = bYOffset;
  fShiftImage = TRUE;

  uiFlags |= BUTTON_DIRTY;
}

void GUI_BUTTON::AllowDisabledFastHelp() { Area.uiFlags |= MSYS_ALLOW_DISABLED_FASTHELP; }

void GUI_BUTTON::SetFastHelpText(const wchar_t *const text) { Area.SetFastHelpText(text); }

/* Dispatches all button callbacks for mouse movement. This function gets
 * called by the Mouse System. *DO NOT CALL DIRECTLY*
 */
static void QuickButtonCallbackMMove(MOUSE_REGION *reg, int32_t reason) {
  Assert(reg != NULL);
  GUI_BUTTON *const b = reg->GetUserPtr<GUI_BUTTON>();

  // ATE: New stuff for toggle buttons that work with new Win95 paradigm
  if (b->uiFlags & BUTTON_NEWTOGGLE && reason & MSYS_CALLBACK_REASON_LOST_MOUSE &&
      b->ubToggleButtonActivated) {
    b->uiFlags ^= BUTTON_CLICKED_ON;
    b->ubToggleButtonActivated = FALSE;
  }

  if (!b->Enabled()) return;

  if (reason & (MSYS_CALLBACK_REASON_LOST_MOUSE | MSYS_CALLBACK_REASON_GAIN_MOUSE)) {
    b->uiFlags |= BUTTON_DIRTY;
  }

  if (b->MoveCallback) b->MoveCallback(b, reason);
}

/* Dispatches all button callbacks for button presses. This function is called
 * by the Mouse System. *DO NOT CALL DIRECTLY*
 */
static void QuickButtonCallbackMButn(MOUSE_REGION *reg, int32_t reason) {
  Assert(reg != NULL);
  GUI_BUTTON *const b = reg->GetUserPtr<GUI_BUTTON>();

  // ATE: New stuff for toggle buttons that work with new Win95 paradigm
  if (!b->Enabled()) {
    // Should we play a sound if clicked on while disabled?
    if (b->ubSoundSchemeID &&
        reason & (MSYS_CALLBACK_REASON_LBUTTON_DWN | MSYS_CALLBACK_REASON_RBUTTON_DWN)) {
      PlayButtonSound(b, BUTTON_SOUND_DISABLED_CLICK);
    }
    return;
  }

  bool StateBefore = b->Clicked();
  bool StateAfter = true;  // XXX HACK000E

  if (b->uiFlags & BUTTON_NEWTOGGLE) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
      if (!b->ubToggleButtonActivated) {
        b->uiFlags ^= BUTTON_CLICKED_ON;
        b->ubToggleButtonActivated = TRUE;
      }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      b->ubToggleButtonActivated = FALSE;
    }
  }

  /* Kris:
   * Set the anchored button incase the user moves mouse off region while still
   * holding down the button, but only if the button is up.  In Win95, buttons
   * that are already down, and anchored never change state, unless you release
   * the mouse in the button area.
   */
  if (b->MoveCallback == DefaultMoveCallback) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
      gpAnchoredButton = b;
      gfAnchoredState = StateBefore;
      b->uiFlags |= BUTTON_CLICKED_ON;
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      b->uiFlags &= ~BUTTON_CLICKED_ON;
    }
  } else if (b->uiFlags & BUTTON_CHECKBOX) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
      /* The check box button gets anchored, though it doesn't actually use the
       * anchoring move callback.  The effect is different, we don't want to
       * toggle the button state, but we do want to anchor this button so that
       * we don't effect any other buttons while we move the mouse around in
       * anchor mode.
       */
      gpAnchoredButton = b;
      gfAnchoredState = StateBefore;

      /* Trick the before state of the button to be different so the sound will
       * play properly as checkbox buttons are processed differently.
       */
      StateBefore = !b->Clicked();
      StateAfter = !StateBefore;
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      b->uiFlags ^= BUTTON_CLICKED_ON;  // toggle the checkbox state upon release
                                        // inside button area.
      /* Trick the before state of the button to be different so the sound will
       * play properly as checkbox buttons are processed differently.
       */
      StateBefore = !b->Clicked();
      StateAfter = !StateBefore;
    }
  }

  // If there is a callback function with this button, call it
  if (b->ClickCallback != NULL) {
    /* Kris:  January 6, 1998
     * Added these checks to avoid a case where it was possible to process a
     * leftbuttonup message when the button wasn't anchored, and should have
     * been.
     */
    gfDelayButtonDeletion = TRUE;
    if (!(reason & MSYS_CALLBACK_REASON_LBUTTON_UP) || b->MoveCallback != DefaultMoveCallback ||
        gpPrevAnchoredButton == b) {
      b->ClickCallback(b, reason);
    }
    gfDelayButtonDeletion = FALSE;
  } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    // Otherwise, do default action with this button.
    b->uiFlags ^= BUTTON_CLICKED_ON;
  }

  if (b->uiFlags & BUTTON_CHECKBOX) {
    StateAfter = b->Clicked();
  }

  // Play sounds for this enabled button (disabled sounds have already been
  // done)
  if (b->ubSoundSchemeID && b->Enabled()) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      if (StateBefore && !StateAfter) {
        PlayButtonSound(b, BUTTON_SOUND_CLICKED_OFF);
      }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
      if (!StateBefore && StateAfter) {
        PlayButtonSound(b, BUTTON_SOUND_CLICKED_ON);
      }
    }
  }

  if (StateBefore != StateAfter) {
    InvalidateRegion(b->X(), b->Y(), b->BottomRightX(), b->BottomRightY());
  }

  if (gfPendingButtonDeletion) RemoveButtonsMarkedForDeletion();
}

static void DrawButtonFromPtr(GUI_BUTTON *b);

void RenderButtons() {
  SaveFontSettings();
  FOR_EACH_BUTTON(i) {
    // If the button exists, and it's not owned by another object, draw it
    // Kris:  and make sure that the button isn't hidden.
    GUI_BUTTON *const b = *i;
    if (!(b->Area.uiFlags & MSYS_REGION_ENABLED)) continue;

    if ((b->uiFlags ^ b->uiOldFlags) & (BUTTON_CLICKED_ON | BUTTON_ENABLED)) {
      // Something is different, set dirty!
      b->uiFlags |= BUTTON_DIRTY;
    }

    // Set old flags
    b->uiOldFlags = b->uiFlags;

    if (b->uiFlags & BUTTON_FORCE_UNDIRTY) {
      b->uiFlags &= ~BUTTON_DIRTY;
      b->uiFlags &= ~BUTTON_FORCE_UNDIRTY;
    }

    // Check if we need to update!
    if (b->uiFlags & BUTTON_DIRTY) {
      // Turn off dirty flag
      b->uiFlags &= ~BUTTON_DIRTY;
      DrawButtonFromPtr(b);

      InvalidateRegion(b->X(), b->Y(), b->BottomRightX(), b->BottomRightY());
    }
  }

  RestoreFontSettings();
}

void MarkAButtonDirty(GUIButtonRef const b) {
  // surgical dirtying -> marks a user specified button dirty, without dirty the
  // whole lot of them
  CHECKV(b != NULL);  // XXX HACK000C
  b->uiFlags |= BUTTON_DIRTY;
}

void MarkButtonsDirty() {
  FOR_EACH_BUTTON(i) { (*i)->uiFlags |= BUTTON_DIRTY; }
}

void UnMarkButtonDirty(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->uiFlags &= ~BUTTON_DIRTY;
}

void UnmarkButtonsDirty() {
  FOR_EACH_BUTTON(i) { UnMarkButtonDirty(*i); }
}

void ForceButtonUnDirty(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->uiFlags &= ~BUTTON_DIRTY;
  b->uiFlags |= BUTTON_FORCE_UNDIRTY;
}

void GUI_BUTTON::Draw() {
  if (string) SaveFontSettings();
  if (Area.uiFlags & MSYS_REGION_ENABLED) DrawButtonFromPtr(this);
  if (string) RestoreFontSettings();
}

static void DrawCheckBoxButton(const GUI_BUTTON *b);
static void DrawGenericButton(const GUI_BUTTON *b);
static void DrawHatchOnButton(const GUI_BUTTON *b);
static void DrawIconOnButton(const GUI_BUTTON *b);
static void DrawQuickButton(const GUI_BUTTON *b);
static void DrawShadeOnButton(const GUI_BUTTON *b);
static void DrawTextOnButton(const GUI_BUTTON *b);

// Given a pointer to a GUI_BUTTON structure, draws the button on the screen.
static void DrawButtonFromPtr(GUI_BUTTON *b) {
  // Draw the appropriate button according to button type
  gbDisabledButtonStyle = GUI_BUTTON::DISABLED_STYLE_NONE;
  switch (b->uiFlags & BUTTON_TYPES) {
    case BUTTON_QUICK:
      DrawQuickButton(b);
      break;
    case BUTTON_GENERIC:
      DrawGenericButton(b);
      break;
    case BUTTON_CHECKBOX:
      DrawCheckBoxButton(b);
      break;

    case BUTTON_HOT_SPOT:
      return;  // hotspots don't have text, but if you want to, change this to a
               // break!
  }
  if (b->icon) DrawIconOnButton(b);
  if (b->string) DrawTextOnButton(b);
  /* If the button is disabled, and a style has been calculated, then draw the
   * style last.
   */
  switch (gbDisabledButtonStyle) {
    case GUI_BUTTON::DISABLED_STYLE_HATCHED:
      DrawHatchOnButton(b);
      break;
    case GUI_BUTTON::DISABLED_STYLE_SHADED:
      DrawShadeOnButton(b);
      break;
  }
}

// Draws a QuickButton type button on the screen.
static void DrawQuickButton(const GUI_BUTTON *b) {
  const BUTTON_PICS *const pics = b->image;

  int32_t UseImage = 0;
  if (b->Enabled()) {
    if (b->Clicked()) {
      // Is the mouse over this area, and we have a hilite image?
      if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights && pics->OnHilite != -1) {
        UseImage = pics->OnHilite;
      } else if (pics->OnNormal != -1) {
        UseImage = pics->OnNormal;
      }
    } else {
      // Is the mouse over the button, and do we have hilite image?
      if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights && pics->OffHilite != -1) {
        UseImage = pics->OffHilite;
      } else if (pics->OffNormal != -1) {
        UseImage = pics->OffNormal;
      }
    }
  } else if (pics->Grayed != -1) {
    // Button is diabled so use the "Grayed-out" image
    UseImage = pics->Grayed;
  } else {
    UseImage = pics->OffNormal;
    switch (b->bDisabledStyle) {
      case GUI_BUTTON::DISABLED_STYLE_DEFAULT:
        gbDisabledButtonStyle =
            b->string ? GUI_BUTTON::DISABLED_STYLE_SHADED : GUI_BUTTON::DISABLED_STYLE_HATCHED;
        break;

      case GUI_BUTTON::DISABLED_STYLE_HATCHED:
      case GUI_BUTTON::DISABLED_STYLE_SHADED:
        gbDisabledButtonStyle = b->bDisabledStyle;
        break;
    }
  }

  BltVideoObject(ButtonDestBuffer, pics->vobj, UseImage, b->X(), b->Y());
}

static void DrawHatchOnButton(const GUI_BUTTON *b) {
  SGPRect ClipRect;
  ClipRect.iLeft = b->X();
  ClipRect.iRight = b->BottomRightX() - 1;
  ClipRect.iTop = b->Y();
  ClipRect.iBottom = b->BottomRightY() - 1;
  SGPVSurface::Lock l(ButtonDestBuffer);
  Blt16BPPBufferHatchRect(l.Buffer<uint16_t>(), l.Pitch(), &ClipRect);
}

static void DrawShadeOnButton(const GUI_BUTTON *b) {
  ButtonDestBuffer->ShadowRect(b->X(), b->Y(), b->BottomRightX(), b->BottomRightY());
}

void GUI_BUTTON::DrawCheckBoxOnOff(BOOLEAN const on) {
  BOOLEAN const fLeftButtonState = gfLeftButtonState;

  gfLeftButtonState = on;
  Area.uiFlags |= MSYS_MOUSE_IN_AREA;
  Draw();

  gfLeftButtonState = fLeftButtonState;
}

static void DrawCheckBoxButton(const GUI_BUTTON *b) {
  const BUTTON_PICS *const pics = b->image;

  int32_t UseImage = 0;
  if (b->Enabled()) {
    if (b->Clicked()) {
      // Is the mouse over this area, and we have a hilite image?
      if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights && gfLeftButtonState &&
          pics->OnHilite != -1) {
        UseImage = pics->OnHilite;
      } else if (pics->OnNormal != -1) {
        UseImage = pics->OnNormal;
      }
    } else {
      // Is the mouse over the button, and do we have hilite image?
      if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights && gfLeftButtonState &&
          pics->OffHilite != -1) {
        UseImage = pics->OffHilite;
      } else if (pics->OffNormal != -1) {
        UseImage = pics->OffNormal;
      }
    }
  } else if (pics->Grayed != -1) {
    // Button is disabled so use the "Grayed-out" image
    UseImage = pics->Grayed;
  } else  // use the disabled style
  {
    if (b->Clicked()) {
      UseImage = pics->OnHilite;
    } else {
      UseImage = pics->OffHilite;
    }
    switch (b->bDisabledStyle) {
      case GUI_BUTTON::DISABLED_STYLE_DEFAULT:
        gbDisabledButtonStyle = GUI_BUTTON::DISABLED_STYLE_HATCHED;
        break;

      case GUI_BUTTON::DISABLED_STYLE_HATCHED:
      case GUI_BUTTON::DISABLED_STYLE_SHADED:
        gbDisabledButtonStyle = b->bDisabledStyle;
        break;
    }
  }

  BltVideoObject(ButtonDestBuffer, pics->vobj, UseImage, b->X(), b->Y());
}

static void DrawIconOnButton(const GUI_BUTTON *b) {
  if (!b->icon) return;

  // Get width and height of button area
  int32_t const width = b->W();
  int32_t const height = b->H();

  // Compute viewable area (inside borders)
  SGPRect NewClip;
  NewClip.iLeft = b->X() + 3;
  NewClip.iRight = b->X() + width - 3;
  NewClip.iTop = b->Y() + 2;
  NewClip.iBottom = b->Y() + height - 2;

  // Get Icon's blit start coordinates
  int32_t IconX = NewClip.iLeft;
  int32_t IconY = NewClip.iTop;

  // Get current clip area
  SGPRect OldClip;
  GetClippingRect(&OldClip);

  // Clip button's viewable area coords to screen
  if (NewClip.iLeft < OldClip.iLeft) NewClip.iLeft = OldClip.iLeft;

  // Is button right off the right side of the screen?
  if (NewClip.iLeft > OldClip.iRight) return;

  if (NewClip.iRight > OldClip.iRight) NewClip.iRight = OldClip.iRight;

  // Is button completely off the left side of the screen?
  if (NewClip.iRight < OldClip.iLeft) return;

  if (NewClip.iTop < OldClip.iTop) NewClip.iTop = OldClip.iTop;

  // Are we right off the bottom of the screen?
  if (NewClip.iTop > OldClip.iBottom) return;

  if (NewClip.iBottom > OldClip.iBottom) NewClip.iBottom = OldClip.iBottom;

  // Are we off the top?
  if (NewClip.iBottom < OldClip.iTop) return;

  // Did we clip the viewable area out of existance?
  if (NewClip.iRight <= NewClip.iLeft || NewClip.iBottom <= NewClip.iTop) return;

  // Get the width and height of the icon itself
  SGPVObject const *const hvObject = b->icon;
  ETRLEObject const &pTrav = hvObject->SubregionProperties(b->usIconIndex);

  /* Compute coordinates for centering the icon on the button or use the offset
   * system.
   */
  int32_t xp;
  if (b->bIconXOffset == -1) {
    const int32_t IconW = pTrav.usWidth + pTrav.sOffsetX;
    xp = IconX + (width - 6 - IconW) / 2;
  } else {
    xp = b->X() + b->bIconXOffset;
  }

  int32_t yp;
  if (b->bIconYOffset == -1) {
    const int32_t IconH = pTrav.usHeight + pTrav.sOffsetY;
    yp = IconY + (height - 4 - IconH) / 2;
  } else {
    yp = b->Y() + b->bIconYOffset;
  }

  /* Was the button clicked on? if so, move the image slightly for the illusion
   * that the image moved into the screen.
   */
  if (b->Clicked() && b->fShiftImage) {
    xp++;
    yp++;
  }

  // Set the clipping rectangle to the viewable area of the button
  SetClippingRect(&NewClip);

  BltVideoObject(ButtonDestBuffer, hvObject, b->usIconIndex, xp, yp);

  // Restore previous clip region
  SetClippingRect(&OldClip);
}

// If a button has text attached to it, then it'll draw it last.
static void DrawTextOnButton(const GUI_BUTTON *b) {
  // If this button actually has a string to print
  if (b->string == NULL) return;

  // Get the width and height of this button
  int32_t const width = b->W();
  int32_t const height = b->H();

  // Compute the viewable area on this button
  SGPRect NewClip;
  NewClip.iLeft = b->X() + 3;
  NewClip.iRight = b->X() + width - 3;
  NewClip.iTop = b->Y() + 2;
  NewClip.iBottom = b->Y() + height - 2;

  // Get the starting coordinates to print
  const int32_t TextX = NewClip.iLeft;
  const int32_t TextY = NewClip.iTop;

  // Get the current clipping area
  SGPRect OldClip;
  GetClippingRect(&OldClip);

  // Clip the button's viewable area to the screen
  if (NewClip.iLeft < OldClip.iLeft) NewClip.iLeft = OldClip.iLeft;

  // Are we off hte right side?
  if (NewClip.iLeft > OldClip.iRight) return;

  if (NewClip.iRight > OldClip.iRight) NewClip.iRight = OldClip.iRight;

  // Are we off the left side?
  if (NewClip.iRight < OldClip.iLeft) return;

  if (NewClip.iTop < OldClip.iTop) NewClip.iTop = OldClip.iTop;

  // Are we off the bottom of the screen?
  if (NewClip.iTop > OldClip.iBottom) return;

  if (NewClip.iBottom > OldClip.iBottom) NewClip.iBottom = OldClip.iBottom;

  // Are we off the top?
  if (NewClip.iBottom < OldClip.iTop) return;

  // Did we clip the viewable area out of existance?
  if (NewClip.iRight <= NewClip.iLeft || NewClip.iBottom <= NewClip.iTop) return;

  // Set the font printing settings to the buttons viewable area
  SetFontDestBuffer(ButtonDestBuffer, NewClip.iLeft, NewClip.iTop, NewClip.iRight, NewClip.iBottom);

  // Compute the coordinates to center the text
  int32_t yp;
  if (b->bTextYOffset == -1) {
    yp = TextY + (height - GetFontHeight(b->usFont)) / 2 - 1;
  } else {
    yp = b->Y() + b->bTextYOffset;
  }

  int32_t xp;
  if (b->bTextXOffset == -1) {
    switch (b->bJustification) {
      case GUI_BUTTON::TEXT_LEFT:
        xp = TextX + 3;
        break;
      case GUI_BUTTON::TEXT_RIGHT:
        xp = NewClip.iRight - StringPixLength(b->string, b->usFont) - 3;
        break;
      default:
      case GUI_BUTTON::TEXT_CENTER:
        xp = TextX + (width - 6 - StringPixLength(b->string, b->usFont)) / 2;
        break;
    }
  } else {
    xp = b->X() + b->bTextXOffset;
  }

  // print the text

  // Override the colors if necessary.
  int16_t sForeColor;
  if (b->Enabled() && b->Area.uiFlags & MSYS_MOUSE_IN_AREA && b->sForeColorHilited != -1) {
    sForeColor = b->sForeColorHilited;
  } else if (b->Clicked() && b->sForeColorDown != -1) {
    sForeColor = b->sForeColorDown;
  } else {
    sForeColor = b->sForeColor;
  }

  uint8_t shadow;
  if (b->Enabled() && b->Area.uiFlags & MSYS_MOUSE_IN_AREA && b->sShadowColorHilited != -1) {
    shadow = b->sShadowColorHilited;
  } else if (b->Clicked() && b->sShadowColorDown != -1) {
    shadow = b->sShadowColorDown;
  } else if (b->sShadowColor != -1) {
    shadow = b->sShadowColor;
  } else {
    shadow = DEFAULT_SHADOW;
  }

  SetFontAttributes(b->usFont, sForeColor, shadow);

  if (b->Clicked() && b->fShiftText) {
    /* Was the button clicked on? if so, move the text slightly for the illusion
     * that the text moved into the screen. */
    xp++;
    yp++;
  }

  if (b->sWrappedWidth != -1) {
    uint8_t bJustified = 0;
    switch (b->bJustification) {
      case GUI_BUTTON::TEXT_LEFT:
        bJustified = LEFT_JUSTIFIED;
        break;
      case GUI_BUTTON::TEXT_RIGHT:
        bJustified = RIGHT_JUSTIFIED;
        break;
      case GUI_BUTTON::TEXT_CENTER:
        bJustified = CENTER_JUSTIFIED;
        break;
      default:
        Assert(0);
        break;
    }
    if (b->bTextXOffset == -1) {
      /* Kris:
       * There needs to be recalculation of the start positions based on the
       * justification and the width specified wrapped width.  I was drawing a
       * double lined word on the right side of the button to find it drawing
       * way over to the left.  I've added the necessary code for the right and
       * center justification.
       */
      yp = b->Y() + 2;

      switch (b->bJustification) {
        case GUI_BUTTON::TEXT_RIGHT:
          xp = b->BottomRightX() - 3 - b->sWrappedWidth;
          if (b->fShiftText && b->Clicked()) {
            xp++;
            yp++;
          }
          break;

        case GUI_BUTTON::TEXT_CENTER:
          xp = b->X() + 3 + b->sWrappedWidth / 2;
          if (b->fShiftText && b->Clicked()) {
            xp++;
            yp++;
          }
          break;
      }
    }
    yp += b->bTextYSubOffSet;
    xp += b->bTextXSubOffSet;
    DisplayWrappedString(xp, yp, b->sWrappedWidth, 1, b->usFont, sForeColor, b->string,
                         FONT_MCOLOR_BLACK, bJustified);
  } else {
    yp += b->bTextYSubOffSet;
    xp += b->bTextXSubOffSet;
    MPrint(xp, yp, b->string);
  }
}

/* This function is called by the DrawIconicButton and DrawTextButton routines
 * to draw the borders and background of the buttons.
 */
static void DrawGenericButton(const GUI_BUTTON *b) {
  // Select the graphics to use depending on the current state of the button
  HVOBJECT BPic;
  if (!b->Enabled()) {
    BPic = GenericButtonOffNormal;
    switch (b->bDisabledStyle) {
      case GUI_BUTTON::DISABLED_STYLE_DEFAULT:
        gbDisabledButtonStyle =
            b->string ? GUI_BUTTON::DISABLED_STYLE_SHADED : GUI_BUTTON::DISABLED_STYLE_HATCHED;
        break;

      case GUI_BUTTON::DISABLED_STYLE_HATCHED:
      case GUI_BUTTON::DISABLED_STYLE_SHADED:
        gbDisabledButtonStyle = b->bDisabledStyle;
        break;
    }
  } else if (b->Clicked()) {
    if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && GenericButtonOnHilite != NULL && gfRenderHilights) {
      BPic = GenericButtonOnHilite;
    } else {
      BPic = GenericButtonOnNormal;
    }
  } else {
    if (b->Area.uiFlags & MSYS_MOUSE_IN_AREA && GenericButtonOffHilite != NULL &&
        gfRenderHilights) {
      BPic = GenericButtonOffHilite;
    } else {
      BPic = GenericButtonOffNormal;
    }
  }

  const int32_t iBorderWidth = 3;
  const int32_t iBorderHeight = 2;

  // Compute the number of button "chunks" needed to be blitted
  int32_t const width = b->W();
  int32_t const height = b->H();
  const int32_t NumChunksWide = width / iBorderWidth;
  int32_t NumChunksHigh = height / iBorderHeight;
  const int32_t hremain = height % iBorderHeight;
  const int32_t wremain = width % iBorderWidth;

  int32_t const bx = b->X();
  int32_t const by = b->Y();
  int32_t const cx = bx + (NumChunksWide - 1) * iBorderWidth + wremain;
  int32_t const cy = by + (NumChunksHigh - 1) * iBorderHeight + hremain;

  // Fill the button's area with the button's background color
  ColorFillVideoSurfaceArea(ButtonDestBuffer, b->X(), b->Y(), b->BottomRightX(), b->BottomRightY(),
                            GenericButtonFillColors);

  SGPVSurface::Lock l(ButtonDestBuffer);
  uint16_t *const pDestBuf = l.Buffer<uint16_t>();
  uint32_t const uiDestPitchBYTES = l.Pitch();

  SGPRect ClipRect;
  GetClippingRect(&ClipRect);

  // Draw the button's borders and corners (horizontally)
  for (int32_t q = 0; q < NumChunksWide; q++) {
    int32_t const ImgNum = (q == 0 ? 0 : 1);
    int32_t const x = bx + q * iBorderWidth;
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, x, by, ImgNum,
                                            &ClipRect);
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, x, cy, ImgNum + 5,
                                            &ClipRect);
  }
  // Blit the right side corners
  Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, cx, by, 2, &ClipRect);
  Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, cx, cy, 7, &ClipRect);
  // Draw the vertical members of the button's borders
  NumChunksHigh--;

  if (hremain != 0) {
    int32_t const y = by + NumChunksHigh * iBorderHeight - iBorderHeight + hremain;
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, bx, y, 3, &ClipRect);
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, cx, y, 4, &ClipRect);
  }

  for (int32_t q = 1; q < NumChunksHigh; q++) {
    int32_t const y = by + q * iBorderHeight;
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, bx, y, 3, &ClipRect);
    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, BPic, cx, y, 4, &ClipRect);
  }
}

GUIButtonRef CreateCheckBoxButton(int16_t x, int16_t y, const char *filename, int16_t Priority,
                                  GUI_CALLBACK ClickCallback) {
  Assert(filename != NULL);
  BUTTON_PICS *const ButPic = LoadButtonImage(filename, -1, 0, 1, 2, 3);
  GUIButtonRef const b = QuickCreateButtonInternal(ButPic, x, y, BUTTON_CHECKBOX, Priority,
                                                   MSYS_NO_CALLBACK, ClickCallback);

  // change the flags so that it isn't a quick button anymore
  b->uiFlags &= ~BUTTON_QUICK;
  b->uiFlags |= BUTTON_CHECKBOX | BUTTON_SELFDELETE_IMAGE;

  return b;
}

/* Generic Button Movement Callback to reset the mouse button if the mouse is no
 * longer in the button region.
 */
static void DefaultMoveCallback(GUI_BUTTON *btn, int32_t reason) {
  // If the button isn't the anchored button, then we don't want to modify the
  // button state.
  if (btn != gpAnchoredButton) return;

  if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    if (!gfAnchoredState) {
      btn->uiFlags &= ~BUTTON_CLICKED_ON;
      if (btn->ubSoundSchemeID) {
        PlayButtonSound(btn, BUTTON_SOUND_CLICKED_OFF);
      }
    }
    InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
  } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    btn->uiFlags |= BUTTON_CLICKED_ON;
    if (btn->ubSoundSchemeID) {
      PlayButtonSound(btn, BUTTON_SOUND_CLICKED_ON);
    }
    InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
  }
}

void ReleaseAnchorMode() {
  GUI_BUTTON *const b = gpAnchoredButton;
  if (!b) return;

  if (gusMouseXPos < b->X() || b->BottomRightX() < gusMouseXPos || gusMouseYPos < b->Y() ||
      b->BottomRightY() < gusMouseYPos) {
    // released outside button area, so restore previous button state.
    if (gfAnchoredState) {
      b->uiFlags |= BUTTON_CLICKED_ON;
    } else {
      b->uiFlags &= ~BUTTON_CLICKED_ON;
    }
    InvalidateRegion(b->X(), b->Y(), b->BottomRightX(), b->BottomRightY());
  }
  gpPrevAnchoredButton = b;
  gpAnchoredButton = 0;
}

void GUI_BUTTON::Hide() {
  Area.Disable();
  uiFlags |= BUTTON_DIRTY;
  InvalidateRegion(X(), Y(), BottomRightX(), BottomRightY());
}

void HideButton(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->Hide();
}

void GUI_BUTTON::Show() {
  Area.Enable();
  uiFlags |= BUTTON_DIRTY;
  InvalidateRegion(X(), Y(), BottomRightX(), BottomRightY());
}

void ShowButton(GUIButtonRef const b) {
  CHECKV(b != NULL);  // XXX HACK000C
  b->Show();
}

uint16_t GetGenericButtonFillColor() { return GenericButtonFillColors; }
