#include "AirInput.h"

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

void AirInput_forceAperture( AirInput * self, int aperture) {
  aperture = constrain(aperture, self->minValue, self->maxValue); //TODO: MIN_VALEUR IS NOT ZERO FOR THE PRIMARY
  self->aperture = aperture;
  self->setPoint = aperture;
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
