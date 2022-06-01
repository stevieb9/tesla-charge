#include "/Users/steve/repos/tesla-charge/inc/Garage.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeCommon.h"

bool            gotData = false;
bool            doorAutoCloseAuthorized = false;
bool            configSaveNeeded = false;

int8_t*         teslaData;
uint8_t         lastDoorPosition = -1;

unsigned long   lastLoopCycleTime = 0;
unsigned long   doorOpenTime = 0;

enum            shiftState {P, R, D};
enum            doorStatus {DOOR_CLOSED, DOOR_OPEN, DOOR_CLOSING, DOOR_OPENING};
enum            operation {NONE, OPERATE_DOOR};

String apiTokenString = "";

garageData garageStruct;

HTTPClient http;
BearSSL::WiFiClientSecure wifi;
WiFiManager wifiManager;

void setup() {
    pinMode(DOOR_RELAY_PIN, OUTPUT);
    digitalWrite(DOOR_RELAY_PIN, LOW);

    pinMode(DOOR_OPEN_PIN, INPUT_PULLUP);
    pinMode(DOOR_CLOSED_PIN, INPUT_PULLUP);

    pinMode(DOOR_OPEN_LED, OUTPUT);
    digitalWrite(DOOR_OPEN_LED, LOW);

    Serial.begin(9600);

    while (! Serial) {
        continue;
    }
    configRead();

    wifiManager.setConfigPortalTimeout(180);

    if (CONFIG_RESET) {
        wifiManager.resetSettings();
    }

    // To allow connecting to HTTPS
    wifi.setInsecure();

    wifiManager.setBreakAfterConfig(true);
    wifiManager.setSaveConfigCallback(saveConfig);

    WiFiManagerParameter custom_api_token("api_token", "API Token", apiToken, sizeof(apiToken));
    WiFiManagerParameter custom_api_url("api_url", "API URL", apiURL, sizeof(apiURL));
    WiFiManagerParameter custom_garage_url("garage_url", "Garage URL", garageURL, sizeof(garageURL));
    WiFiManagerParameter custom_update_url("update_url", "Update URL", updateURL, sizeof(updateURL));

    wifiManager.addParameter(&custom_api_token);
    wifiManager.addParameter(&custom_api_url);
    wifiManager.addParameter(&custom_garage_url);
    wifiManager.addParameter(&custom_update_url);

    // Manage the wifi connection, including checking config or switch to see if
    // we should be in AP config mode

    if (CONFIG_RESET) {
        // Give us time to re-upload the sketch with CONFIG_RESET disabled
        spl(F("\nConfig was reset, waiting for sketch upload with reset disabled"));
        delay(100000);
    }

    if (digitalRead(WIFI_CONFIG_PIN) == LOW) {
        spl(F("Going into config mode"));

        if (! wifiManager.startConfigPortal(apNameGarage)){
            Serial.println(F("Failed to start the configuration portal"));
            delay(3000);
            ESP.restart();
            delay(5000);
        }
        Serial.println(F("Connected to the configuration portal"));
    }
    else if (! wifiManager.autoConnect(apNameGarage)) {
        spl(F("Failed to connect to wifi..."));
        delay(3000);
        ESP.restart();
        delay(5000);
    }

    strcpy(apiURL, custom_api_url.getValue());
    strcpy(garageURL, custom_garage_url.getValue());
    strcpy(updateURL, custom_update_url.getValue());
    strcpy(apiToken, custom_api_token.getValue());

    apiTokenString = String("{\"token\":\"") + String(apiToken) + String("\"}");

    s(F("Token: "));
    spl(apiToken);
    s(F("Token string: "));
    spl(apiTokenString);

    configWrite();

    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    if (millis() - lastLoopCycleTime > LOOP_CYCLE_DELAY) {
        int apiResult = fetchGarageData();

        if (apiResult < 0) {
            spl(F("Error retrieving garage API data"));
            delay(500);
            return;
        }

        uint8_t garageDoorState = doorState();

        autoCloseDoor();
        pendingOperations();
        uint8_t doorPosition = doorState(); // Update the API if needed

        lastLoopCycleTime = millis();
    }
}

bool doorAutoCloseCondition () {
    if (! garageStruct.autoCloseEnabled) {
        spl(F("Door auto close disabled"));
        return false;
    }

    if (doorState() == DOOR_CLOSED) {
        spl(F("Door already closed"));
        return false;
    }

    uint8_t* teslaData = fetchTeslaData();

    uint8_t carInGarage     = teslaData[1];
    uint8_t teslaError      = teslaData[5];
    uint8_t teslaFetching   = teslaData[7];

    uint8_t attempts = 0;

    while (teslaError == 1 || teslaFetching == 1) {
        delay(TESLA_API_DELAY);

        teslaData = fetchTeslaData();

        carInGarage     = teslaData[1];
        teslaError      = teslaData[5];
        teslaFetching   = teslaData[7];

        if (attempts == TESLA_API_RETRIES) {
            break;
        }

        attempts++;
    }

    if (! carInGarage) {
        spl(F("All auto close conditions met (car not in garage)"));
        return true;
    }

    spl(F("Car is in garage"));
    return false;
}

