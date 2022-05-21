#ifndef TESLA_CHARGE_GARAGE_DOOR_PROTOTYPE_H
#define TESLA_CHARGE_GARAGE_DOOR_PROTOTYPE_H

#include <Arduino.h>
#include <AccelStepper.h>

#define STEPPER_FULLSTEP 4

#define PIN_RESET       5   // D1
#define PIN_ACTIVATE    4   // D2
#define PIN_STEPPER_IN1 2   // D4
#define PIN_STEPPER_IN2 12  // D6
#define PIN_STEPPER_IN3 13  // D7
#define PIN_STEPPER_IN4 14  // D5

#define MAX_SPEED       4000.0
#define SET_SPEED       4000
#define ACCELERATION    100.0

#define POSITION_CLOSED 0
#define POSITION_OPEN   3225

#define DOOR_ACTIVATE_DEBOUNCE_DELAY 255

#endif //TESLA_CHARGE_GARAGE_DOOR_PROTOTYPE_H
