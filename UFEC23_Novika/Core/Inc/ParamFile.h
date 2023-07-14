/*
 * ParameterFileDef.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#ifndef _PARAMETERFILEDEF_H_
#define _PARAMETERFILEDEF_H_

#include "ParameterFileLib.h"

#define PF_PRIMARY_MINIMUM_OPENING				6 //5.4 degres @ 0.9 deg/pas
#define PF_PRIMARY_CLOSED  						PF_PRIMARY_MINIMUM_OPENING
#define PF_PRIMARY_FULL_OPEN			   		97

#define PF_SECONDARY_MINIMUM_OPENING			6
#define PF_SECONDARY_CLOSED						PF_SECONDARY_MINIMUM_OPENING
#define PF_SECONDARY_FULL_OPEN				    97

#define PF_GRILL_CLOSED                   		0
#define PF_GRILL_MINIMUM_OPENING 				PF_GRILL_CLOSED
#define PF_GRILL_FULL_OPEN                      97

typedef struct
{

	int32_t s32SEC_PER_STEP;
	int32_t s32APERTURE_OFFSET;
	int32_t s32FAN_KIP;
	int32_t s32FAN_KOP;
	int32_t s32ManualOverride;
	int32_t s32ManualPrimary;
	int32_t s32ManualSecondary;
	int32_t s32ManualGrill;

	int32_t s32VslowOpen; // Seconds per step corresponding to all speeds of opening/closing
	int32_t s32SlowOpen;
	int32_t s32AvgOpen;
	int32_t s32FastOpen;

	int32_t s32VslowClose;
	int32_t s32SlowClose;
	int32_t s32AvgClose;
	int32_t s32FastClose;
	int32_t s32StateEntryWaitTime;


} PF_UsrParam;

typedef struct
{
	int32_t s32MAJ_CORR_INTERVAL;
	int32_t s32FAST_CORR_INTERVAL;
	int32_t s32AVG_CORR_INTERVAL;
	int32_t s32SLOW_CORR_INTERVAL;
	int32_t s32VERY_SLOW_CORR_INTERVAL;


	int32_t s32TBUF_FLOSS;
	int32_t s32TBUF_OVERHEAT;
	int32_t s32TBUF_WORKRANGE;
	int32_t s32CRIT_THRESHOLD_H;
	int32_t s32CRIT_THRESHOLD_L;
	int32_t s32DIFF_TRESHOLD_L;
	int32_t s32DIFF_TRESHOLD_H;
	int32_t s32DT_THRESHOLD_L;
	int32_t s32DT_THRESHOLD_H;
	int32_t s32T_KIP_RISE;
	int32_t s32DT_RISE;
	int32_t s32TLSGAIN;
	int32_t s32TSLINT;
	int32_t s32DACCMD;
	int32_t s32TIMEINTERVAL;

}PF_PartParam;

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


#define PFD_SECPERSTEP			"usr_SecPerStepOffset"
#define PFD_APERTUREOFFSET		"usr_ApertureOffset"
#define PFD_FANKIP				"usr_FansKickInPoint"
#define PFD_FANKOP				"usr_FansKickOutPoint"
#define PFD_MANUALBOOL			"usr_ManualOverride"
#define PFD_MANUALPRIM			"usr_PrimaryOverrideValue"
#define PFD_MANUALSEC			"usr_SecondaryOverrideValue"
#define PFD_MANUALGRILL			"usr_GrillOverrideValue"
#define PFD_OPEN_VSLOW			"usr_SecPerStepVerySlowOpen"
#define PFD_OPEN_SLOW			"usr_SecPerStepSlowOpen"
#define PFD_OPEN_AVG			"usr_SecPerStepAverageOpen"
#define PFD_OPEN_FAST			"usr_SecPerStepFastOpen"
#define PFD_CLOSE_VSLOW			"usr_SecPerStepVerySlowClose"
#define PFD_CLOSE_SLOW			"usr_SecPerStepSlowClose"
#define PFD_CLOSE_AVG			"usr_SecPerStepAverageClose"
#define PFD_CLOSE_FAST			"usr_SecPerStepFastClose"
#define PFD_STATE_ENTRY_WAIT	"usr_MinutesToWait_StateEntry"


#define PFD_MAJ_CORR			"part_secsToWaitAfterMajCorr"
#define PFD_FAST_CORR			"part_secsToWaitAfterFastCorr"
#define PFD_AVG_CORR			"part_secsToWaitAfterAvgCorr"
#define PFD_SLOW_CORR			"part_secsToWaitAfterSlowCorr"
#define PFD_TBUF_FLOSS			"part_buffer_TempFlameLoss"
#define PFD_TBUF_OVERHEAT		"part_buffer_TempOverHeat"
#define PFD_TBUF_WORKRANGE		"part_buffer_TempWorkingRange"
#define PFD_KIP_PART_RISE		"part_TempKickInPointTempRise"
#define PFD_CRIT_THRESHOLD_H	"part_crit_HigherThresh"
#define PFD_CRIT_THRESHOLD_L	"part_crit_LowerThresh"
#define PFD_DIFF_TRESHOLD_L		"part_diff_LowerThresh"
#define PFD_DIFF_TRESHOLD_H		"part_diff_HigherThresh"
#define PFD_DT_THRESHOLD_L		"part_deltaTemp_LowerThresh"
#define PFD_DT_THRESHOLD_H		"part_deltaTemp_HigherThresh"
#define PFD_DT_RISE				"part_deltaTemp_TempRise"
#define PFD_TSLGAIN 			"part_TSLgain"
#define PFD_TSLINT 				"part_TSLint"
#define PFD_DACCMD 				"part_DACcmd"
#define PFD_TIMEINTERVAL 		"part_TimeInterval"

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

const PF_UsrParam* PB_GetUserParam();

const PF_PartParam* PB_GetParticlesParam();

#endif

