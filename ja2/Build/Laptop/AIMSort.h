#ifndef __AIMSORT_H_
#define __AIMSORT_H_

#include <cstdint>

extern uint8_t gubCurrentSortMode;
extern uint8_t gubCurrentListMode;


#define		AIM_ASCEND									6
#define		AIM_DESCEND									7


void GameInitAimSort(void);
void EnterAimSort(void);
void ExitAimSort(void);
void RenderAimSort(void);

#endif
