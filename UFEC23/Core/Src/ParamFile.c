/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#include "ParamFile.h"

static PF_CombTempParam_t m_sTemperatureParam =
{
		// THe param file matrix will automatically fill it.
	//UFEC23 - Test du 2022-11-29 NOUVEAU PCB (une seule carte)               //tenth of F
		/*
	.WaitingToIgnition = 1000,
	.IgnitionToTrise = 6530,
	.TriseTargetLow = 8870,
	.TriseTargetHigh = 9050,
	.CombLowTarget = 8000,
	.CombHighTarget = 9320,
	.CombLowtoSuperLow = 7500,
	.FlameLoss = 6000,
	.FlameLossDelta = 1750,
	.CoalCrossOverRearLow = 7500,
	.CoalCrossOverRearHigh = 8500,
	.CoalDeltaTemp = 2500,
	.CoalStoveTemp = 1500,
	.OverheatPlenum = 2200,
	.OverheatPlenumExit = 2100,
	.OverheatBaffle = 15000,
	.OverheatChamber = 15000,*/
};

static PF_MotorOpeningsParam_t m_sPrimaryMotorParam =
{
	// THe param file matrix will automatically fill it.
/*
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
	.MinCoalLow = 0,*/
};

static PF_MotorOpeningsParam_t m_sGrillMotorParam =
{
	// THe param file matrix will automatically fill it.
/*
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
	.MinCoalLow = 24,*/
};

static PF_MotorOpeningsParam_t m_sSecondaryMotorParam =
{
	//Added for current PCB model (parameters must be adjusted by user)
/*
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
	.MinCoalLow = 10,*/
};

static PF_UsrParam m_sMemBlock = { 0xFF }; // Simulate a flash memory
static PF_PartParam m_sPartMemBlock = { 0xFF};

