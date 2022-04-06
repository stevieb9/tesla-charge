#ifndef TESLA_CHARGE_VEHICLE_H
#define TESLA_CHARGE_VEHICLE_H

enum shiftState { P, R, D };

enum statusState {
    UNKNOWN,
    ERROR,
    FETCHING,
    RAINBOW,
    OFFLINE,
    HOME,
    HOME_CHARGING,
    AWAY_CHARGING,
    AWAY_PARKED,
    AWAY_DRIVING
};

class TeslaVehicle {

private:

    uint8_t _online;
    uint8_t _garage;
    uint8_t _gear;
    uint8_t _charge;
    uint8_t _charging;
    uint8_t _error;
    uint8_t _rainbow;
    uint8_t _fetching;
    uint8_t _alarmEnabled;

public:

    TeslaVehicle () {}
    ~TeslaVehicle() {}

    void data (uint8_t* data);
    uint8_t state ();

    uint8_t online          () { return _online; }
    uint8_t garage          () { return _garage; }
    uint8_t gear            () { return _gear; }
    uint8_t charge          () { return _charge; }
    uint8_t charging        () { return _charging; }
    uint8_t error           () { return _error; }
    uint8_t rainbow         () { return _rainbow; }
    uint8_t fetching        () { return _fetching; }
    uint8_t alarmEnabled    () { return _fetching; }
};

void TeslaVehicle::data (uint8_t* data)  {
    _online         = data[0];
    _garage         = data[1];
    _gear           = data[2];
    _charge         = data[3];
    _charging       = data[4];
    _error          = data[5];
    _rainbow        = data[6];
    _fetching       = data[7];
    _alarmEnabled   = data[8];
}

uint8_t TeslaVehicle::state () {
    uint8_t vehicleState = 0;

    if (this->error()) {
        // Error
        vehicleState = ERROR;
    }
    else if (this->fetching()) {
        // Fetching
        vehicleState = FETCHING;
    }
    else if (this->rainbow()) {
        // Rainbow
        vehicleState = RAINBOW;
    }
    else if (! this->online()) {
        // Offline
        vehicleState = OFFLINE;
    }
    else if (! this->garage()) {
        // Not in garage
        if (this->charging) {
            vehicleState = AWAY_CHARGING;
        }
        else if (this->gear == P) {
            vehicleState = AWAY_PARKED;
        }
        else {
            vehicleState = AWAY_DRIVING;
        }
    }
    else if (this->charging) {
        vehicleState = HOME_CHARGING;
    }
    else if (this->garage) {
        vehicleState = HOME;
    }

    return vehicleState;
}
#endif //TESLA_CHARGE_VEHICLE_H
