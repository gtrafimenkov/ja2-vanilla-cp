#include "Laptop/IMPVideoObjects.h"

#include "Directories.h"
#include "Laptop/IMPAttributeSelection.h"
#include "Laptop/Laptop.h"
#include "Local.h"
#include "SGP/ButtonSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/MultiLanguageGraphicUtils.h"

// video object handles
static SGPVObject *guiBACKGROUND;
static SGPVObject *guiIMPSYMBOL;
static SGPVObject *guiBEGININDENT;
static SGPVObject *guiACTIVATIONINDENT;
static SGPVObject *guiFRONTPAGEINDENT;
static SGPVObject *guiNAMEINDENT;
static SGPVObject *guiNICKNAMEINDENT;
static SGPVObject *guiGENDERINDENT;
SGPVObject *guiANALYSE;
SGPVObject *guiATTRIBUTEGRAPH;
SGPVObject *guiSMALLSILHOUETTE;
static SGPVObject *guiLARGESILHOUETTE;
static SGPVObject *guiPORTRAITFRAME;
static SGPVObject *guiSLIDERBAR;
static SGPVObject *guiATTRIBUTEFRAME;
static SGPVObject *guiBUTTON2IMAGE;
static SGPVObject *guiBUTTON4IMAGE;
static SGPVObject *guiMAININDENT;
static SGPVObject *guiLONGINDENT;
static SGPVObject *guiSHORTINDENT;
static SGPVObject *guiSHORTHINDENT;
static SGPVObject *guiSHORT2INDENT;
static SGPVObject *guiLONGHINDENT;
static SGPVObject *guiQINDENT;
static SGPVObject *guiA1INDENT;
static SGPVObject *guiA2INDENT;
static SGPVObject *guiAVGMERCINDENT;
static SGPVObject *guiABOUTUSINDENT;
static SGPVObject *guiSHORT2HINDENT;

// position defines
#define CHAR_PROFILE_BACKGROUND_TILE_WIDTH 125
#define CHAR_PROFILE_BACKGROUND_TILE_HEIGHT 100

extern void DrawBonusPointsRemaining();

void LoadProfileBackGround() {
  // this procedure will load in the graphics for the generic background
  guiBACKGROUND = AddVideoObjectFromFile(LAPTOPDIR "/metalbackground.sti");
}

void RemoveProfileBackGround() {
  // remove background
  DeleteVideoObject(guiBACKGROUND);
}

