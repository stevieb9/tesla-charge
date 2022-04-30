#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeInterface.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeCommon.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaVehicle.h"

bool oledInit = false;
bool oledClear = true;
bool alarmEnabled = true;

String apiTokenString = "";

uint8_t lastCharge = CHARGE_MAX;

unsigned long alarmOnTime;
unsigned long alarmOffTime;
unsigned long dataRefreshTime;

bool gotData = false;
bool configSaveNeeded = false;

SSD1306Wire oled(0x3c, SDA_PIN, SCL_PIN);
HTTPClient http;
BearSSL::WiFiClientSecure wifi;
WiFiManager wifiManager;
TeslaVehicle car;
VehicleData vehicleData;

void setup() {
    pinMode(PIR_PIN, INPUT);
    pinMode(REED_PIN, INPUT_PULLUP);
    pinMode(ALARM_PIN, OUTPUT);

    digitalWrite(ALARM_PIN, LOW);

    Serial.begin(9600);

    while (! Serial) {
        continue;
    }

    dataRefreshTime   = millis();
    alarmOnTime       = millis();
    alarmOffTime      = millis();

    oled.init();
    oled.flipScreenVertically();
    oled.setFont(myFont_53);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    displayClear();

    if (esp_now_init() != 0) {
        Serial.println(F("Error initializing ESP-NOW"));
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(vehicleDataSent);
    esp_now_add_peer(MacController, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

    configRead();

    // Wipe the wifi creds so we can access the AP config screen

    if (CONFIG_RESET) {
        spl(F("BEFORE RESET"));
        wifiManager.resetSettings();
    }

    // To allow connecting to HTTPS
    wifi.setInsecure();

    wifiManager.setSaveConfigCallback(saveConfig);

    spl(F("BEFORE PARAMS"));

    WiFiManagerParameter custom_api_token("api_token", "API Token", apiToken, sizeof(apiToken));
    WiFiManagerParameter custom_api_url("api_url", "API URL", apiURL, sizeof(apiURL));

    wifiManager.addParameter(&custom_api_token);
    wifiManager.addParameter(&custom_api_url);

    spl(F("AFTER PARAMS"));

    if (! wifiManager.autoConnect(apNameInterface)) {
        spl(F("Failed to connect to wifi..."));
        delay(3000);
        ESP.restart();
        delay(5000);
    }

    spl(F("BEFORE STRCPY"));

    strcpy(apiURL, custom_api_url.getValue());
    spl(F("AFTER URL"));

    strcpy(apiToken, custom_api_token.getValue());
    spl(F("AFTER TOKEN"));

    apiTokenString = String("{\"token\":\"") + String(apiToken) + String("\"}");

    configWrite();

    if (CONFIG_RESET) {
        // Give us time to re-upload the sketch with CONFIG_RESET disabled
        spl(F("\nConfig was reset, waiting for sketch upload with reset disabled"));
        delay(100000);
    }

    spl(apiToken);
    spl(apiURL);

    delay(1000);
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    bool magnet = digitalRead(REED_PIN);
    bool motion = digitalRead(PIR_PIN);

    if (motion || DEBUG_MOTION || DEBUG_DEVEL) {
        unsigned long currentTime = millis();

        if (currentTime - dataRefreshTime >= DATA_DELAY) {
            gotData = false;
            dataRefreshTime = currentTime;
        }

        if (! gotData) {
            car.load(fetchData());
        }

        switch (car.state()) {
            case HOME:
                displayCharge(car.charge(), true);
                break;
            case HOME_CHARGING:
                displayCharge(car.charge(), false);
                break;
            case AWAY_CHARGING:
                displayCharge(car.charge(), false);
                break;
            case AWAY_PARKED:
                displayCharge(car.charge(), false);
                break;
            case AWAY_DRIVING:
                displayCharge(car.charge(), false);
                break;
            default:
                displayClear();
            break;
        }

        vehicleData.state = car.state();
        vehicleData.charge = car.charge();
    }
    else {
        gotData = false;
        displayClear();
        lastCharge = CHARGE_MAX;
        alarm(0);
        vehicleData.state = UNKNOWN;
    }

    esp_now_send(MacController, (uint8_t *) &vehicleData, sizeof(vehicleData));
}

void configWrite () {
    if (configSaveNeeded) {
        Serial.println(F("Saving config"));

        File configFile = SPIFFS.open("/config.json", "w");

        if (! configFile) {
            Serial.println(F("failed to open config file for writing"));
            return;
        }

        StaticJsonDocument<256> json;

        json["api_url"] = apiURL;
        json["api_token"] = apiToken;

        sp("URL: ");
        spl(apiURL);

        sp("Token: ");
        spl(apiToken);

        if (serializeJson(json, configFile) == 0) {
            Serial.println(F("Failed to write to file"));
        }

        configFile.close();
    }
}

void displayClear () {
    OLEDClear();
}

void displayCharge (uint8_t batteryLevel, bool soundAlarm) {
    if (batteryLevel < ALARM_CHARGE && soundAlarm) {
        alarm(1);
    }

    OLEDDisplay(batteryLevel);
}

void alarm (bool state) {
    uint8_t alarmState          = digitalRead(ALARM_PIN);
    unsigned long currentTime   = millis();

    if (alarmEnabled) {
        if (state) {
            if (alarmState) {
                if (currentTime - alarmOnTime >= ALARM_ON_TIME) {
                    digitalWrite(ALARM_PIN, LOW);
                    alarmOffTime = currentTime;
                    alarmOnTime = currentTime;
                }
            } else {
                if (currentTime - alarmOffTime >= ALARM_OFF_TIME) {
                    digitalWrite(ALARM_PIN, HIGH);
                    alarmOnTime = currentTime;
                }
            }
        } else {
            if (alarmState) {
                digitalWrite(ALARM_PIN, LOW);
            }
        }
    }
}

void OLEDClear () {
    if (! oledClear) {
        oled.clear();
        oled.display();
        oledClear = true;
    }
}

void OLEDDisplay (uint8_t charge) {
    if (! oledInit || charge != lastCharge || oledClear) {
        displayClear();
        oled.drawString(0, 0, (String) charge);
        oled.display();
        lastCharge = charge;
        oledInit = true;
        oledClear = false;
    }
}

uint8_t* fetchData () {

    http.begin(wifi, apiURL);
    http.setTimeout(8000);

    static uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(apiTokenString);

    delay(1000);
    if (httpCode < 0) {
        sp(F("HTTP Error Code: "));
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

    alarmEnabled = json["alarm"];

    http.end();

    gotData = true;

    return data;
}

void vehicleDataSent(uint8_t *mac, uint8_t sendStatus) {
    /*
    Serial.print(F("Last Packet Send Status: "));
    if (sendStatus == 0) {
        Serial.println(F("Delivery success"));
    }
    else {
        Serial.println(F("Delivery fail"));
    }
    */
}

void configRead () {
    if (SPIFFS.begin()) {
        Serial.println(F("Mounted file system"));
        if (SPIFFS.exists("/config.json")) {
            Serial.println(F("Reading config file"));
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println(F("Opened config file"));

                StaticJsonDocument<256> json;

                DeserializationError error = deserializeJson(json, configFile);

                spl(F("After deserial"));

                if (! error) {
                    strcpy(apiURL, json["api_url"]);
                    strcpy(apiToken, json["api_token"]);

                    sp(F("JSON url: "));
                    sp(apiURL);
                    sp(F(" Token: "));
                    spl(apiToken);
                } else {
                    Serial.println(F("Failed to load json config"));
                }
                configFile.close();
            }
        }
    } else {
        Serial.println(F("Failed to mount FS"));
    }
};

void saveConfig () {
    configSaveNeeded = true;
}