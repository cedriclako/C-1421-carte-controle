/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */

#ifndef SRC_PARAMFILE_C_
#define SRC_PARAMFILE_C_

#include "ParamFile.h"
#include "ParameterBlock.h"

typedef struct
{
	int32_t s32TLSGain;
	int32_t s32TSLINT;
	int32_t s32DACCMD;
	int32_t s32TIMEINTERVAL;
} SMemBlock;

static SMemBlock m_sMemBlock = { 0xFF }; // Simulate a flash memory

static const PFL_SParameterItem m_sParameterItems[] =
{
	// KEY								DESCRIPTION									VARIABLE POINTER						DEFAULT, MIN, MAX
	PFL_INIT_SINT32(PFD_TSLGAIN, 		"Gain du module de detection lumnineuse", &m_sMemBlock.s32TLSGain, 					0, 0, 3),
	PFL_INIT_SINT32(PFD_TSLINT,  		"Temps d'integration du module de detection lumineuse", &m_sMemBlock.s32TSLINT, 	0, 0, 5),
	PFL_INIT_SINT32(PFD_DACCMD,  		"Commande de DAC qui gere le courant des DELs", &m_sMemBlock.s32DACCMD, 			0, 0, 255),
	PFL_INIT_SINT32(PFD_TIMEINTERVAL, 	"Intervale entre mesure lumiere (sec)", &m_sMemBlock.s32TIMEINTERVAL, 				0, 0, 255),

	// Temperature
	PFL_INIT_SINT32(PFD_WAITINGTOIGNITION, 		"", &PB_g_sTemperatureParam.WaitingToIgnition, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_IGNITIONTOTRISE, 		"", &PB_g_sTemperatureParam.IgnitionToTrise, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_TRISETARGETLOW, 		"", &PB_g_sTemperatureParam.TriseTargetLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_TRISETARGETHIGH, 		"", &PB_g_sTemperatureParam.TriseTargetHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COMBLOWTARGET, 			"", &PB_g_sTemperatureParam.CombLowTarget, 			0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COMBHIGHTARGET, 		"", &PB_g_sTemperatureParam.CombHighTarget, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COMBLOWTOSUPERLOW, 		"", &PB_g_sTemperatureParam.CombLowtoSuperLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_FLAMELOSS, 				"", &PB_g_sTemperatureParam.FlameLoss, 				0, -65535, 65535),
	PFL_INIT_SINT32(PFD_FLAMELOSSDELTA, 		"", &PB_g_sTemperatureParam.FlameLossDelta, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COALCROSSOVERREARLOW, 	"", &PB_g_sTemperatureParam.CoalCrossOverRearLow, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COALCROSSOVERREARHIGH, 	"", &PB_g_sTemperatureParam.CoalCrossOverRearHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COALDELTATEMP, 			"", &PB_g_sTemperatureParam.CoalDeltaTemp, 			0, -65535, 65535),
	PFL_INIT_SINT32(PFD_COALSTOVETEMP, 			"", &PB_g_sTemperatureParam.CoalStoveTemp, 			0, -65535, 65535),
	PFL_INIT_SINT32(PFD_OVERHEATPLENUM, 		"", &PB_g_sTemperatureParam.OverheatPlenum, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_OVERHEATPLENUMEXIT, 	"", &PB_g_sTemperatureParam.OverheatPlenumExit, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_OVERHEATBAFFLE, 		"", &PB_g_sTemperatureParam.OverheatBaffle, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_OVERHEATCHAMBER, 		"", &PB_g_sTemperatureParam.OverheatChamber, 		0, -65535, 65535),

	// Primary motor
	PFL_INIT_SINT32(PFD_PM_MAXWAITING,     	    "", &PB_g_sPrimaryMotorParam.MaxWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINWAITING,     	    "", &PB_g_sPrimaryMotorParam.MinWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXRELOAD,      	    "", &PB_g_sPrimaryMotorParam.MaxReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINRELOAD,      	    "", &PB_g_sPrimaryMotorParam.MinReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXTEMPRISE,    	    "", &PB_g_sPrimaryMotorParam.MaxTempRise, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINTEMPRISE,    	    "", &PB_g_sPrimaryMotorParam.MinTempRise, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXCOMBLOW,     	    "", &PB_g_sPrimaryMotorParam.MaxCombHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINCOMBLOW,     	    "", &PB_g_sPrimaryMotorParam.MinCombLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXCOMBSUPERLOW,	    "", &PB_g_sPrimaryMotorParam.MaxCombSuperLow, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINCOMBSUPERLOW,	    "", &PB_g_sPrimaryMotorParam.MinCombSuperLow, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXCOMBHIGH,    	    "", &PB_g_sPrimaryMotorParam.MaxCombHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINCOMBHIGH,    	    "", &PB_g_sPrimaryMotorParam.MinCombHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXCOALHIGH,    	    "", &PB_g_sPrimaryMotorParam.MaxCoalHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINCOALHIGH,    	    "", &PB_g_sPrimaryMotorParam.MinCoalHigh, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MAXCOALLOW,     	    "", &PB_g_sPrimaryMotorParam.MaxCoalLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_PM_MINCOALLOW,     	    "", &PB_g_sPrimaryMotorParam.MinCoalLow, 		0, -65535, 65535),

	// Secondary motor
	PFL_INIT_SINT32(PFD_SM_MAXWAITING,     	    "", &PB_g_sSecondaryMotorParam.MaxWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINWAITING,     	    "", &PB_g_sSecondaryMotorParam.MinWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXRELOAD,      	    "", &PB_g_sSecondaryMotorParam.MaxReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINRELOAD,      	    "", &PB_g_sSecondaryMotorParam.MinReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXTEMPRISE,    	    "", &PB_g_sSecondaryMotorParam.MaxTempRise, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINTEMPRISE,    	    "", &PB_g_sSecondaryMotorParam.MinTempRise, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBLOW,     	    "", &PB_g_sSecondaryMotorParam.MaxCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINCOMBLOW,     	    "", &PB_g_sSecondaryMotorParam.MinCombLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBSUPERLOW,	    "", &PB_g_sSecondaryMotorParam.MaxCombSuperLow, 0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINCOMBSUPERLOW,	    "", &PB_g_sSecondaryMotorParam.MinCombSuperLow, 0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXCOMBHIGH,    	    "", &PB_g_sSecondaryMotorParam.MaxCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINCOMBHIGH,    	    "", &PB_g_sSecondaryMotorParam.MinCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXCOALHIGH,    	    "", &PB_g_sSecondaryMotorParam.MaxCoalHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINCOALHIGH,    	    "", &PB_g_sSecondaryMotorParam.MinCoalHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MAXCOALLOW,     	    "", &PB_g_sSecondaryMotorParam.MaxCoalLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_SM_MINCOALLOW,     	    "", &PB_g_sSecondaryMotorParam.MinCoalLow, 		0, -65535, 65535),

	// Grill motor
	PFL_INIT_SINT32(PFD_GM_MAXWAITING,     	    "", &PB_g_sGrillMotorParam.MaxWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINWAITING,     	    "", &PB_g_sGrillMotorParam.MinWaiting, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXRELOAD,      	    "", &PB_g_sGrillMotorParam.MaxReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINRELOAD,      	    "", &PB_g_sGrillMotorParam.MinReload, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXTEMPRISE,    	    "", &PB_g_sGrillMotorParam.MaxTempRise, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINTEMPRISE,    	    "", &PB_g_sGrillMotorParam.MinTempRise, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBLOW,     	    "", &PB_g_sGrillMotorParam.MaxCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINCOMBLOW,     	    "", &PB_g_sGrillMotorParam.MinCombLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBSUPERLOW,	    "", &PB_g_sGrillMotorParam.MaxCombSuperLow, 0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINCOMBSUPERLOW,	    "", &PB_g_sGrillMotorParam.MinCombSuperLow, 0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXCOMBHIGH,    	    "", &PB_g_sGrillMotorParam.MaxCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINCOMBHIGH,    	    "", &PB_g_sGrillMotorParam.MinCombHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXCOALHIGH,    	    "", &PB_g_sGrillMotorParam.MaxCoalHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINCOALHIGH,    	    "", &PB_g_sGrillMotorParam.MinCoalHigh, 	0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MAXCOALLOW,     	    "", &PB_g_sGrillMotorParam.MaxCoalLow, 		0, -65535, 65535),
	PFL_INIT_SINT32(PFD_GM_MINCOALLOW,     	    "", &PB_g_sGrillMotorParam.MinCoalLow, 		0, -65535, 65535),
};

#define PARAMETERITEM_COUNT ( sizeof(m_sParameterItems) / sizeof(m_sParameterItems[0]) )

static void LoadAllCallback(const PFL_SHandle* psHandle);
static void CommitAllCallback(const PFL_SHandle* psHandle);

PFL_SHandle PARAMFILE_g_sHandle;
const PFL_SConfig m_sConfig = { .ptrLoadAll = LoadAllCallback, .ptrCommitAll = CommitAllCallback };

void PARAMFILE_Init()
{
	PFL_Init(&PARAMFILE_g_sHandle,  m_sParameterItems, PARAMETERITEM_COUNT, &m_sConfig);
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

static void LoadAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash reading is not yet implemented
}

static void CommitAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash writing is not yet implemented
}

#endif /* SRC_PARAMETERFILEDEF_C_ */
