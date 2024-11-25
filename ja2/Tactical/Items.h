// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef ITEMS_H
#define ITEMS_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"
#include "Tactical/Weapons.h"

void DamageObj(OBJECTTYPE *pObj, int8_t bAmount);

extern uint8_t SlotToPocket[7];

BOOLEAN WeaponInHand(const SOLDIERTYPE *pSoldier);

int8_t FindObj(const SOLDIERTYPE *pSoldier, uint16_t usItem);
int8_t FindAmmo(SOLDIERTYPE const *, AmmoKind, uint8_t ubMagSize, int8_t bExcludeSlot);

int8_t FindAttachment(const OBJECTTYPE *pObj, uint16_t usItem);
int8_t FindObjClass(const SOLDIERTYPE *s, uint32_t usItemClass);
extern int8_t FindAIUsableObjClass(SOLDIERTYPE *pSoldier, uint32_t usItemClass);
extern int8_t FindAIUsableObjClassWithin(SOLDIERTYPE *pSoldier, uint32_t usItemClass, int8_t bLower,
                                         int8_t bUpper);
extern int8_t FindEmptySlotWithin(SOLDIERTYPE *pSoldier, int8_t bLower, int8_t bUpper);
extern int8_t FindExactObj(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj);
int8_t FindObjInObjRange(const SOLDIERTYPE *s, uint16_t usItem1, uint16_t usItem2);
extern int8_t FindLaunchable(SOLDIERTYPE *pSoldier, uint16_t usWeapon);
extern int8_t FindGLGrenade(SOLDIERTYPE *pSoldier);
extern int8_t FindThrowableGrenade(SOLDIERTYPE *pSoldier);
extern int8_t FindUsableObj(SOLDIERTYPE *pSoldier, uint16_t usItem);

extern void DeleteObj(OBJECTTYPE *pObj);
extern void SwapObjs(OBJECTTYPE *pObj1, OBJECTTYPE *pObj2);

extern void RemoveObjFrom(OBJECTTYPE *pObj, uint8_t ubRemoveIndex);
// Returns true if swapped, false if added to end of stack
extern BOOLEAN PlaceObjectAtObjectIndex(OBJECTTYPE *pSourceObj, OBJECTTYPE *pTargetObj,
                                        uint8_t ubIndex);
extern void GetObjFrom(OBJECTTYPE *pObj, uint8_t ubGetIndex, OBJECTTYPE *pDest);

bool AttachObject(SOLDIERTYPE *, OBJECTTYPE *pTargetObj, OBJECTTYPE *pAttachment);
extern BOOLEAN RemoveAttachment(OBJECTTYPE *pObj, int8_t bAttachPos, OBJECTTYPE *pNewObj);

uint8_t CalculateObjectWeight(const OBJECTTYPE *pObject);
uint32_t CalculateCarriedWeight(const SOLDIERTYPE *pSoldier);

extern uint16_t TotalPoints(const OBJECTTYPE *);
uint16_t UseKitPoints(OBJECTTYPE &, uint16_t points, SOLDIERTYPE const &);

extern BOOLEAN EmptyWeaponMagazine(OBJECTTYPE *pWeapon, OBJECTTYPE *pAmmo);
void CreateItem(uint16_t usItem, int8_t bStatus, OBJECTTYPE *);
void CreateItems(uint16_t usItem, int8_t bStatus, uint8_t ubNumber, OBJECTTYPE *);
void CreateMoney(uint32_t uiMoney, OBJECTTYPE *);
uint16_t DefaultMagazine(uint16_t gun);
uint16_t RandomMagazine(uint16_t usItem, uint8_t ubPercentStandard);
extern BOOLEAN ReloadGun(SOLDIERTYPE *pSoldier, OBJECTTYPE *pGun, OBJECTTYPE *pAmmo);

uint8_t ItemSlotLimit(uint16_t usItem, int8_t bSlot);

// Function to put an item in a soldier profile
// It's very primitive, just finds an empty place!
BOOLEAN PlaceObjectInSoldierProfile(uint8_t ubProfile, OBJECTTYPE *pObject);
BOOLEAN RemoveObjectFromSoldierProfile(uint8_t ubProfile, uint16_t usItem);
int8_t FindObjectInSoldierProfile(MERCPROFILESTRUCT const &, uint16_t item_id);

void SetMoneyInSoldierProfile(uint8_t ubProfile, uint32_t uiMoney);

void CheckEquipmentForDamage(SOLDIERTYPE *pSoldier, int32_t iDamage);
BOOLEAN ArmBomb(OBJECTTYPE *pObj, int8_t bSetting);

// NOTE TO ANDREW:
//
// The following functions expect that pObj points to the object
// "in the cursor", which should have memory allocated for it already
BOOLEAN PlaceObject(SOLDIERTYPE *pSoldier, int8_t bPos, OBJECTTYPE *pObj);

// Send fNewItem to true to set off new item glow in inv panel
BOOLEAN AutoPlaceObject(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, BOOLEAN fNewItem);
BOOLEAN RemoveObjectFromSlot(SOLDIERTYPE *pSoldier, int8_t bPos, OBJECTTYPE *pObj);

// Swap keys in keyring slot and keys in pocket
void SwapKeysToSlot(SOLDIERTYPE &, int8_t key_ring_pos, OBJECTTYPE &);

