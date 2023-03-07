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
	uint16_t normalized_zero;
	int slope;
	uint16_t LED_current_meas;
	uint16_t Lux_ON;
	uint16_t Lux_OFF;
	uint32_t TimeSinceInit;

	uint16_t time_window;
	uint8_t TSL_gain;
	uint8_t TSL_integration_time;
	uint8_t LED_current_CMD;

}MeasureParticles_t;

uint16_t Particle_getCH0(void);
uint16_t Particle_getCH1(void);
uint16_t Particle_getCH0_OFF(void);
uint16_t Particle_getCH1_OFF(void);
uint16_t Particle_getCurrent(void);
uint16_t Particle_getZeroNorm(void);
uint16_t Particle_getTemperature(void);
uint16_t Particle_getVariance(void);
uint16_t Particle_getLuxON(void);
uint16_t Particle_getLuxOFF(void);
uint32_t Particle_getTime(void);

int Particle_getSlope(void);
void Particle_setConfig(void);

void Particle_requestZero(void);
void ParticlesManager(void const * argument);

#endif /* INC_PARTICLESMANAGER_H_ */
