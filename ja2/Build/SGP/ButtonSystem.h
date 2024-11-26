// by Kris Morness (originally created by Bret Rowden)

#ifndef BUTTON_SYSTEM_H
#define BUTTON_SYSTEM_H

#include "SGP/MouseSystem.h"

#define MAX_BUTTONS 400
#define MAX_BUTTON_PICS 256

// Some GUI_BUTTON system defines
#define BUTTON_NO_IMAGE -1

// effects how the button is rendered.
#define BUTTON_TYPES (BUTTON_QUICK | BUTTON_GENERIC | BUTTON_HOT_SPOT | BUTTON_CHECKBOX)

// button flags
#define BUTTON_TOGGLE 0x00000000
#define BUTTON_QUICK 0x00000000
#define BUTTON_ENABLED 0x00000001
#define BUTTON_CLICKED_ON 0x00000002
#define BUTTON_GENERIC 0x00000020
#define BUTTON_HOT_SPOT 0x00000040
#define BUTTON_SELFDELETE_IMAGE 0x00000080
#define BUTTON_DELETION_PENDING 0x00000100
#define BUTTON_DIRTY 0x00000400
#define BUTTON_CHECKBOX 0x00001000
#define BUTTON_NEWTOGGLE 0x00002000
#define BUTTON_FORCE_UNDIRTY \
  0x00004000  // no matter what happens this buttons does NOT get marked dirty
#define BUTTON_NO_DUPLICATE 0x80000000  // Exclude button from duplicate check

extern SGPVSurface *ButtonDestBuffer;

struct GUI_BUTTON;

// GUI_BUTTON callback function type
typedef void (*GUI_CALLBACK)(GUI_BUTTON *, int32_t);

// GUI_BUTTON structure definitions.
struct GUI_BUTTON {
  GUI_BUTTON(uint32_t flags, int16_t left, int16_t top, int16_t width, int16_t height,
             int8_t priority, GUI_CALLBACK click, GUI_CALLBACK move);
  ~GUI_BUTTON();

  bool Clicked() const { return uiFlags & BUTTON_CLICKED_ON; }

  bool Enabled() const { return uiFlags & BUTTON_ENABLED; }

  // Set the text that will be displayed as the FastHelp
  void SetFastHelpText(wchar_t const *text);

  void Hide();
  void Show();

  // Draw the button on the screen.
  void Draw();

  void SpecifyDownTextColors(int16_t fore_colour_down, int16_t shadow_colour_down);
  void SpecifyHilitedTextColors(int16_t fore_colour_highlighted, int16_t shadow_colour_highlighted);

  enum Justification { TEXT_LEFT = -1, TEXT_CENTER = 0, TEXT_RIGHT = 1 };
  void SpecifyTextJustification(Justification);

  void SpecifyText(wchar_t const *text);
  void SpecifyGeneralTextAttributes(wchar_t const *string, Font, int16_t fore_colour,
                                    int16_t shadow_colour);
  void SpecifyTextOffsets(int8_t text_x_offset, int8_t text_y_offset, BOOLEAN shift_text);
  void SpecifyTextSubOffsets(int8_t text_x_offset, int8_t text_y_offset, BOOLEAN shift_text);
  void SpecifyTextWrappedWidth(int16_t wrapped_width);

  void AllowDisabledFastHelp();

  enum DisabledStyle {
    DISABLED_STYLE_NONE,     // for dummy buttons, panels, etc.  Always displays
                             // normal state.
    DISABLED_STYLE_DEFAULT,  // if button has text then shade, else hatch
    DISABLED_STYLE_HATCHED,  // always hatches the disabled button
    DISABLED_STYLE_SHADED    // always shades the disabled button 25% darker
  };
  void SpecifyDisabledStyle(DisabledStyle);

  /* Note:  Text is always on top
   * If fShiftImage is true, then the image will shift down one pixel and right
   * one pixel just like the text does.
   */
  void SpecifyIcon(SGPVObject const *icon, uint16_t usVideoObjectIndex, int8_t bXOffset,
                   int8_t bYOffset, BOOLEAN fShiftImage);

  // will simply set the cursor for the mouse region the button occupies
  void SetCursor(uint16_t const cursor) { Area.ChangeCursor(cursor); }

  void DrawCheckBoxOnOff(BOOLEAN on);

  // Coordinates where button is on the screen
  int16_t X() const { return Area.RegionTopLeftX; }
  int16_t Y() const { return Area.RegionTopLeftY; }
  int16_t W() const { return Area.RegionBottomRightX - Area.RegionTopLeftX; }
  int16_t H() const { return Area.RegionBottomRightY - Area.RegionTopLeftY; }
  int16_t BottomRightX() const { return Area.RegionBottomRightX; }
  int16_t BottomRightY() const { return Area.RegionBottomRightY; }