uint8_t doorState () {
    uint8_t doorOpen = ! digitalRead(DOOR_OPEN_PIN);
    uint8_t doorClosed = ! digitalRead(DOOR_CLOSED_PIN);

    uint8_t doorState = lastDoorPosition;

    if (doorOpen) {
        doorState = DOOR_OPEN;
        if (! digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, HIGH);
        }
    }
    else if (doorClosed) {
        doorState = DOOR_CLOSED;
        if (digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, LOW);
        }
    }
    else {
        if (lastDoorPosition == DOOR_OPEN || lastDoorPosition == DOOR_CLOSING) {
            doorState = DOOR_CLOSING;
        }
        else if (lastDoorPosition == DOOR_CLOSED || lastDoorPosition == DOOR_OPENING) {
            doorState = DOOR_OPENING;
        }
        else {
            spl(F("Door is in an unknown state"));
            doorState = -1;
        }
    }

    if (doorState != lastDoorPosition) {
        updateData(doorState);

        s(F("lastDoorPosition: "));
        spl(lastDoorPosition);
        s(F("doorState: "));
        spl(doorState);
    }

    lastDoorPosition = doorState;

    return doorState;
}

void autoCloseDoor () {

    uint8_t doorPosition = doorState();

    if (doorPosition == DOOR_OPEN) {
        if (doorOpenTime == 0) {
            doorOpenTime = millis();
        }

        if (millis() - doorOpenTime >= DOOR_CLOSE_TIME) {
            bool canCloseDoor = doorAutoCloseCondition();

            if (canCloseDoor) {
                doorOperate();
            }

            doorOpenTime = 0;
        }
    }
    else {
        if (doorOpenTime > 0) {
            doorOpenTime = 0;
        }
    }
}

void pendingOperations () {
    if (garageStruct.appEnabled) {
        int8_t activity = garageStruct.activity;

        if (activity > 0) {
            // Reset pending activity flag
            garageStruct.activity = 0;
        }

        if (activity == OPERATE_DOOR) {
            spl(F("Manually operating garage door via app"));
            doorOperate();
        }
    }
}

void doorOperate () {
    uint8_t door = doorState();

    if (door == DOOR_OPEN) {
        spl(F("Closing door"));
        doorActivate();
    }
    else if (door == DOOR_CLOSED) {
        spl(F("Opening door"));
        doorActivate();
    }
}

void doorActivate () {
    if (garageStruct.relayEnabled) {
        digitalWrite(DOOR_RELAY_PIN, HIGH);
        delay(250);
        digitalWrite(DOOR_RELAY_PIN, LOW);
    }
}

int fetchGarageData () {

    http.begin(wifi, garageURL);
    http.setTimeout(8000);

    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(apiTokenString);

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        http.end();
        return httpCode;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        s(F("Error: "));
        spl(error.c_str());
        http.end();
        return -1;
    }

    garageStruct.garageDoorState    = json[F("garage_door_state")];
    garageStruct.teslaInGarage      = json[F("tesla_in_garage")];
    garageStruct.activity           = json[F("activity")];
    garageStruct.relayEnabled       = json[F("relay_enabled")];
    garageStruct.appEnabled         = json[F("app_enabled")];
    garageStruct.autoCloseEnabled   = json[F("auto_close_enabled")];

    http.end();

    return 0;
}

uint8_t* fetchTeslaData () {

    http.begin(wifi, apiURL);
    http.setTimeout(8000);

    static uint8_t teslaData[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        gotData = false;
        teslaData[5] = 1;
        http.end();
        return teslaData;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        gotData = false;
        teslaData[5] = 1;
        http.end();
        return teslaData;
    }

    teslaData[0] = json[F("online")];
    teslaData[1] = json[F("garage")];
    teslaData[2] = json[F("gear")];
    teslaData[3] = json[F("charge")];
    teslaData[4] = json[F("charging")];
    teslaData[5] = json[F("error")];
    teslaData[6] = json[F("rainbow")];
    teslaData[7] = json[F("fetching")];

    http.end();

    gotData = true;

    return teslaData;
}

void updateData (uint8_t doorState) {

    http.begin(wifi, updateURL);
    http.setTimeout(8000);
    http.addHeader(F("Content-Type"), F("application/json"));

    DynamicJsonDocument jsonDoc(384);

    jsonDoc[F("token")] = apiToken;
    jsonDoc[F("door_state")] = doorState;
    jsonDoc[F("activity")] = garageStruct.activity;

    char jsonData[384];
    serializeJson(jsonDoc, jsonData);

    int httpCode = http.POST(jsonData);

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        http.end();
    }

    http.end();
}

void configRead () {
    if (SPIFFS.begin()) {
        Serial.println(F("Mounted file system"));
        if (SPIFFS.exists("/config.json")) {
            Serial.println(F("Reading config file"));
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println(F("Opened config file"));

                StaticJsonDocument<384> json;

                DeserializationError error = deserializeJson(json, configFile);

                if (! error) {
                    strcpy(apiURL, json["api_url"]);
                    strcpy(garageURL, json["garage_url"]);
                    strcpy(updateURL, json["update_url"]);
                    strcpy(apiToken, json["api_token"]);

                    s(F("API url: "));
                    spl(apiURL);
                    s(F("Token: "));
                    spl(apiToken);
                    s(F("JSON url: "));
                    spl(apiURL);
                    s(F("JSON url: "));
                    spl(apiURL);
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

void configWrite () {
    if (configSaveNeeded) {
        Serial.println(F("Saving config"));

        File configFile = SPIFFS.open("/config.json", "w");

        if (! configFile) {
            Serial.println(F("failed to open config file for writing"));
            return;
        }

        StaticJsonDocument<384> json;

        json["api_url"] = apiURL;
        json["garage_url"] = garageURL;
        json["update_url"] = updateURL;
        json["api_token"] = apiToken;

        if (serializeJson(json, configFile) == 0) {
            Serial.println(F("Failed to write to file"));
        }

        configFile.close();
    }
}

void saveConfig () {
    configSaveNeeded = true;
}