// Tests the TeslaVehicle object

/*
 *  clang++ -o car vehicle.cpp
 */

#include <stdio.h>
#include <stdint.h>

#include "TeslaVehicle.h"

int main () {

    TeslaVehicle car;

    uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    car.data(data);

    printf("garage: %d, fetching: %d\n", car.garage(), car.fetching());

    return 0;
}