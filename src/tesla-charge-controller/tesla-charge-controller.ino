#include "/Users/steve/repos/tesla-charge/inc/TeslaCharge.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaVehicle.h"

unsigned long alarmOnTime;
unsigned long alarmOffTime;
unsigned long fetchLEDBlinkTime;
bool fetchBlinkStatus = false;

CRGB leds[NUM_LEDS];

void setup() {
    pinMode(ALARM, OUTPUT);
    digitalWrite(ALARM, LOW);

    Serial.begin(9600);
    FastLED.addLeds<WS2812B, LED, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    alarmOnTime       = millis();
    alarmOffTime      = millis();
    fetchLEDBlinkTime = millis();
}

void loop() {
    uint16_t state = 2;

    if (1) {
        switch (state) {
            case ERROR:
                error();
                break;
            case FETCHING:
                fetching();
                break;
            case RAINBOW:
                rainbow();
                break;
            case OFFLINE:
                offline();
                break;
            case HOME:
                home();
                break;
            case HOME_CHARGING:
                home_charging();
                break;
            case AWAY_CHARGING:
                away_charging();
                break;
            case AWAY_PARKED:
                away_parked();
                break;
            case AWAY_DRIVING:
                away_driving();
                break;
        }
    }
}

void alarm (bool state) {
    uint8_t alarmState          = digitalRead(ALARM);
    unsigned long currentTime   = millis();

    if (state) {
        if (alarmState) {
            if (currentTime - alarmOnTime >= ALARM_ON_TIME) {
                digitalWrite(ALARM, LOW);
                spl(F("Alarm off"));
                alarmOffTime = currentTime;
                alarmOnTime  = currentTime;
            }
        }
        else {
            if (currentTime - alarmOffTime >= ALARM_OFF_TIME) {
                digitalWrite(ALARM, HIGH);
                spl(F("Alarm on"));
                alarmOnTime = currentTime;
            }
        }
    }
    else {
        if (alarmState) {
            digitalWrite(ALARM, LOW);
            spl(F("Alarm off: state"));
        }
    }
}

void drawLED(uint8_t led, CRGB colour) {
    leds[led] = colour;
}

void ledSet (CRGB led5, CRGB led4, CRGB led3, CRGB led2, CRGB led1, CRGB led0) {
    bool colourChanged = false;

    if (leds[5] != led5) {
        colourChanged = true;
    }
    if (leds[4] != led4) {
        colourChanged = true;
    }
    if (leds[3] != led3) {
        colourChanged = true;
    } if (leds[2] != led2) {
        colourChanged = true;
    }
    if (leds[1] != led1) {
        colourChanged = true;
    }
    if (leds[0] != led0) {
        colourChanged = true;
    }

    if (colourChanged) {
        drawLED(5, led5);
        drawLED(4, led4);
        drawLED(3, led3);
        drawLED(2, led2);
        drawLED(1, led1);
        drawLED(0, led0);

        FastLED.show();
    }
}

void rainbowCycle(int SpeedDelay) {
    byte *c;
    uint16_t i, j;

    for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
            drawLED(i, CRGB(*c, *(c + 1), *(c + 2)));
            //setPixel(i, *c, *(c + 1), *(c + 2));
        }
        FastLED.show();
        delay(SpeedDelay);
    }
}

byte * Wheel(byte WheelPos) {
    static byte c[3];

    if (WheelPos < 85) {
        c[0] = WheelPos * 3;
        c[1] = 255 - WheelPos * 3;
        c[2] = 0;
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        c[0] = 255 - WheelPos * 3;
        c[1] = 0;
        c[2] = WheelPos * 3;
    } else {
        WheelPos -= 170;
        c[0] = 0;
        c[1] = WheelPos * 3;
        c[2] = 255 - WheelPos * 3;
    }

    return c;
}

void fetching () {
    unsigned long currentTime = millis();

    if (currentTime - fetchLEDBlinkTime >= FETCH_BLINK_DELAY) {
        spl(F("FETCHING"));
        if (! fetchBlinkStatus) {
            ledSet(
                CRGB::Green,
                CRGB::Black,
                CRGB::Black,
                CRGB::Black,
                CRGB::Black,
                CRGB::Black
            );

            fetchBlinkStatus = true;
        }
        else {
            ledSet(
                    CRGB::Black,
                    CRGB::Black,
                    CRGB::Black,
                    CRGB::Black,
                    CRGB::Black,
                    CRGB::Black
            );

            fetchBlinkStatus = false;
        }

        fetchLEDBlinkTime = currentTime;
    }
}

void error () {
    ledSet(
        CRGB::Yellow,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black
    );
}

void rainbow () {
    spl(F("Rainbow"));
    //rainbowEnabled = true;
    rainbowCycle(1);
}

void offline () {
    spl(F("Offline"));
    ledSet(
        CRGB::Blue,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black
    );
}

void home () {
/*
    spl("HOME");
    if (car.charge() >= 85 && car.charge() <= 100) {
        ledSet(CRGB::Black, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green);
    }
    else if (car.charge() < 85) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green);
    }
    else if (car.charge() < 80) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Green);
    }
    else if (car.charge() < 60) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green);
    }
    else if (car.charge() < 40) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Green);
    }
    else if (car.charge() < 20) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red);
    }
*/
}

void home_charging () {
    ledSet(
        CRGB::Purple,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black
    );
}

void away_charging () {
    ledSet(
            CRGB::White,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Purple
    );
}

void away_parked () {
    ledSet(
            CRGB::White,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Red
    );
}

void away_driving() {
    ledSet(
            CRGB::White,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Black,
            CRGB::Green
    );
}
