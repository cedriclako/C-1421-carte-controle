/*
 * ParticlesManager.h
 *
 *  Created on: 12 oct. 2022
 *      Author: crichard
 */

#ifndef INC_PARTICLESMANAGER_H_
#define INC_PARTICLESMANAGER_H_
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "ParamFile.h"

typedef struct MeasureParticles
{
	uint16_t ch0_ON;
	uint16_t ch0_OFF;
	uint16_t ch1_ON;
	uint16_t ch1_OFF;
	uint16_t particles;
	uint16_t variance;
	uint16_t temperature;
	uint16_t zero;
	float normalized_zero;
	int slope;
	uint16_t LED_current_meas;
	uint16_t Lux_ON;
	uint16_t Lux_OFF;
	uint32_t TimeSinceInit;
	uint32_t last_particle_time;



}MeasureParticles_t;

bool PM_isPboard_absent(void);

void Particle_requestZero(void);
void Particle_IncFireCount(void);
void Particle_setConfig(void);
void Particle_Init(void);
void ParticlesManager(uint32_t u32Time_ms);
const MeasureParticles_t* ParticlesGetObject(void);

#endif /* INC_PARTICLESMANAGER_H_ */
