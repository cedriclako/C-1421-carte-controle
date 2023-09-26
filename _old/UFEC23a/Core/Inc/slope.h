#ifndef SLOPE_H
#define SLOPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  int * dataStore;
  unsigned int nbDataMax;
  unsigned int nbDataInDataStore;
  unsigned int dataIndex;
  float samplingRate;
} Slope;

/* With samplingRate in [s] */
void Slope_init(Slope * slope, int dataStore[], unsigned int nbDataMax, float samplingRate);

/* Add data for any kind of unit. the slope will be computed in [unit value / s]
   according to the configured samplingRate. */
void Slope_addData(Slope * slope, int data);

/* Compute and returns the slope in [value unit / s] by considering only the given nbData. */
float Slope_compute(Slope * slope, unsigned int nbData);

#ifdef __cplusplus
}
#endif

#endif
