#include "WeatherShield.h"

// Initialize
WeatherShield::WeatherShield() {
}

void WeatherShield::begin(byte oversample, bool regParticleVars = false) {
    Wire.begin();

    uint8_t ID_Barro = IICRead(WHO_AM_I);
    uint8_t ID_Temp_Hum = checkID();

    int x, y = 0;

    // Ping WhoAmI register
    if (ID_Barro == 0xC4) {
        x = 1;
    } else {
        x = 0;
    }

    // Ping CheckID register
    if (ID_Temp_Hum == 0x15) {
        y = 1;
    } else if (ID_Temp_Hum == 0x32) {
        y = 2;
    } else {
        y = 0;
    }

    // if (x == 1 && y == 1) {
    //   Serial.println("MPL3115A2 Found");
    //   Serial.println("Si7021 Found");
    // } else if (x == 1 && y == 2) {
    //   Serial.println("MPL3115A2 Found");
    //   Serial.println("HTU21D Found");
    // } else if (x == 0 && y == 1) {
    //   Serial.println("MPL3115A2 NOT Found");
    //   Serial.println("Si7021 Found");
    // } else if (x == 0 && y == 2) {
    //   Serial.println("MPL3115A2 NOT Found");
    //   Serial.println("HTU21D Found");
    // } else if (x == 1 && y == 0) {
    //   Serial.println("MPL3115A2 Found");
    //   Serial.println("No Temp/Humidity Device Detected");
    // } else {
    //   Serial.println("No Devices Detected");
    // }

    // input from windspeed sensor
    pinMode(WIND_SPEED, INPUT_PULLUP);

    // input from rain gauge sensor
    pinMode(RAIN, INPUT_PULLUP);

    // Set Oversample rate
    // Call with a rate from 0 to 7. See page 33 for table of ratios.
    // Sets the over sample rate. Datasheet calls for 128 but you can set it
    // from 1 to 128 samples. The higher the oversample rate the greater
    // the time between data samples.
    setOversampleRate(oversample);

    // Necessary register calls to enble temp, baro and alt
    enableEventFlags();

    if (regParticleVars) {
        registerParticleVars();
    }
}

void WeatherShield::registerParticleVars() {
    Particle.variable("tempF", data.tempF);
}

// TODO: use the RTC for this time related stuff
void WeatherShield::update() {
    seconds += updateInterval;
    if (seconds > 59) {
        seconds = 0;

        if (++minutes > 59) {
            minutes = 0;
        }

        // Zero out this minute's rainfall amount
        data.rainByMinute[minutes] = 0;
        data.windSpeedMax = 0.0;
    }

    data.rainPerHour = 0;
    for (int i = 0; i < 60; i++) {
        data.rainPerHour += data.rainByMinute[i];
    }

    if (Time.hour() == 23 && Time.minute() == 59) {
        data.rainPerDay = 0.0;
    }
}

WeatherData *WeatherShield::getWeather() {
    // Measure Relative Humidity from the HTU21D or Si7021
    data.humidity = getRH();

    // Measure Temperature from the HTU21D or Si7021
    // Temperature is measured every time RH is requested.
    // It is faster, therefore, to read it from previous RH
    // measurement with getTemp() instead with readTemp()
    data.tempF = getTempF();

    // TODO: make these private?
    readBarometer();
    // readAltimeter();

    // Calc windDirection
    data.windDirection = getWindDirection();

    // Calc windspeed
    data.windSpeedMPH = getWindSpeed();

    // Avg Wind
    data.windSpeedMeasurements[seconds] = data.windSpeedMPH;

    for (byte i = 0; i < 100; i++) {
        data.windSpeedAvg += data.windSpeedMeasurements[i];
    }
    data.windSpeedAvg /= 100;

    // Max Wind
    if (data.windSpeedMPH > data.windSpeedMax) {
        data.windSpeedMax = data.windSpeedMPH;
    }

    update();

    return &data;
}

