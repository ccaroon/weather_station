## SparkFunMPL3115A2
A few changes are necessary in order to get this library to work with the
Particle Photon.

### SparkFunMPL3115A2.h
Replace the following block with `#include "application.h"`

    #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
    #else
    #include "WProgram.h"
    #endif

    #include <Wire.h>

### SparkFunMPL3115A2.cpp
* Remove `#include <Wire.h>`
* Prefix all binary notation numbers with `0`
    - Example: B101010 --> 0B101010
