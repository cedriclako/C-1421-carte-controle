/*
 * ParameterFileDef.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */

#ifndef SRC_PARAMFILE_C_
#define SRC_PARAMFILE_C_

#include "ParamFile.h"

static const PFL_SParameterItem m_sParameterItems[] =
{
	{ .szKey = PFD_TSLGAIN, .szDesc = "Gain du module de detection lumnineuse", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 3 } } },
	{ .szKey = PFD_TSLINT, .szDesc = "Temps d'integration du module de detection lumineuse", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 5 } } },
	{ .szKey = PFD_DACCMD, .szDesc = "Commande de DAC qui gere le courant des DELs", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 255 } } },
	{ .szKey = PFD_TIMEINTERVAL, .szDesc = "Intervale entre mesure lumiere (sec)", .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = 0, .s32Min = 0, .s32Max = 255 } } },
};

#define PARAMETERITEM_COUNT ( sizeof(m_sParameterItems) / sizeof(m_sParameterItems[0]) )

PFL_SHandle PARAMFILE_g_sHandle;

void PARAMFILE_Init()
{
	PFL_Init(&PARAMFILE_g_sHandle,  m_sParameterItems, PARAMETERITEM_COUNT);
}

#endif /* SRC_PARAMETERFILEDEF_C_ */
