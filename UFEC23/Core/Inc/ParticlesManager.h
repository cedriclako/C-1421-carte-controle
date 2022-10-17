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
	uint16_t standardDev;
}MeasureParticles_t;


void ParticlesManager(void const * argument);

#endif /* INC_PARTICLESMANAGER_H_ */
