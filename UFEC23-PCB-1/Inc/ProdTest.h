#ifndef PRODTEST_H
#define PRODTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void TestRunner();

typedef enum {
  COMPLETED = 0,
  MOTOR_SPEED1_TEST,
  //MOTOR_SPEED2_TEST,
  //MOTOR_SPEED3_TEST,
  //MOTOR_SPEED4_TEST,
  THERMO_REAR_TEST,
  THERMO_BAFFLE_TEST,
  PLENUM_RTD_TEST,
  STEPPER_MOTOR1_TEST,
  STEPPER_MOTOR2_TEST,
  THERMOSTAT_TEST,
  INTERLOCK_TEST,
  NB_OF_TEST
} Test;

Test getTestState();

#ifdef __cplusplus
}
#endif

#endif //PRODTEST_H
