#ifndef __IMP_VIDEO_H
#define __IMP_VIDEO_H

#include "SGP/Types.h"

// the functions

// metal background
void RenderProfileBackGround();
void RemoveProfileBackGround();
void LoadProfileBackGround();

// imp symbol
void RenderIMPSymbol(INT16 sX, INT16 sY);
void DeleteIMPSymbol();
void LoadIMPSymbol();

void LoadBeginIndent();
void DeleteBeginIndent();
void RenderBeginIndent(INT16 sX, INT16 sY);

void LoadActivationIndent();
void DeleteActivationIndent();
void RenderActivationIndent(INT16 sX, INT16 sY);

void LoadFrontPageIndent();
void DeleteFrontPageIndent();
void RenderFrontPageIndent(INT16 sX, INT16 sY);

void LoadAnalyse();
void DeleteAnalyse();

void LoadAttributeGraph();
void DeleteAttributeGraph();

void LoadNameIndent();
void DeleteNameIndent();
void RenderNameIndent(INT16 sX, INT16 sY);

void LoadNickNameIndent();
void DeleteNickNameIndent();
void RenderNickNameIndent(INT16 sX, INT16 sY);

void LoadGenderIndent();
void DeleteGenderIndent();
void RenderGenderIndent(INT16 sX, INT16 sY);

void LoadSmallSilhouette();
void DeleteSmallSilhouette();

void LoadLargeSilhouette();
void DeleteLargeSilhouette();
void RenderLargeSilhouette(INT16 sX, INT16 sY);

void LoadAttributeFrame();
void DeleteAttributeFrame();
void RenderAttributeFrame(INT16 sX, INT16 sY);

void LoadSliderBar();
void DeleteSliderBar();
void RenderSliderBar(INT16 sX, INT16 sY);

void LoadButton2Image();
void DeleteButton2Image();
void RenderButton2Image(INT16 sX, INT16 sY);

void LoadButton4Image();
void DeleteButton4Image();
void RenderButton4Image(INT16 sX, INT16 sY);

void LoadPortraitFrame();
void DeletePortraitFrame();
void RenderPortraitFrame(INT16 sX, INT16 sY);

void LoadMainIndentFrame();
void DeleteMainIndentFrame();
void RenderMainIndentFrame(INT16 sX, INT16 sY);

void LoadQtnLongIndentFrame();
void DeleteQtnLongIndentFrame();
void RenderQtnLongIndentFrame(INT16 sX, INT16 sY);

void LoadQtnShortIndentFrame();
void DeleteQtnShortIndentFrame();
void RenderQtnShortIndentFrame(INT16 sX, INT16 sY);

void LoadQtnLongIndentHighFrame();
void DeleteQtnLongIndentHighFrame();
void RenderQtnLongIndentHighFrame(INT16 sX, INT16 sY);

void LoadQtnShortIndentHighFrame();
void DeleteQtnShortIndentHighFrame();
void RenderQtnShortIndentHighFrame(INT16 sX, INT16 sY);

void LoadQtnShort2IndentFrame();
void DeleteQtnShort2IndentFrame();
void RenderQtnShort2IndentFrame(INT16 sX, INT16 sY);

void LoadQtnShort2IndentHighFrame();
void DeleteQtnShort2IndentHighFrame();
void RenderQtnShort2IndentHighFrame(INT16 sX, INT16 sY);

void LoadQtnIndentFrame();
void DeleteQtnIndentFrame();
void RenderQtnIndentFrame(INT16 sX, INT16 sY);

void LoadAttrib1IndentFrame();
void DeleteAttrib1IndentFrame();
void RenderAttrib1IndentFrame(INT16 sX, INT16 sY);

void LoadAttrib2IndentFrame();
void DeleteAttrib2IndentFrame();
void RenderAttrib2IndentFrame(INT16 sX, INT16 sY);

void LoadAvgMercIndentFrame();
void DeleteAvgMercIndentFrame();
void RenderAvgMercIndentFrame(INT16 sX, INT16 sY);

void LoadAboutUsIndentFrame();
void DeleteAboutUsIndentFrame();
void RenderAboutUsIndentFrame(INT16 sX, INT16 sY);

void RenderAttributeFrameForIndex(INT16 sX, INT16 sY, INT32 iIndex);

// graphical handles

extern SGPVObject *guiANALYSE;
extern SGPVObject *guiATTRIBUTEGRAPH;
extern SGPVObject *guiSMALLSILHOUETTE;

#endif
