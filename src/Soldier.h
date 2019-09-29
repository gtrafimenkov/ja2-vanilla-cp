#pragma once

#include "Types.h"

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

  /** Switch to the best night goggles. */
  void putNightHeadGear();

  /** Switch to sungoggles or remove the night goggles. */
  void putDayHeadGear();

protected:

  const char* getPofileName() const;

  /** Get free head slot or NO_SLOT if the both are occupied. */
  int8_t getFreeHeadSlot() const;

  /** Find free pocket.  Return the slot number or NO_SLOT. */
  int8_t getFreePocket() const;

  /** Swap inventory slots. */
  void swapInventorySlots(int8_t firstSlot, int8_t secondSlot);

private:
  SOLDIERTYPE* mSoldier;
};
