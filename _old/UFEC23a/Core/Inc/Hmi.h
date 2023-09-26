#ifndef HMI_H
#define HMI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//public typedef
typedef enum Furnace{
	HEATMAX,
	CADDY_ADVANCED,
	HEATPACK,
	MINI_CADDY,
	HEATPRO,
	MAX_CADDY,
	NB_OF_MODEL
} FurnaceModel;

void HmiManager();
FurnaceModel readModel();
void setStatusBit(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif //HMI_H
