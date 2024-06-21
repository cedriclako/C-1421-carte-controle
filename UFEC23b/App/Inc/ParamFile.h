/*
 * ParameterFileDef.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#ifndef _PARAMETERFILEDEF_H_
#define _PARAMETERFILEDEF_H_

#include "ParameterFileLib.h"
#include "main.h"
#include "Algo.h"
#define PF_UNUSED (0) // changed to 0, we're managing particulates in coal

#define PF_PRIMARY_MINIMUM_OPENING				6 //5.4 degres @ 0.9 deg/pas
#define PF_PRIMARY_CLOSED  						PF_PRIMARY_MINIMUM_OPENING
#define PF_PRIMARY_FULL_OPEN			   		97
#define PF_PRIMARY_REST_POSITION				PF_PRIMARY_CLOSED

#define PF_SECONDARY_MINIMUM_OPENING			6
#define PF_SECONDARY_CLOSED						PF_SECONDARY_MINIMUM_OPENING
#define PF_SECONDARY_FULL_OPEN				    90

#if NOVIKA_SETUP
#define PF_SECONDARY_REST_POSITION				PF_SECONDARY_CLOSED
#else
#define PF_SECONDARY_REST_POSITION				PF_SECONDARY_FULL_OPEN
#endif


#define PF_GRILL_MINIMUM_OPENING                   		0
#define PF_GRILL_CLOSED 				PF_GRILL_MINIMUM_OPENING
// 85 est la valeur max pour ne pas frapper la vis en tout temps
#define PF_GRILL_FULL_OPEN                      85
#define PF_GRILL_REST_POSITION				PF_GRILL_CLOSED

typedef struct
{

	int32_t s32FAN_LO_KIP;
	int32_t s32FAN_LO_KOP;
	int32_t s32FAN_HI_KIP;
	int32_t s32FAN_HI_KOP;
	int32_t s32AFK_LOW_SPD;
	int32_t s32FANL_LOW_SPD;

	int32_t s32ManualOverride;
	int32_t s32ParticleReset;
	int32_t s32ManualPrimary;
	int32_t s32ManualSecondary;
	int32_t s32ManualGrill;

	int32_t s32TimeBetweenComputations_ms;


} PF_UsrParam;

typedef struct
{

	int32_t fTarget;

	int32_t fTolerance;
	int32_t fAbsMaxDiff;

} PF_BinnedParam_t;

typedef struct
{
	int32_t OverheatPlenum; 	// To handle excessive heat
	int32_t OverheatPlenumExit;
	int32_t OverheatBaffle;
	int32_t OverheatChamber;

}PF_OverHeat_Thresholds_t;

typedef struct
{
	int32_t i32Max;
	int32_t i32Min;
} PF_MotorOpeningsParam_t;

typedef struct
{

	int32_t i32EntryWaitTimeSeconds;
	int32_t i32MinimumTimeInStateMinutes;
	int32_t i32MaximumTimeInStateMinutes;

}PF_SuperStateParam_t;

typedef struct
{

	int32_t fTempToSkipWaiting;
	int32_t fTempToQuitWaiting;

	PF_MotorOpeningsParam_t sPrimary;
	PF_MotorOpeningsParam_t sSecondary;
	PF_MotorOpeningsParam_t sGrill;

}PF_WaitingParam_t;

typedef struct
{

	int32_t fTempToSkipReload;
	int32_t fTempToQuitReload;

	PF_BinnedParam_t sTempSlope;
	PF_BinnedParam_t sParticles;
	PF_BinnedParam_t sPartStdev;

	PF_MotorOpeningsParam_t sPrimary;
	PF_MotorOpeningsParam_t sSecondary;
	PF_MotorOpeningsParam_t sGrill;


}PF_ReloadParam_t;

typedef struct
{

	int32_t fTempToStartReg;
	int32_t fTempToCombLow;
	int32_t fTempToCombHigh;

	PF_BinnedParam_t sTempSlope;
	PF_BinnedParam_t sParticles;
	PF_BinnedParam_t sPartStdev;

	PF_MotorOpeningsParam_t sPrimary;
	PF_MotorOpeningsParam_t sSecondary;
	PF_MotorOpeningsParam_t sGrill;


}PF_TriseParam_t;

typedef struct
{

	PF_BinnedParam_t sTemperature;
	PF_BinnedParam_t sTempSlope;
	PF_BinnedParam_t sParticles;
	PF_BinnedParam_t sPartStdev;

	PF_MotorOpeningsParam_t sPrimary;
	PF_MotorOpeningsParam_t sSecondary;
	PF_MotorOpeningsParam_t sGrill;


}PF_CombustionParam_t;

typedef struct
{

	int32_t i32TimeBeforeMovingPrim;
	int32_t i32TimeBeforeMovingSec;

	PF_BinnedParam_t sTemperature;
	PF_BinnedParam_t sParticles;
	PF_BinnedParam_t sPartStdev;

	PF_MotorOpeningsParam_t sPrimary;
	PF_MotorOpeningsParam_t sSecondary;
	PF_MotorOpeningsParam_t sGrill;


}PF_CoalParam_t;


typedef struct
{
	int32_t fVerySlow;
	int32_t fSlow;
	int32_t fNormal;
	int32_t fFast;
	int32_t fVeryFast;

}PF_StepperStepsPerSec_t;

typedef struct
{
	int32_t bThermostat;
	int32_t bBoostReq;
	int32_t i32LowerSpeed;
	int32_t i32DistribSpeed;

}PF_RemoteParams_t;

#define PFD_RMT_TSTAT			"rmt_TstatReqBool"
#define PFD_RMT_BOOST			"rmt_BoostBool"
#define PFD_RMT_LOWFAN			"rmt_BlowerFanSpeed"
#define PFD_RMT_DISTFAN			"rmt_DistribFanSpeed"


#define PFD_FAN_LO_KIP			"usr_FansLowSpeedKIP"
#define PFD_FAN_LO_KOP			"usr_FansLowSpeedKOP"
#define PFD_FAN_HI_KIP			"usr_FansHighSpeedKIP"
#define PFD_FAN_HI_KOP			"usr_FansHighSpeedKOP"
#define PFD_DIST_SPD				"fan_Dist_LowSpeedPercent"
#define PFD_BLOW_SPD			  "fan_Blow_LowSpeedPercent"

#define PFD_PART_RESET			"usr_ParticleReset"
#define PFD_MANUALBOOL			"usr_ManualOverride"
#define PFD_MANUALPRIM			"usr_PrimaryOverrideValue"
#define PFD_MANUALSEC			"usr_SecondaryOverrideValue"
#define PFD_MANUALGRILL			"usr_GrillOverrideValue"
#define PFD_ALGO_PERIOD			"usr_AlgoComputingPeriodMs"

// Overheat
#define PFD_OVERHEATPLENUM        "temp_OverheatPlenum"
#define PFD_OVERHEATPLENUMEXIT    "temp_OverheatPlenumExit"
#define PFD_OVERHEATBAFFLE        "temp_OverheatBaffle"
#define PFD_OVERHEATCHAMBER       "temp_OverheatChamber"


// State
#define PFD_TR_T_TARGETH         "tRise_TemperatureTargetHigh"
#define PFD_TR_T_TARGETL         "tRise_TemperatureTargetLow"
#define PFD_TR_T_TOL            "tRise_TemperatureToRegulate"
#define PFD_TR_TS_TARGET        "tRise_TempSlopeTarget"
#define PFD_TR_TS_TOL           "tRise_TempSlopeTolerance"
#define PFD_TR_TS_ABS           "tRise_TempSlopeAbsMaxDiff"
#define PFD_TR_P_TARGET		 	"tRise_PartTarget"
#define PFD_TR_P_TOL			"tRise_PartTol"
#define PFD_TR_P_ABS			"tRise_PartAbs"
#define PFD_TR_PS_TOL			"tRise_PartStdevTolerance"
#define PFD_TR_PS_ABS			"tRise_PartStdevAbsMax"
#define PFD_TR_ENTRY_TIME		"tRise_StateEntryDelay"
#define PFD_TR_MIN_TIME			"tRise_MinTimeInState"
#define PFD_TR_MAX_TIME			"tRise_MaxTimeInState"
#define PFD_TR_PM_MAX      		"tRise_pm_Max"
#define PFD_TR_PM_MIN      		"tRise_pm_Min"
#define PFD_TR_SM_MAX      		"tRise_sm_Max"
#define PFD_TR_SM_MIN      		"tRise_sm_Min"
#define PFD_TR_GM_MAX      		"tRise_gm_Max"
#define PFD_TR_GM_MIN      		"tRise_gm_Min"

#define PFD_CBL_T_TARGET         "combL_TemperatureTarget"
#define PFD_CBL_T_TOL            "combL_TemperatureTolerance"
#define PFD_CBL_T_ABS            "combL_TemperatureAbsMaxDiff"
#define PFD_CBL_TS_TARGET        "combL_TempSlopeTarget"
#define PFD_CBL_TS_TOL           "combL_TempSlopeTolerance"
#define PFD_CBL_TS_ABS           "combL_TempSlopeAbsMaxDiff"
#define PFD_CBL_P_TARGET		 "combL_PartTarget"
#define PFD_CBL_P_TOL			 "combL_PartTol"
#define PFD_CBL_P_ABS			 "combL_PartAbs"
#define PFD_CBL_PS_TOL			 "combL_PartStdevTolerance"
#define PFD_CBL_PS_ABS			 "combL_PartStdevAbsMax"
#define PFD_CBL_ENTRY_TIME 	 	 "combL_StateEntryDelay"
#define PFD_CBL_MIN_TIME	 	 "combL_MinTimeInState"
#define PFD_CBL_MAX_TIME		 "combL_MaxTimeInState"
#define PFD_CBL_PM_MAX       	 "combL_pm_Max"
#define PFD_CBL_PM_MIN       	 "combL_pm_Min"
#define PFD_CBL_SM_MAX       	 "combL_sm_Max"
#define PFD_CBL_SM_MIN       	 "combL_sm_Min"
#define PFD_CBL_GM_MAX       	 "combL_gm_Max"
#define PFD_CBL_GM_MIN       	 "combL_gm_Min"

#define PFD_CBH_T_TARGET         "combH_TemperatureTarget"
#define PFD_CBH_T_TOL            "combH_TemperatureTolerance"
#define PFD_CBH_T_ABS            "combH_TemperatureAbsMaxDiff"
#define PFD_CBH_TS_TARGET        "combH_TempSlopeTarget"
#define PFD_CBH_TS_TOL           "combH_TempSlopeTolerance"
#define PFD_CBH_TS_ABS           "combH_TempSlopeAbsMaxDiff"
#define PFD_CBH_P_TARGET		 "combH_PartTarget"
#define PFD_CBH_P_TOL			 "combH_PartTol"
#define PFD_CBH_P_ABS			 "combH_PartAbs"
#define PFD_CBH_PS_TOL			 "combH_PartStdevTolerance"
#define PFD_CBH_PS_ABS			 "combH_PartStdevAbsMax"
#define PFD_CBH_ENTRY_TIME 	 	 "combH_StateEntryDelay"
#define PFD_CBH_MIN_TIME	 	 "combH_MinTimeInState"
#define PFD_CBH_MAX_TIME		 "combH_MaxTimeInState"
#define PFD_CBH_PM_MAX      		"combH_pm_Max"
#define PFD_CBH_PM_MIN      		"combH_pm_Min"
#define PFD_CBH_SM_MAX      		"combH_sm_Max"
#define PFD_CBH_SM_MIN      		"combH_sm_Min"
#define PFD_CBH_GM_MAX      		"combH_gm_Max"
#define PFD_CBH_GM_MIN      		"combH_gm_Min"


#define PFD_COL_TIME_P			 "coalL_TimeBeforeClosingPrim"
#define PFD_COL_TIME_S			 "coalL_TimeBeforeClosingSec"
#define PFD_COL_T_TARGET         "coalL_TemperatureTarget"
#define PFD_COL_T_TOL            "coalL_TemperatureTolerance"
#define PFD_COL_T_ABS            "coalL_TemperatureAbsMaxDiff"
#if !PF_UNUSED
#define PFD_COL_TS_TARGET        "coalL_TempSlopeTarget"
#define PFD_COL_TS_TOL           "coalL_TempSlopeTolerance"
#define PFD_COL_TS_ABS           "coalL_TempSlopeAbsMaxDiff"
#define PFD_COL_P_TARGET		 "coalL_PartTarget"
#define PFD_COL_P_TOL			 "coalL_PartTol"
#define PFD_COL_P_ABS			 "coalL_PartAbs"
#define PFD_COL_PS_ABS			 "coalL_PartStdevAbsMax"
#endif
#define PFD_COL_PS_TOL			 "coalL_PartStdevTolerance"

#define PFD_COL_ENTRY_TIME 	 	 "coalL_StateEntryDelay"
#define PFD_COL_MIN_TIME	 	 "coalL_MinTimeInState"
#define PFD_COL_MAX_TIME		 "coalL_MaxTimeInState"
#define PFD_COL_PM_MAX       		"coalL_pm_Max"
#define PFD_COL_PM_MIN       		"coalL_pm_Min"
#define PFD_COL_SM_MAX       		"coalL_sm_Max"
#define PFD_COL_SM_MIN      		"coalL_sm_Min"
#define PFD_COL_GM_MAX     		 	"coalL_gm_Max"
#define PFD_COL_GM_MIN     		  	"coalL_gm_Min"

#define PFD_COH_T_TARGET         "coalH_TemperatureTarget"
#define PFD_COH_T_TOL           "coalH_TemperatureTolerance"
#define PFD_COH_T_ABS           "coalH_TemperatureAbsMaxDiff"
#if !PF_UNUSED
#define PFD_COH_TS_TARGET        "coalH_TempSlopeTarget"
#define PFD_COH_TS_TOL          "coalH_TempSlopeTolerance"
#define PFD_COH_TS_ABS          "coalH_TempSlopeAbsMaxDiff"
#define PFD_COH_P_TARGET		 "coalH_PartTarget"
#define PFD_COH_P_TOL			 "coalH_PartTol"
#define PFD_COH_P_ABS			 "coalH_PartAbs"
#define PFD_COH_PS_TOL			 "coalH_PartStdevTolerance"
#define PFD_COH_PS_ABS			 "coalH_PartStdevAbsMax"
#endif
#define PFD_COH_ENTRY_TIME 	 	 "coalH_StateEntryDelay"
#define PFD_COH_MIN_TIME	 	 "coalH_MinTimeInState"
#define PFD_COH_MAX_TIME		 "coalH_MaxTimeInState"
#define PFD_COH_PM_MAX      	"coalH_pm_Max"
#define PFD_COH_PM_MIN     	 	"coalH_pm_Min"
#define PFD_COH_SM_MAX      	"coalH_sm_Max"
#define PFD_COH_SM_MIN      	"coalH_sm_Min"
#define PFD_COH_GM_MAX      	"coalH_gm_Max"
#define PFD_COH_GM_MIN      	"coalH_gm_Min"

#define PFD_WA_T_TARGET     	"waiting_TempToIgnition"
#define PFD_WA_T_SKIP     		"waiting_TempToTrise"
#define PFD_WA_PM_POS		    "waiting_pm_pos"
#define PFD_WA_SM_POS		    "waiting_sm_pos"
#define PFD_WA_GM_POS		    "waiting_gm_pos"

#define PFD_REL_T_TARGET       	"reload_TempToTrise"
#define PFD_REL_T_SKIP       	"reload_SkipToTrise"
#define PFD_REL_PM_POS		    "reload_pm_pos"
#define PFD_REL_SM_POS		    "reload_sm_pos"
#define PFD_REL_GM_POS		    "reload_gm_pos"

#define PFD_SPS_VSLOW			"secPerStep_verySlow"
#define PFD_SPS_SLOW			"secPerStep_slow"
#define PFD_SPS_NORMAL			"secPerStep_normal"
#define PFD_SPS_FAST			"secPerStep_fast"
#define PFD_SPS_VFAST			"secPerStep_veryFast"

extern PFL_SHandle PARAMFILE_g_sHandle;

void PARAMFILE_Init();

void PARAMFILE_Load();

uint32_t PARAMFILE_GetParamEntryCount();

const PFL_SParameterItem* PARAMFILE_GetParamEntryByIndex(uint32_t u32Index);

uint16_t PARAMFILE_GetParamValueByKey(const char* key);

void PARAMFILE_SetParamValueByKey(int32_t newValue, const char* szName);

const PF_UsrParam* PB_GetUserParam();

const PF_OverHeat_Thresholds_t* PB_GetOverheatParams(void);

const PF_WaitingParam_t *PB_GetWaitingParams(void);
const PF_ReloadParam_t *PB_GetReloadParams(void);
const PF_TriseParam_t *PB_GetTRiseParams(void);
PF_CombustionParam_t *PB_GetCombLowParams(void);
PF_CombustionParam_t *PB_GetCombHighParams(void);
const PF_CoalParam_t *PB_GetCoalLowParams(void);
const PF_CoalParam_t *PB_GetCoalHighParams(void);

const PF_StepperStepsPerSec_t *PB_SpeedParams(void);

const PF_RemoteParams_t *PB_GetRemoteParams(void);

const PF_SuperStateParam_t *PB_GetSuperStateParams(uint8_t state);

#endif

