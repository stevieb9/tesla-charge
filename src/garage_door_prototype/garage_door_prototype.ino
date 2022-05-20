#include "/Users/steve/repos/tesla-charge/inc/GarageDoorPrototype.h"

AccelStepper doorOpener (
        STEPPER_FULLSTEP,
        STEPPER_PIN_IN1,
        STEPPER_PIN_IN3,
        STEPPER_PIN_IN2,
        STEPPER_PIN_IN4
);

bool doorClosed = true;
bool doorOperating = false;

unsigned long lastDoorActivateDebounceTime = 0;

void setup() {
    pinMode(PIN_ACTIVATE, INPUT);
    pinMode(PIN_RESET, INPUT_PULLUP);

    Serial.begin(9600);

    while (! Serial) {
        continue;
    }

    doorOpener.setMaxSpeed(MAX_SPEED);
    doorOpener.setAcceleration(ACCELERATION);
    doorOpener.setSpeed(SET_SPEED);
}

void loop() {
    bool activatorState = digitalRead(PIN_ACTIVATE);

    bool doorOperate = false;

    if (digitalRead(PIN_RESET) == LOW) {
       doorOpener.moveTo(POSITION_CLOSED);
       doorOperating = true;
    }

    if (activatorState == HIGH && ! doorOperating) {
        doorOperate = true;
    }

    if (doorOperate && doorClosed && ! doorOperating) {
        doorOpener.moveTo(POSITION_OPEN);
        doorOperating = true;
    }
    if (doorOperate && ! doorClosed && ! doorOperating) {
        doorOpener.moveTo(POSITION_CLOSED);
        doorOperating = true;
    }

    if (doorOpener.distanceToGo() == 0) {
        doorOperating = false;
        doorClosed = ! doorClosed;
    }

    /*
        Serial.print("Pos: ");
        Serial.print(doorOpener.currentPosition());
        Serial.print(" ToGo: ");
        Serial.print(doorOpener.distanceToGo());
        Serial.print(" Operating: ");
        Serial.print(doorOperating);
        Serial.print(" Operate: ");
        Serial.println(doorOperate);
    */

    doorOpener.run();
}