static const PFL_SParameterItem m_sParameterItems[] =
{
	// KEY										    VARIABLE POINTER								DEFAULT, MIN, MAX
	PFL_INIT_SINT32(PFD_MANUALBOOL, 			"", &m_sMemBlock.s32ManualOverride, 		                0, 0, 1),
	PFL_INIT_SINT32(PFD_MANUALPRIM, 			"", &m_sMemBlock.s32ManualPrimary, 			                50, 0, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_MANUALSEC,		 		"", &m_sMemBlock.s32ManualSecondary, 		                50, 0, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_MANUALGRILL, 			"", &m_sMemBlock.s32ManualGrill, 		    	            50, 0, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TSLGAIN, 				"", &m_sMemBlock.s32TLSGain, 				                0, 0, 3),
	PFL_INIT_SINT32(PFD_TSLINT,  				"", &m_sMemBlock.s32TSLINT, 					    	    0, 0, 5),
	PFL_INIT_SINT32(PFD_DACCMD,  				"", &m_sMemBlock.s32DACCMD, 							  	0, 0, 255),
	PFL_INIT_SINT32(PFD_TIMEINTERVAL, 			"", &m_sMemBlock.s32TIMEINTERVAL, 					      	0, 0, 255),
	PFL_INIT_SINT32(PFD_SECPERSTEP, 			"", &m_sMemBlock.s32SEC_PER_STEP, 					       	0, 0, 10),
	PFL_INIT_SINT32(PFD_MAXAPERTURE, 			"", &m_sMemBlock.s32MAX_APERTURE, 					       	0, 0, 100),
	PFL_INIT_SINT32(PFD_MINAPERTURE, 			"", &m_sMemBlock.s32MIN_APERTURE, 					       	0, 0, 100),
	PFL_INIT_SINT32(PFD_APERTUREOFFSET, 		"", &m_sMemBlock.s32APERTURE_OFFSET, 					   	0, 0, 100),
	PFL_INIT_SINT32(PFD_FANKIP,			 		"", &m_sMemBlock.s32FAN_KIP,   	 					   		3500, 0, 20000),
	PFL_INIT_SINT32(PFD_FANKOP,			 	    "", &m_sMemBlock.s32FAN_KOP,		 				   		6500, 0, 20000),


	PFL_INIT_SINT32(PFD_MAJ_CORR,			 	"", &m_sPartMemBlock.s32MAJ_CORR_INTERVAL,		 			10, 0, 100),
	PFL_INIT_SINT32(PFD_TBUF_FLOSS,			 	"", &m_sPartMemBlock.s32TBUF_FLOSS,		 				   	10, 0, 100),
	PFL_INIT_SINT32(PFD_TBUF_OVERHEAT,			"", &m_sPartMemBlock.s32TBUF_OVERHEAT,		 				100, 0, 1000),
	PFL_INIT_SINT32(PFD_TBUF_WORKRANGE,			"", &m_sPartMemBlock.s32TBUF_WORKRANGE,		 				40, 0, 1000),
	PFL_INIT_SINT32(PFD_KIP_PART_RISE,			"", &m_sPartMemBlock.s32T_KIP_RISE,		 				   	500, 0, 1000),
	PFL_INIT_SINT32(PFD_CRIT_THRESHOLD_H,		"", &m_sPartMemBlock.s32CRIT_THRESHOLD_H,		 			40, 0, 100),
	PFL_INIT_SINT32(PFD_CRIT_THRESHOLD_L,		"", &m_sPartMemBlock.s32CRIT_THRESHOLD_L,		 			80, 0, 100),
	PFL_INIT_SINT32(PFD_DIFF_TRESHOLD_L,		"", &m_sPartMemBlock.s32DIFF_TRESHOLD_L,		 			20, 0, 1000),
	PFL_INIT_SINT32(PFD_DIFF_TRESHOLD_H,		"", &m_sPartMemBlock.s32DIFF_TRESHOLD_H,		 			50, 0, 1000),
	PFL_INIT_SINT32(PFD_DT_THRESHOLD_L,			"", &m_sPartMemBlock.s32DT_THRESHOLD_H,		 				6, 0, 100),
	PFL_INIT_SINT32(PFD_DT_THRESHOLD_H,			"", &m_sPartMemBlock.s32DT_THRESHOLD_L,		 				1, 0, 20000),
	PFL_INIT_SINT32(PFD_DT_RISE,				"", &m_sPartMemBlock.s32DT_Rise,			 				10, 0, 20),

	// Temperature parameters
	PFL_INIT_SINT32(PFD_WAITINGTOIGNITION, 		"", &m_sTemperatureParam.WaitingToIgnition, 	  			1500, 0, 20000),
	PFL_INIT_SINT32(PFD_IGNITIONTOTRISE, 		"", &m_sTemperatureParam.IgnitionToTrise, 	  				5250, 0, 20000),
	PFL_INIT_SINT32(PFD_TRISETARGETLOW, 		"", &m_sTemperatureParam.TriseTargetLow, 	  				6300, 0, 20000),
	PFL_INIT_SINT32(PFD_TRISETARGETHIGH, 		"", &m_sTemperatureParam.TriseTargetHigh, 	  				7100, 0, 20000),
	PFL_INIT_SINT32(PFD_COMBLOWTARGET, 			"", &m_sTemperatureParam.CombLowTarget, 		  			6300, 0, 20000),
	PFL_INIT_SINT32(PFD_COMBLOWTOSUPERLOW, 		"", &m_sTemperatureParam.CombLowtoSuperLow, 	  			5500, 0, 20000),
	PFL_INIT_SINT32(PFD_COMBHIGHTARGET, 		"", &m_sTemperatureParam.CombHighTarget, 	  				7100, 0, 20000),
	PFL_INIT_SINT32(PFD_COALCROSSOVERREARLOW, 	"", &m_sTemperatureParam.CoalCrossOverRearLow, 				8000, 0, 20000),
	PFL_INIT_SINT32(PFD_COALCROSSOVERREARHIGH, 	"", &m_sTemperatureParam.CoalCrossOverRearHigh,				8000, 0, 20000),
	PFL_INIT_SINT32(PFD_COALDELTATEMP, 			"", &m_sTemperatureParam.CoalDeltaTemp, 		  			2500, 0, 20000),
	PFL_INIT_SINT32(PFD_COALSTOVETEMP, 			"", &m_sTemperatureParam.CoalStoveTemp, 		    		6500, 0, 20000),
	// FlameLoss
	PFL_INIT_SINT32(PFD_FLAMELOSS, 				"", &m_sTemperatureParam.FlameLoss, 			  			5500, 0, 20000),
	PFL_INIT_SINT32(PFD_FLAMELOSSDELTA, 		"", &m_sTemperatureParam.FlameLossDelta, 	  				1750, 0, 20000),
	// OverHeat
	PFL_INIT_SINT32(PFD_OVERHEATPLENUM, 		"", &m_sTemperatureParam.OverheatPlenum, 	  				2200, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATPLENUMEXIT, 	"", &m_sTemperatureParam.OverheatPlenumExit,   				2100, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATBAFFLE, 		"", &m_sTemperatureParam.OverheatBaffle, 	 	   			14720, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATCHAMBER, 		"", &m_sTemperatureParam.OverheatChamber, 	 	   			15000, 0, 20000),

	// Waiting parameters
	PFL_INIT_SINT32(PFD_PM_MAXWAITING,     	    "", &m_sPrimaryMotorParam.MaxWaiting, 		 				0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINWAITING,     	    "", &m_sPrimaryMotorParam.MinWaiting, 		 				0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXWAITING,     	    "", &m_sSecondaryMotorParam.MaxWaiting, 		 			0, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINWAITING,     	    "", &m_sSecondaryMotorParam.MinWaiting, 		 			0, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXWAITING,     	    "", &m_sGrillMotorParam.MaxWaiting, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINWAITING,     	    "", &m_sGrillMotorParam.MinWaiting, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Reload parameters
	PFL_INIT_SINT32(PFD_PM_MAXRELOAD,      	    "", &m_sPrimaryMotorParam.MaxReload, 						97, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINRELOAD,      	    "", &m_sPrimaryMotorParam.MinReload, 						58, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXRELOAD,      	    "", &m_sSecondaryMotorParam.MaxReload, 						97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINRELOAD,      	    "", &m_sSecondaryMotorParam.MinReload, 						97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXRELOAD,      	    "", &m_sGrillMotorParam.MaxReload, 							97, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINRELOAD,      	    "", &m_sGrillMotorParam.MinReload, 			 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// TempRise parameters
	PFL_INIT_SINT32(PFD_PM_MAXTEMPRISE,    	    "", &m_sPrimaryMotorParam.MaxTempRise, 						85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINTEMPRISE,    	    "", &m_sPrimaryMotorParam.MinTempRise, 						17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXTEMPRISE,    	    "", &m_sSecondaryMotorParam.MaxTempRise, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINTEMPRISE,    	    "", &m_sSecondaryMotorParam.MinTempRise, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXTEMPRISE,    	    "", &m_sGrillMotorParam.MaxTempRise, 						48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINTEMPRISE,    	    "", &m_sGrillMotorParam.MinTempRise, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// CombLow parameters
	PFL_INIT_SINT32(PFD_PM_MAXCOMBLOW,     	    "", &m_sPrimaryMotorParam.MaxCombLow, 		 				48, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINCOMBLOW,     	    "", &m_sPrimaryMotorParam.MinCombLow, 		 				6, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBLOW,     	    "", &m_sSecondaryMotorParam.MaxCombLow, 	 				97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINCOMBLOW,     	    "", &m_sSecondaryMotorParam.MinCombLow, 	 				97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBLOW,     	    "", &m_sGrillMotorParam.MaxCombLow, 		 				20, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINCOMBLOW,     	    "", &m_sGrillMotorParam.MinCombLow, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// CombSuperLow parameters
	PFL_INIT_SINT32(PFD_PM_MAXCOMBSUPERLOW,	    "", &m_sPrimaryMotorParam.MaxCombSuperLow, 					25, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINCOMBSUPERLOW,	    "", &m_sPrimaryMotorParam.MinCombSuperLow, 	 				0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBSUPERLOW,	    "", &m_sSecondaryMotorParam.MaxCombSuperLow,				97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINCOMBSUPERLOW,	    "", &m_sSecondaryMotorParam.MinCombSuperLow, 				97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBSUPERLOW,	    "", &m_sGrillMotorParam.MaxCombSuperLow,  	 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINCOMBSUPERLOW,	    "", &m_sGrillMotorParam.MinCombSuperLow,  	 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// CombHigh parameters
	PFL_INIT_SINT32(PFD_PM_MAXCOMBHIGH,    	    "", &m_sPrimaryMotorParam.MaxCombHigh, 						70, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINCOMBHIGH,    	    "", &m_sPrimaryMotorParam.MinCombHigh, 						14, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBHIGH,    	    "", &m_sSecondaryMotorParam.MaxCombHigh, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINCOMBHIGH,    	    "", &m_sSecondaryMotorParam.MinCombHigh, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBHIGH,    	    "", &m_sGrillMotorParam.MaxCombHigh, 	 	 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINCOMBHIGH,    	    "", &m_sGrillMotorParam.MinCombHigh, 	 	 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// CoalHigh parameters
	PFL_INIT_SINT32(PFD_PM_MAXCOALHIGH,    	    "", &m_sPrimaryMotorParam.MaxCoalHigh, 		 				0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINCOALHIGH,    	    "", &m_sPrimaryMotorParam.MinCoalHigh, 		 				0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXCOALHIGH,    	    "", &m_sSecondaryMotorParam.MaxCoalHigh, 					50, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINCOALHIGH,    	    "", &m_sSecondaryMotorParam.MinCoalHigh, 					50, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXCOALHIGH,    	    "", &m_sGrillMotorParam.MaxCoalHigh, 						97, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINCOALHIGH,    	    "", &m_sGrillMotorParam.MinCoalHigh, 						97, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// CoalLow parameters
	PFL_INIT_SINT32(PFD_PM_MAXCOALLOW,     	    "", &m_sPrimaryMotorParam.MaxCoalLow, 		 				5, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_PM_MINCOALLOW,     	    "", &m_sPrimaryMotorParam.MinCoalLow, 		 				5, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MAXCOALLOW,     	    "", &m_sSecondaryMotorParam.MaxCoalLow, 					11, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_SM_MINCOALLOW,     	    "", &m_sSecondaryMotorParam.MinCoalLow, 					11, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MAXCOALLOW,     	    "", &m_sGrillMotorParam.MaxCoalLow, 						24, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_GM_MINCOALLOW,     	    "", &m_sGrillMotorParam.MinCoalLow, 						24, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
};

#define PARAMETERITEM_COUNT ( sizeof(m_sParameterItems) / sizeof(m_sParameterItems[0]) )

static void LoadAllCallback(const PFL_SHandle* psHandle);
static void CommitAllCallback(const PFL_SHandle* psHandle);

PFL_SHandle PARAMFILE_g_sHandle;
const PFL_SConfig m_sConfig = { .ptrLoadAll = LoadAllCallback, .ptrCommitAll = CommitAllCallback };

void PARAMFILE_Init()
{
	PFL_Init(&PARAMFILE_g_sHandle,  m_sParameterItems, PARAMETERITEM_COUNT, &m_sConfig);
	//PFL_LoadAll(&PARAMFILE_g_sHandle);
	PFL_LoadAllDefault(&PARAMFILE_g_sHandle);
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

static void LoadAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash reading is not yet implemented
}

static void CommitAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash writing is not yet implemented
}


const PF_CombTempParam_t* PB_GetTemperatureParam()
{
	return &m_sTemperatureParam;
}

const PF_MotorOpeningsParam_t* PB_GetPrimaryMotorParam()
{
	return &m_sPrimaryMotorParam;
}

const PF_MotorOpeningsParam_t* PB_GetSecondaryMotorParam()
{
	return &m_sSecondaryMotorParam;
}

const PF_MotorOpeningsParam_t* PB_GetGrillMotorParam()
{
	return &m_sGrillMotorParam;
}

const PF_UsrParam* PB_GetUserParam()
{
	return &m_sMemBlock;
}

const PF_PartParam* PB_GetParticlesParam()
{
	return &m_sPartMemBlock;
}
