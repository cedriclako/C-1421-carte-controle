/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */

#ifndef SRC_PARAMFILE_C_
#define SRC_PARAMFILE_C_

#include "ParamFile.h"

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
	{ .szKey = PFD_TSLGAIN, 		.vdVar = &m_sMemBlock.s32TLSGain, .szDesc = "Gain du module de detection lumnineuse", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 3 } } },
	{ .szKey = PFD_TSLINT, 			.vdVar = &m_sMemBlock.s32TSLINT, .szDesc = "Temps d'integration du module de detection lumineuse", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 5 } } },
	{ .szKey = PFD_DACCMD, 			.vdVar = &m_sMemBlock.s32DACCMD, .szDesc = "Commande de DAC qui gere le courant des DELs", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 255 } } },
	{ .szKey = PFD_TIMEINTERVAL, 	.vdVar = &m_sMemBlock.s32TIMEINTERVAL, .szDesc = "Intervale entre mesure lumiere (sec)", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 255 } } },
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

static void LoadAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash reading is not yet implemented
}

static void CommitAllCallback(const PFL_SHandle* psHandle)
{
	// TODO: Flash writing is not yet implemented
}


#endif /* SRC_PARAMETERFILEDEF_C_ */