void RenderProfileBackGround() {
  INT32 iCounter = 0;

  // this procedure will render the generic backgound to the screen

  // render each row 5 times wide, 5 tiles high
  const SGPVObject *hHandle = guiBACKGROUND;
  for (iCounter = 0; iCounter < 4; iCounter++) {
    // blt background to screen from left to right
    const INT32 x = LAPTOP_SCREEN_UL_X;
    const INT32 y = LAPTOP_SCREEN_WEB_UL_Y + iCounter * CHAR_PROFILE_BACKGROUND_TILE_HEIGHT;
    BltVideoObject(FRAME_BUFFER, hHandle, 0, x + 0 * CHAR_PROFILE_BACKGROUND_TILE_WIDTH, y);
    BltVideoObject(FRAME_BUFFER, hHandle, 0, x + 1 * CHAR_PROFILE_BACKGROUND_TILE_WIDTH, y);
    BltVideoObject(FRAME_BUFFER, hHandle, 0, x + 2 * CHAR_PROFILE_BACKGROUND_TILE_WIDTH, y);
    BltVideoObject(FRAME_BUFFER, hHandle, 0, x + 3 * CHAR_PROFILE_BACKGROUND_TILE_WIDTH, y);
  }

  // dirty buttons
  MarkButtonsDirty();

  // force refresh of screen
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void LoadIMPSymbol() {
  // this procedure will load the IMP main symbol into memory
  const char *const ImageFile = GetMLGFilename(MLG_IMPSYMBOL);
  guiIMPSYMBOL = AddVideoObjectFromFile(ImageFile);
}

void DeleteIMPSymbol() {
  // remove IMP symbol
  DeleteVideoObject(guiIMPSYMBOL);
}

void RenderIMPSymbol(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiIMPSYMBOL, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadBeginIndent() {
  // this procedure will load the indent main symbol into memory
  guiBEGININDENT = AddVideoObjectFromFile(LAPTOPDIR "/beginscreenindent.sti");
}

void DeleteBeginIndent() {
  // remove indent symbol
  DeleteVideoObject(guiBEGININDENT);
}

void RenderBeginIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiBEGININDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadActivationIndent() {
  // this procedure will load the activation indent into memory
  guiACTIVATIONINDENT = AddVideoObjectFromFile(LAPTOPDIR "/activationindent.sti");
}

void DeleteActivationIndent() {
  // remove activation indent symbol
  DeleteVideoObject(guiACTIVATIONINDENT);
}

void RenderActivationIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiACTIVATIONINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadFrontPageIndent() {
  // this procedure will load the activation indent into memory
  guiFRONTPAGEINDENT = AddVideoObjectFromFile(LAPTOPDIR "/frontpageindent.sti");
}

void DeleteFrontPageIndent() {
  // remove activation indent symbol
  DeleteVideoObject(guiFRONTPAGEINDENT);
}

void RenderFrontPageIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiFRONTPAGEINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAnalyse() {
  // this procedure will load the activation indent into memory
  guiANALYSE = AddVideoObjectFromFile(LAPTOPDIR "/analyze.sti");
}

void DeleteAnalyse() {
  // remove activation indent symbol
  DeleteVideoObject(guiANALYSE);
}

void LoadAttributeGraph() {
  // this procedure will load the activation indent into memory
  guiATTRIBUTEGRAPH = AddVideoObjectFromFile(LAPTOPDIR "/attributegraph.sti");
}

void DeleteAttributeGraph() {
  // remove activation indent symbol
  DeleteVideoObject(guiATTRIBUTEGRAPH);
}

void LoadNickNameIndent() {
  // this procedure will load the activation indent into memory
  guiNICKNAMEINDENT = AddVideoObjectFromFile(LAPTOPDIR "/nickname.sti");
}

void DeleteNickNameIndent() {
  // remove activation indent symbol
  DeleteVideoObject(guiNICKNAMEINDENT);
}

void RenderNickNameIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiNICKNAMEINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadNameIndent() {
  // this procedure will load the activation indent into memory
  guiNAMEINDENT = AddVideoObjectFromFile(LAPTOPDIR "/nameindent.sti");
}

void DeleteNameIndent() {
  // remove activation indent symbol
  DeleteVideoObject(guiNAMEINDENT);
}

void RenderNameIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiNAMEINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadGenderIndent() {
  // this procedure will load the activation indent into memory
  guiGENDERINDENT = AddVideoObjectFromFile(LAPTOPDIR "/genderindent.sti");
}

void DeleteGenderIndent() {
  // remove activation indent symbol
  DeleteVideoObject(guiGENDERINDENT);
}

void RenderGenderIndent(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiGENDERINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadSmallSilhouette() {
  // this procedure will load the activation indent into memory
  guiSMALLSILHOUETTE = AddVideoObjectFromFile(LAPTOPDIR "/smallsilhouette.sti");
}

void DeleteSmallSilhouette() {
  // remove activation indent symbol
  DeleteVideoObject(guiSMALLSILHOUETTE);
}

void LoadLargeSilhouette() {
  // this procedure will load the activation indent into memory
  guiLARGESILHOUETTE = AddVideoObjectFromFile(LAPTOPDIR "/largesilhouette.sti");
}

void DeleteLargeSilhouette() {
  // remove activation indent symbol
  DeleteVideoObject(guiLARGESILHOUETTE);
}

void RenderLargeSilhouette(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiLARGESILHOUETTE, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAttributeFrame() {
  // this procedure will load the activation indent into memory
  guiATTRIBUTEFRAME = AddVideoObjectFromFile(LAPTOPDIR "/attributeframe.sti");
}

void DeleteAttributeFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiATTRIBUTEFRAME);
}

void RenderAttributeFrame(INT16 sX, INT16 sY) {
  INT32 iCounter = 0;
  INT16 sCurrentY = 0;

  const SGPVObject *const hHandle = guiATTRIBUTEFRAME;

  // blt to sX, sY relative to upper left corner
  BltVideoObject(FRAME_BUFFER, hHandle, 0, LAPTOP_SCREEN_UL_X + sX, LAPTOP_SCREEN_WEB_UL_Y + sY);

  sCurrentY += 10;
  for (iCounter = 0; iCounter < 10; iCounter++) {
    // blt to sX, sY relative to upper left corner
    BltVideoObject(FRAME_BUFFER, hHandle, 2, LAPTOP_SCREEN_UL_X + sX + 134,
                   LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY);
    BltVideoObject(FRAME_BUFFER, hHandle, 1, LAPTOP_SCREEN_UL_X + sX,
                   LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY);
    BltVideoObject(FRAME_BUFFER, hHandle, 3, LAPTOP_SCREEN_UL_X + sX + 368,
                   LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY);

    sCurrentY += 20;
  }

  BltVideoObject(FRAME_BUFFER, hHandle, 4, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY);
}

void RenderAttributeFrameForIndex(INT16 sX, INT16 sY, INT32 iIndex) {
  INT16 sCurrentY = 0;

  // valid index?
  if (iIndex == -1) {
    return;
  }

  sCurrentY = (INT16)(10 + (iIndex * 20));

  BltVideoObject(FRAME_BUFFER, guiATTRIBUTEFRAME, 2, LAPTOP_SCREEN_UL_X + sX + 134,
                 LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY);

  RenderAttrib2IndentFrame(350, 42);

  // amt of bonus pts
  DrawBonusPointsRemaining();

  // render attribute boxes
  RenderAttributeBoxes();

  InvalidateRegion(LAPTOP_SCREEN_UL_X + sX + 134, LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY,
                   LAPTOP_SCREEN_UL_X + sX + 400, LAPTOP_SCREEN_WEB_UL_Y + sY + sCurrentY + 21);
}

void LoadSliderBar() {
  // this procedure will load the activation indent into memory
  guiSLIDERBAR = AddVideoObjectFromFile(LAPTOPDIR "/attributeslider.sti");
}

void DeleteSliderBar() {
  // remove activation indent symbol
  DeleteVideoObject(guiSLIDERBAR);
}

void RenderSliderBar(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiSLIDERBAR, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadButton2Image() {
  // this procedure will load the activation indent into memory
  guiBUTTON2IMAGE = AddVideoObjectFromFile(LAPTOPDIR "/button_2.sti");
}

void DeleteButton2Image() {
  // remove activation indent symbol
  DeleteVideoObject(guiBUTTON2IMAGE);
}

void RenderButton2Image(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiBUTTON2IMAGE, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadButton4Image() {
  // this procedure will load the activation indent into memory
  guiBUTTON4IMAGE = AddVideoObjectFromFile(LAPTOPDIR "/button_4.sti");
}

void DeleteButton4Image() {
  // remove activation indent symbol
  DeleteVideoObject(guiBUTTON4IMAGE);
}

void RenderButton4Image(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiBUTTON4IMAGE, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadPortraitFrame() {
  // this procedure will load the activation indent into memory
  guiPORTRAITFRAME = AddVideoObjectFromFile(LAPTOPDIR "/voice_portraitframe.sti");
}

void DeletePortraitFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiPORTRAITFRAME);
}

void RenderPortraitFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiPORTRAITFRAME, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadMainIndentFrame() {
  // this procedure will load the activation indent into memory
  guiMAININDENT = AddVideoObjectFromFile(LAPTOPDIR "/mainprofilepageindent.sti");
}

void DeleteMainIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiMAININDENT);
}

void RenderMainIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiMAININDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnLongIndentFrame() {
  // this procedure will load the activation indent into memory
  guiLONGINDENT = AddVideoObjectFromFile(LAPTOPDIR "/longindent.sti");
}

void DeleteQtnLongIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiLONGINDENT);
}

void RenderQtnLongIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiLONGINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnShortIndentFrame() {
  // this procedure will load the activation indent into memory
  guiSHORTINDENT = AddVideoObjectFromFile(LAPTOPDIR "/shortindent.sti");
}

void DeleteQtnShortIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiSHORTINDENT);
}

void RenderQtnShortIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiSHORTINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnLongIndentHighFrame() {
  // this procedure will load the activation indent into memory
  guiLONGHINDENT = AddVideoObjectFromFile(LAPTOPDIR "/longindenthigh.sti");
}

void DeleteQtnLongIndentHighFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiLONGHINDENT);
}

void RenderQtnLongIndentHighFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiLONGHINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnShortIndentHighFrame() {
  // this procedure will load the activation indent into memory
  guiSHORTHINDENT = AddVideoObjectFromFile(LAPTOPDIR "/shortindenthigh.sti");
}

void DeleteQtnShortIndentHighFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiSHORTHINDENT);
}

void RenderQtnShortIndentHighFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiSHORTHINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnIndentFrame() {
  // this procedure will load the activation indent into memory
  guiQINDENT = AddVideoObjectFromFile(LAPTOPDIR "/questionindent.sti");
}

void DeleteQtnIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiQINDENT);
}

void RenderQtnIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiQINDENT, 0, LAPTOP_SCREEN_UL_X + sX, LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAttrib1IndentFrame() {
  // this procedure will load the activation indent into memory
  guiA1INDENT = AddVideoObjectFromFile(LAPTOPDIR "/attributescreenindent_1.sti");
}

