#include "Tactical/StructureWrap.h"

#include "Strategic/StrategicMap.h"
#include "Tactical/Overhead.h"
#include "Tactical/RottingCorpses.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

BOOLEAN IsFencePresentAtGridno(int16_t sGridNo) {
  if (FindStructure(sGridNo, STRUCTURE_ANYFENCE) != NULL) {
    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN IsRoofPresentAtGridno(int16_t sGridNo) {
  if (FindStructure(sGridNo, STRUCTURE_ROOF) != NULL) {
    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN IsJumpableFencePresentAtGridno(int16_t sGridNo) {
  STRUCTURE *pStructure;

  pStructure = FindStructure(sGridNo, STRUCTURE_OBSTACLE);

  if (pStructure) {
    if (pStructure->fFlags & STRUCTURE_FENCE && !(pStructure->fFlags & STRUCTURE_SPECIAL)) {
      return (TRUE);
    }
    if (pStructure->pDBStructureRef->pDBStructure->ubArmour == MATERIAL_SANDBAG &&
        StructureHeight(pStructure) < 2) {
      return (TRUE);
    }
  }

  return (FALSE);
}

BOOLEAN IsTreePresentAtGridno(int16_t sGridNo) {
  if (FindStructure(sGridNo, STRUCTURE_TREE) != NULL) {
    return (TRUE);
  }

  return (FALSE);
}

STRUCTURE *GetWallStructOfSameOrientationAtGridno(GridNo const grid_no, int8_t const orientation) {
  FOR_EACH_STRUCTURE(pStructure, grid_no, STRUCTURE_WALLSTUFF) {
    if (pStructure->ubWallOrientation != orientation) continue;

    STRUCTURE *const base = FindBaseStructure(pStructure);
    if (!base) continue;

    return base;
  }
  return 0;
}

BOOLEAN IsDoorVisibleAtGridNo(int16_t sGridNo) {
  STRUCTURE *pStructure;
  int16_t sNewGridNo;

  pStructure = FindStructure(sGridNo, STRUCTURE_ANYDOOR);

  if (pStructure != NULL) {
    // Check around based on orientation
    switch (pStructure->ubWallOrientation) {
      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:

        // Here, check north direction
        sNewGridNo = NewGridNo(sGridNo, DirectionInc(NORTH));

        if (IsRoofVisible2(sNewGridNo)) {
          // OK, now check south, if true, she's not visible
          sNewGridNo = NewGridNo(sGridNo, DirectionInc(SOUTH));

          if (IsRoofVisible2(sNewGridNo)) {
            return (FALSE);
          }
        }
        break;

      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:

        // Here, check west direction
        sNewGridNo = NewGridNo(sGridNo, DirectionInc(WEST));

        if (IsRoofVisible2(sNewGridNo)) {
          // OK, now check south, if true, she's not visible
          sNewGridNo = NewGridNo(sGridNo, DirectionInc(EAST));

          if (IsRoofVisible2(sNewGridNo)) {
            return (FALSE);
          }
        }
        break;
    }
  }

  // Return true here, even if she does not exist
  return (TRUE);
}

BOOLEAN WallExistsOfTopLeftOrientation(int16_t sGridNo) {
  // CJC: changing to search only for normal walls, July 16, 1998
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_WALL) {
    // Check orientation
    if (pStructure->ubWallOrientation == INSIDE_TOP_LEFT ||
        pStructure->ubWallOrientation == OUTSIDE_TOP_LEFT) {
      return (TRUE);
    }
  }

  return (FALSE);
}

BOOLEAN WallExistsOfTopRightOrientation(int16_t sGridNo) {
  // CJC: changing to search only for normal walls, July 16, 1998
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_WALL) {
    // Check orientation
    if (pStructure->ubWallOrientation == INSIDE_TOP_RIGHT ||
        pStructure->ubWallOrientation == OUTSIDE_TOP_RIGHT) {
      return (TRUE);
    }
  }

  return (FALSE);
}

BOOLEAN WallOrClosedDoorExistsOfTopLeftOrientation(int16_t sGridNo) {
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_WALLSTUFF) {
    // skip it if it's an open door
    if (!((pStructure->fFlags & STRUCTURE_ANYDOOR) && (pStructure->fFlags & STRUCTURE_OPEN))) {
      // Check orientation
      if (pStructure->ubWallOrientation == INSIDE_TOP_LEFT ||
          pStructure->ubWallOrientation == OUTSIDE_TOP_LEFT) {
        return (TRUE);
      }
    }
  }

  return (FALSE);
}

BOOLEAN WallOrClosedDoorExistsOfTopRightOrientation(int16_t sGridNo) {
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_WALLSTUFF) {
    // skip it if it's an open door
    if (!((pStructure->fFlags & STRUCTURE_ANYDOOR) && (pStructure->fFlags & STRUCTURE_OPEN))) {
      // Check orientation
      if (pStructure->ubWallOrientation == INSIDE_TOP_RIGHT ||
          pStructure->ubWallOrientation == OUTSIDE_TOP_RIGHT) {
        return (TRUE);
      }
    }
  }

  return (FALSE);
}

BOOLEAN OpenRightOrientedDoorWithDoorOnRightOfEdgeExists(int16_t sGridNo) {
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_ANYDOOR) {
    if (!(pStructure->fFlags & STRUCTURE_OPEN)) break;
    // Check orientation
    if (pStructure->ubWallOrientation == INSIDE_TOP_RIGHT ||
        pStructure->ubWallOrientation == OUTSIDE_TOP_RIGHT) {
      if ((pStructure->fFlags & STRUCTURE_DOOR) || (pStructure->fFlags & STRUCTURE_DDOOR_RIGHT)) {
        return (TRUE);
      }
    }
  }

  return (FALSE);
}

