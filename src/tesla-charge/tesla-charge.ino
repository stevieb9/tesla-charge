/*
    This sketch must be compiled with Debug enabled
    and set to Serial.

    Set it to Serial1 to disable the debug output from going
    to the serial monitor.
*/

#include "TeslaCharge.h"

char* url;

bool oledInit        = false;
bool oledClear       = false;
bool rainbowEnabled  = false;
bool statusLEDClear  = true;
bool fetchBlinkStatus = false;

uint8_t lastCharge = CHARGE_MAX;
enum    shiftState {P, R, D};

unsigned long alarmOnTime;
unsigned long alarmOffTime;
unsigned long dataRefreshTime;
unsigned long fetchLEDBlinkTime;

uint8_t*        data;
bool            gotData = false;
unsigned long   count   = 0;
unsigned long   errors  = 0;

SSD1306Wire oled(0x3c, 4, 5);
CRGB leds[NUM_LEDS];
HTTPClient http;
WiFiClient wifi;

void setup() {
    pinMode(PIR, INPUT);
    pinMode(REED, INPUT_PULLUP);
    pinMode(ALARM, OUTPUT);

    digitalWrite(ALARM, LOW);

    Serial.begin(9600);
    FastLED.addLeds<WS2811, LED, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    alarmOnTime       = millis();
    alarmOffTime      = millis();
    dataRefreshTime   = millis();
    fetchLEDBlinkTime = millis();

    oled.init();
    oled.flipScreenVertically();
    oled.setFont(myFont_53);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    resetOLED();
    resetChargeLED();

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
        oledClear = false;
        rainbowCycle(1);
        //serialLEDColour();
        return;
    }
    else if (motion || DEBUG_MOTION || DEBUG_DEVEL) {

        unsigned long currentTime = millis();

        if (currentTime - dataRefreshTime >= DATA_DELAY) {
            gotData = false;
            dataRefreshTime = currentTime;
        }

        if (! gotData && ! rainbowEnabled) {
            data = fetchData();
        }

        uint8_t online      = data[0];
        uint8_t garage      = data[1];
        uint8_t gear        = data[2];
        uint8_t charge      = data[3];
        uint8_t charging    = data[4];
        uint8_t error       = data[5];
        uint8_t rainbow     = data[6];
        uint8_t fetching    = data[7];

        s(F("\n**** Count:\t\t\t"));
        spl(count);
        count++;

        if (fetching) {
            spl(F("                   FETCHING"));

            currentTime = millis();

            if (currentTime - fetchLEDBlinkTime >= FETCH_BLINK_DELAY) {
                resetChargeLED();

                if (! fetchBlinkStatus) {
                    statusLED(CRGB::Green, CRGB::Black);
                    statusLEDClear = false;
                    fetchBlinkStatus = true;
                }
                else {
                    resetStatusLED();
                    fetchBlinkStatus = false;
                }

                //serialLEDColour();
                fetchLEDBlinkTime = currentTime;
            }

            return;
        }
        if (errors) {
            s(F("**** Errors:\t\t\t"));
            spl(errors);
        }

        if (rainbow) {
            spl(F("Rainbow"));
            rainbowEnabled = true;
            oledClear = false;
            rainbowCycle(1);
            //serialLEDColour();
            return;
        }
        if (error) {
            spl(F("Error"));
            resetChargeLED();
            statusLED(CRGB::Yellow, CRGB::Black);
            //serialLEDColour();
            statusLEDClear = false;
            gotData = false;
            errors++;
            return;
        }
        else if (! online) {
            spl(F("Offline"));
            resetChargeLED();
            statusLED(CRGB::Blue, CRGB::Black);
            //serialLEDColour();
            statusLEDClear = false;
            return;
        }
        else if (! garage) {
            s(F("Not in garage... "));
            resetChargeLED();

            if (charging) {
                spl(F("Charging"));
                statusLED(CRGB::White, CRGB::Purple);
            }
            else if (gear == P) {
                spl(F("Parked"));
                statusLED(CRGB::White, CRGB::Red);
            }
            else if (gear == D || gear == R) {
                spl(F("Driving"));
                statusLED(CRGB::White, CRGB::Green);
            }

            //serialLEDColour();
            statusLEDClear = false;
            displayOLED(charge);
            return;
        }
        else if (charging) {
            spl(F("Charging..."));
            resetChargeLED();
            statusLED(CRGB::Purple, CRGB::Black);
            //serialLEDColour();
            statusLEDClear = false;
            displayOLED(charge);
            return;
        }
        else {
            resetStatusLED();
        }

        if (gear == P || gear == R || gear == D) {
            spl(F("Garage"));
            chargeLED(charge);
            displayOLED(charge);
            oledClear = false;

            if (charge < ALARM_CHARGE) {
                alarm(true);
            }

            //serialLEDColour();
        }
    }
    else {
        resetStatusLED();
        resetChargeLED();

        gotData         = false;
        rainbowEnabled  = false;

        if (! oledClear) {
            alarm(false);
            lastCharge = CHARGE_MAX;
            FastLED.show();
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

void resetStatusLED () {
    if (! statusLEDClear) {
        statusLED(CRGB::Black, CRGB::Black);
        statusLEDClear = true;
    }
}

void statusLED (CRGB statusColour, CRGB stateColour) {

    CRGB statusCurrentColour = leds[LED_STATUS];
    CRGB stateCurrentColour = leds[LED_STATE];

    bool colourChanged = false;

    if (statusColour != statusCurrentColour) {
        drawLED(LED_STATUS, statusColour);
        colourChanged = true;
    }

    if (stateColour != stateCurrentColour) {
        drawLED(LED_STATE, stateColour);
        colourChanged = true;
    }

    FastLED.show();
    if (colourChanged) {
        FastLED.show();
    }
}

void resetChargeLED () {
    for (uint8_t i = 0; i < 5; i++) {
        drawLED(i, CRGB::Black);
    }
}

void chargeLED (uint8_t charge) {

    for (uint8_t i = 0; i < 5; i++) {
        drawLED(i, CRGB::Green);
    }

    if (charge < 85) {
        drawLED(4, CRGB::Red);
    }
    if (charge < 80) {
        drawLED(3, CRGB::Red);
    }
    if (charge < 60) {
        drawLED(2, CRGB::Red);
    }
    if (charge < 40) {
        drawLED(1, CRGB::Red);
    }
    if (charge < 20) {
        drawLED(0, CRGB::Red);
    }

    FastLED.show();
}

void drawLED(uint8_t led, CRGB colour) {
    leds[led] = colour;
}

char* serialLEDColour () {
    for (uint8_t i = NUM_LEDS - 1; i < 255; i = i - 1) {

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