  int16_t MouseX() const { return Area.MouseXPos; }
  int16_t MouseY() const { return Area.MouseYPos; }
  int16_t RelativeX() const { return Area.RelativeXPos; }
  int16_t RelativeY() const { return Area.RelativeYPos; }

  int32_t GetUserData() const { return User.Data; }
  void SetUserData(int32_t const data) { User.Data = data; }

  template <typename T>
  T *GetUserPtr() const {
    return static_cast<T *>(User.Ptr);
  }
  void SetUserPtr(void *const p) { User.Ptr = p; }

  int32_t IDNum;               // ID Number, contains it's own button number
  BUTTON_PICS *image;          // Image to use (see DOCs for details)
  MouseRegion Area;            // Mouse System's mouse region to use for this button
  GUI_CALLBACK ClickCallback;  // Button Callback when button is clicked
  GUI_CALLBACK MoveCallback;   // Button Callback when mouse moved on this region
  uint32_t uiFlags;            // Button state flags etc.( 32-bit )
  uint32_t uiOldFlags;         // Old flags from previous render loop
  union                        // Place holder for user data etc.
  {
    int32_t Data;
    void *Ptr;
  } User;
  int8_t bDisabledStyle;  // Button disabled style

  // For buttons with text
  wchar_t *string;     // the string
  Font usFont;         // font for text
  int16_t sForeColor;  // text colors if there is text
  int16_t sShadowColor;
  int16_t sForeColorDown;  // text colors when button is down (optional)
  int16_t sShadowColorDown;
  int16_t sForeColorHilited;  // text colors when button is down (optional)
  int16_t sShadowColorHilited;
  int8_t bJustification;  // BUTTON_TEXT_LEFT, BUTTON_TEXT_CENTER, BUTTON_TEXT_RIGHT
  int8_t bTextXOffset;
  int8_t bTextYOffset;
  int8_t bTextXSubOffSet;
  int8_t bTextYSubOffSet;
  BOOLEAN fShiftText;
  int16_t sWrappedWidth;

  // For buttons with icons (don't confuse this with quickbuttons which have up
  // to 5 states)
  const SGPVObject *icon;
  int16_t usIconIndex;
  int8_t bIconXOffset;  // -1 means horizontally centered
  int8_t bIconYOffset;  // -1 means vertically centered
  BOOLEAN
  fShiftImage;  // if true, icon is shifted +1,+1 when button state is down.

  uint8_t ubToggleButtonActivated;

  uint8_t ubSoundSchemeID;
};

extern GUI_BUTTON *ButtonList[MAX_BUTTONS];  // Button System's Main Button List

class GUIButtonRef {
 public:
  GUIButtonRef() : btn_id_(0) {}

  GUIButtonRef(GUI_BUTTON *const b) : btn_id_(b->IDNum) {}

  void Reset() { btn_id_ = 0; }

  int32_t ID() const { return btn_id_; }

  GUI_BUTTON *operator->() const { return ButtonList[btn_id_]; }

  operator GUI_BUTTON *() const { return ButtonList[btn_id_]; }

 private:
  int32_t btn_id_;
};

/* Initializes the GUI button system for use. Must be called before using any
 * other button functions.
 */
void InitButtonSystem();

/* Shuts down and cleans up the GUI button system. Must be called before exiting
 * the program.  Button functions should not be used after calling this
 * function.
 */
void ShutdownButtonSystem();

#if defined _JA2_RENDER_DIRTY

void RenderButtonsFastHelp();
#define RenderButtonsFastHelp() RenderFastHelp()

#endif

// Loads an image file for use as a button icon.
int16_t LoadGenericButtonIcon(const char *filename);

// Removes a button icon graphic from the system
void UnloadGenericButtonIcon(int16_t GenImg);

// Load images for use with QuickButtons.
BUTTON_PICS *LoadButtonImage(const char *filename, int32_t Grayed, int32_t OffNormal,
                             int32_t OffHilite, int32_t OnNormal, int32_t OnHilite);
BUTTON_PICS *LoadButtonImage(char const *filename, int32_t off_normal, int32_t on_normal);

/* Uses a previously loaded quick button image for use with QuickButtons.  The
 * function simply duplicates the vobj!
 */
BUTTON_PICS *UseLoadedButtonImage(BUTTON_PICS *LoadedImg, int32_t Grayed, int32_t OffNormal,
                                  int32_t OffHilite, int32_t OnNormal, int32_t OnHilite);
