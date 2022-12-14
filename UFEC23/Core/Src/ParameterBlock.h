/*
 * ParameterBlock.h
 *
 *  Created on: Dec 14, 2022
 *      Author: mcarrier
 */

#ifndef SRC_PARAMETERBLOCK_H_
#define SRC_PARAMETERBLOCK_H_

#include <stddef.h>
#include <stdint.h>

typedef struct CombTempParam
{
	int32_t WaitingToIgnition; //max environ 32000dixieme de f
	int32_t IgnitionToTrise;
	int32_t TriseTargetLow;
	int32_t TriseTargetHigh;
	int32_t CombLowTarget;
	int32_t CombHighTarget;
	int32_t CombLowtoSuperLow;
	int32_t FlameLoss;
	int32_t FlameLossDelta;
	int32_t CoalCrossOverRearLow;
	int32_t CoalCrossOverRearHigh;
	int32_t CoalDeltaTemp;
	int32_t CoalStoveTemp;
	int32_t OverheatPlenum;
	int32_t OverheatPlenumExit;
	int32_t OverheatBaffle;
	int32_t OverheatChamber;
} CombTempParam_t;

typedef struct MotorOpeningsParam
{
	int32_t MaxWaiting; //max environ 32000 dixieme de f
	int32_t MinWaiting;
	int32_t MaxReload;
	int32_t MinReload;
	int32_t MaxTempRise;
	int32_t MinTempRise;
	int32_t MaxCombLow;
	int32_t MinCombLow;
	int32_t MaxCombSuperLow;
	int32_t MinCombSuperLow;
	int32_t MaxCombHigh;
	int32_t MinCombHigh;
	int32_t MaxCoalHigh;
	int32_t MinCoalHigh;
	int32_t MaxCoalLow;
	int32_t MinCoalLow;
}MotorOpeningsParam_t;

// Global variables
extern CombTempParam_t PB_g_sTemperatureParam;

extern MotorOpeningsParam_t PB_g_sPrimaryMotorParam;
extern MotorOpeningsParam_t PB_g_sGrillMotorParam;
extern MotorOpeningsParam_t PB_g_sSecondaryMotorParam;

#endif /* SRC_PARAMETERBLOCK_H_ */
