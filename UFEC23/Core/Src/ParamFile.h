/*
 * ParameterFileDef.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#ifndef _PARAMETERFILEDEF_H_
#define _PARAMETERFILEDEF_H_

#include "../Libs/ParameterFileLib.h"

#define PF_PRIMARY_MINIMUM_OPENING				6 //5.4 degres @ 0.9 deg/pas
#define PF_PRIMARY_CLOSED  						PF_PRIMARY_MINIMUM_OPENING
#define PF_PRIMARY_FULL_OPEN			   		100

#define PF_SECONDARY_MINIMUM_OPENING			6
#define PF_SECONDARY_CLOSED						PF_SECONDARY_MINIMUM_OPENING
#define PF_SECONDARY_FULL_OPEN				    100

#define PF_GRILL_CLOSED                   		0
#define PF_GRILL_MINIMUM_OPENING 				PF_GRILL_CLOSED
#define PF_GRILL_FULL_OPEN                      100

typedef struct
{
	int32_t s32TLSGain;
	int32_t s32TSLINT;
	int32_t s32DACCMD;
	int32_t s32TIMEINTERVAL;
} PF_UsrParam;

typedef struct
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
} PF_CombTempParam_t;

typedef struct
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
} PF_MotorOpeningsParam_t;

#define PFD_TSLGAIN 		"TSLgain"
#define PFD_TSLINT 			"TSLint"
#define PFD_DACCMD 			"DACcmd"
#define PFD_TIMEINTERVAL 	"TimeInterval"
#define PFD_SECPERSTEP		"SecondsPerStepOffset"
#define PFD_MAXAPERTURE		"MaxApertureOffset"
#define PFD_MINAPERTURE		"MinApertureOffset"
#define PFD_APERTUREOFFSET	"ApertureOffset"

// Temperature
#define PFD_WAITINGTOIGNITION     "temp_WaitingToIgnition"
#define PFD_IGNITIONTOTRISE       "temp_IgnitionToTrise"
#define PFD_TRISETARGETLOW        "temp_TriseTargetLow"
#define PFD_TRISETARGETHIGH       "temp_TriseTargetHigh"
#define PFD_COMBLOWTARGET         "temp_CombLowTarget"
#define PFD_COMBHIGHTARGET        "temp_CombHighTarget"
#define PFD_COMBLOWTOSUPERLOW     "temp_CombLowtoSuperLow"
#define PFD_FLAMELOSS             "temp_FlameLoss"
#define PFD_FLAMELOSSDELTA        "temp_FlameLossDelta"
#define PFD_COALCROSSOVERREARLOW  "temp_CoalCrossOverRearLow"
#define PFD_COALCROSSOVERREARHIGH "temp_CoalCrossOverRearHigh"
#define PFD_COALDELTATEMP         "temp_CoalDeltaTemp"
#define PFD_COALSTOVETEMP         "temp_CoalStoveTemp"
#define PFD_OVERHEATPLENUM        "temp_OverheatPlenum"
#define PFD_OVERHEATPLENUMEXIT    "temp_OverheatPlenumExit"
#define PFD_OVERHEATBAFFLE        "temp_OverheatBaffle"
#define PFD_OVERHEATCHAMBER       "temp_OverheatChamber"

// Primary motor
#define PFD_PM_MAXWAITING       "pm_MaxWaiting"
#define PFD_PM_MINWAITING       "pm_MinWaiting"
#define PFD_PM_MAXRELOAD        "pm_MaxReload"
#define PFD_PM_MINRELOAD        "pm_MinReload"
#define PFD_PM_MAXTEMPRISE      "pm_MaxTempRise"
#define PFD_PM_MINTEMPRISE      "pm_MinTempRise"
#define PFD_PM_MAXCOMBLOW       "pm_MaxCombLow"
#define PFD_PM_MINCOMBLOW       "pm_MinCombLow"
#define PFD_PM_MAXCOMBSUPERLOW  "pm_MaxCombSuperLow"
#define PFD_PM_MINCOMBSUPERLOW  "pm_MinCombSuperLow"
#define PFD_PM_MAXCOMBHIGH      "pm_MaxCombHigh"
#define PFD_PM_MINCOMBHIGH      "pm_MinCombHigh"
#define PFD_PM_MAXCOALHIGH      "pm_MaxCoalHigh"
#define PFD_PM_MINCOALHIGH      "pm_MinCoalHigh"
#define PFD_PM_MAXCOALLOW       "pm_MaxCoalLow"
#define PFD_PM_MINCOALLOW       "pm_MinCoalLow"

// SeconDARY MOTOR
#define PFD_SM_MAXWAITING       "sm_MaxWaiting"
#define PFD_SM_MINWAITING       "sm_MinWaiting"
#define PFD_SM_MAXRELOAD        "sm_MaxReload"
#define PFD_SM_MINRELOAD        "sm_MinReload"
#define PFD_SM_MAXTEMPRISE      "sm_MaxTempRise"
#define PFD_SM_MINTEMPRISE      "sm_MinTempRise"
#define PFD_SM_MAXCOMBLOW       "sm_MaxCombLow"
#define PFD_SM_MINCOMBLOW       "sm_MinCombLow"
#define PFD_SM_MAXCOMBSUPERLOW  "sm_MaxCombSuperLow"
#define PFD_SM_MINCOMBSUPERLOW  "sm_MinCombSuperLow"
#define PFD_SM_MAXCOMBHIGH      "sm_MaxCombHigh"
#define PFD_SM_MINCOMBHIGH      "sm_MinCombHigh"
#define PFD_SM_MAXCOALHIGH      "sm_MaxCoalHigh"
#define PFD_SM_MINCOALHIGH      "sm_MinCoalHigh"
#define PFD_SM_MAXCOALLOW       "sm_MaxCoalLow"
#define PFD_SM_MINCOALLOW       "sm_MinCoalLow"

// Grill MOTOR
#define PFD_GM_MAXWAITING       "gm_MaxWaiting"
#define PFD_GM_MINWAITING       "gm_MinWaiting"
#define PFD_GM_MAXRELOAD        "gm_MaxReload"
#define PFD_GM_MINRELOAD        "gm_MinReload"
#define PFD_GM_MAXTEMPRISE      "gm_MaxTempRise"
#define PFD_GM_MINTEMPRISE      "gm_MinTempRise"
#define PFD_GM_MAXCOMBLOW       "gm_MaxCombLow"
#define PFD_GM_MINCOMBLOW       "gm_MinCombLow"
#define PFD_GM_MAXCOMBSUPERLOW  "gm_MaxCombSuperLow"
#define PFD_GM_MINCOMBSUPERLOW  "gm_MinCombSuperLow"
#define PFD_GM_MAXCOMBHIGH      "gm_MaxCombHigh"
#define PFD_GM_MINCOMBHIGH      "gm_MinCombHigh"
#define PFD_GM_MAXCOALHIGH      "gm_MaxCoalHigh"
#define PFD_GM_MINCOALHIGH      "gm_MinCoalHigh"
#define PFD_GM_MAXCOALLOW       "gm_MaxCoalLow"
#define PFD_GM_MINCOALLOW       "gm_MinCoalLow"

extern PFL_SHandle PARAMFILE_g_sHandle;

void PARAMFILE_Init();

uint32_t PARAMFILE_GetParamEntryCount();

const PFL_SParameterItem* PARAMFILE_GetParamEntryByIndex(uint32_t u32Index);

const PF_CombTempParam_t* PB_GetTemperatureParam();

const PF_MotorOpeningsParam_t* PB_GetPrimaryMotorParam();

const PF_MotorOpeningsParam_t* PB_GetSecondaryMotorParam();

const PF_MotorOpeningsParam_t* PB_GetGrillMotorParam();

const PF_UsrParam* PB_GetParticlesParam();

#endif

