#ifndef __INSURANCE_CONTRACT_H
#define __INSURANCE_CONTRACT_H

#include "JA2Types.h"

void EnterInsuranceContract();
void ExitInsuranceContract();
void HandleInsuranceContract();
void RenderInsuranceContract();

extern int16_t gsCurrentInsuranceMercIndex;

// determines if a merc will run out of there insurance contract
void DailyUpdateOfInsuredMercs();

// void InsuranceContractPayLifeInsuranceForDeadMerc( LIFE_INSURANCE_PAYOUT
// *pPayoutStruct );

void AddLifeInsurancePayout(SOLDIERTYPE *);
void InsuranceContractPayLifeInsuranceForDeadMerc(uint8_t ubPayoutID);
void StartInsuranceInvestigation(uint8_t ubPayoutID);
void EndInsuranceInvestigation(uint8_t ubPayoutID);

int32_t CalculateInsuranceContractCost(int32_t iLength, uint8_t ubMercID);

void InsuranceContractEndGameShutDown();

void PurchaseOrExtendInsuranceForSoldier(SOLDIERTYPE *pSoldier, uint32_t uiInsuranceLength);

#endif
