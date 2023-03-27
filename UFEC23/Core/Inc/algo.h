#ifndef ALGO_H
#define ALGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define SAMPLING_RATE  0.2  // Unit: [samples/s]

#define SECONDS(x) (x*1000)
#define MINUTES(x) (SECONDS(60)*x)
#define CELSIUS_TO_FAHRENHEIT(TEMP) (TEMP*9/5+32)
#define FAHRENHEIT_TO_CELSIUS(TEMP) ((TEMP-32)*5/9)

typedef enum {
  ZEROING_STEPPER,
  WAITING,
  RELOAD_IGNITION,
  TEMPERATURE_RISE,
  COMBUSTION_HIGH,
  COMBUSTION_LOW,
  COMBUSTION_SUPERLOW,
  COAL_LOW,
  FLAME_LOSS,
  COAL_HIGH,
  OVERTEMP,
  SAFETY,
  PRODUCTION_TEST,
  MANUAL_CONTROL,
#ifdef DEMO_MODE
  COMBUSTION_HIGH_OPEN,
  COMBUSTION_HIGH_CLOSE,
  COMBUSTION_LOW_OPEN,
  COMBUSTION_LOW_CLOSE,
#endif
  NB_OF_STATE
} State;

typedef enum {
  ALGO_DEL_OFF,
  ALGO_DEL_BLINKING,
  ALGO_DEL_ON
} Algo_DELState;

typedef enum {
	UART,
	I2C,
	Particle,
}ErrorType;

extern float Algo_Simulator_slopeTempAvant;
extern int16_t PIDTrapPosition;
extern bool fanPauseRequired;
extern uint32_t getStateTime();
extern void Algo_setSimulatorMode(bool simulatorMode);
extern void Algo_setState(State state);
extern State Algo_getState();
extern int Algo_getBaffleTemp();
extern int Algo_getFrontTemp();
extern int Algo_getPlenumTemp();
extern void managePlenumSpeed(int plenumTemp, bool thermostatRequest, uint32_t Time_ms);
extern void manageButtonLed();
void setErrorFlag(uint32_t errorcode, ErrorType type);

/* Parameter temp in [tenth *C] */
extern void Algo_setBaffleTemp(int temp);

/* Parameter temp in [tenth *C] */
extern void Algo_setFrontTemp(int temp);

/* Parameter temp in [tenth *C] */
void Algo_setPlenumTemp(int temp);

/* Returns a value in [degrees] */
int Algo_getPrimary();
int Algo_getSecondary();

int Algo_getPrimarySetPoint(void);
int Algo_getGrillSetPoint(void);
int Algo_getSecondarySetPoint(void);

/* Returns a value in [degrees] */
extern int Algo_getGrill();
uint32_t Algo_getTimeOfReloadRequest();

bool IsDoorOpen(void);
float* get_algomod(void);


extern float Algo_getBaffleTempSlope();

bool Algo_getThermostatRequest();
void Algo_setThermostatRequest(bool demande);

Algo_DELState Algo_getStateFinChargemenent();
Algo_DELState Algo_getStateFermeturePorte();

void Algo_setInterlockRequest(bool demand);
bool Algo_getInterlockRequest();

void Algo_startChargement();
bool Algo_IsFanPauseRequested();
void Algo_clearReloadRequest();

void Algo_init();

/* currentTime_ms is in [ms] */
void Algo_task(uint32_t currentTime_ms);



#ifdef __cplusplus
}
#endif

#endif //ALGO_H
