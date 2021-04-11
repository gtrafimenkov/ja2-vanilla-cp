#pragma once

#include "sgp/Types.h"

struct SOLDIERTYPE;

class Soldier
{
public:
  Soldier(SOLDIERTYPE* s);

  /** Remove pending action. */
  void removePendingAction();

  /** Remove any pending animation. */
  void removePendingAnimation();

  bool hasPendingAction() const;

  void setPendingAction(UINT8 action);

  /**
   * Handle pending action.
   * Return true, when all further processing should be stopped. */
  bool handlePendingAction(bool inCombat);

protected:

  const char* getPofileName() const;

private:
  SOLDIERTYPE* mSoldier;
};