/*
You can only receive accurate barometric readings or accurate altitude
readings at a given time, not both at the same time.
*/
//------------------------------------------------------------------------------
// Measure pressure and temperature from MPL3115A2
void WeatherShield::readBarometer() {
    setModeBarometer();
    data.baroTempF = readBaroTempF();
    data.pressurePa = readPressure();
}
//------------------------------------------------------------------------------
void WeatherShield::readAltimeter() {
    setModeAltimeter();
    data.altFeet = readAltitudeFt();
}

// Si7021 & HTU21D* Functions
float WeatherShield::getRH() {
    // Measure the relative humidity
    uint16_t RH_Code = makeMeasurment(HUMD_MEASURE_NOHOLD);
    float result = (125.0 * RH_Code / 65536) - 6;
    return result;
}

float WeatherShield::readTemp() {
    // Read temperature from previous RH measurement.
    uint16_t tempCode = makeMeasurment(TEMP_PREV);
    float result = (175.25 * tempCode / 65536) - 46.85;
    return result;
}

float WeatherShield::getTemp() {
    // Measure temperature
    uint16_t tempCode = makeMeasurment(TEMP_MEASURE_NOHOLD);
    float result = (175.25 * tempCode / 65536) - 46.85;
    return result;
}
// Give me temperature in fahrenheit!
float WeatherShield::readTempF() {
    return ((readTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

float WeatherShield::getTempF() {
    return ((getTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

void WeatherShield::heaterOn() {
    // Turns on the ADDRESS heater
    uint8_t regVal = readReg();
    regVal |= _BV(HTRE);
    // turn on the heater
    writeReg(regVal);
}

void WeatherShield::heaterOff() {
    // Turns off the ADDRESS heater
    uint8_t regVal = readReg();
    regVal &= ~_BV(HTRE);
    writeReg(regVal);
}

void WeatherShield::changeResolution(uint8_t i) {
    // Changes to resolution of ADDRESS measurements.
    // Set i to:
    //      RH         Temp
    // 0: 12 bit       14 bit (default)
    // 1:  8 bit       12 bit
    // 2: 10 bit       13 bit
    // 3: 11 bit       11 bit

    uint8_t regVal = readReg();
    // zero resolution bits
    regVal &= 0b011111110;
    switch (i) {
    case 1:
        regVal |= 0b00000001;
        break;
    case 2:
        regVal |= 0b10000000;
        break;
    case 3:
        regVal |= 0b10000001;
    default:
        regVal |= 0b00000000;
        break;
    }
    // write new resolution settings to the register
    writeReg(regVal);
}

void WeatherShield::reset() {
    // Reset user resister
    writeReg(SOFT_RESET);
}

uint8_t WeatherShield::checkID() {
    uint8_t ID_1;

    // Check device ID
    Wire.beginTransmission(ADDRESS);
    Wire.write(0xFC);
    Wire.write(0xC9);
    Wire.endTransmission();

    Wire.requestFrom(ADDRESS, 1);

    ID_1 = Wire.read();

    return (ID_1);
}

uint16_t WeatherShield::makeMeasurment(uint8_t command) {
    // Take one ADDRESS measurement given by command.
    // It can be either temperature or relative humidity
    // TODO: implement checksum checking

    uint16_t nBytes = 3;
    // if we are only reading old temperature, read olny msb and lsb
    if (command == 0xE0)
        nBytes = 2;

    Wire.beginTransmission(ADDRESS);
    Wire.write(command);
    Wire.endTransmission();
    // When not using clock stretching (*_NOHOLD commands) delay here
    // is needed to wait for the measurement.
    // According to datasheet the max. conversion time is ~22ms
    delay(100);

    Wire.requestFrom(ADDRESS, nBytes);
    // Wait for data
    int counter = 0;
    while (Wire.available() < nBytes) {
        delay(1);
        counter++;
        if (counter > 100) {
            // Timeout: Sensor did not return any data
            return 100;
        }
    }

    unsigned int msb = Wire.read();
    unsigned int lsb = Wire.read();
    // Clear the last to bits of LSB to 00.
    // According to datasheet LSB of RH is always xxxxxx10
    lsb &= 0xFC;
    unsigned int mesurment = msb << 8 | lsb;

    return mesurment;
}

void WeatherShield::writeReg(uint8_t value) {
    // Write to user register on ADDRESS
    Wire.beginTransmission(ADDRESS);
    Wire.write(WRITE_USER_REG);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t WeatherShield::readReg() {
    // Read from user register on ADDRESS
    Wire.beginTransmission(ADDRESS);
    Wire.write(READ_USER_REG);
    Wire.endTransmission();
    Wire.requestFrom(ADDRESS, 1);
    uint8_t regVal = Wire.read();
    return regVal;
}

/****************MPL3115A2 Functions**************************************/
// Returns the number of meters above sea level
// Returns -1 if no new data is available
float WeatherShield::readAltitude() {
    toggleOneShot(); // Toggle the OST bit causing the sensor to immediately
                     // take
    // another reading

    // Wait for PDR bit, indicates we have new pressure data
    int counter = 0;
    while ((IICRead(STATUS) & (1 << 1)) == 0) {
        if (++counter > 600)
            return (-999); // Error out after max of 512ms for a read
        delay(1);
    }

    // Read pressure registers
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(OUT_P_MSB);       // Address of data to get
    Wire.endTransmission(false); // Send data to I2C dev with option for a
                                 // repeated start. THIS IS NECESSARY and not
                                 // supported before Arduino V1.0.1!
    if (Wire.requestFrom(MPL3115A2_ADDRESS, 3) != 3) { // Request three bytes
        return -999;
    }

    byte msb, csb, lsb;
    msb = Wire.read();
    csb = Wire.read();
    lsb = Wire.read();

    // The least significant bytes l_altitude and l_temp are 4-bit,
    // fractional values, so you must cast the calulation in (float),
    // shift the value over 4 spots to the right and divide by 16 (since
    // there are 16 values in 4-bits).
    float tempcsb = (lsb >> 4) / 16.0;

    float altitude = (float)((msb << 8) | csb) + tempcsb;

    return (altitude);
}

// Returns the number of feet above sea level
float WeatherShield::readAltitudeFt() {
    return (readAltitude() * 3.28084);
}

// Reads the current pressure in Pa
// Unit must be set in barometric pressure mode
// Returns -1 if no new data is available
float WeatherShield::readPressure() {
    // Check PDR bit, if it's not set then toggle OST
    if (IICRead(STATUS) & (1 << 2) == 0)
        toggleOneShot(); // Toggle the OST bit causing the sensor to immediately
                         // take another reading

    // Wait for PDR bit, indicates we have new pressure data
    int counter = 0;
    while (IICRead(STATUS) & (1 << 2) == 0) {
        if (++counter > 600)
            return (-999); // Error out after max of 512ms for a read
        delay(1);
    }

    // Read pressure registers
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(OUT_P_MSB);       // Address of data to get
    Wire.endTransmission(false); // Send data to I2C dev with option for a
                                 // repeated start. THIS IS NECESSARY and not
                                 // supported before Arduino V1.0.1!
    if (Wire.requestFrom(MPL3115A2_ADDRESS, 3) != 3) { // Request three bytes
        return -999;
    }

    byte msb, csb, lsb;
    msb = Wire.read();
    csb = Wire.read();
    lsb = Wire.read();

    toggleOneShot(); // Toggle the OST bit causing the sensor to immediately
                     // take
    // another reading

    // Pressure comes back as a left shifted 20 bit number
    long pressure_whole = (long)msb << 16 | (long)csb << 8 | (long)lsb;
    pressure_whole >>=
        6; // Pressure is an 18 bit number with 2 bits of decimal.
           // Get rid of decimal portion.

    lsb &= 0b00110000; // Bits 5/4 represent the fractional component
    lsb >>= 4;         // Get it right aligned
    float pressure_decimal = (float)lsb / 4.0; // Turn it into fraction

    float pressure = (float)pressure_whole + pressure_decimal;

    return (pressure);
}

float WeatherShield::readBaroTemp() {
    if (IICRead(STATUS) & (1 << 1) == 0)
        toggleOneShot(); // Toggle the OST bit causing the sensor to immediately
                         // take another reading

    // Wait for TDR bit, indicates we have new temp data
    int counter = 0;
    while ((IICRead(STATUS) & (1 << 1)) == 0) {
        if (++counter > 600)
            return (-999); // Error out after max of 512ms for a read
        delay(1);
    }

    // Read temperature registers
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(OUT_T_MSB);       // Address of data to get
    Wire.endTransmission(false); // Send data to I2C dev with option for a
                                 // repeated start. THIS IS NECESSARY and not
                                 // supported before Arduino V1.0.1!
    if (Wire.requestFrom(MPL3115A2_ADDRESS, 2) != 2) { // Request two bytes
        return -999;
    }

    byte msb, lsb;
    msb = Wire.read();
    lsb = Wire.read();

    toggleOneShot(); // Toggle the OST bit causing the sensor to immediately
                     // take
    // another reading

    // Negative temperature fix by D.D.G.
    uint16_t foo = 0;
    bool negSign = false;

    // Check for 2s compliment
    if (msb > 0x7F) {
        foo = ~((msb << 8) + lsb) + 1; // 2’s complement
        msb = foo >> 8;
        lsb = foo & 0x00F0;
        negSign = true;
    }

    // The least significant bytes l_altitude and l_temp are 4-bit,
    // fractional values, so you must cast the calulation in (float),
    // shift the value over 4 spots to the right and divide by 16 (since
    // there are 16 values in 4-bits).
    float templsb = (lsb >> 4) / 16.0; // temp, fraction of a degree

    float temperature = (float)(msb + templsb);

    if (negSign)
        temperature = 0 - temperature;

    return (temperature);
}

// Give me temperature in fahrenheit!
float WeatherShield::readBaroTempF() {
    return ((readBaroTemp() * 9.0) / 5.0 +
            32.0); // Convert celsius to fahrenheit
}

// Sets the mode to Barometer
// CTRL_REG1, ALT bit
void WeatherShield::setModeBarometer() {
    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting &= ~(1 << 7);              // Clear ALT bit
    IICWrite(CTRL_REG1, tempSetting);
}

// Sets the mode to Altimeter
// CTRL_REG1, ALT bit
void WeatherShield::setModeAltimeter() {
    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting |= (1 << 7);               // Set ALT bit
    IICWrite(CTRL_REG1, tempSetting);
}

// Puts the sensor in standby mode
// This is needed so that we can modify the major control registers
void WeatherShield::setModeStandby() {
    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting &= ~(1 << 0);              // Clear SBYB bit for Standby mode
    IICWrite(CTRL_REG1, tempSetting);
}

// Puts the sensor in active mode
// This is needed so that we can modify the major control registers
void WeatherShield::setModeActive() {
    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting |= (1 << 0);               // Set SBYB bit for Active mode
    IICWrite(CTRL_REG1, tempSetting);
}

// Call with a rate from 0 to 7. See page 33 for table of ratios.
// Sets the over sample rate. Datasheet calls for 128 but you can set it
// from 1 to 128 samples. The higher the oversample rate the greater
// the time between data samples.
void WeatherShield::setOversampleRate(byte sampleRate) {
    if (sampleRate > 7)
        sampleRate = 7; // OS cannot be larger than 0b.0111
    sampleRate <<= 3;   // Align it for the CTRL_REG1 register

    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting &= 0b11000111;             // Clear out old OS bits
    tempSetting |= sampleRate;             // Mask in new OS bits
    IICWrite(CTRL_REG1, tempSetting);
}

// Enables the pressure and temp measurement event flags so that we can
// test against them. This is recommended in datasheet during setup.
void WeatherShield::enableEventFlags() {
    IICWrite(PT_DATA_CFG,
             0x07); // Enable all three pressure and temp event flags
}

// Clears then sets the OST bit which causes the sensor to immediately take
// another reading
// Needed to sample faster than 1Hz
void WeatherShield::toggleOneShot(void) {
    byte tempSetting = IICRead(CTRL_REG1); // Read current settings
    tempSetting &= ~(1 << 1);              // Clear OST bit
    IICWrite(CTRL_REG1, tempSetting);

    tempSetting = IICRead(CTRL_REG1); // Read current settings to be safe
    tempSetting |= (1 << 1);          // Set OST bit
    IICWrite(CTRL_REG1, tempSetting);
}

// These are the two I2C functions in this sketch.
byte WeatherShield::IICRead(byte regAddr) {
    // This function reads one byte over IIC
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(regAddr);         // Address of CTRL_REG1
    Wire.endTransmission(false); // Send data to I2C dev with option for a
                                 // repeated start. THIS IS NECESSARY and not
                                 // supported before Arduino V1.0.1!
    Wire.requestFrom(MPL3115A2_ADDRESS, 1); // Request the data...
    return Wire.read();
}

void WeatherShield::IICWrite(byte regAddr, byte value) {
    // This function writes one byte over IIC
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(regAddr);
    Wire.write(value);
    Wire.endTransmission(true);
}

char *WeatherShield::windDirToCompasPoint(int dir) {
    char *cp = "??";

    switch (dir) {
    case 0:
        cp = "N";
        break;
    case 1:
        cp = "NE";
        break;
    case 2:
        cp = "E";
        break;
    case 3:
        cp = "SE";
        break;
    case 4:
        cp = "S";
        break;
    case 5:
        cp = "SW";
        break;
    case 6:
        cp = "W";
        break;
    case 7:
        cp = "NW";
        break;
    default:
        cp = "??";
    }

    return (cp);
}

// Activated by the magnet in the anemometer (2 ticks per rotation), attached to
// input D3
void WeatherShield::windSpeedIRQ() {
    // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after
    // the
    // reed switch closes
    if (millis() - lastWindIRQ > 10) {
        lastWindIRQ = millis(); // Grab the current time
        windClicks++;           // There is 1.492MPH for each click per second.
    }
}

//---------------------------------------------------------------
// Read the wind direction sensor, return heading in degrees
int WeatherShield::getWindDirection() {
    unsigned int adc;

    // get the current reading from the sensor
    adc = analogRead(WIND_DIR);

    // The following table is ADC readings for the wind direction sensor output,
    // sorted from low to high.
    // Each threshold is the midpoint between adjacent headings. The output is
    // degrees for that ADC reading.
    // Note that these are not in compass degree order! See Weather Meters
    // datasheet for more information.

    // Wind Vains may vary in the values they return. To get exact wind
    // direction,
    // it is recomended that you AnalogRead the Wind Vain to make sure the
    // values
    // your wind vain output fall within the values listed below.
    if (adc > 2270 && adc < 2290)
        return (0); // North
    if (adc > 3220 && adc < 3299)
        return (1); // NE
    if (adc > 3890 && adc < 3999)
        return (2); // East
    if (adc > 3780 && adc < 3850)
        return (3); // SE

    if (adc > 3570 && adc < 3650)
        return (4); // South
    if (adc > 2790 && adc < 2850)
        return (5); // SW
    if (adc > 1580 && adc < 1610)
        return (6); // West
    if (adc > 1930 && adc < 1950)
        return (7); // NW

    return (-1); // error, disconnected?
}

//---------------------------------------------------------------
// Returns the instataneous wind speed
float WeatherShield::getWindSpeed() {
    float deltaTime = millis() - lastWindCheck; // 750ms

    deltaTime /= 1000.0; // Covert to seconds

    float windSpeed = (float)windClicks / deltaTime; // 3 / 0.750s = 4

    windClicks = 0; // Reset and start watching for new wind
    lastWindCheck = millis();

    windSpeed *= 1.492; // 4 * 1.492 = 5.968MPH

    return (windSpeed);
}

// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge, attached to input
// D2
void WeatherShield::rainIRQ() {
    unsigned long now = millis();

    // ignore switch-bounce glitches less than 10mS after initial edge
    if (now - rainLastMeasure > 10) {
        // Each dump is RAIN_PER_DUMP of water
        // TODO: where/when does this zero out?
        data.rainPerDay += RAIN_PER_DUMP;

        // Increase this minute's amount of rain
        data.rainByMinute[minutes] += RAIN_PER_DUMP;

        rainLastMeasure = now;
    }
}

float WeatherShield::getRainPerMinute() {
    return data.rainByMinute[minutes];
}
float WeatherShield::getRainPerHour() {
    return data.rainPerHour;
}
float WeatherShield::getRainPerDay() {
    return data.rainPerDay;
}
