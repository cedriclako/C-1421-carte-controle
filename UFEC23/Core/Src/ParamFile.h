/*
 * ParameterFileDef.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#ifndef _PARAMETERFILEDEF_H_
#define _PARAMETERFILEDEF_H_

#include "ParameterFileLib.h"

#define PFD_TSLGAIN 		"TSLgain"
#define PFD_TSLINT 			"TSLint"
#define PFD_DACCMD 			"DACcmd"
#define PFD_TIMEINTERVAL 	"TimeInterval"

extern PFL_SHandle PARAMFILE_g_sHandle;

void PARAMFILE_Init();

uint32_t PARAMFILE_GetParamEntryCount();

const PFL_SParameterItem* PARAMFILE_GetParamEntryByIndex(uint32_t u32Index);

#endif