BUTTON_PICS *UseLoadedButtonImage(BUTTON_PICS *img, int32_t off_normal, int32_t on_normal);

// Removes a QuickButton image from the system.
void UnloadButtonImage(BUTTON_PICS *);

// Enables an already created button.
void EnableButton(GUIButtonRef);

/* Disables a button. The button remains in the system list, and can be
 * reactivated by calling EnableButton.  Diabled buttons will appear "grayed
 * out" on the screen (unless the graphics for such are not available).
 */
void DisableButton(GUIButtonRef);

void EnableButton(GUIButtonRef, bool enable);

/* Removes a button from the system's list. All memory associated with the
 * button is released.
 */
void RemoveButton(GUIButtonRef &);

void HideButton(GUIButtonRef);
void ShowButton(GUIButtonRef);

void RenderButtons();

extern BOOLEAN gfRenderHilights;

/* Creates a QuickButton. QuickButtons only have graphics associated with them.
 * They cannot be re-sized, nor can the graphic be changed.  Providing you have
 * allocated your own image, this is a somewhat simplified function.
 */
GUIButtonRef QuickCreateButton(BUTTON_PICS *image, int16_t x, int16_t y, int16_t priority,
                               GUI_CALLBACK click);
GUIButtonRef QuickCreateButtonNoMove(BUTTON_PICS *image, int16_t x, int16_t y, int16_t priority,
                                     GUI_CALLBACK click);
GUIButtonRef QuickCreateButtonToggle(BUTTON_PICS *image, int16_t x, int16_t y, int16_t priority,
                                     GUI_CALLBACK click);

GUIButtonRef QuickCreateButtonImg(const char *gfx, int32_t grayed, int32_t off_normal,
                                  int32_t off_hilite, int32_t on_normal, int32_t on_hilite,
                                  int16_t x, int16_t y, int16_t priority, GUI_CALLBACK click);
GUIButtonRef QuickCreateButtonImg(const char *gfx, int32_t off_normal, int32_t on_normal, int16_t x,
                                  int16_t y, int16_t priority, GUI_CALLBACK click);

GUIButtonRef CreateCheckBoxButton(int16_t x, int16_t y, const char *filename, int16_t Priority,
                                  GUI_CALLBACK ClickCallback);

// Creates an Iconic type button.
GUIButtonRef CreateIconButton(int16_t Icon, int16_t IconIndex, int16_t xloc, int16_t yloc,
                              int16_t w, int16_t h, int16_t Priority, GUI_CALLBACK ClickCallback);

/* Creates a button like HotSpot. HotSpots have no graphics associated with
 * them.
 */
GUIButtonRef CreateHotSpot(int16_t xloc, int16_t yloc, int16_t Width, int16_t Height,
                           int16_t Priority, GUI_CALLBACK ClickCallback);

// Creates a generic button with text on it.
GUIButtonRef CreateTextButton(const wchar_t *string, Font, int16_t sForeColor, int16_t sShadowColor,
                              int16_t xloc, int16_t yloc, int16_t w, int16_t h, int16_t Priority,
                              GUI_CALLBACK ClickCallback);

GUIButtonRef CreateIconAndTextButton(BUTTON_PICS *Image, const wchar_t *string, Font,
                                     int16_t sForeColor, int16_t sShadowColor,
                                     int16_t sForeColorDown, int16_t sShadowColorDown, int16_t xloc,
                                     int16_t yloc, int16_t Priority, GUI_CALLBACK ClickCallback);

/* This is technically not a clickable button, but just a label with text. It is
 * implemented as button */
GUIButtonRef CreateLabel(const wchar_t *text, Font, int16_t forecolor, int16_t shadowcolor,
                         int16_t x, int16_t y, int16_t w, int16_t h, int16_t priority);

void MarkAButtonDirty(GUIButtonRef);    // will mark only selected button dirty
void MarkButtonsDirty();                // Function to mark buttons dirty ( all will redraw
                                        // at next RenderButtons )
void UnMarkButtonDirty(GUIButtonRef);   // unmark button
void UnmarkButtonsDirty();              // unmark ALL the buttoms on the screen dirty
void ForceButtonUnDirty(GUIButtonRef);  // forces button undirty no matter the
                                        // reason, only lasts one frame

struct ButtonDimensions {
  uint32_t w;
  uint32_t h;
};

const ButtonDimensions *GetDimensionsOfButtonPic(const BUTTON_PICS *);

uint16_t GetGenericButtonFillColor();

void ReleaseAnchorMode();

#endif
