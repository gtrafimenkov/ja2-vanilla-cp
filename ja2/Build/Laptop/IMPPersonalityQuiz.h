#ifndef _IMP_PERSOANLITY_QUIZ_H
#define _IMP_PERSOANLITY_QUIZ_H

#include "SGP/Types.h"

void EnterIMPPersonalityQuiz();
void RenderIMPPersonalityQuiz();
void ExitIMPPersonalityQuiz();
void HandleIMPPersonalityQuiz();

void BltAnswerIndents(INT32 iNumberOfIndents);

extern INT32 giCurrentPersonalityQuizQuestion;
extern INT32 iCurrentAnswer;

#endif
