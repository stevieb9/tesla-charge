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
        1,  // error
        6,
        1,  // fetching
        8
    };

    car.data(data);

    switch (car.state()) {
        case car.ERROR:
            printf("ERROR\n");
            break;
        case car.FETCHING:
            printf("FETCHING\n");
            break;
        default:
            printf("Error\n");
    }
}