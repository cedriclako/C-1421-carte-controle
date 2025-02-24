#include "air_input.h"

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

void AirInput_forceAperture( AirInput * self, int aperture) {
  aperture = constrain(aperture, self->minValue, self->maxValue); //TODO: MIN_VALEUR IS NOT ZERO FOR THE PRIMARY
  self->aperture = aperture;
  self->setPoint = aperture;
}

int AirInput_getAperture( AirInput * self) {
  return self->aperture;
}

int AirInput_getSetPoint( AirInput * self) {
  return self->setPoint;
}

bool AirInput_InPosition( AirInput * self)
{
	return self->aperture == self->setPoint;
}

void AirInput_setAjustement( AirInput * self, int adjustement, float secPerStep) {
  self->setPoint += adjustement;
  self->setPoint = constrain(self->setPoint, self->minValue, self->maxValue);
  self->secPerStep = secPerStep;
}


void AirInput_task( AirInput * self, uint32_t currentTime_ms) {

  if (self->aperture != self->setPoint) {
    if ((currentTime_ms - self->timeRefRampe) >= (self->secPerStep * 1000)) {
      self->timeRefRampe = currentTime_ms;
      if (self->setPoint > self->aperture) {
        self->aperture++;
      } else {
        self->aperture--;
      }
    }
  }
}
