#include <AccelStepper.h>
#include "garage_door_prototype.h"

AccelStepper doorOpener (
        STEPPER_FULLSTEP,
        PIN_IN1,
        PIN_IN3,
        PIN_IN2,
        PIN_IN4
);

void setup() {
    doorOpener.setMaxSpeed(MAX_SPEED);
    doorOpener.setAcceleration(ACCELERATION);
    doorOpener.setSpeed(SET_SPEED);
    doorOpener.moveTo(2038);
}

void loop() {
    if (doorOpener.distanceToGo() == 0) {
        doorOpener.moveTo(-doorOpener.currentPosition());
    }

    doorOpener.run();
}
