// Tests the TeslaVehicle object

/*
 *  clang++ -o car vehicle.cpp
 */

#include <stdio.h>
#include <stdint.h>

#include "TeslaVehicle.h"

int main () {

    TeslaVehicle car;

    uint8_t data[9] = {
        0,
        1,
        2,
        3,
        4,
        0,  // error
        6,
        0,  // fetching
        8
    };

    car.data(data);

    switch (car.state()) {
        case ERROR:
            printf("ERROR\n");
            break;
        case FETCHING:
            printf("FETCHING\n");
            break;
        default:
            printf("UNKNOWN\n");
    }
}