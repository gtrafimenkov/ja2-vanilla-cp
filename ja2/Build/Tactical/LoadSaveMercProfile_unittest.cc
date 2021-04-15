﻿// -*-coding: utf-8-with-signature-unix;-*-

#include "Tactical/LoadSaveMercProfile.h"

#include "Laptop/IMPCompileCharacter.h"
#include "SGP/EncodingCorrectors.h"
#include "SGP/FileMan.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierProfileType.h"
#include "gtest/gtest.h"

TEST(LoadSaveMercProfileTest, vanillaProfile) {
  MERCPROFILESTRUCT p;
  INT32 profileId;
  INT32 portraitNumber;
  ExtractImpProfileFromFile("unittests/saves/vanilla-russian/IMP.dat", &profileId, &portraitNumber,
                            p);
  EXPECT_EQ(profileId, PLAYER_GENERATED_CHARACTER_ID);
  EXPECT_EQ(portraitNumber, 0);
  EXPECT_STREQ(p.zName, L"Foo Bar .....................");
  EXPECT_STREQ(p.zNickname, L".FooBar.");
  // UINT8		ubFaceIndex;
  // PaletteRepID		PANTS;
  // PaletteRepID		VEST;
  // PaletteRepID		SKIN;
  // PaletteRepID		HAIR;
  EXPECT_EQ(p.bSex, MALE);
  // INT8		bArmourAttractiveness;
  // UINT8		ubMiscFlags2;
  // INT8		bEvolution;
  // UINT8		ubMiscFlags;
  // UINT8		bSexist;
  // INT8		bLearnToHate;
  // UINT8		ubQuoteRecord;
  // INT8		bDeathRate;
  // INT16		sExpLevelGain;
  // INT16		sLifeGain;
  // INT16		sAgilityGain;
  // INT16		sDexterityGain;
  // INT16		sWisdomGain;
  // INT16		sMarksmanshipGain;
  // INT16		sMedicalGain;
  // INT16		sMechanicGain;
  // INT16		sExplosivesGain;
  // UINT8		ubBodyType;
  // INT8		bMedical;
  // UINT16	usEyesX;
  // UINT16	usEyesY;
  // UINT16	usMouthX;
  // UINT16	usMouthY;
  // UINT32	uiBlinkFrequency;
  // UINT32	uiExpressionFrequency;
  EXPECT_EQ(p.sSectorX, 0);
  EXPECT_EQ(p.sSectorY, 0);
  // UINT32	uiDayBecomesAvailable;
  EXPECT_EQ(p.bStrength, 55);
  EXPECT_EQ(p.bLifeMax, 55);
  // INT8		bExpLevelDelta;
  // INT8		bLifeDelta;
  // INT8		bAgilityDelta;
  // INT8		bDexterityDelta;
  // INT8		bWisdomDelta;
  // INT8		bMarksmanshipDelta;
  // INT8		bMedicalDelta;
  // INT8		bMechanicDelta;
  // INT8		bExplosivesDelta;
  // INT8    bStrengthDelta;
  // INT8    bLeadershipDelta;
  // UINT16  usKills;
  // UINT16  usAssists;
  // UINT16  usShotsFired;
  // UINT16  usShotsHit;
  // UINT16  usBattlesFought;
  // UINT16  usTimesWounded;
  // UINT16  usTotalDaysServed;
  // INT16		sLeadershipGain;
  // INT16		sStrengthGain;
  // UINT32	uiBodyTypeSubFlags;
  // INT16	sSalary;
  EXPECT_EQ(p.bLife, 55);
  EXPECT_EQ(p.bDexterity, 55);
  // INT8	bPersonalityTrait;
  // INT8	bSkillTrait;
  // INT8	bReputationTolerance;
  EXPECT_EQ(p.bExplosive, 55);
  // INT8	bSkillTrait2;
  EXPECT_EQ(p.bLeadership, 55);
  // INT8	bBuddy[5];
  // INT8	bHated[5];
  // INT8	bExpLevel;
  EXPECT_EQ(p.bMarksmanship, 85);
  EXPECT_EQ(p.bWisdom, 65);
  // UINT8	bInvStatus[19];
  // UINT8 bInvNumber[19];
  // UINT16 usApproachFactor[4];
  // INT8	bMainGunAttractiveness;
  EXPECT_EQ(p.bAgility, 55);
  // BOOLEAN	fUseProfileInsertionInfo;
  // INT16		sGridNo;
  // UINT8		ubQuoteActionID;
  // INT8		bMechanical;
  // UINT8	ubInvUndroppable;
  // UINT8	ubRoomRangeStart[2];
  // UINT16 inv[19];
  // UINT16 usStatChangeChances[ 12 ];
  // UINT16 usStatChangeSuccesses[ 12 ];
  // UINT8	ubStrategicInsertionCode;
  // UINT8	ubRoomRangeEnd[2];
  // UINT8 ubLastQuoteSaid;
  // INT8 bRace;
  // INT8 bNationality;
  // INT8 bAppearance;
  // INT8 bAppearanceCareLevel;
  // INT8 bRefinement;
  // INT8 bRefinementCareLevel;
  // INT8 bHatedNationality;
  // INT8 bHatedNationalityCareLevel;
  // INT8 bRacist;
  // UINT32 uiWeeklySalary;
  // UINT32 uiBiWeeklySalary;
  // INT8 bMedicalDeposit;
  // INT8 bAttitude;
  // UINT16 sMedicalDepositAmount;
  // INT8 bLearnToLike;
  // UINT8 ubApproachVal[4];
  // UINT8 ubApproachMod[3][4];
  // INT8 bTown;
  // INT8 bTownAttachment;
  // UINT16 usOptionalGearCost;
  // INT8 bMercOpinion[75];
  // INT8 bApproached;
  // INT8 bMercStatus;
  // INT8 bHatedTime[5];
  // INT8 bLearnToLikeTime;
  // INT8 bLearnToHateTime;
  // INT8 bHatedCount[5];
  // INT8 bLearnToLikeCount;
  // INT8 bLearnToHateCount;
  // UINT8 ubLastDateSpokenTo;
  // UINT8 bLastQuoteSaidWasSpecial;
  // INT8	bSectorZ;
  // UINT16 usStrategicInsertionData;
  // INT8 bFriendlyOrDirectDefaultResponseUsedRecently;
  // INT8 bRecruitDefaultResponseUsedRecently;
  // INT8 bThreatenDefaultResponseUsedRecently;
  // INT8 bNPCData;
  // INT32	iBalance;
  // UINT8	ubCivilianGroup;
  // UINT8	ubNeedForSleep;
  // UINT32	uiMoney;
  // INT8	bNPCData2;
  // UINT8	ubMiscFlags3;
  // UINT8 ubDaysOfMoraleHangover;
  // UINT8	ubNumTimesDrugUseInLifetime;
  // UINT32	uiPrecedentQuoteSaid;
  // INT16		sPreCombatGridNo;
  // UINT8		ubTimeTillNextHatedComplaint;
  // UINT8		ubSuspiciousDeath;
  // INT32	iMercMercContractLength;
  // UINT32	uiTotalCostToDate;
}

