#include "ParameterBlock.h"

CombTempParam_t PB_g_sTemperatureParam =
{
	//UFEC23 - Test du 2022-11-29 NOUVEAU PCB (une seule carte)               //tenth of F
	.WaitingToIgnition = 1000,
	.IgnitionToTrise = 5250,
	.TriseTargetLow = 6500,
	.TriseTargetHigh = 6900,
	.CombLowTarget = 6600,
	.CombHighTarget = 7000,
	.CombLowtoSuperLow = 7000,
	.FlameLoss = 7500,
	.FlameLossDelta = 1750,
	.CoalCrossOverRearLow = 8000,
	.CoalCrossOverRearHigh = 7000,
	.CoalDeltaTemp = 2500,
	.CoalStoveTemp = 900,
	.OverheatPlenum = 2200,
	.OverheatPlenumExit = 2100,
	.OverheatBaffle = 15000,
	.OverheatChamber = 15000,
};

MotorOpeningsParam_t PB_g_sPrimaryMotorParam =
{
	.MaxWaiting = 6,
	.MinWaiting = 6,
	.MaxReload = 97,
	.MinReload = 58,
	.MaxTempRise = 85,
	.MinTempRise = 17,
	.MaxCombHigh = 70,
	.MinCombHigh = 14,
	.MaxCombLow = 39,
	.MinCombLow = 0,
	.MaxCombSuperLow = 25,
	.MinCombSuperLow = 0,
	.MaxCoalHigh = 0,
	.MinCoalHigh = 0,
	.MaxCoalLow = 0,
	.MinCoalLow = 0,

};

MotorOpeningsParam_t PB_g_sGrillMotorParam =
{
	.MaxWaiting = 0,
	.MinWaiting = 0,
	.MaxReload = 97,
	.MinReload = 0,
	.MaxTempRise = 30,
	.MinTempRise = 0,
	.MaxCombHigh = 0,
	.MinCombHigh = 0,
	.MaxCombLow = 0,
	.MinCombLow = 0,
	.MaxCombSuperLow = 0,
	.MinCombSuperLow = 0,
	.MaxCoalHigh = 97,
	.MinCoalHigh = 97,
	.MaxCoalLow = 24,
	.MinCoalLow = 24,


};

MotorOpeningsParam_t PB_g_sSecondaryMotorParam =
{//Added for current PCB model (parameters must be adjusted by user)
	.MaxWaiting = 6,
	.MinWaiting = 6,
	.MaxReload = 97,
	.MinReload = 97,
	.MaxTempRise = 58,
	.MinTempRise = 58,
	.MaxCombHigh = 97,
	.MinCombHigh = 97,
	.MaxCombLow = 97,
	.MinCombLow = 97,
	.MaxCombSuperLow = 50,
	.MinCombSuperLow = 50,
	.MaxCoalHigh = 50,
	.MinCoalHigh = 50,
	.MaxCoalLow = 10,
	.MinCoalLow = 10,
};
