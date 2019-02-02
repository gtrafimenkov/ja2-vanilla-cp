#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#define BOBBYR_ITEM_DESC_NAME_SIZE  80
#define BOBBYR_ITEM_DESC_INFO_SIZE 320
#define BOBBYR_ITEM_DESC_FILE_SIZE 400

void LoadBobbyRayItemName(uint16_t index, wchar_t *buf, int bufSize);
void LoadBobbyRayItemDescription(uint16_t index, wchar_t *buf, int bufSize);

#endif
