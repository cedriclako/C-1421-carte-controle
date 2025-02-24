/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 *
 *
 *
 *      The default values of the parameters are set here but they can be modified "on the fly" via the web page that is accessible first
 *      via the adress 192.168.4.1 at the wifi network broadcasted by the stove with ssid similar to SBI_IoT_SVR_XXXXX and then on the local network
 *      after the connection informations are entered.
 *
 *      The main parameters are temperature limits, targets and tolerances relative to their respective states.
 *      There are also aperture speeds, fan speeds and min and max times in state
 *
 *
 *
 *
 *
 */
#include <string.h>
#include "ParamFile.h"
#include "ParameterFileLib.h"
#include "Algo.h"
#include "FlashMap.h"

// Could be literally anything except 0 or FFFFFFFF
#define PF_MAGIC_MASK (0x55555555)

static PF_SuperStateParam_t m_sSuperParams[ALGO_NB_OF_STATE] = {0};

static PF_RemoteParams_t m_sRemoteParams = {0};
static PF_WaitingParam_t m_sWaitingParams = {0};
static PF_ReloadParam_t m_sReloadParams = {0};
static PF_TriseParam_t m_sTriseParams = {0};
static PF_CombustionParam_t m_sCombLowParams = {0};
static PF_CombustionParam_t m_sCombHighParams = {0};
static PF_CoalParam_t m_sCoalLowParams = {0};
static PF_CoalParam_t m_sCoalHighParams = {0};

static PF_OverHeat_Thresholds_t m_sOverheatParams = {0x00};

static PF_UsrParam m_sMemBlock = {0x00};

static PF_StepperStepsPerSec_t m_sSpeedParams = {0x00};

