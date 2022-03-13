#include <AccelStepper.h>
#include "garage_door_prototype.h"

AccelStepper doorOpener (
        STEPPER_FULLSTEP,
        PIN_IN1,
        PIN_IN2,
        PIN_IN3,
        PIN_IN4
);

void setup() {
    doorOpener.setMaxSpeed(4000.0);
    doorOpener.setAcceleration(100.0);
    doorOpener.setSpeed(4000);
    doorOpener.moveTo(2038);
}

void loop() {
    if (doorOpener.distanceToGo() == 0)
        //
        doorOpener.moveTo(-doorOpener.currentPosition());

    // Move the motor one step
    doorOpener.run();
}
