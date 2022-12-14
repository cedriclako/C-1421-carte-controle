/*
 * ParameterFileDef.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#ifndef _PARAMETERFILEDEF_H_
#define _PARAMETERFILEDEF_H_

#include "../Libs/ParameterFileLib.h"

#define PFD_TSLGAIN 		"TSLgain"
#define PFD_TSLINT 			"TSLint"
#define PFD_DACCMD 			"DACcmd"
#define PFD_TIMEINTERVAL 	"TimeInterval"

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

#endif