static const PFL_SParameterItem m_sParameterItems[] =
{
	// KEY										    VARIABLE POINTER										DEFAULT, 	MIN,	 MAX,   DEF
	PFL_INIT_SINT32(PFD_MANUALBOOL, 			 &m_sMemBlock.s32ManualOverride, 		                0, 			0, 		1, "1 = ON,0 = OFF"),
	PFL_INIT_SINT32(PFD_MANUALPRIM, 			 &m_sMemBlock.s32ManualPrimary, 			                50, 		0, 		PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_MANUALSEC,		 		 &m_sMemBlock.s32ManualSecondary, 		                50, 		0,	 	PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_MANUALGRILL, 			 &m_sMemBlock.s32ManualGrill, 		    	            50, 		0, 		PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_ALGO_PERIOD, 			 &m_sMemBlock.s32TimeBetweenComputations_ms, 		    5000, 		0, 		20000, "OFF to LO;Hi to LO"),
	PFL_INIT_SINT32(PFD_FAN_LO_KIP,			 	 &m_sMemBlock.s32FAN_LO_KIP,   	 					   		950, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_FAN_LO_KOP,			 	 &m_sMemBlock.s32FAN_LO_KOP,		 				   		600, 		0, 		20000, "Lo to OFF"),
	PFL_INIT_SINT32(PFD_FAN_HI_KIP,			 	 &m_sMemBlock.s32FAN_HI_KIP,   	 					   		1000, 		0, 		20000, "x to HI"),
	PFL_INIT_SINT32(PFD_BLOW_SPD,			 	 &m_sMemBlock.s32FANL_LOW_SPD,		 				   		43, 		0, 		100, ""),
	PFL_INIT_SINT32(PFD_DIST_SPD,			 	 &m_sMemBlock.s32AFK_LOW_SPD,		 				   		40, 		0, 		100, ""),
	PFL_INIT_SINT32(PFD_PART_RESET,			 	 &m_sMemBlock.s32ParticleReset,		 				   		0, 		0, 		1, "1 = REQ RESET"),

	// KEY										    VARIABLE POINTER										DEFAULT, 	MIN,	 MAX
	// Waiting parameters
	PFL_INIT_SINT32(PFD_WA_T_TARGET,    	     &m_sWaitingParams.fTempToQuitWaiting, 				150, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_WA_T_SKIP,    	         &m_sWaitingParams.fTempToSkipWaiting, 				525, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_WA_PM_POS,    	    	 &m_sWaitingParams.sPrimary.i32Max, 						PF_PRIMARY_MINIMUM_OPENING, 			PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_WA_SM_POS,    	    	 &m_sWaitingParams.sSecondary.i32Max, 					PF_SECONDARY_MINIMUM_OPENING, 			PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_WA_GM_POS,    	    	 &m_sWaitingParams.sGrill.i32Max, 						PF_GRILL_MINIMUM_OPENING, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// Reload parameters
	PFL_INIT_SINT32(PFD_REL_T_TARGET,    	     &m_sReloadParams.fTempToQuitReload, 					350, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_REL_T_SKIP,    	         &m_sReloadParams.fTempToSkipReload, 					530, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_REL_PM_POS,    	    	 &m_sReloadParams.sPrimary.i32Max, 						PF_PRIMARY_FULL_OPEN, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_REL_SM_POS,    	    	 &m_sReloadParams.sSecondary.i32Max, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_REL_GM_POS,    	    	 &m_sReloadParams.sGrill.i32Max, 						PF_GRILL_FULL_OPEN, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// TempRise parameters
	PFL_INIT_SINT32(PFD_TR_T_TARGETH, 			 &m_sTriseParams.fTempToCombHigh, 		  			640, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_T_TARGETL, 			 &m_sTriseParams.fTempToCombLow, 		  			600, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_T_TOL, 				 &m_sTriseParams.fTempToStartReg,				  			425, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_TS_TARGET, 			 &m_sTriseParams.sTempSlope.fTarget, 		  			50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_TS_TOL, 				 &m_sTriseParams.sTempSlope.fTolerance, 					50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_TS_ABS, 				 &m_sTriseParams.sTempSlope.fAbsMaxDiff, 				100, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_P_TARGET, 			 &m_sTriseParams.sParticles.fTarget, 					0, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_P_TOL, 				 &m_sTriseParams.sParticles.fTolerance, 					50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_P_ABS, 				 &m_sTriseParams.sParticles.fAbsMaxDiff, 				100, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_PS_TOL, 				 &m_sTriseParams.sPartStdev.fTolerance, 					12, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_PS_ABS, 				 &m_sTriseParams.sPartStdev.fAbsMaxDiff, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_PM_MAX,    	    	 &m_sTriseParams.sPrimary.i32Max, 						85, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_TR_PM_MIN,    	    	 &m_sTriseParams.sPrimary.i32Min, 						17, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_TR_SM_MAX,    	    	 &m_sTriseParams.sSecondary.i32Max, 						PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_TR_SM_MIN,    	    	 &m_sTriseParams.sSecondary.i32Min, 						PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_TR_GM_MAX,    	    	 &m_sTriseParams.sGrill.i32Max, 							48, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_TR_GM_MIN,    	    	 &m_sTriseParams.sGrill.i32Min, 		 					0, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// KEY										    VARIABLE POINTER										DEFAULT, 	MIN,	 MAX
	// Comb low parameters
	PFL_INIT_SINT32(PFD_CBL_T_TARGET, 			 &m_sCombLowParams.sTemperature.fTarget, 		  		600, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_T_TOL, 				 &m_sCombLowParams.sTemperature.fTolerance, 	  			10, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_T_ABS, 				 &m_sCombLowParams.sTemperature.fAbsMaxDiff, 			40, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_TS_TARGET, 			 &m_sCombLowParams.sTempSlope.fTarget, 		  			0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_TS_TOL, 			 &m_sCombLowParams.sTempSlope.fTolerance, 				10, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_TS_ABS, 			 &m_sCombLowParams.sTempSlope.fAbsMaxDiff, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_P_TARGET, 			 &m_sCombLowParams.sParticles.fTarget, 					0, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_P_TOL, 				 &m_sCombLowParams.sParticles.fTolerance, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_P_ABS, 				 &m_sCombLowParams.sParticles.fAbsMaxDiff, 				100, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_PS_TOL, 			 &m_sCombLowParams.sPartStdev.fTolerance, 				12, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_PS_ABS, 			 &m_sCombLowParams.sPartStdev.fAbsMaxDiff, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_PM_MAX,    	    	 &m_sCombLowParams.sPrimary.i32Max, 						85, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBL_PM_MIN,    	    	 &m_sCombLowParams.sPrimary.i32Min, 						6, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBL_SM_MAX,    	    	 &m_sCombLowParams.sSecondary.i32Max, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBL_SM_MIN,    	    	 &m_sCombLowParams.sSecondary.i32Min, 					65, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""), //tentative on ferme max 65 - pas sur du 35 2024-06-12 GTF
	PFL_INIT_SINT32(PFD_CBL_GM_MAX,    	    	 &m_sCombLowParams.sGrill.i32Max, 						30, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBL_GM_MIN,    	    	 &m_sCombLowParams.sGrill.i32Min, 		 				0, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// Comb high parameters
	PFL_INIT_SINT32(PFD_CBH_T_TARGET, 			 &m_sCombHighParams.sTemperature.fTarget, 		  		650, 		0, 		20000, ""), //2024-06-12 gtf
	PFL_INIT_SINT32(PFD_CBH_T_TOL, 				 &m_sCombHighParams.sTemperature.fTolerance, 	  		10, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_T_ABS, 				 &m_sCombHighParams.sTemperature.fAbsMaxDiff, 			40, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_TS_TARGET, 			 &m_sCombHighParams.sTempSlope.fTarget, 		  			0, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_TS_TOL, 			 &m_sCombHighParams.sTempSlope.fTolerance, 				10, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_TS_ABS, 			 &m_sCombHighParams.sTempSlope.fAbsMaxDiff, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_P_TARGET, 			 &m_sCombHighParams.sParticles.fTarget, 					0, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_P_TOL, 				 &m_sCombHighParams.sParticles.fTolerance, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_P_ABS, 				 &m_sCombHighParams.sParticles.fAbsMaxDiff, 				100, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_PS_TOL, 			 &m_sCombHighParams.sPartStdev.fTolerance, 				12, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_PS_ABS, 			 &m_sCombHighParams.sPartStdev.fAbsMaxDiff, 				50, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_PM_MAX,    	    	 &m_sCombHighParams.sPrimary.i32Max, 					85, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBH_PM_MIN,    	    	 &m_sCombHighParams.sPrimary.i32Min, 					7, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBH_SM_MAX,    	    	 &m_sCombHighParams.sSecondary.i32Max, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBH_SM_MIN,    	    	 &m_sCombHighParams.sSecondary.i32Min, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBH_GM_MAX,    	    	 &m_sCombHighParams.sGrill.i32Max, 						30, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_CBH_GM_MIN,    	    	 &m_sCombHighParams.sGrill.i32Min, 		 				0, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// Coal low parameters
	/* Free Params will contain time to wait before positionning prim and sec */
	PFL_INIT_SINT32(PFD_COL_TIME_P, 			 &m_sCoalLowParams.i32TimeBeforeMovingPrim,	 					1, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_TIME_S, 			 &m_sCoalLowParams.i32TimeBeforeMovingSec,		 				5, 			0, 		20000, ""),

	PFL_INIT_SINT32(PFD_COL_T_TARGET, 			 &m_sCoalLowParams.sTemperature.fTarget, 		  		500, 		0, 		20000, ""),//Changer de 450 à 500 pour que la grille ferme plus tôt en coal 2024-06-12 GTF
	PFL_INIT_SINT32(PFD_COL_T_TOL, 				 &m_sCoalLowParams.sTemperature.fTolerance, 	  			0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_T_ABS, 				 &m_sCoalLowParams.sTemperature.fAbsMaxDiff, 			0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_PS_TOL, 			 &m_sCoalLowParams.sPartStdev.fTolerance, 				12, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_PS_ABS, 			 &m_sCoalLowParams.sPartStdev.fAbsMaxDiff, 				50, 		0, 		20000,""),
	PFL_INIT_SINT32(PFD_COL_P_TARGET, 			 &m_sCoalLowParams.sParticles.fTarget, 					80, 		0, 		20000,""),
	PFL_INIT_SINT32(PFD_COL_P_TOL, 				 &m_sCoalLowParams.sParticles.fTolerance, 				30, 		0, 		20000,""),
	PFL_INIT_SINT32(PFD_COL_P_ABS, 				 &m_sCoalLowParams.sParticles.fAbsMaxDiff, 				100, 		0, 		20000,""),




	PFL_INIT_SINT32(PFD_COL_PM_MAX,    	    	 &m_sCoalLowParams.sPrimary.i32Max, 						85, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COL_PM_MIN,    	    	 &m_sCoalLowParams.sPrimary.i32Min, 						8, 			PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COL_SM_MAX,    	    	 &m_sCoalLowParams.sSecondary.i32Max, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COL_SM_MIN,    	    	 &m_sCoalLowParams.sSecondary.i32Min, 					12, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COL_GM_MAX,    	    	 &m_sCoalLowParams.sGrill.i32Max, 						24, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COL_GM_MIN,    	    	 &m_sCoalLowParams.sGrill.i32Min, 		 				0, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// Coal high parameters
	PFL_INIT_SINT32(PFD_COH_T_TARGET, 			 &m_sCoalHighParams.sTemperature.fTarget, 		  		630, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COH_T_TOL, 				 &m_sCoalHighParams.sTemperature.fTolerance, 	  		0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COH_T_ABS, 				 &m_sCoalHighParams.sTemperature.fAbsMaxDiff, 			0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COH_PM_MAX,    	    	 &m_sCoalHighParams.sPrimary.i32Max, 					85, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COH_PM_MIN,    	    	 &m_sCoalHighParams.sPrimary.i32Min, 					5, 		PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COH_SM_MAX,    	    	 &m_sCoalHighParams.sSecondary.i32Max, 					PF_SECONDARY_FULL_OPEN, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COH_SM_MIN,    	    	 &m_sCoalHighParams.sSecondary.i32Min, 					PF_SECONDARY_MINIMUM_OPENING, 		PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COH_GM_MAX,    	    	 &m_sCoalHighParams.sGrill.i32Max, 						48, 		PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),
	PFL_INIT_SINT32(PFD_COH_GM_MIN,    	    	 &m_sCoalHighParams.sGrill.i32Min, 		 				0, 			PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN, ""),

	// KEY										    VARIABLE POINTER										DEFAULT, 	MIN,	 MAX

	//SuperStateParameters
	PFL_INIT_SINT32(PFD_TR_ENTRY_TIME, 			 &m_sSuperParams[TEMPERATURE_RISE].i32EntryWaitTimeSeconds, 				5, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_MIN_TIME, 			   &m_sSuperParams[TEMPERATURE_RISE].i32MinimumTimeInStateMinutes, 			2, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_TR_MAX_TIME, 			   &m_sSuperParams[TEMPERATURE_RISE].i32MaximumTimeInStateMinutes, 			60, 		0, 		20000, ""),

	// TODO: confirmer lew temps de timeout des états de COMB, peut-être trop bas pour rien. (à faire GTF)
	PFL_INIT_SINT32(PFD_CBL_ENTRY_TIME, 		 &m_sSuperParams[COMBUSTION_LOW].i32EntryWaitTimeSeconds, 				30, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_MIN_TIME, 			 &m_sSuperParams[COMBUSTION_LOW].i32MinimumTimeInStateMinutes, 			30, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBL_MAX_TIME, 			 &m_sSuperParams[COMBUSTION_LOW].i32MaximumTimeInStateMinutes, 			600, 		0, 		20000, ""),

	PFL_INIT_SINT32(PFD_CBH_ENTRY_TIME, 		 &m_sSuperParams[COMBUSTION_HIGH].i32EntryWaitTimeSeconds, 				30, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_MIN_TIME, 			 &m_sSuperParams[COMBUSTION_HIGH].i32MinimumTimeInStateMinutes, 		30, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_CBH_MAX_TIME, 			 &m_sSuperParams[COMBUSTION_HIGH].i32MaximumTimeInStateMinutes, 		600, 		0, 		20000, ""),

	PFL_INIT_SINT32(PFD_COL_ENTRY_TIME, 		 &m_sSuperParams[COAL_LOW].i32EntryWaitTimeSeconds, 					0, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_MIN_TIME, 			 &m_sSuperParams[COAL_LOW].i32MinimumTimeInStateMinutes, 				30, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COL_MAX_TIME, 			 &m_sSuperParams[COAL_LOW].i32MaximumTimeInStateMinutes, 				600, 		0, 		20000, ""),

	PFL_INIT_SINT32(PFD_COH_ENTRY_TIME, 		 &m_sSuperParams[COAL_HIGH].i32EntryWaitTimeSeconds, 					60, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COH_MIN_TIME, 			 &m_sSuperParams[COAL_HIGH].i32MinimumTimeInStateMinutes, 				30, 			0, 		20000, ""),
	PFL_INIT_SINT32(PFD_COH_MAX_TIME, 			 &m_sSuperParams[COAL_HIGH].i32MaximumTimeInStateMinutes, 				600, 		0, 		20000, ""),


	// Overheat parameters
	PFL_INIT_SINT32(PFD_OVERHEATPLENUM, 		 &m_sOverheatParams.OverheatPlenum, 	  					420, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_OVERHEATPLENUMEXIT,  &m_sOverheatParams.OverheatPlenumExit,   					210, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_OVERHEATBAFFLE, 		 &m_sOverheatParams.OverheatBaffle, 	 	   				1300, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_OVERHEATCHAMBER, 		 &m_sOverheatParams.OverheatChamber, 	 	   				1500, 		0, 		20000, ""),


	// KEY										    VARIABLE POINTER										DEFAULT, 	MIN,	 MAX
	// Motor Speed params
	PFL_INIT_SINT32(PFD_SPS_VSLOW, 				 &m_sSpeedParams.fVerySlow, 	  							300, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_SPS_SLOW, 				 &m_sSpeedParams.fSlow, 	  								150, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_SPS_NORMAL, 			 &m_sSpeedParams.fNormal, 	  								100, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_SPS_FAST, 				 &m_sSpeedParams.fFast, 	  								60, 		0, 		20000, ""),
	PFL_INIT_SINT32(PFD_SPS_VFAST, 				 &m_sSpeedParams.fVeryFast, 	  							20, 		0, 		20000, ""),

	PFL_INIT_SINT32_VOLATILE(PFD_RMT_TSTAT, 	 &m_sRemoteParams.bThermostat, 	  						0, 		0, 		1, ""),
	PFL_INIT_SINT32_VOLATILE(PFD_RMT_BOOST, 	 &m_sRemoteParams.bBoostReq, 	  						0, 		0, 		1, ""),
	PFL_INIT_SINT32_VOLATILE(PFD_RMT_LOWFAN,  	 &m_sRemoteParams.i32LowerSpeed, 	  					3, 		0, 		3, ""), //0:OFF, 1:LO, 2: HI, 3: AUTO (HI based on temp)
	PFL_INIT_SINT32_VOLATILE(PFD_RMT_DISTFAN,  	 &m_sRemoteParams.i32DistribSpeed, 	  					3, 		0, 		3, ""),
};

#define PARAMETERITEM_COUNT ( sizeof(m_sParameterItems) / sizeof(m_sParameterItems[0]) )

static void LoadAllCallback(const PFL_SHandle* psHandle);
static void CommitAllCallback(const PFL_SHandle* psHandle);

#ifdef DEBUG
static int32_t GetParameterFileMagicNumber(const PFL_SParameterItem* pItem);
#endif
PFL_SHandle PARAMFILE_g_sHandle;
const PFL_SConfig m_sConfig = { .ptrLoadAll = LoadAllCallback, .ptrCommitAll = CommitAllCallback };

static_assert( (PARAMETERITEM_COUNT * sizeof(int32_t) * 2) < FMAP_PARAMETER_SECTOR_LEN, "Too many parameter to fit into flash memory" );

void PARAMFILE_Init()
{
	PFL_Init(&PARAMFILE_g_sHandle,  m_sParameterItems, PARAMETERITEM_COUNT, &m_sConfig);
}

void PARAMFILE_Load()
{
	//PFL_LoadAll(&PARAMFILE_g_sHandle);
	PFL_LoadAll(&PARAMFILE_g_sHandle);

}

uint32_t PARAMFILE_GetParamEntryCount()
{
	return PARAMFILE_g_sHandle.u32ParameterEntryCount;
}

const PFL_SParameterItem* PARAMFILE_GetParamEntryByIndex(uint32_t u32Index)
{
	if (u32Index >= PARAMETERITEM_COUNT)
		return NULL;
	return &PARAMFILE_g_sHandle.pParameterEntries[u32Index];
}

uint16_t PARAMFILE_GetParamValueByKey(const char* key)
{
	int32_t tempValue;
	PFL_GetValueInt32(&PARAMFILE_g_sHandle, key, &tempValue);


	return (uint16_t) tempValue;
}

void PARAMFILE_SetParamValueByKey(int32_t newValue, const char* szName)
{
    if(PFL_SetValueInt32(&PARAMFILE_g_sHandle, szName, newValue) == PFL_ESETRET_OK)
    {
        PFL_CommitAll(&PARAMFILE_g_sHandle);
    }
}

static void LoadAllCallback(const PFL_SHandle* psHandle)
{
  #ifdef DEBUG
  const uint8_t* pStartAddr = FMAP_GetMemoryAddr(FMAP_EPARTITION_Parameters);
  #endif

	uint32_t u32RelativeAddr = 0;
	for(uint32_t i = 0; i < PARAMFILE_GetParamEntryCount(); i++)
	{
		const PFL_SParameterItem* pItem = PARAMFILE_GetParamEntryByIndex(i);

		if (pItem->eType == PFL_TYPE_Int32)
		{
			int32_t* ps32RAMValue = ((int32_t*)pItem->vdVar);

			*ps32RAMValue = pItem->uType.sInt32.s32Default;

      #ifdef DEBUG
	    const bool bIsVolatile = (pItem->eOpt & PFL_EOPT_IsVolatile) == PFL_EOPT_IsVolatile;
      // If it's allowed to be reloaded from flash, attempt to replace the default value with the good one.
      if (!bIsVolatile)
      {
        const int32_t s32MagicMask = GetParameterFileMagicNumber(pItem);

        const int32_t s32SavedValue = *((int32_t*) (pStartAddr + u32RelativeAddr) );
        const int32_t s32SavedValueInv = *((int32_t*)(pStartAddr + u32RelativeAddr + sizeof(int32_t)));

        // If the magic mask fit, we load the value. If not we just ignore it.
        // the rest of the process will handle it and put it back to the default value.
        if (s32SavedValue == (s32SavedValueInv ^ s32MagicMask))
        {
          *ps32RAMValue = s32SavedValue;
        }
      }
      #endif
			// We need to still increase the address even if we don't write into flash to ensure all settings stay coherent
			u32RelativeAddr += sizeof(int32_t)*2;
		}
	}
}

static void CommitAllCallback(const PFL_SHandle* psHandle)
{
  #ifdef DEBUG
	FMAP_ErasePartition(FMAP_EPARTITION_Parameters);

	uint32_t u32RelativeAddr = 0;
	for(uint32_t i = 0; i < PARAMFILE_GetParamEntryCount(); i++)
	{
		const PFL_SParameterItem* pItem = PARAMFILE_GetParamEntryByIndex(i);
		const bool bIsVolatile = (pItem->eOpt & PFL_EOPT_IsVolatile) == PFL_EOPT_IsVolatile;

		if (!bIsVolatile && pItem->eType == PFL_TYPE_Int32)
		{
			// We save the value twice for more safety.
			// the second save is computed with a MASK to be sure the default erase value (FF)
			// cannot be confused with a real value.
			const int32_t s32MagicMask = GetParameterFileMagicNumber(pItem);

			const int32_t s32Value = *((int32_t*)pItem->vdVar);
			FMAP_WriteAtPartition(FMAP_EPARTITION_Parameters, u32RelativeAddr, (uint8_t*)&s32Value, sizeof(int32_t));
			const int32_t s32ValueInv = *((int32_t*)pItem->vdVar) ^ s32MagicMask;
			FMAP_WriteAtPartition(FMAP_EPARTITION_Parameters, u32RelativeAddr+sizeof(int32_t), (uint8_t*)&s32ValueInv, sizeof(int32_t));
		}

		// We need to still increase the address even if we don't write into flash to ensure all settings stay coherent
		u32RelativeAddr += sizeof(int32_t)*2;
	}
  #endif
}

#ifdef DEBUG
static int32_t GetParameterFileMagicNumber(const PFL_SParameterItem* pItem)
{
	int32_t s32Magic = PF_MAGIC_MASK;
	for(int i = 0; i < strlen(pItem->szKey); i++) {
		s32Magic += (int32_t)pItem->szKey[i];
	}
	s32Magic += (int32_t)pItem->eType;
	for(int i = 0; i < sizeof(pItem->uType.sInt32); i++) {
		s32Magic += ((const uint8_t*)&pItem->uType.sInt32)[i];
	}
	return s32Magic;
}
#endif

const PF_UsrParam* PB_GetUserParam()
{
	return &m_sMemBlock;
}

const PF_OverHeat_Thresholds_t* PB_GetOverheatParams(void)
{
	return &m_sOverheatParams;
}

const PF_WaitingParam_t *PB_GetWaitingParams(void)
{
	return &m_sWaitingParams;
}
const PF_ReloadParam_t *PB_GetReloadParams(void)
{
	return &m_sReloadParams;
}
const PF_TriseParam_t *PB_GetTRiseParams(void)
{
	return &m_sTriseParams;
}
PF_CombustionParam_t *PB_GetCombLowParams(void)
{
	return &m_sCombLowParams;
}
 PF_CombustionParam_t *PB_GetCombHighParams(void)
{
	return &m_sCombHighParams;
}

const PF_CoalParam_t *PB_GetCoalLowParams(void)
{
	return &m_sCoalLowParams;
}

const PF_CoalParam_t *PB_GetCoalHighParams(void)
{
	return &m_sCoalHighParams;
}

const PF_StepperStepsPerSec_t *PB_SpeedParams(void)
{
	return &m_sSpeedParams;
}

const PF_SuperStateParam_t *PB_GetSuperStateParams(uint8_t state)
{
	return &m_sSuperParams[state];
}

const PF_RemoteParams_t *PB_GetRemoteParams(void)
{
	return &m_sRemoteParams;
}
