# Tesla Charge Alarm System

- [Tesla Charge Alarm System](#tesla-charge-alarm-system)
    * [System Information](#system-information)
        + [Perl HTTP API Server](#perl-http-api-server)
        + [Microcontroller - Interface](#microcontroller---interface)
        + [Microcontroller - Controller](#microcontroller---controller)
        + [Microcontroller - Garage](#microcontroller---garage)
        + [Microcontroller - Garage Door Prototype](#microcontroller---garage-door-prototype)
        + [Files - Application](#files---application)
            - [app.psgi](#apppsgi)
        + [Files - Sketches](#files---sketches)
            - [tesla-charge-interface.ino](#tesla-charge-interfaceino)
            - [tesla-charge-controller.ino](#tesla-charge-controllerino)
            - [garage.ino](#garageino)
            - [garage-door-prototype.ino](#garage-door-prototypeino)
        + [Files - Headers](#files---headers)
            - [TeslaChargeInterface.h](#teslachargeinterfaceh)
            - [TeslaChargeController.h](#teslachargecontrollerh)
            - [TeslaChargeCommon.h](#teslachargecommonh)
            - [TeslaVehicle.h](#teslavehicleh)
            - [Garage.h](#garageh)
            - [GarageDoorPrototype.h](#garagedoorprototypeh)
            - [TeslaChargeFont.h](#teslachargefonth)
    * [Configuration](#configuration)
        + [Network Port](#network-port)
        + [WiFi SSID and Password](#wifi-ssid-and-password)
        + [HTTP API URLS](#http-api-urls)
        + [ESP-NOW MAC Addresses](#esp-now-mac-addresses)
        + [GPIO Pins](#gpio-pins)
    * [Install Perl Components](#install-perl-components)
    * [Run the HTTP API Service](#run-the-http-api-service)
        + [Run from crontab](#run-from-crontab)
        + [All logging enabled](#all-logging-enabled)
        + [Hide access logs](#hide-access-logs)
        + [Reload the application if any file changes](#reload-the-application-if-any-file-changes)
        + [Reload the application if the application file changes](#reload-the-application-if-the-application-file-changes)

## System Information

### Perl HTTP API Server

This software is responsible for collecting and aggregating from the Tesla API,
as well as various microcontrollers. It responds to REST API calls, and for the
most part, all return values are JSON.

### Microcontroller - Interface

This device:

- Communicates with the external Perl HTTP API server
- Fetches  configuration and Tesla vehicle state information as JSON data
- Sounds the audible alarm if enabled
- Draws the OLED screen, 
- Manages the "rainbow mode" magnetic reed sensor
- Manages the motion PIR sensor
- Sends the required data to the [Controller Microcontroller](#microcontroller---controller).

This microcontroller's sketch is in 
`src/tesla-charge-interface/tesla-charge-interface.ino`.

It's header file is `inc/TeslaChargeInterface.h`.

It uses a `TeslaVehicle` object and specific enum variables found in
`inc/TeslaVehicle.h`.

It also uses a shared header in `inc/TeslaChargeCommon.h`.

### Microcontroller - Controller

This device:

- Receives data from the [Interface Microcontroller](#microcontroller---interface)
- Manages the LED indicator strip

This microcontroller's sketch is in
`src/tesla-charge-controller/tesla-charge-controller.ino`.

It's header file is `inc/TeslaChargeController.h`.

It also uses a shared header in `inc/TeslaChargeCommon.h`.

### Microcontroller - Garage

This device monitors the garage door for its open and close state and has the
ability to open or close on demand, or auto close. It uses magnetic sensors
to determine the position of the door.

Its configuration and command information is polled from the 
[API Server](#perl-http-api-server).

The sketch is in `src/garage/garage.ino` and its header file is
`inc/Garage.h`.

### Microcontroller - Garage Door Prototype

This microcontroller simulates a garage door using a timed stepper motor.

The sketch is in `src/garage-door-prototype/garage-door-prototype.ino` and
its header file is `inc/GarageDoorPrototype.h`.

### Files - Application

#### app.psgi

This is the main Perl HTTP REST API application.

### Files - Sketches

All sketches can be found under the `src/` directory, inside of a directory
with the same name as the sketch file.

#### tesla-charge-interface.ino

Used by the [Interface Microcontroller](#microcontroller---interface).

#### tesla-charge-controller.ino

Used by the [Controller Microcontroller](#microcontroller---controller).

#### garage.ino

Used by the [Garage Microcontroller](#microcontroller---garage).

#### garage-door-prototype.ino

Used by the [Garage Door Prototype Microcontroller](#microcontroller---garage).

### Files - Headers

All headers can be found under the `inc/` directory.

#### TeslaChargeInterface.h 

Used by the [Interface Microcontroller](#microcontroller---interface) sketch.

#### TeslaChargeController.h

Used by the [Controller Microcontroller](#microcontroller---controller) sketch.

#### TeslaChargeCommon.h

Shared by the [Interface Microcontroller](#microcontroller---interface) and
[Controller Microcontroller](#microcontroller---controller) sketches.

#### TeslaVehicle.h

Provides an object representing the Tesla vehicle and contains various state
data.

Used by the [Interface Microcontroller](#microcontroller---interface) sketch.

#### Garage.h

Used by the [Garage Microcontroller](#microcontroller---garage) sketch.

#### GarageDoorPrototype.h

Used by the [Garage Door Prototype Microcontroller](#microcontroller---garage-door-prototype)
sketch.

#### TeslaChargeFont.h

Used by the [Interface Microcontroller](#microcontroller---interface) sketch
for the custom fonts displayed on the OLED display device.

## Configuration

There are several things that need to be verified and/or modified.

### Network Port

By default, the Perl HTTP API server runs on port `55556`. You can edit this in
the `app.psgi` file, or run `plackup` with the `-p PORTNUM` option to change it.

### WiFi SSID and Password

This must be stored in EEPROM.

- SSID is at EEPROM address 0, and is 16 bytes long maximum
- Password is at EEPROM address 16, and is 16 bytes long

### HTTP API URLS

These are hard coded as `URL` and `URL_DEBUG` in the `inc/TeslaChargeInterface.h`
header file. Each entry should include the URL, port and any route that is
default.

### ESP-NOW MAC Addresses

Both the interface microcontroller and controller microcontroller MAC addresses
are stored in the `inc/TeslaChargeCommon.h` header file. These allow the
microcontrollers to communicate with one another.

Upon bootup, the controller and interface software will display the current MAC
address of the device on the serial console.

### GPIO Pins

The GPIO pin definitions used by each microcontroller can be found in their
respective header file in `inc/`.

## Install Perl Components

    cd tesla-charge
    cpanm --installdeps .

## Run the HTTP API Service

### Run from crontab

    @reboot sleep 10; cd /path/to/tesla-charge; /path/to/perl app.psgi > /tmp/tesla_web.log 2>&1

### All logging enabled 

    perl app.psgi

or...

    plackup app.psgi -p 55556

### Hide access logs

    plackup --access-log /dev/null -p 55556

### Reload the application if any file changes 

    plackup -R . app.psgi

### Reload the application if the application file changes

    plackup -R app.psgi app.psgi
