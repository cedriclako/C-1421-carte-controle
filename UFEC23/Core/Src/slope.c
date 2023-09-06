/*
  Algorithme basé sur:
  https://www.codewithc.com/c-program-for-linear-exponential-curve-fitting/
  https://www.embeddedrelated.com/showcode/323.php
*/

#include "slope.h"

void Slope_init(Slope * slope, int dataStore[], unsigned int nbDataMax, float samplingRate) {

  if (sizeof(uint64_t) != 8) {
    // ERROR: the size of int64_t on this platform is not supported.
    while(true) {;}
  }

  slope->dataStore = dataStore;
  slope->nbDataMax = nbDataMax;
  slope->nbDataInDataStore = 0;
  slope->dataIndex = 0;
  slope->samplingRate = samplingRate;
}

void Slope_addData(Slope * slope, int data) {

  if (slope->nbDataInDataStore < slope->nbDataMax) {
    slope->nbDataInDataStore++;
  }

  slope->dataStore[slope->dataIndex] = data;
  slope->dataIndex++;
  if (slope->dataIndex >= slope->nbDataMax) {
    slope->dataIndex = 0;
  }
}

float Slope_compute(Slope * slope, unsigned int nbData) {

  int64_t nbDataToUse;
  int64_t sumX = 0;
  int64_t sumY = 0;
  int64_t sumXY = 0;
   int64_t sumX2 = 0;
  unsigned int readingIndex;
  float slopeValue;

  if (nbData > slope->nbDataInDataStore) {
    nbDataToUse = slope->nbDataInDataStore;
  } else {
    nbDataToUse = nbData;
  }

  if (slope->dataIndex >= nbDataToUse) {
    readingIndex = slope->dataIndex - nbDataToUse;
  } else {
    readingIndex = slope->nbDataInDataStore - (nbDataToUse - slope->dataIndex);
  }

  for (int32_t i = 1; i < nbDataToUse; i++) {

    sumX += i;
    sumY += slope->dataStore[readingIndex];
    sumXY += (i * slope->dataStore[readingIndex]); // ATTENTION: TODO: trouver un moyen de tester si on fait un overflow
    sumX2 += i * i;

    readingIndex++;
    if (readingIndex >= slope->nbDataInDataStore) {
      readingIndex = 0;
    }
  }

  if ((sumX != 0) || (sumX2 != 0)) {
    slopeValue = (((sumX * sumY) - (nbDataToUse * sumXY)) * 1.0) / (((sumX * sumX) - (nbDataToUse * sumX2)) * 1.0);
  } else {
    slopeValue = 0.0;
  }

  return slopeValue * slope->samplingRate;
}
