#ifndef _IMP_PERSOANLITY_QUIZ_H
#define _IMP_PERSOANLITY_QUIZ_H

#include "SGP/Types.h"

void EnterIMPPersonalityQuiz();
void RenderIMPPersonalityQuiz();
void ExitIMPPersonalityQuiz();
void HandleIMPPersonalityQuiz();

void BltAnswerIndents(int32_t iNumberOfIndents);

extern int32_t giCurrentPersonalityQuizQuestion;
extern int32_t iCurrentAnswer;

#endif
