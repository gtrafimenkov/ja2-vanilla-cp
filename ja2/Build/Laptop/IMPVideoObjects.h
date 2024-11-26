#ifndef __IMP_VIDEO_H
#define __IMP_VIDEO_H

#include "SGP/Types.h"

// the functions

// metal background
void RenderProfileBackGround();
void RemoveProfileBackGround();
void LoadProfileBackGround();

// imp symbol
void RenderIMPSymbol(int16_t sX, int16_t sY);
void DeleteIMPSymbol();
void LoadIMPSymbol();

void LoadBeginIndent();
void DeleteBeginIndent();
void RenderBeginIndent(int16_t sX, int16_t sY);

void LoadActivationIndent();
void DeleteActivationIndent();
void RenderActivationIndent(int16_t sX, int16_t sY);

void LoadFrontPageIndent();
void DeleteFrontPageIndent();
void RenderFrontPageIndent(int16_t sX, int16_t sY);

void LoadAnalyse();
void DeleteAnalyse();

void LoadAttributeGraph();
void DeleteAttributeGraph();

void LoadNameIndent();
void DeleteNameIndent();
void RenderNameIndent(int16_t sX, int16_t sY);

void LoadNickNameIndent();
void DeleteNickNameIndent();
void RenderNickNameIndent(int16_t sX, int16_t sY);

void LoadGenderIndent();
void DeleteGenderIndent();
void RenderGenderIndent(int16_t sX, int16_t sY);

void LoadSmallSilhouette();
void DeleteSmallSilhouette();

void LoadLargeSilhouette();
void DeleteLargeSilhouette();
void RenderLargeSilhouette(int16_t sX, int16_t sY);

void LoadAttributeFrame();
void DeleteAttributeFrame();
void RenderAttributeFrame(int16_t sX, int16_t sY);

void LoadSliderBar();
void DeleteSliderBar();
void RenderSliderBar(int16_t sX, int16_t sY);

void LoadButton2Image();
void DeleteButton2Image();
void RenderButton2Image(int16_t sX, int16_t sY);

void LoadButton4Image();
void DeleteButton4Image();
void RenderButton4Image(int16_t sX, int16_t sY);

void LoadPortraitFrame();
void DeletePortraitFrame();
void RenderPortraitFrame(int16_t sX, int16_t sY);

void LoadMainIndentFrame();
void DeleteMainIndentFrame();
void RenderMainIndentFrame(int16_t sX, int16_t sY);

void LoadQtnLongIndentFrame();
void DeleteQtnLongIndentFrame();
void RenderQtnLongIndentFrame(int16_t sX, int16_t sY);

void LoadQtnShortIndentFrame();
void DeleteQtnShortIndentFrame();
void RenderQtnShortIndentFrame(int16_t sX, int16_t sY);

void LoadQtnLongIndentHighFrame();
void DeleteQtnLongIndentHighFrame();
void RenderQtnLongIndentHighFrame(int16_t sX, int16_t sY);

void LoadQtnShortIndentHighFrame();
void DeleteQtnShortIndentHighFrame();
void RenderQtnShortIndentHighFrame(int16_t sX, int16_t sY);

void LoadQtnShort2IndentFrame();
void DeleteQtnShort2IndentFrame();
void RenderQtnShort2IndentFrame(int16_t sX, int16_t sY);

void LoadQtnShort2IndentHighFrame();
void DeleteQtnShort2IndentHighFrame();
void RenderQtnShort2IndentHighFrame(int16_t sX, int16_t sY);

void LoadQtnIndentFrame();
void DeleteQtnIndentFrame();
void RenderQtnIndentFrame(int16_t sX, int16_t sY);

void LoadAttrib1IndentFrame();
void DeleteAttrib1IndentFrame();
void RenderAttrib1IndentFrame(int16_t sX, int16_t sY);

void LoadAttrib2IndentFrame();
void DeleteAttrib2IndentFrame();
void RenderAttrib2IndentFrame(int16_t sX, int16_t sY);

void LoadAvgMercIndentFrame();
void DeleteAvgMercIndentFrame();
void RenderAvgMercIndentFrame(int16_t sX, int16_t sY);

void LoadAboutUsIndentFrame();
void DeleteAboutUsIndentFrame();
void RenderAboutUsIndentFrame(int16_t sX, int16_t sY);

void RenderAttributeFrameForIndex(int16_t sX, int16_t sY, int32_t iIndex);

// graphical handles

extern SGPVObject *guiANALYSE;
extern SGPVObject *guiATTRIBUTEGRAPH;
extern SGPVObject *guiSMALLSILHOUETTE;

#endif
