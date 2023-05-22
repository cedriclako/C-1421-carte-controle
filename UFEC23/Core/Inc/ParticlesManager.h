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
	uint16_t time_window;
	uint8_t TSL_gain;
	uint8_t TSL_integration_time;
	uint8_t LED_current_CMD;

	float crit;


}MeasureParticles_t;

uint16_t Particle_getCH0(void);
uint16_t Particle_getCH1(void);
uint16_t Particle_getCH0_OFF(void);
uint16_t Particle_getCH1_OFF(void);
uint16_t Particle_getCurrent(void);
float Particle_getZeroNorm(void);
uint16_t Particle_getTemperature(void);
uint16_t Particle_getVariance(void);
uint16_t Particle_getLuxON(void);
uint16_t Particle_getLuxOFF(void);
uint32_t Particle_getTime(void);
bool PM_isPboard_absent(void);

int Particle_getSlope(void);
void Particle_setConfig(void);
bool computeParticleLowAdjustment(const PF_UsrParam* pParam, float dTavant, int* delta, float* speed, uint32_t Time_ms,
		int32_t baffle_temperature, int32_t temperature_limit);
void computeParticleRiseAdjustment(const PF_UsrParam* pParam, float dTbaffle, int* delta, float* speed, uint32_t Time_ms,
		int32_t baffleTemperature, int32_t temperature_limit);

void Particle_requestZero(void);
void ParticleInit(void);
void ParticlesManager(void const * argument);

#endif /* INC_PARTICLESMANAGER_H_ */
