#ifndef AIR_INPUT_H
#define AIR_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  int aperture;
  int setPoint;
  uint32_t secPerStep;
  uint32_t timeRefRampe;
  int maxValue;
  int minValue;
} AirInput;


/* maxVal is the maximum value of the aperture. */
#define AirInput_init(minVal, maxVal) {.aperture = 0, .setPoint = 0, .timeRefRampe = 0, .minValue = minVal, .maxValue = maxVal}

/* Set the aperture bypassing the ramp. */
void AirInput_forceAperture( AirInput * self, int aperture);

bool AirInput_InPosition( AirInput * self);

/* Get the current aperture. */
int AirInput_getAperture( AirInput * self);

/* Get the current setpoint  */
int AirInput_getSetPoint( AirInput * self);

/* Set point of the aperture. The aperture will reach the set point using the rampe. */
void AirInput_setSetPoint( AirInput * self, int setPoint, uint32_t secPerStep);

/* adjustement: +/- a given value added to the set point. */
void AirInput_setAjustement( AirInput * self, int adjustement, uint32_t secPerStep);

/* currentTime in [ms] */
void AirInput_task( AirInput * self, uint32_t currentTime_ms);


#ifdef __cplusplus
}
#endif

#endif
