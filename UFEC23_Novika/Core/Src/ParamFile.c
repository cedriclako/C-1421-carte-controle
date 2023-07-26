/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#include "ParamFile.h"
#include "Algo.h"

static PF_StateParam_t m_sWaitingParams;
static PF_StateParam_t m_sReloadParams;
static PF_StateParam_t m_sTRiseParams;
static PF_StateParam_t m_sCombLowParams;
static PF_StateParam_t m_sCombHighParams;
static PF_StateParam_t m_sCoalLowParams;
static PF_StateParam_t m_sCoalHighParams;

static PF_OverHeat_Thresholds_t m_sOverheatParams = {0x00};

static PF_UsrParam m_sMemBlock = {0x00};

static const PFL_SParameterItem m_sParameterItems[] =
{
	// KEY										    VARIABLE POINTER										DEFAULT, MIN, MAX
	PFL_INIT_SINT32(PFD_MANUALBOOL, 			"", &m_sMemBlock.s32ManualOverride, 		                0, 0, 1),
	PFL_INIT_SINT32(PFD_MANUALPRIM, 			"", &m_sMemBlock.s32ManualPrimary, 			                50, 0, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_MANUALSEC,		 		"", &m_sMemBlock.s32ManualSecondary, 		                50, 0, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_MANUALGRILL, 			"", &m_sMemBlock.s32ManualGrill, 		    	            50, 0, PF_GRILL_FULL_OPEN),

	// Waiting parameters
	PFL_INIT_SINT32(PFD_WA_T_TARGET,    	    "", &m_sWaitingParams.sTemperature.fTarget, 				1500, 0, 20000),
	PFL_INIT_SINT32(PFD_WA_PM_POS,    	    	"", &m_sWaitingParams.sPrimary.i32Max, 						0, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_WA_SM_POS,    	    	"", &m_sWaitingParams.sSecondary.i32Max, 					0, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_WA_GM_POS,    	    	"", &m_sWaitingParams.sGrill.i32Max, 						0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Reload parameters
	PFL_INIT_SINT32(PFD_REL_T_TARGET,    	    "", &m_sReloadParams.sTemperature.fTarget, 					5250, 0, 20000),
	PFL_INIT_SINT32(PFD_REL_PM_POS,    	    	"", &m_sReloadParams.sPrimary.i32Max, 						97, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_REL_SM_POS,    	    	"", &m_sReloadParams.sSecondary.i32Max, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_REL_GM_POS,    	    	"", &m_sReloadParams.sGrill.i32Max, 						97, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// TempRise parameters
	PFL_INIT_SINT32(PFD_TR_T_TARGETH, 			"", &m_sTRiseParams.sTemperature.fTarget, 		  			6300, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_T_TARGETL, 			"", &m_sTRiseParams.sTemperature.fAbsMaxDiff, 		  		7100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_T_TOL, 				"", &m_sTRiseParams.sTemperature.fTolerance, 	  			100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_TS_TARGET, 			"", &m_sTRiseParams.sTempSlope.fTarget, 		  			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_TS_TOL, 				"", &m_sTRiseParams.sTempSlope.fTolerance, 					10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_TS_ABS, 				"", &m_sTRiseParams.sTempSlope.fAbsMaxDiff, 				50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_P_TARGET, 			"", &m_sTRiseParams.sParticles.fTarget, 					80, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_P_TOL, 				"", &m_sTRiseParams.sParticles.fTolerance, 					30, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_P_ABS, 				"", &m_sTRiseParams.sParticles.fAbsMaxDiff, 				100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_PS_TOL, 				"", &m_sTRiseParams.sPartStdev.fTolerance, 					10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_PS_ABS, 				"", &m_sTRiseParams.sPartStdev.fAbsMaxDiff, 				50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_ENTRY_TIME, 			"", &m_sTRiseParams.i32EntryWaitTimeSeconds, 				60, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_MIN_TIME, 			"", &m_sTRiseParams.i32MinimumTimeInStateMinutes, 			1, 		0, 	20000),
	PFL_INIT_SINT32(PFD_TR_MAX_TIME, 			"", &m_sTRiseParams.i32MaximumTimeInStateMinutes, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_TR_PM_MAX,    	    	"", &m_sTRiseParams.sPrimary.i32Max, 						85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TR_PM_MIN,    	    	"", &m_sTRiseParams.sPrimary.i32Min, 						17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TR_SM_MAX,    	    	"", &m_sTRiseParams.sSecondary.i32Max, 						97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TR_SM_MIN,    	    	"", &m_sTRiseParams.sSecondary.i32Min, 						97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TR_GM_MAX,    	    	"", &m_sTRiseParams.sGrill.i32Max, 							48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_TR_GM_MIN,    	    	"", &m_sTRiseParams.sGrill.i32Min, 		 					0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Comb low parameters
	PFL_INIT_SINT32(PFD_CBL_T_TARGET, 			"", &m_sCombLowParams.sTemperature.fTarget, 		  		6300, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_T_TOL, 				"", &m_sCombLowParams.sTemperature.fTolerance, 	  			40, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_T_ABS, 				"", &m_sCombLowParams.sTemperature.fAbsMaxDiff, 			100, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_TS_TARGET, 			"", &m_sCombLowParams.sTempSlope.fTarget, 		  			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_TS_TOL, 				"", &m_sCombLowParams.sTempSlope.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_TS_ABS, 				"", &m_sCombLowParams.sTempSlope.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_P_TARGET, 			"", &m_sCombLowParams.sParticles.fTarget, 					80, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_P_TOL, 				"", &m_sCombLowParams.sParticles.fTolerance, 				30, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_P_ABS, 				"", &m_sCombLowParams.sParticles.fAbsMaxDiff, 				100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_PS_TOL, 				"", &m_sCombLowParams.sPartStdev.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_PS_ABS, 				"", &m_sCombLowParams.sPartStdev.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_ENTRY_TIME, 			"", &m_sCombLowParams.i32EntryWaitTimeSeconds, 			60, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_MIN_TIME, 			"", &m_sCombLowParams.i32MinimumTimeInStateMinutes, 		1, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_MAX_TIME, 			"", &m_sCombLowParams.i32MaximumTimeInStateMinutes, 		10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBL_PM_MAX,    	    	"", &m_sCombLowParams.sPrimary.i32Max, 						85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBL_PM_MIN,    	    	"", &m_sCombLowParams.sPrimary.i32Min, 						17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBL_SM_MAX,    	    	"", &m_sCombLowParams.sSecondary.i32Max, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBL_SM_MIN,    	    	"", &m_sCombLowParams.sSecondary.i32Min, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBL_GM_MAX,    	    	"", &m_sCombLowParams.sGrill.i32Max, 						48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBL_GM_MIN,    	    	"", &m_sCombLowParams.sGrill.i32Min, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Comb high parameters
	PFL_INIT_SINT32(PFD_CBH_T_TARGET, 			"", &m_sCombHighParams.sTemperature.fTarget, 		  		7100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_T_TOL, 				"", &m_sCombHighParams.sTemperature.fTolerance, 	  		0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_T_ABS, 				"", &m_sCombHighParams.sTemperature.fAbsMaxDiff, 			0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_TS_TARGET, 			"", &m_sCombHighParams.sTempSlope.fTarget, 		  			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_TS_TOL, 				"", &m_sCombHighParams.sTempSlope.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_TS_ABS, 				"", &m_sCombHighParams.sTempSlope.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_P_TARGET, 			"", &m_sCombHighParams.sParticles.fTarget, 					80, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_P_TOL, 				"", &m_sCombHighParams.sParticles.fTolerance, 				30, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_P_ABS, 				"", &m_sCombHighParams.sParticles.fAbsMaxDiff, 				100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_PS_TOL, 				"", &m_sCombHighParams.sPartStdev.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_PS_ABS, 				"", &m_sCombHighParams.sPartStdev.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_ENTRY_TIME, 			"", &m_sCombHighParams.i32EntryWaitTimeSeconds, 		60, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_MIN_TIME, 			"", &m_sCombHighParams.i32MinimumTimeInStateMinutes, 		1, 		0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_MAX_TIME, 			"", &m_sCombHighParams.i32MaximumTimeInStateMinutes, 		10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_CBH_PM_MAX,    	    	"", &m_sCombHighParams.sPrimary.i32Max, 					85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBH_PM_MIN,    	    	"", &m_sCombHighParams.sPrimary.i32Min, 					17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBH_SM_MAX,    	    	"", &m_sCombHighParams.sSecondary.i32Max, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBH_SM_MIN,    	    	"", &m_sCombHighParams.sSecondary.i32Min, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBH_GM_MAX,    	    	"", &m_sCombHighParams.sGrill.i32Max, 						48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_CBH_GM_MIN,    	    	"", &m_sCombHighParams.sGrill.i32Min, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Coal low parameters
	PFL_INIT_SINT32(PFD_COL_T_TARGET, 			"", &m_sCoalLowParams.sTemperature.fTarget, 		  		6300, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_T_TOL, 				"", &m_sCoalLowParams.sTemperature.fTolerance, 	  			0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COL_T_ABS, 				"", &m_sCoalLowParams.sTemperature.fAbsMaxDiff, 			0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COL_TS_TARGET, 			"", &m_sCoalLowParams.sTempSlope.fTarget, 		  			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_TS_TOL, 				"", &m_sCoalLowParams.sTempSlope.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_TS_ABS, 				"", &m_sCoalLowParams.sTempSlope.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_P_TARGET, 			"", &m_sCoalLowParams.sParticles.fTarget, 					80, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_P_TOL, 				"", &m_sCoalLowParams.sParticles.fTolerance, 				30, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_P_ABS, 				"", &m_sCoalLowParams.sParticles.fAbsMaxDiff, 				100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_PS_TOL, 				"", &m_sCoalLowParams.sPartStdev.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_PS_ABS, 				"", &m_sCoalLowParams.sPartStdev.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_ENTRY_TIME, 			"", &m_sCoalLowParams.i32EntryWaitTimeSeconds, 			60, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_MIN_TIME, 			"", &m_sCoalLowParams.i32MinimumTimeInStateMinutes, 		1, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COL_MAX_TIME, 			"", &m_sCoalLowParams.i32MaximumTimeInStateMinutes, 		10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COL_PM_MAX,    	    	"", &m_sCoalLowParams.sPrimary.i32Max, 						85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COL_PM_MIN,    	    	"", &m_sCoalLowParams.sPrimary.i32Min, 						17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COL_SM_MAX,    	    	"", &m_sCoalLowParams.sSecondary.i32Max, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COL_SM_MIN,    	    	"", &m_sCoalLowParams.sSecondary.i32Min, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COL_GM_MAX,    	    	"", &m_sCoalLowParams.sGrill.i32Max, 						48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COL_GM_MIN,    	    	"", &m_sCoalLowParams.sGrill.i32Min, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),

	// Coal high parameters
	PFL_INIT_SINT32(PFD_COH_T_TARGET, 			"", &m_sCoalHighParams.sTemperature.fTarget, 		  		6300, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_T_TOL, 				"", &m_sCoalHighParams.sTemperature.fTolerance, 	  		0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COH_T_ABS, 				"", &m_sCoalHighParams.sTemperature.fAbsMaxDiff, 			0, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COH_TS_TARGET, 			"", &m_sCoalHighParams.sTempSlope.fTarget, 		  			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_TS_TOL, 				"", &m_sCoalHighParams.sTempSlope.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_TS_ABS, 				"", &m_sCoalHighParams.sTempSlope.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_P_TARGET, 			"", &m_sCoalHighParams.sParticles.fTarget, 					80, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_P_TOL, 				"", &m_sCoalHighParams.sParticles.fTolerance, 				30, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_P_ABS, 				"", &m_sCoalHighParams.sParticles.fAbsMaxDiff, 				100, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_PS_TOL, 				"", &m_sCoalHighParams.sPartStdev.fTolerance, 			10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_PS_ABS, 				"", &m_sCoalHighParams.sPartStdev.fAbsMaxDiff, 			50, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_ENTRY_TIME, 			"", &m_sCoalHighParams.i32EntryWaitTimeSeconds, 		60, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_MIN_TIME, 			"", &m_sCoalHighParams.i32MinimumTimeInStateMinutes, 		1, 		0, 	20000),
	PFL_INIT_SINT32(PFD_COH_MAX_TIME, 			"", &m_sCoalHighParams.i32MaximumTimeInStateMinutes, 		10, 	0, 	20000),
	PFL_INIT_SINT32(PFD_COH_PM_MAX,    	    	"", &m_sCoalHighParams.sPrimary.i32Max, 					85, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COH_PM_MIN,    	    	"", &m_sCoalHighParams.sPrimary.i32Min, 					17, PF_PRIMARY_MINIMUM_OPENING, PF_PRIMARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COH_SM_MAX,    	    	"", &m_sCoalHighParams.sSecondary.i32Max, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COH_SM_MIN,    	    	"", &m_sCoalHighParams.sSecondary.i32Min, 					97, PF_SECONDARY_MINIMUM_OPENING, PF_SECONDARY_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COH_GM_MAX,    	    	"", &m_sCoalHighParams.sGrill.i32Max, 						48, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),
	PFL_INIT_SINT32(PFD_COH_GM_MIN,    	    	"", &m_sCoalHighParams.sGrill.i32Min, 		 				0, PF_GRILL_MINIMUM_OPENING, PF_GRILL_FULL_OPEN),


	// Overheat parameters
	PFL_INIT_SINT32(PFD_OVERHEATPLENUM, 		"", &m_sOverheatParams.OverheatPlenum, 	  					2200, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATPLENUMEXIT, 	"", &m_sOverheatParams.OverheatPlenumExit,   				2100, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATBAFFLE, 		"", &m_sOverheatParams.OverheatBaffle, 	 	   				14720, 0, 20000),
	PFL_INIT_SINT32(PFD_OVERHEATCHAMBER, 		"", &m_sOverheatParams.OverheatChamber, 	 	   			15000, 0, 20000),


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


const PF_UsrParam* PB_GetUserParam()
{
	return &m_sMemBlock;
}

const PF_OverHeat_Thresholds_t* PB_GetOverheatParams(void)
{
	return &m_sOverheatParams;
}


const PF_StateParam_t *PB_GetWaitingParams(void)
{
	return &m_sWaitingParams;
}
const PF_StateParam_t *PB_GetReloadParams(void)
{
	return &m_sReloadParams;
}
const PF_StateParam_t *PB_GetTRiseParams(void)
{
	return &m_sTRiseParams;
}
const PF_StateParam_t *PB_GetCombLowParams(void)
{
	return &m_sCombLowParams;
}
const PF_StateParam_t *PB_GetCombHighParams(void)
{
	return &m_sCombHighParams;
}
const PF_StateParam_t *PB_GetCoalLowParams(void)
{
	return &m_sCoalLowParams;
}
const PF_StateParam_t *PB_GetCoalHighParams(void)
{
	return &m_sCoalHighParams;
}
