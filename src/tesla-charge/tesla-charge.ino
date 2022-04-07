/*
    This sketch must be compiled with Debug enabled
    and set to Serial.

    Set it to Serial1 to disable the debug output from going
    to the serial monitor.
*/

#include "TeslaCharge.h"
#include "TeslaVehicle.h"

bool rainbowEnabled = false;
bool oledInit = false;
bool oledClear = true;
bool fetchBlinkStatus = false;

char* url;

uint8_t lastCharge = CHARGE_MAX;

unsigned long alarmOnTime;
unsigned long alarmOffTime;
unsigned long dataRefreshTime;
unsigned long fetchLEDBlinkTime;

bool            gotData = false;
unsigned long   count   = 0;

SSD1306Wire oled(0x3c, 4, 5);
CRGB leds[NUM_LEDS];
HTTPClient http;
WiFiClient wifi;
TeslaVehicle car;

void setup() {
    pinMode(PIR, INPUT);
    pinMode(REED, INPUT_PULLUP);
    pinMode(ALARM, OUTPUT);

    digitalWrite(ALARM, LOW);

    Serial.begin(9600);
    FastLED.addLeds<WS2812B, LED, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    alarmOnTime       = millis();
    alarmOffTime      = millis();
    dataRefreshTime   = millis();
    fetchLEDBlinkTime = millis();

    oled.init();
    oled.flipScreenVertically();
    oled.setFont(myFont_53);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    resetOLED();

    wifiSetup();

    if (DEBUG_URL) {
        url = URL_DEBUG;
    }
    else {
        url = URL;
    }

    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    bool magnet = digitalRead(REED);
    bool motion = digitalRead(PIR);

    if ((magnet == LOW || DEBUG_MAGNET) && ! DEBUG_DEVEL) {
        spl(F("Rainbow - Magnet mode"));
        rainbowCycle(1);
        return;
    }
    else if (motion || DEBUG_MOTION || DEBUG_DEVEL) {
        unsigned long currentTime = millis();

        if (currentTime - dataRefreshTime >= DATA_DELAY) {
            gotData = false;
            dataRefreshTime = currentTime;
        }

        if (!gotData && !rainbowEnabled) {
            car.load(fetchData());
        }

        switch (car.state()) {
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
    else {
        gotData         = false;
        rainbowEnabled  = false;

        if (! oledClear) {
            alarm(false);
            lastCharge = CHARGE_MAX;
            resetOLED();
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

void displayOLED (uint8_t charge) {

    if (! oledInit || charge != lastCharge) {
        resetOLED();
        oled.drawString(0, 0, (String) charge);
        oled.display();
        lastCharge = charge;
        oledInit = true;
        oledClear = false;
    }
}

uint8_t* fetchData () {

    http.begin(wifi, url);
    http.setTimeout(8000);

    static uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        gotData = false;
        data[5] = 1;
        http.end();
        return data;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        gotData = false;
        data[5] = 1;
        http.end();
        return data;
    }

    data[0] = json["online"];
    data[1] = json["garage"];
    data[2] = json["gear"];
    data[3] = json["charge"];
    data[4] = json["charging"];
    data[5] = json["error"];
    data[6] = json["rainbow"];
    data[7] = json["fetching"];

    http.end();

    gotData = true;

    return data;
}

void resetOLED () {
    oled.clear();
    oled.display();
    oledClear = true;
}

void drawLED(uint8_t led, CRGB colour) {
    leds[led] = colour;
}

void ledSet (CRGB led5, CRGB led4, CRGB led3, CRGB led2, CRGB led1, CRGB led0) {
    bool colourChanged = false;

//    serialLEDColour();

    if (leds[5] != led5) {
        colourChanged = true;
    }
    if (leds[4] != led4) {
        colourChanged = true;
    }
    if (leds[3] != led3) {
        colourChanged = true;
    }
    if (leds[2] != led2) {
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

void serialLEDColour () {
    int8_t i = NUM_LEDS - 1;

    while (i >= 0) {

        char* colour = "Unknown";

        CRGB currentColour = leds[i];

        if (currentColour == CRGB(CRGB::Yellow)) {
            colour = "Yellow";
        }
        else if (currentColour == CRGB(CRGB::Red)) {
            colour = "Red";
        }
        else if (currentColour == CRGB(CRGB::Green)) {
            colour = "Green";
        }
        else if (currentColour == CRGB(CRGB::Purple)) {
            colour = "Purple";
        }
        else if (currentColour == CRGB(CRGB::Blue)) {
            colour = "Blue";
        }
        else if (currentColour == CRGB(CRGB::White)) {
            colour = "White";
        }
        else if (currentColour == CRGB(CRGB::Black)) {
            colour = "Off";
        }

        s(F("LED "));
        s(i);
        s(F(": "));
        spl(colour);
        if (i == 0) {
            spl(F("\n"));
        }

        i--;
    }
}

void readEEPROM(int startAdr, int maxLength, char* dest) {
    EEPROM.begin(512);
    delay(10);
    for (int i = 0; i < maxLength; i++) {
        dest[i] = char(EEPROM.read(startAdr + i));
    }
    EEPROM.end();
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

void wifiSetup () {

    s(F("MAC Address: "));
    spl(WiFi.macAddress());

    char ssid[16];
    char ssidPassword[16];

    readEEPROM(0,  16, ssid);
    readEEPROM(16, 16, ssidPassword);

    WiFi.begin(ssid, ssidPassword);

    while (WiFi.status() != WL_CONNECTED) {
        spl("RSSI: " + (String) WiFi.RSSI());
        delay(500);
    }
    spl(F("Wifi Connected"));

    oled.setFont(ArialMT_Plain_16);
    oled.drawString(0, 0, F("Wifi Connected"));
    oled.display();

    delay(1000);
    oled.setFont(myFont_53);
}

void fetching () {
    unsigned long currentTime = millis();

    if (currentTime - fetchLEDBlinkTime >= FETCH_BLINK_DELAY) {
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
    gotData = false;

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
    rainbowEnabled = true;
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
