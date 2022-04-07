#include "/Users/steve/repos/tesla-charge/inc/TeslaCharge.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaVehicle.h"

bool oledInit = false;
bool oledClear = true;

char* url;

uint8_t lastCharge = CHARGE_MAX;

unsigned long dataRefreshTime;

bool gotData = false;

SSD1306Wire oled(0x3c, 4, 5);
HTTPClient http;
WiFiClient wifi;
TeslaVehicle car;

void setup() {
    pinMode(PIR, INPUT);
    pinMode(REED, INPUT_PULLUP);

    Serial.begin(9600);

    dataRefreshTime   = millis();

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

    if (motion || DEBUG_MOTION || DEBUG_DEVEL) {
        unsigned long currentTime = millis();

        if (currentTime - dataRefreshTime >= DATA_DELAY) {
            gotData = false;
            dataRefreshTime = currentTime;
        }

        if (!gotData) {
            car.load(fetchData());
        }

        switch (car.state()) {
            case ERROR:
                break;
            case FETCHING:
                break;
            case RAINBOW:
                break;
            case OFFLINE:
                break;
            case HOME:
                break;
            case HOME_CHARGING:
                break;
            case AWAY_CHARGING:
                break;
            case AWAY_PARKED:
                break;
            case AWAY_DRIVING:
                break;
        }
    }
    else {
        gotData         = false;

        if (! oledClear) {
            lastCharge = CHARGE_MAX;
            resetOLED();
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

void readEEPROM(int startAdr, int maxLength, char* dest) {
    EEPROM.begin(512);
    delay(10);
    for (int i = 0; i < maxLength; i++) {
        dest[i] = char(EEPROM.read(startAdr + i));
    }
    EEPROM.end();
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