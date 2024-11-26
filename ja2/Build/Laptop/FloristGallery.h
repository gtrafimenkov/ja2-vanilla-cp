#ifndef __FLORIST_GALLERY_H
#define __FLORIST_GALLERY_H

#include "SGP/Types.h"

#define FLOR_GALLERY_TEXT_FILE BINARYDATADIR "/flowerdesc.edt"
#define FLOR_GALLERY_TEXT_TITLE_SIZE 80
#define FLOR_GALLERY_TEXT_PRICE_SIZE 80
#define FLOR_GALLERY_TEXT_DESC_SIZE 80 * 4
#define FLOR_GALLERY_TEXT_TOTAL_SIZE 80 * 6

BOOLEAN EnterFloristGallery();
void ExitFloristGallery();
void HandleFloristGallery();
void RenderFloristGallery();
void EnterInitFloristGallery();

extern uint32_t guiCurrentlySelectedFlower;
extern uint8_t gubCurFlowerIndex;

#endif
