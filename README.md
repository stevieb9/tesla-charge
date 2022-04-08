# Tesla Charge Alarm System

This software consists of a Perl application that fetches vehicle data from the
Tesla API, and presents a REST API interface that allows applications and
micro-controllers to access that data.

It also includes four Arduino sketches for microcontrollers. The primary two
([Interface](#microcontroller---interface) and
[Controller](#microcontroller---controller)) make up a device that attaches to
the garage wall. It has an LED strip and OLED screen that displays charge
information about the vehicle, with an audible alarm if the vehicle is below a
certain battery level.

The other two are related to a garage door management system, as well as a
garage door prototype.

* [Vehicle and LED States](#vehicle-and-led-states)
    + [LED State Quick Table](#led-state-quick-table)
    + [UNKNOWN](#unknown)
    + [ERROR](#error)
    + [FETCHING](#fetching)
    + [RAINBOW](#rainbow)
    + [OFFLINE](#offline)
    + [HOME](#home-display-charge-level)
    + [HOME_CHARGING](#home_charging)
    + [AWAY_CHARGING](#away_charging)
    + [AWAY_PARKED](#away_parked)
    + [AWAY_DRIVING](#away_driving)
* [System Information](#system-information)
    + [Perl HTTP API Server](#perl-http-api-server)
    + [Microcontroller - Interface](#microcontroller---interface)
    + [Microcontroller - Controller](#microcontroller---controller)
    + [Microcontroller - Garage](#microcontroller---garage)
    + [Microcontroller - Garage Door Prototype](#microcontroller---garage-door-prototype)
    + [Files - Application](#files---application)
    + [Files - Sketches](#files---sketches)
    + [Files - Headers](#files---headers)
* [Configuration](#configuration)
    + [Network Port](#network-port)
    + [WiFi SSID and Password](#wifi-ssid-and-password)
    + [HTTP API URLS](#http-api-urls)
    + [ESP-NOW MAC Addresses](#esp-now-mac-addresses)
    + [GPIO Pins](#gpio-pins)
    + [Configuration File](#configuration-file)
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

### Configuration File

Initially, copy the `config.json-dist` file to `config.json` in the root
directory.

All values below are default.

    {
        "system": {
            "secure_ip":    0,          # Refuse access based on IP
            "secure_auth":  0           # Allow only authorized users
        },
        "tesla_vehicle": {
            "debug":        0,          # Display debug output in app.psgi
            "debug_return": 1,          # Return the below debug data to interface microcontroller
            "retry":        3,          # app.psgi will retry Tesla API this many times
            "rainbow":      0,          # Force enable "rainbow" mode
            "alarm":        1,          # Toggle the audible alarm
            "debug_data": {
                "online":   0,          # Bool - Vehicle awake
                "charge":   0,          # 0-100 - Set the battery level
                "charging": 0,          # Bool - Vehicle charging
                "garage":   0,          # Bool - Car in garage
                "gear":     0,          # 0-2  - Car gear (0 - P, 1 - R, 2 - N & D)
                "error":    0,          # Bool - Simulate API fetch error
                "rainbow":  0,          # Bool - Rainbow mode
                "fetching": 0,          # Bool - Fetching data
                "alarm":    0           # Bool - Audible alarm enabled
            }
        },
        "garage": {
            "debug":            0,      # Bool - Display debug output in app.psgi
            "debug_return":     0,      # Bool - Return the below debug data instead of live data
            "app_enabled":      1,      # Bool - Mobile app allowed to make changes
            "relay_enabled":    1,      # Bool - Garage door opener relay enabled
            "auto_close_enabled": 1,    # Bool - Allow garage door auto-close
            "debug_data": {
                "garage_door_state":    -1, # -1 - Uninit, 0 - Closed, 1 - Open, 2 - Closing, 3 - Opening
                "tesla_in_garage":      -1, # -1 - Uninit, 0 - Away, 1 - In garage
                "activity":             0   # Bool - Pending garage door action
            }
        }
    }

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

## Vehicle and LED States

Vehicle state definitions can be found in the `inc/TeslaVehicle.h` file, in the
`statusState` enum definition.

### LED State Quick Table

| Description                    | LED 6                | LED 5         | LED 4         | LED 3         | LED 2         | LED 1         |
|:-------------------------------|----------------------|---------------|---------------|---------------|---------------|---------------|
| Device dormant                 | -                    | -             | -             | -             | -             | -             |
| Error with Tesla API           | **yellow**           | -             | -             | -             | -             | -             |
| Fetching data from Tesla       | **green** (flashing) | -             | -             | -             | -             | -             |
| Rainbow mode                   | **multi**            | **multi**     | **multi**     | **multi**     | **multi**     | **multi**     |
| Vehicle is offline             | **blue**             | -             | -             | -             | -             | -             |
| Vehicle is home (not charging) | -                    | **green/red** | **green/red** | **green/red** | **green/red** | **green/red** |
| Vehicle is home (charging)     | **purple**           | -             | -             | -             | -             | -             |
| Vehicle is away (charging)     | **white**            | -             | -             | -             | -             | **purple**    |
| Vehicle is away (parked)       | **white**            | -             | -             | -             | -             | **red**       |
| Vehicle is away (driving)      | **white**            | -             | -             | -             | -             | **green**     |

### UNKNOWN

The software is in a waiting state and has no information on the vehicle.

LED State: All LEDs off

| LED State |
|:----------|
| Off       |
| Off       |
| Off       |
| Off       |
| Off       |
| Off       |

### ERROR

The software has encountered an error while fetching data from the Tesla API,
or the micro-controller device can't contact the REST API software.

| LED State  |
|:-----------|
| **Yellow** |
| Off        |
| Off        |
| Off        |
| Off        |
| Off        |

## FETCHING

The REST API server is currently attempting to fetch data from the Tesla API.

| LED State            |
|:---------------------|
| **Green (blinking)** |
| Off                  |
| Off                  |
| Off                  |
| Off                  |
| Off                  |

## RAINBOW

This is a special fun mode, and simply rotates all the LEDs through a rainbow of
colours.

## OFFLINE

This is the state if the vehicle is currently offline (ie. not awake).


| LED State |
|:----------|
| **Blue**  |
| Off       |
| Off       |
| Off       |
| Off       |
| Off       |

## HOME (Display charge level)

The vehicle is at home and is online, but is not charging. In this state, the
LEDs display the actual car charge level.


| LED State                              |
|:---------------------------------------|
| Off                                    |
| **Red/Green** (Green if battery > 84%) |
| **Red/Green** (Green if battery > 79%) |
| **Red/Green** (Green if battery > 59%) |
| **Red/Green** (Green if battery > 39%) |
| **Red/Green** (Green if battery > 19%) |

## HOME_CHARGING

The vehicle is at home, and is currently charging.

| LED State  |
|:-----------|
| **Purple** |
| Off        |
| Off        |
| Off        |
| Off        |
| Off        |

## AWAY_CHARGING

The vehicle is charging, but it's not at home.

| LED State  |
|:-----------|
| **White**  |
| Off        |
| Off        |
| Off        |
| Off        |
| **Purple** |

## AWAY_PARKED

The vehicle is not at home, and is currently in park.

| LED State |
|:----------|
| **White** |
| Off       |
| Off       |
| Off       |
| Off       |
| **Red**   |

## AWAY_DRIVING

The vehicle is currenty away from home and is in a gear other than park.

| LED State |
|:----------|
| **White** |
| Off       |
| Off       |
| Off       |
| Off       |
| **Green** |