// create a keyobject
void CreateKeyObject(OBJECTTYPE *, uint8_t ubNumberOfKeys, uint8_t ubKeyIdValue);
BOOLEAN DeleteKeyObject(OBJECTTYPE *pObj);
void AllocateObject(OBJECTTYPE **pObj);

// removes a key from a *KEYRING* slot
BOOLEAN RemoveKeyFromSlot(SOLDIERTYPE *pSoldier, int8_t bKeyRingPosition, OBJECTTYPE *pObj);

// take several
BOOLEAN RemoveKeysFromSlot(SOLDIERTYPE *pSoldier, int8_t bKeyRingPosition, uint8_t ubNumberOfKeys,
                           OBJECTTYPE *pObj);

// add the keys to an inventory slot
uint8_t AddKeysToSlot(SOLDIERTYPE &, int8_t key_ring_pos, OBJECTTYPE const &key);

// Kris:  December 9, 1997
// I need a bunch of validation functions for ammo, attachments, etc., so I'll
// be adding them here. Chris, maybe you might find these useful, or add your
// own.  I don't really know what I'm doing yet, so feel free to correct me...

// Simple check to see if the item has any attachments
bool ItemHasAttachments(OBJECTTYPE const &);

// Determine if this item can receive this attachment.  This is different, in
// that it may be possible to have this attachment on this item, but may already
// have an attachment on it which doesn't work simultaneously with the new
// attachment (like a silencer and duckbill).
BOOLEAN ValidItemAttachment(const OBJECTTYPE *pObj, uint16_t usAttachment,
                            BOOLEAN fAttemptingAttachment);

// Determines if it is possible to equip this weapon with this ammo.
BOOLEAN ValidAmmoType(uint16_t usItem, uint16_t usAmmoType);

// Determine if it is possible to add this attachment to the item
bool ValidAttachment(uint16_t attachment, uint16_t item);

BOOLEAN ValidLaunchable(uint16_t usLaunchable, uint16_t usItem);
uint16_t GetLauncherFromLaunchable(uint16_t usLaunchable);

BOOLEAN ValidMerge(uint16_t usMerge, uint16_t usItem);

// Is the item passed a medical kit?
BOOLEAN IsMedicalKitItem(const OBJECTTYPE *pObject);

BOOLEAN AutoReload(SOLDIERTYPE *pSoldier);
int8_t FindAmmoToReload(SOLDIERTYPE *pSoldier, int8_t bWeaponIn, int8_t bExcludeSlot);

void SwapHandItems(SOLDIERTYPE *pSoldier);

int8_t FindAttachmentByClass(OBJECTTYPE const *, uint32_t uiItemClass);
void RemoveObjs(OBJECTTYPE *pObj, uint8_t ubNumberToRemove);

void WaterDamage(SOLDIERTYPE &);

int8_t FindObjWithin(SOLDIERTYPE *pSoldier, uint16_t usItem, int8_t bLower, int8_t bUpper);

BOOLEAN ApplyCamo(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, BOOLEAN *pfGoodAPs);

BOOLEAN ItemIsLegal(uint16_t usItemIndex);
BOOLEAN ExtendedGunListGun(uint16_t usGun);
uint16_t StandardGunListReplacement(uint16_t usGun);
uint16_t FindReplacementMagazineIfNecessary(uint16_t old_gun_id, uint16_t old_ammo_id,
                                            uint16_t new_gun_id);

BOOLEAN DamageItemOnGround(OBJECTTYPE *pObject, int16_t sGridNo, int8_t bLevel, int32_t iDamage,
                           SOLDIERTYPE *owner);

BOOLEAN ApplyCanteen(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, BOOLEAN *pfGoodAPs);
BOOLEAN ApplyElixir(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, BOOLEAN *pfGoodAPs);

BOOLEAN CompatibleFaceItem(uint16_t usItem1, uint16_t usItem2);

uint32_t MoneySlotLimit(int8_t bSlot);

void CheckEquipmentForFragileItemDamage(SOLDIERTYPE *pSoldier, int32_t iDamage);

// Range of Xray device
#define XRAY_RANGE 40
// Seconds that Xray lasts
#define XRAY_TIME 5

extern void ActivateXRayDevice(SOLDIERTYPE *pSoldier);
extern void TurnOffXRayEffects(SOLDIERTYPE *pSoldier);
int8_t FindLaunchableAttachment(const OBJECTTYPE *pObj, uint16_t usWeapon);

BOOLEAN CanItemFitInPosition(SOLDIERTYPE *s, OBJECTTYPE *pObj, int8_t bPos,
                             BOOLEAN fDoingPlacement);

void SetNewItem(SOLDIERTYPE *pSoldier, uint8_t ubInvPos, BOOLEAN fNewItem);
void CleanUpStack(OBJECTTYPE *pObj, OBJECTTYPE *pCursorObj);
void StackObjs(OBJECTTYPE *pSourceObj, OBJECTTYPE *pTargetObj, uint8_t ubNumberToCopy);
bool ItemIsCool(OBJECTTYPE const &);

uint16_t StandardGunListAmmoReplacement(uint16_t usAmmo);

bool HasObjectImprint(OBJECTTYPE const &);

#endif