void DeleteAttrib1IndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiA1INDENT);
}

void RenderAttrib1IndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiA1INDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAttrib2IndentFrame() {
  // this procedure will load the activation indent into memory
  guiA2INDENT = AddVideoObjectFromFile(LAPTOPDIR "/attributescreenindent_2.sti");
}

void DeleteAttrib2IndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiA2INDENT);
}

void RenderAttrib2IndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiA2INDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAvgMercIndentFrame() {
  // this procedure will load the activation indent into memory
  guiAVGMERCINDENT = AddVideoObjectFromFile(LAPTOPDIR "/anaveragemercindent.sti");
}

void DeleteAvgMercIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiAVGMERCINDENT);
}

void RenderAvgMercIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiAVGMERCINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadAboutUsIndentFrame() {
  // this procedure will load the activation indent into memory
  guiABOUTUSINDENT = AddVideoObjectFromFile(LAPTOPDIR "/aboutusindent.sti");
}

void DeleteAboutUsIndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiABOUTUSINDENT);
}

void RenderAboutUsIndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiABOUTUSINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnShort2IndentFrame() {
  // this procedure will load the activation indent into memory
  guiSHORT2INDENT = AddVideoObjectFromFile(LAPTOPDIR "/shortindent2.sti");
}

void DeleteQtnShort2IndentFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiSHORT2INDENT);
}

void RenderQtnShort2IndentFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiSHORT2INDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}

void LoadQtnShort2IndentHighFrame() {
  // this procedure will load the activation indent into memory
  guiSHORT2HINDENT = AddVideoObjectFromFile(LAPTOPDIR "/shortindent2high.sti");
}

void DeleteQtnShort2IndentHighFrame() {
  // remove activation indent symbol
  DeleteVideoObject(guiSHORT2HINDENT);
}

void RenderQtnShort2IndentHighFrame(INT16 sX, INT16 sY) {
  BltVideoObject(FRAME_BUFFER, guiSHORT2HINDENT, 0, LAPTOP_SCREEN_UL_X + sX,
                 LAPTOP_SCREEN_WEB_UL_Y + sY);
}
