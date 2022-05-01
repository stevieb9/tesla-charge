#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeCommon.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeController.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaVehicle.h"

unsigned long fetchLEDBlinkTime;
bool fetchBlinkStatus = false;

WiFiManager wifiManager;
CRGB leds[NUM_LEDS];
VehicleData vehicleData;

void setup() {
    pinMode(WIFI_CONFIG_PIN, INPUT_PULLUP);

    Serial.begin(9600);

    while (! Serial) {
        continue;
    }

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    fetchLEDBlinkTime = millis();

    ledReset();

    if (CONFIG_RESET) {
        // Give us time to re-upload the sketch with CONFIG_RESET disabled
        spl(F("\nConfig was reset, waiting for sketch upload with reset disabled"));
        delay(100000);
    }

    if (digitalRead(WIFI_CONFIG_PIN) == LOW) {
        spl(F("Going into config mode"));

        if (! wifiManager.startConfigPortal(apNameController)){
            Serial.println(F("Failed to start the configuration portal"));
            delay(3000);
            ESP.restart();
            delay(5000);
        }
        Serial.println(F("Connected to the configuration portal"));
    }
    else if (! wifiManager.autoConnect(apNameController)) {
        spl(F("Failed to connect to wifi..."));
        delay(3000);
        ESP.restart();
        delay(5000);
    }

    vehicleData.state = UNKNOWN;
    vehicleData.charge = 0;

    if (esp_now_init() != 0) {
        spl(F("Error initializing ESP-NOW"));
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(vehicleDataRecv);
    esp_now_add_peer(MacInterface, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    switch (vehicleData.state) {
        case UNKNOWN:
            unknown();
            break;
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

void drawLED(uint8_t led, CRGB colour) {
    leds[led] = colour;
}

void ledReset () {
    ledSet(
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black
    );
}

void ledSet (CRGB led5, CRGB led4, CRGB led3, CRGB led2, CRGB led1, CRGB led0) {
    drawLED(5, led5);
    drawLED(4, led4);
    drawLED(3, led3);
    drawLED(2, led2);
    drawLED(1, led1);
    drawLED(0, led0);

    FastLED.show();
}

void rainbowCycle(int SpeedDelay) {
    byte *c;
    uint16_t i, j;

    for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
            drawLED(i, CRGB(*c, *(c + 1), *(c + 2)));
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

void unknown () {
    ledReset();
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
    //rainbowEnabled = true;
    rainbowCycle(1);
}

void offline () {
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
    if (vehicleData.charge < 20) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red);
    }
    else if (vehicleData.charge < 40) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Green);
    }
    else if (vehicleData.charge < 60) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green);
    }
    else if (vehicleData.charge < 80) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Green);
    }
    else if (vehicleData.charge < 85) {
        ledSet(CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green);
    }
    else {
        ledSet(CRGB::Black, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green);
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

void vehicleDataRecv(uint8_t* mac, uint8_t *dataRecv, uint8_t len) {
    memcpy(&vehicleData, dataRecv, sizeof(vehicleData));

    /*
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print(F("State: "));
    Serial.println(vehicleData.state);
    Serial.print(F("Settings: "));
    Serial.println(vehicleData.charge);
    */
}