TEST(LoadSaveMercProfileTest, stracLinuxProfile) {
  MERCPROFILESTRUCT p;
  INT32 profileId;
  INT32 portraitNumber;
  ExtractImpProfileFromFile("unittests/saves/strac-macos/imp.dat", &profileId, &portraitNumber, p);
  EXPECT_EQ(profileId, PLAYER_GENERATED_CHARACTER_ID);
  EXPECT_EQ(portraitNumber, 0);
  EXPECT_STREQ(p.zName, L"Vasya Вася Курочкин Kurochki");
  EXPECT_STREQ(p.zNickname, L"ВАСЯКУРА");
  // UINT8		ubFaceIndex;
  // PaletteRepID		PANTS;
  // PaletteRepID		VEST;
  // PaletteRepID		SKIN;
  // PaletteRepID		HAIR;
  EXPECT_EQ(p.bSex, MALE);
  // INT8		bArmourAttractiveness;
  // UINT8		ubMiscFlags2;
  // INT8		bEvolution;
  // UINT8		ubMiscFlags;
  // UINT8		bSexist;
  // INT8		bLearnToHate;
  // UINT8		ubQuoteRecord;
  // INT8		bDeathRate;
  // INT16		sExpLevelGain;
  // INT16		sLifeGain;
  // INT16		sAgilityGain;
  // INT16		sDexterityGain;
  // INT16		sWisdomGain;
  // INT16		sMarksmanshipGain;
  // INT16		sMedicalGain;
  // INT16		sMechanicGain;
  // INT16		sExplosivesGain;
  // UINT8		ubBodyType;
  // INT8		bMedical;
  // UINT16	usEyesX;
  // UINT16	usEyesY;
  // UINT16	usMouthX;
  // UINT16	usMouthY;
  // UINT32	uiBlinkFrequency;
  // UINT32	uiExpressionFrequency;
  EXPECT_EQ(p.sSectorX, 0);
  EXPECT_EQ(p.sSectorY, 0);
  // UINT32	uiDayBecomesAvailable;
  EXPECT_EQ(p.bStrength, 55);
  EXPECT_EQ(p.bLifeMax, 55);
  // INT8		bExpLevelDelta;
  // INT8		bLifeDelta;
  // INT8		bAgilityDelta;
  // INT8		bDexterityDelta;
  // INT8		bWisdomDelta;
  // INT8		bMarksmanshipDelta;
  // INT8		bMedicalDelta;
  // INT8		bMechanicDelta;
  // INT8		bExplosivesDelta;
  // INT8    bStrengthDelta;
  // INT8    bLeadershipDelta;
  // UINT16  usKills;
  // UINT16  usAssists;
  // UINT16  usShotsFired;
  // UINT16  usShotsHit;
  // UINT16  usBattlesFought;
  // UINT16  usTimesWounded;
  // UINT16  usTotalDaysServed;
  // INT16		sLeadershipGain;
  // INT16		sStrengthGain;
  // UINT32	uiBodyTypeSubFlags;
  // INT16	sSalary;
  EXPECT_EQ(p.bLife, 55);
  EXPECT_EQ(p.bDexterity, 55);
  // INT8	bPersonalityTrait;
  // INT8	bSkillTrait;
  // INT8	bReputationTolerance;
  EXPECT_EQ(p.bExplosive, 55);
  // INT8	bSkillTrait2;
  EXPECT_EQ(p.bLeadership, 55);
  // INT8	bBuddy[5];
  // INT8	bHated[5];
  // INT8	bExpLevel;
  EXPECT_EQ(p.bMarksmanship, 85);
  EXPECT_EQ(p.bWisdom, 65);
  // UINT8	bInvStatus[19];
  // UINT8 bInvNumber[19];
  // UINT16 usApproachFactor[4];
  // INT8	bMainGunAttractiveness;
  EXPECT_EQ(p.bAgility, 55);
  // BOOLEAN	fUseProfileInsertionInfo;
  // INT16		sGridNo;
  // UINT8		ubQuoteActionID;
  // INT8		bMechanical;
  // UINT8	ubInvUndroppable;
  // UINT8	ubRoomRangeStart[2];
  // UINT16 inv[19];
  // UINT16 usStatChangeChances[ 12 ];
  // UINT16 usStatChangeSuccesses[ 12 ];
  // UINT8	ubStrategicInsertionCode;
  // UINT8	ubRoomRangeEnd[2];
  // UINT8 ubLastQuoteSaid;
  // INT8 bRace;
  // INT8 bNationality;
  // INT8 bAppearance;
  // INT8 bAppearanceCareLevel;
  // INT8 bRefinement;
  // INT8 bRefinementCareLevel;
  // INT8 bHatedNationality;
  // INT8 bHatedNationalityCareLevel;
  // INT8 bRacist;
  // UINT32 uiWeeklySalary;
  // UINT32 uiBiWeeklySalary;
  // INT8 bMedicalDeposit;
  // INT8 bAttitude;
  // UINT16 sMedicalDepositAmount;
  // INT8 bLearnToLike;
  // UINT8 ubApproachVal[4];
  // UINT8 ubApproachMod[3][4];
  // INT8 bTown;
  // INT8 bTownAttachment;
  // UINT16 usOptionalGearCost;
  // INT8 bMercOpinion[75];
  // INT8 bApproached;
  // INT8 bMercStatus;
  // INT8 bHatedTime[5];
  // INT8 bLearnToLikeTime;
  // INT8 bLearnToHateTime;
  // INT8 bHatedCount[5];
  // INT8 bLearnToLikeCount;
  // INT8 bLearnToHateCount;
  // UINT8 ubLastDateSpokenTo;
  // UINT8 bLastQuoteSaidWasSpecial;
  // INT8	bSectorZ;
  // UINT16 usStrategicInsertionData;
  // INT8 bFriendlyOrDirectDefaultResponseUsedRecently;
  // INT8 bRecruitDefaultResponseUsedRecently;
  // INT8 bThreatenDefaultResponseUsedRecently;
  // INT8 bNPCData;
  // INT32	iBalance;
  // UINT8	ubCivilianGroup;
  // UINT8	ubNeedForSleep;
  // UINT32	uiMoney;
  // INT8	bNPCData2;
  // UINT8	ubMiscFlags3;
  // UINT8 ubDaysOfMoraleHangover;
  // UINT8	ubNumTimesDrugUseInLifetime;
  // UINT32	uiPrecedentQuoteSaid;
  // INT16		sPreCombatGridNo;
  // UINT8		ubTimeTillNextHatedComplaint;
  // UINT8		ubSuspiciousDeath;
  // INT32	iMercMercContractLength;
  // UINT32	uiTotalCostToDate;
}
