#include "/Users/steve/repos/tesla-charge/inc/GarageDoorPrototype.h"

AccelStepper doorOpener (
        STEPPER_FULLSTEP,
        STEPPER_IN1_PIN,
        STEPPER_IN3_PIN,
        STEPPER_IN2_PIN,
        STEPPER_IN4_PIN
);

bool doorClosed = true;
bool doorOperating = false;

unsigned long lastDoorActivateDebounceTime = 0;

void setup() {
    pinMode(ACTIVATE_PIN, INPUT);
    pinMode(RESET_PIN, INPUT_PULLUP);

    Serial.begin(9600);

    while (! Serial) {
        continue;
    }

    doorOpener.setMaxSpeed(MAX_SPEED);
    doorOpener.setAcceleration(ACCELERATION);
    doorOpener.setSpeed(SET_SPEED);
}

void loop() {
    bool activatorState = digitalRead(ACTIVATE_PIN);

    bool doorOperate = false;

    if (digitalRead(RESET_PIN) == LOW) {
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
