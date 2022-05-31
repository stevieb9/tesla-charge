# Tesla Charge Alarm System

This project is essentially a device that goes on your garage wall to inform
you visually and audibly when your Tesla vehicle battery level is low so you
don't forget to plug the charger in. It is activated only while it senses motion
through its PIR motion sensor.

It is also capable of acting as a garage door manager, with an auto-close
feature.

The software consists of a Perl application that fetches vehicle data from the
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

* [Installation](#installation)
* [Configuration](#configuration)
  + [Webserver Configuration](#webserver-configuration)
  + [Network Port](#network-port)
  + [WiFi SSID and Password](#wifi-ssid-and-password)
  + [HTTP API URL](#http-api-url)
  + [HTTP API Token](#http-api-token)
  + [ESP-NOW MAC Addresses](#esp-now-mac-addresses)
  + [Garage Location Coordinates](#garage-location-coordinates)
  + [GPIO Pins](#gpio-pins)
  + [App Security](#security)
  + [Configuration File](#configuration-file)
* [System Information](#system-information)
    + [Hardware Used](#hardware-used)
    + [Perl HTTP API Server](#perl-http-api-server)
    + [Microcontroller - Interface](#microcontroller---interface)
    + [Microcontroller - Controller](#microcontroller---controller)
    + [Microcontroller - Garage](#microcontroller---garage)
    + [Microcontroller - Garage Door Prototype](#microcontroller---garage-door-prototype)
    + [Files - Application](#files---application)
    + [Files - Sketches](#files---sketches)
    + [Files - Headers](#files---headers)
* [Running the HTTP API Service](#run-the-http-api-service)
    + [Daemon mode](#daemon-mode)
    + [Run from crontab](#run-from-crontab)
    + [All logging enabled](#all-logging-enabled)
    + [Hide access logs](#hide-access-logs)
    + [With an SSL certificate](#with-ssl-enabled)
    + [Reload the application if any file changes](#reload-the-application-if-any-file-changes)
    + [Reload the application if the application file changes](#reload-the-application-if-the-application-file-changes)
* [REST API routes](#rest-api-routes)
* [REST API functions](#rest-api-functions)
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
* [Caveats](#caveats)

## Installation

- `cd /path/to/application`

If on Ubuntu, some packages are required:

- `sudo apt-get install libssl-dev`
- `sudo apt-get install zlib1g-dev`

Install the dependencies:

- `cpanm --installdeps .`

If things fail:

- `sudo apt-get install libnet-ssleay-perl`
- `sudo apt-get install libcrypt-ssleay-perl`

Copy and edit the configuration file:

- `cp config/config.json-dist config/config.json`

Proceed through the [configuration](#configuration) section of this document.

## Configuration

There are several things that need to be verified and/or modified.

First things first, copy the `config/config.json-dist` file to
`config/config.json` in the root directory of the repository.

### Webserver configuration 

If you are running in [Daemon mode](#daemon-mode), you can use the `webserver`
section in the [config file](#configuration-file) to configure how the web
service behaves.

The daemon mode configuration elements are described with examples below.

#### port

An integer representing the port to run the webservice on.

    "port": 55556

#### debug

Set to `1` to enable debug output, and `0` to disable it.

    "debug": 0

#### access_log

Leave blank to write web service access entries to `STDOUT` (will be redirected
to whatever `stderr` points to). If set to `/dev/null`, these logs will be
discarded entirely. Otherwise, enter the path and filename of a file you'd
like these entries saved to.

    "access_log": "/dev/null"

#### stdout

The web service will write its standard output to this file (leave blank to have
entries written to the console).

    "stdout": "/tmp/tesla-charge.out"

The web service will write its error output to this file (leave blank to have
entries written to the console).

    "stderr": "/tmp/tesla-charge.err"
       
#### ssl

`1` to enable SSL (HTTPS), `0` to disable it. If set, both `ssl_key_file` and
`ssl_cert_file` must point to legitimate and valid files.

    "ssl" : 1

#### ssl_key_file

The path and name of the SSL private key file. Can be left blank if `ssl` is set
to false.

    "ssl_key_file": "/etc/letsencrypt/live/tesla.hellbent.app/privkey.pem"

#### ssl_cert_file

The path and name of the SSL certificate file. Can be left blank if `ssl` is set
to false.

    "ssl_cert_file": "/etc/letsencrypt/live/tesla.hellbent.app/fullchain.pem"

#### reload_files

A list of files that the web service will monitor, and if they change, will
automatically restart itself.

        "reload_files": [
            "/Users/steve/repos/tesla-charge/bin/app.psgi"
        ]

### Network port

By default, the Perl HTTP API server runs on port `55556`. You can edit this in
the `webserver` section of the [config file](#configuration-file) if using 
[daemon mode](#daemon-mode), or run `plackup` with the `-p PORTNUM` option to
change it.

### Header File Paths

The way Arduino sketches are built, we can't use relative paths when including
header files in the sketches. At the top of each `.ino` sketch file, replace the
absolute path of the includes to reflect the real location of the headers on
your system.

### WiFi SSID and Password

We use `WiFiManager` to manage the WiFi credentials. If a known network isn't
available to automatically log into, the controllers will start up in Access
Point mode. Simply connect to the AP (`TeslaInterface`, `TeslaController` and
`TeslaGarage`), and using a web browser, connect to `http://192.168.4.1`.

You can enter in the  WiFi details there.

You can manually access the configuration screen by setting the
`WIFI_CONFIG_PIN` to `LOW` and rebooting the unit. Don't forget to put the pin
back to `HIGH` before saving settings.

One last way to access the configuration mode Access Point captive portal is to
set `CONFIG_RESET` to `1` in the `TeslaChargeCommon.h` file, and compiling then
installing the new sketch. Make your changes, save them, then the
microcontroller will sleep so that you can flip the variable back to `0` and
re-upload the newly compiled sketch.

### HTTP API URL

This is configured in the WiFi configuration captive portal, along with the
WiFi credentials. It is available on the
[Interface Controller](#microcontroller---interface) and
[Garage Controller](#microcontroller---garage). See
[WiFi Configuration](#wifi-ssid-and-password) for how to change or set it.

In the `src/tesla-charge-interface/data` and `src/tesla-charge-garage/data`
directories, there is a `config.json` file that you can populate with the URL
and access token, then use the sketch data uploader in the Arduino IDE as
opposed to using the method described in the 
[WiFi config](#wifi-ssid-and-password) section.

It's size is set to 64 chars, and can be configured in the
[TeslaChargeCommon header file](#teslachargecommonh).

The [Garage Controller](#microcontroller---garage) has two additional parameters
that need to be set through the WiFi configuration captive portal,
`Garage Update URL` and the `Garage Data URL`. These are the `/garage_update`
and `/garage_data` API routes respectively.

Example:

    API URL: https://www.mywebsite.com:55556/path/to/app

### HTTP API Token

This is set in the WiFi configuration captive portal along with the WiFi
credentials, and the API URL. See the [auth security](#api-token-security)
section of this document for generating a token, which you'd then paste into
the `API Token` section of the captive portal. It is available on the
[Interface Controller](#microcontroller---interface) and
[Garage Controller](#microcontroller---garage). See
[WiFi Configuration](#wifi-ssid-and-password) for how to change or set it.

In the `src/tesla-charge-interface/data` and `src/tesla-charge-garage/data` 
directory, there is a `config.json` file that you can populate with the URL and
access token, then use the sketch data uploader in the Arduino IDE as opposed to
using the method described in the [WiFi config](#wifi-ssid-and-password) section.

### ESP-NOW MAC Addresses

Both the interface microcontroller and controller microcontroller MAC addresses
are stored in the `inc/TeslaChargeCommon.h` header file. These allow the
microcontrollers to communicate with one another.

Upon bootup, the controller and interface software will display the current MAC
address of the device on the serial console.

### Garage Location Coordinates

In the [config file](#configuration-file), under the `tesla_vehicle` section,
insert the coordinates of your garage. See the config file link above for an
example. This is how we identify whether the car is in the garage or not.

### GPIO Pins

The GPIO pin definitions used by each microcontroller can be found in their
respective header file in `inc/`.

Currently, these are (note the 'Protoboard pin' is custom to my setup only):

#### Interface Microcontroller

| Name            | GPIO Pin | Board Pin | Protoboard Pin | Description           |
|:----------------|----------|-----------|----------------|-----------------------|
| SCL_PIN         | 5        | D1        | A13            | I2C SDA (OLED)        |  
| SDA_PIN         | 4        | D2        | A14            | I2C SDA (OLED)        |  
| ALARM_PIN       | 13       | D7        | A15            | Alarm buzzer          |  
| REED_PIN        | 2        | D4        | A16            | Magnetic sensor       |  
| PIR_PIN         | 12       | D6        | A17            | Motion sensor         |
| WIFI_CONFIG_PIN | 14       | D5        | 19             | AP config mode switch |

#### Controller Microcontroller

| Name            | GPIO Pin | Board Pin | Protoboard Pin | Description          |
|:----------------|----------|-----------|----------------|-----------------------|
| LED_PIN         | 14       | D5        | A26            | LED strip data pin    |  
| WIFI_CONFIG_PIN | 13       | D7        | 20             | AP config mode switch |

#### Garage Microcontroller

| Name            | GPIO Pin | Board Pin | Protoboard Pin | Description                 |
|:----------------|----------|-----------|----------------|-----------------------------|
| DOOR_OPEN_PIN   | 4        | D2        | N/A            | Door open magnetic sensor   |  
| DOOR_CLOSED_PIN | 14       | D5        | N/A            | Door closed magnetic sensor | 
| DOOR_RELAY_PIN  | 12       | D6        | N/A            | Garage door opener relay    |
| DOOR_OPEN_LED   | 13       | D7        | N/A            | LED to indicate door open   |
| WIFI_CONFIG_PIN | 5        | D1        | N/A            | AP config mode switch       |

### Security

#### Host Security

To enable host/IP security, set `secure_host` to `1` in the `system` section of
the configuration file, and add the allowed IP addresses, address blocks or DNS
hostnames to the `allowed_hosts` array in the same section of the file.

The following are all valid entries:

    192.168.0.0/24
    www.example.com
    206.49.222.17
    localhost
    10.10.5.27/29

#### API Token Security

To enable an authentication token, set `secure_auth` to `1` in the `system`
section of the configuration file, then generate a token by executing the
`scripts/token_generator.pl` script. Follow the prompts on the command line. It
will ask for a token name, then it will generate JSON that you must paste into
the `tokens` section of the configuration file. Here's an example output:

    "vps": "YzU0MWNhMmRmYzI4MGFmOWJmYjAxZmMzMzQ0ZjZkOWNhNTY3ZTIyMTcwOTI5MWRhMWE3NmM5MmVlYzRmMGZkMw"

You can configure and add as many tokens as you wish.

### Configuration File

Initially, copy the `config/config.json-dist` file to `config/config.json` in
the root directory.

All values below are default. `allowed_hosts` and `tokens` sections are exanples
and do not appear in the distribution config file.

    {
        "system": {
            "secure_host":  0,          # Allow access based on IP/DNS hostname
            "secure_auth":  0,          # Allow only authorized tokens
            "allowed_hosts": [          # IP addresses, address blocks or hosts allowed
                "host.example.com",     # Example
                "192.168.1.17/24",      # Example
                "127.0.0.1"             # Example
            ],
            "tokens": {                 # API tokens (example below)
                "vps": "MzM3MWFlZDMyODViMDJmZGIxZjgwMzE3MmNkYWRhMzlmNjEyZTRhZDFkMjhmMTA5OGZiM2Y3ZWQ0YzkzYTEwYw"
            }
        },
        "tesla_vehicle": {
            "debug":        0,          # Display debug output in app.psgi
            "debug_return": 0,          # Return the below debug data to interface microcontroller
            "retry":        3,          # app.psgi will retry Tesla API this many times
            "rainbow":      0,          # Force enable "rainbow" mode
            "alarm":        1,          # Toggle the audible alarm
            "latitude":     "xx.xxxxx", # Vehicle latitude (5 decimal points)
            "longitude":    "-xxx.xxxx",# Vehicle longitude (4 decimal points)
            "debug_data": {
                "online":   0,          # Bool - Vehicle awake
                "charge":   0,          # 0-100 - Current battery level percent
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
            "debug":              0, # Bool - Display debug output in app.psgi
            "debug_return":       0, # Bool - Return the below debug data instead of live data
            "app_enabled":        1, # Bool - Mobile app allowed to make changes
            "relay_enabled":      1, # Bool - Garage door opener relay enabled
            "auto_close_enabled": 1, # Bool - Allow garage door auto-close
            "debug_data": {
                "garage_door_state":    -1, # -1 - Uninit, 0 - Closed, 1 - Open, 2 - Closing, 3 - Opening
                "tesla_in_garage":      -1, # -1 - Uninit, 0 - Away, 1 - In garage
                "activity":              0  # Bool - Pending garage door action
            }
        },
        "webserver": {
            "debug": 0,                         # Enable webapp debug output
            "port": 55556,                      # Port plackup runs on
            "access_log": "",                   # Alternate access log location ('/dev/null' to disable logs)
            "stdout": "/tmp/tesla-charge.out",  # Webserver STDOUT location (leave blank for console output)
            "stderr": "/tmp/tesla-charge.err",  # Webserver STDERR location (leave blank for console output)
            "ssl" : 0,                          # Enable SSL (ie. HTTPS) (needs ssl_key_file and ssl_cert_file)
            "ssl_key_file": "",                 # Path to the SSL key file
            "ssl_cert_file": "",                # Path to the SSL cert file
            "reload_files": [                   # List of files to auto-reload on if changed 
            ]
        } 
    }

## System Information

### Hardware Used

For the core of the system, three of the micro-controllers used are
ESP8266-based Wemos D1 units (these show up as "Lolin(WEMOS) D1 R2 & mini" as
the Board type in the Arduino IDE). For the garage door  prototype, an Arduino
Uno was used.

### Perl HTTP API Server

The Perl backend API software can be run by any computer that is accessible
over the network or Internet. I used to use a Raspberry Pi 3, but then decided
to have it run on an external VPS.

This software is responsible for collecting and aggregating data from the Tesla
API, as well as the various microcontrollers. It responds to REST API calls, and
for the most part, all return values are JSON encoded.

### Microcontroller - Interface

Board used: WeMOS D1 R2

This device:

- Communicates with the external Perl HTTP API server
- Fetches  configuration and Tesla vehicle state information as JSON data
- Sounds the audible alarm if enabled
- Draws the OLED screen
- Manages the "rainbow mode" magnetic reed sensor
- Manages the motion PIR sensor
- Sends the required data to the [Controller Microcontroller](#microcontroller---controller).
- Manages the API URL and authorization token

This microcontroller's sketch is in 
`src/tesla-charge-interface/tesla-charge-interface.ino`.

Its header file is `inc/TeslaChargeInterface.h`.

It uses a `TeslaVehicle` object and specific enum variables found in
`inc/TeslaVehicle.h`.

It also uses the shared header in `inc/TeslaChargeCommon.h`.

### Microcontroller - Controller

Board used: WeMOS D1 R2

This device:

- Receives data from the [Interface Microcontroller](#microcontroller---interface)
- Manages the LED indicator strip

This microcontroller's sketch is in
`src/tesla-charge-controller/tesla-charge-controller.ino`.

Its header file is `inc/TeslaChargeController.h`.

It also uses the shared header in `inc/TeslaChargeCommon.h`.

### Microcontroller - Garage

Board used: WeMOS D1 R2

This device monitors the garage door for its open and close state and has the
ability to open or close on demand, or auto close. It uses magnetic sensors
to determine the position of the door.

See [WiFi Configuration](#wifi-ssid-and-password) and [API URL](#http-api-url)
sections for configuration of API communication configuration.

Its configuration and command information is polled from the 
[API Server](#perl-http-api-server).

The sketch is in `src/garage/garage.ino` and its header file is
`inc/Garage.h`.

### Microcontroller - Garage Door Prototype

Board used: Arduino Uno R3

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

All headers can be found under the `inc/` directory. Aside from the
[Tesla Vehicle class](#teslavehicleh), they contain user definable variables
such as GPIO pin configurations, timing configurations etc.

#### TeslaChargeInterface.h 

Used by the [Interface Microcontroller](#microcontroller---interface) sketch.

#### TeslaChargeController.h

Used by the [Controller Microcontroller](#microcontroller---controller) sketch.

#### TeslaChargeCommon.h

Shared by the [Interface Microcontroller](#microcontroller---interface),
[Controller Microcontroller](#microcontroller---controller) and the
[Garage Microcontroller](#microcontroller---garage) sketches.

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

## Run the HTTP API Service

For every request made to the REST API service, a pre-hook is called before the
client is directed to the route they requested. This pre-hook reads in the
[configuration file](#configuration-file), and updates any configuration data
that has changed. This means that the application never needs a restart while
fiddling with changes in the config file.

### Daemon mode

We have a wrapper script, `bin/tesla-charge`, that allows the web application
to run in daemon mode. 

First, you need to set the `TESLA_CHARGE_PATH` environment variable to the root
path of the tesla-charge directory. Example:

    export TESLA_CHARGE_PATH=/Users/steve/repos/tesla-charge

Basic commands for the daemon are:

#### start

Starts the application using the configuration from the `webserver` section of
the [config file](#configuration-file). See that link for information on what
tweaks can be made to the operation of the program.

    bin/tesla-charge start

#### stop

Stops the application and cleans up after it.

    bin/tesla-charge stop

#### restart

Restarts the application.

    bin/tesla-charge restart

#### Run from crontab

    @reboot sleep 10; /Users/steve/repos/tesla-charge/bin/tesla-charge start 

### All logging enabled 

    perl app.psgi

or...

    plackup app.psgi -p 55556

### Hide access logs

    plackup --access-log /dev/null -p 55556

### With SSL enabled

    sudo plackup -p 443 --enable-ssl --ssl-key-file /etc/letsencrypt/live/tesla.hellbent.app/privkey.pem --ssl-cert-file /etc/letsencrypt/live/tesla.hellbent.app/fullchain.pem --access-log /dev/null app.psgi

### Reload the application if any file changes 

    plackup -R . app.psgi

### Reload the application if the application file changes

    plackup -R app.psgi app.psgi

## REST API routes

All available routes less `/garage_update` are both `GET` and `POST` accessible.
If `secure_auth` is set in the `system` section of the
[config file](#configuration-file), you **must** `POST` your request, and supply
a `{"token": "token_value"}` JSON encoded string as the body content or body
parameter.

If `secure_auth` is disabled, you can use simple `GET` requests.

#### / (main page)

Retrieves the data about the Tesla vehicle and returns it as a JSON encoded
string.

Example:

    {
      "rainbow":0,
      "charge":44,
      "error":0,
      "alarm":0,
      "fetching":0,
      "gear":0,
      "online":1,
      "charging":0,
      "garage":1
    }

#### /debug

Returns the debug data as defined in the [config file](#configuration-file),
without having to set `debug_return`.

#### /debug_garage

Same as `/debug`, but for the garage door state data.

#### /wake

Attempts to wake up the Tesla vehicle if it's currently offline. On success,
redirects to the main page and returns the data from there. Otherwise, returns
an HTML error string.

#### /garage

Displays the HTML template for garage door operation in the browser.

#### /garage_data

Retrieves the garage door state data and information.

#### /garage_door_state

Returns the current state of the garage door. `1` if the garage is open, and `0`
if its closed.

#### /garage_update

This route is a POST only route. It's used to update the state of the garage
door by the garage microcontroller.

The body content must be JSON encoded:

    {
      "door_state" : 1,
      "activity" : 0
    }

Parameter `door_state` is one of `0` for `DOOR_CLOSED`, `1` for `DOOR_OPEN`, `2`
for `DOOR_CLOSING` and `3` for `DOOR_OPENING`.

Parameter `activity` is a bool which is a flag to say that there is a pending
operation needed. Typically, you should always set this to `0` to indicate that
the last pending activity required has been acknowleded and operated on.

#### /garage_door_operate

This route sets a flag that informs the system that a garage door operation is
required.

## REST API functions

#### security()

Checks the configuration file to see if either `secure_host` or `secure_auth` are
enabled, and if so, verifies the caller's IP address and/or the supplied token
to the data provided in the [config file](#configuration-file).

Returns true (`1`) if authorization is successful (or auth is disabled), and
false (`0`) otherwise.

#### config_load()

Reads in the `config.json` configuration file.

#### debug_data()

Parses, sorts, encodes and returns the Tesla vehicle debug data as defined in
the `tesla_vehicle` `debug_data` section of the [config file](#configuration-file).

The data is JSON encoded before being returned.

#### debug_garage_data()

Same thing as [debug_data()](#debug_data), but for the `garage` `debug_data`
[config file](#configuration-file) section.

#### update()

Fetches the vehicle data from the Tesla API, and stores it in the local cache.
If we can't fetch the data in a number of tries, we set the `error` flag to `1`
in the returned JSON string. This number of retries is set in the `tesla_vehicle`
`retry` section of the [config file](#configuration-file).

This function is called in a separate process from the main REST server, using
[Async::Event::Interval](https://metacpan.org/pod/Async::Event::Interval). The
Tesla API calls are made through
[Tesla::Vehicle](https://metacpan.org/pod/Tesla::Vehicle).

#### fetch($conf)

Compiles the vehicle data to identify the actual state of the vehicle.

Returns a JSON string of the vehicle data. See [here](#-main-page) for details
of the actual return value.

#### deviation($what, $coord)

Verifies whether the vehicle is within the set range of the garage, as defined
by the `ACCURACY`, `RANGE`, `LAT` and `LON` constants.

Parameters:

    $what

Either `lat` for latitude or `lon` for longitude.

    $coord

A floating point number representing either `lat` or `lon`.

To verify the vehicle's location in proximity to the garage, both `lat` and `lon`
need to be checked.

Returns `1` if vehicle is in the garage, and `0` if not.

#### distance

Calculates the distance the vehicle is from the garage. This is only used for
debug output.

Parameters:

    $what

Either `lat` for latitude or `lon` for longitude.

    $coord

A floating point number representing either `lat` or `lon`.

Returns a floating point number representing the number of metres the vehicle is
away from the garage.

#### gear($gear)

Converts the single alpha character of the transmission state to an integer
value:

    P => 0
    R => 1
    D => 2
    N => 2

Returns the integer associated with the character value.

#### _default_data()

Fetches and returns the default vehicle data from the
[config file](#configuration-file).

#### _default_garage_data()

Fetches and returns the default garage data from the
[config file](#configuration-file).

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

### FETCHING

The REST API server is currently attempting to fetch data from the Tesla API.

| LED State            |
|:---------------------|
| **Green (blinking)** |
| Off                  |
| Off                  |
| Off                  |
| Off                  |
| Off                  |

### RAINBOW

This is a special fun mode, and simply rotates all the LEDs through a rainbow of
colours.

### OFFLINE

This is the state if the vehicle is currently offline (ie. not awake).

| LED State |
|:----------|
| **Blue**  |
| Off       |
| Off       |
| Off       |
| Off       |
| Off       |

### HOME (Display charge level)

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

### HOME_CHARGING

The vehicle is at home, and is currently charging.

| LED State  |
|:-----------|
| **Purple** |
| Off        |
| Off        |
| Off        |
| Off        |
| Off        |

### AWAY_CHARGING

The vehicle is charging, but it's not at home.

| LED State  |
|:-----------|
| **White**  |
| Off        |
| Off        |
| Off        |
| Off        |
| **Purple** |

### AWAY_PARKED

The vehicle is not at home, and is currently in park.

| LED State |
|:----------|
| **White** |
| Off       |
| Off       |
| Off       |
| Off       |
| **Red**   |

### AWAY_DRIVING

The vehicle is currenty away from home and is in a gear other than park.

| LED State |
|:----------|
| **White** |
| Off       |
| Off       |
| Off       |
| Off       |
| **Green** |

## Caveats

For each start and stop of the application, we seem to leak two shared memory
segments. This is an issue regarding running multiple layers of a stack whereby
`Async::Event::Interval` and/or `IPC::Shareable` are not processing their
cleanup handlers properly. This won't be an issue in normal operation.