BOOLEAN OpenLeftOrientedDoorWithDoorOnLeftOfEdgeExists(int16_t sGridNo) {
  FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_ANYDOOR) {
    if (!(pStructure->fFlags & STRUCTURE_OPEN)) break;
    // Check orientation
    if (pStructure->ubWallOrientation == INSIDE_TOP_LEFT ||
        pStructure->ubWallOrientation == OUTSIDE_TOP_LEFT) {
      if ((pStructure->fFlags & STRUCTURE_DOOR) || (pStructure->fFlags & STRUCTURE_DDOOR_LEFT)) {
        return (TRUE);
      }
    }
  }

  return (FALSE);
}

static STRUCTURE *FindCuttableWireFenceAtGridNo(int16_t sGridNo) {
  STRUCTURE *pStructure;

  pStructure = FindStructure(sGridNo, STRUCTURE_WIREFENCE);
  if (pStructure != NULL && pStructure->ubWallOrientation != NO_ORIENTATION &&
      !(pStructure->fFlags & STRUCTURE_OPEN)) {
    return (pStructure);
  }
  return (NULL);
}

BOOLEAN CutWireFence(int16_t sGridNo) {
  STRUCTURE *pStructure;

  pStructure = FindCuttableWireFenceAtGridNo(sGridNo);
  if (pStructure) {
    pStructure = SwapStructureForPartnerAndStoreChangeInMap(pStructure);
    if (pStructure) {
      RecompileLocalMovementCosts(sGridNo);
      SetRenderFlags(RENDER_FLAG_FULL);
      return (TRUE);
    }
  }
  return (FALSE);
}

BOOLEAN IsCuttableWireFenceAtGridNo(int16_t sGridNo) {
  return (FindCuttableWireFenceAtGridNo(sGridNo) != NULL);
}

BOOLEAN IsRepairableStructAtGridNo(const int16_t sGridNo, SOLDIERTYPE **const tgt) {
  // OK, first look for a vehicle....
  SOLDIERTYPE *const s = WhoIsThere2(sGridNo, 0);
  if (tgt != NULL) *tgt = s;

  if (s != NULL && s->uiStatusFlags & SOLDIER_VEHICLE) return 2;
  // Then for over a robot....

  // then for SAM site....
  if (DoesSAMExistHere(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, sGridNo)) {
    return (3);
  }

  return (FALSE);
}

SOLDIERTYPE *GetRefuelableStructAtGridNo(int16_t sGridNo) {
  // OK, first look for a vehicle....
  SOLDIERTYPE *const tgt = WhoIsThere2(sGridNo, 0);
  return tgt != NULL && tgt->uiStatusFlags & SOLDIER_VEHICLE ? tgt : NULL;
}

static BOOLEAN IsCutWireFenceAtGridNo(int16_t sGridNo) {
  STRUCTURE *pStructure;

  pStructure = FindStructure(sGridNo, STRUCTURE_WIREFENCE);
  if (pStructure != NULL && (pStructure->ubWallOrientation != NO_ORIENTATION) &&
      (pStructure->fFlags & STRUCTURE_OPEN)) {
    return (TRUE);
  }
  return (FALSE);
}

int16_t FindDoorAtGridNoOrAdjacent(int16_t sGridNo) {
  STRUCTURE *pStructure;
  STRUCTURE *pBaseStructure;
  int16_t sTestGridNo;

  sTestGridNo = sGridNo;
  pStructure = FindStructure(sTestGridNo, STRUCTURE_ANYDOOR);
  if (pStructure) {
    pBaseStructure = FindBaseStructure(pStructure);
    return (pBaseStructure->sGridNo);
  }

  sTestGridNo = sGridNo + DirectionInc(NORTH);
  pStructure = FindStructure(sTestGridNo, STRUCTURE_ANYDOOR);
  if (pStructure) {
    pBaseStructure = FindBaseStructure(pStructure);
    return (pBaseStructure->sGridNo);
  }

  sTestGridNo = sGridNo + DirectionInc(WEST);
  pStructure = FindStructure(sTestGridNo, STRUCTURE_ANYDOOR);
  if (pStructure) {
    pBaseStructure = FindBaseStructure(pStructure);
    return (pBaseStructure->sGridNo);
  }

  return (NOWHERE);
}

BOOLEAN IsCorpseAtGridNo(int16_t sGridNo, uint8_t ubLevel) {
  if (GetCorpseAtGridNo(sGridNo, ubLevel) != NULL) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

BOOLEAN SetOpenableStructureToClosed(int16_t sGridNo, uint8_t ubLevel) {
  STRUCTURE *pStructure;
  STRUCTURE *pNewStructure;

  pStructure = FindStructure(sGridNo, STRUCTURE_OPENABLE);
  if (!pStructure) {
    return (FALSE);
  }

  if (pStructure->fFlags & STRUCTURE_OPEN) {
    pNewStructure = SwapStructureForPartner(pStructure);
    if (pNewStructure != NULL) {
      RecompileLocalMovementCosts(sGridNo);
      SetRenderFlags(RENDER_FLAG_FULL);
    }
  }
  // else leave it as is!
  return (TRUE);
}
