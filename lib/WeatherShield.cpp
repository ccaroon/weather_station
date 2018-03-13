#include "WeatherShield.h"
#include <math.h>

// Initialize
WeatherShield::WeatherShield() {
}

void WeatherShield::begin(bool regParticleVars) {
    // Init HTU21D
    htu21d.begin();

    // Init MPL3115A2
    mpl3115a2.begin();
    mpl3115a2.setModeBarometer();
    mpl3115a2.setOversampleRate(7);
    mpl3115a2.enableEventFlags();

    // input from windspeed sensor
    pinMode(WIND_SPEED, INPUT_PULLUP);

    // input from rain gauge sensor
    pinMode(RAIN, INPUT_PULLUP);

    this->currMinute = Time.minute();
    this->currHour = Time.hour();

    if (regParticleVars) {
        registerParticleVars();
    }
}

void WeatherShield::registerParticleVars() {
    // Note: variable names are limted to 12 chars in length
    Particle.variable("humidity", data.humidity);
    Particle.variable("moonIllume", data.moonIllumination);
    Particle.variable("pressurePa", data.pressurePa);
    Particle.variable("rainPerDay", data.rainPerDay);
    Particle.variable("rainPerHour", data.rainPerHour);
    Particle.variable("tempF", data.tempF);
    Particle.variable("tempFDailyL", data.tempFDailyLow);
    Particle.variable("tempFDailyH", data.tempFDailyHigh);
    Particle.variable("windSpeedAvg", data.windSpeedAvg);
    Particle.variable("pressureDir", data.pressureDir);
    Particle.variable("moonWaxWane", data.moonWaxWane);
}

// Housekeeping stuff
void WeatherShield::update() {

    // Historical type stuff
    if (Time.now() - (15 * 60) > lastHistoricalDataCheck) {
        lastHistoricalDataCheck = Time.now();

        // Is the Moon Waning or Waxing
        if (data.moonIllumination > data.lastMoonIllume) {
            data.moonWaxWane = MOON_WAXING;
        } else if (data.moonIllumination < data.lastMoonIllume) {
            data.moonWaxWane = MOON_WANING;
        }
        data.lastMoonIllume = data.moonIllumination;

        // Is the Pressure Falling or Rising
        // Convert to InHg, round to 2 places(ish)
        float currentP = data.pressurePa * 0.0002953;
        currentP = roundf(currentP * 100.0) / 100.0;

        float lastP = data.lastPressurePa * 0.0002953;
        lastP = roundf(lastP * 100.0) / 100.0;

        if (currentP > lastP) {
            data.pressureDir = MERCURY_RISING;
        } else if (currentP < lastP) {
            data.pressureDir = MERCURY_FALLING;
        } else if (currentP == lastP) {
            data.pressureDir = MERCURY_STEADY;
        }
        data.lastPressurePa = data.pressurePa;

        // Daily Temperature High & Low
        if (data.tempF > data.tempFDailyHigh) {
            data.tempFDailyHigh = data.tempF;
        }

        if (data.tempF < data.tempFDailyLow) {
            data.tempFDailyLow = data.tempF;
        }
    }

    // Process Time based resets
    int newMin = Time.minute();
    if (newMin != currMinute) {
        data.rainByMinute[newMin] = 0.0;
        currMinute = newMin;
    }

    int newHour = Time.hour();
    if (newHour != currHour) {
        data.windSpeedMax = 0.0;
        currHour = newHour;
    }

    // Reset per day vars everyday at midnight'ish
    if (Time.hour() == 23 && Time.minute() == 59) {
        data.rainPerDay = 0.0;
        data.tempFDailyLow = 999.0;
        data.tempFDailyHigh = 0.0;
    }

    data.rainPerHour = 0.0;
    for (int i = 0; i < 60; i++) {
        data.rainPerHour += data.rainByMinute[i];
    }
}

WeatherData *WeatherShield::getWeather() {

    // Humidity from HTU21D
    data.humidity = htu21d.readHumidity();

    // Temperature from HTU21D (Fahrenheit)
    data.tempF = (htu21d.readTemperature() * 1.8) + 32.0;

    // Pressure from MPL3115A2 (Pascals)
    data.pressurePa = mpl3115a2.readPressure();

    // Calc windDirection
    data.windDirection = getWindDirection();

    // Calc windspeed
    data.windSpeedMPH = getWindSpeed();

    // Avg Wind
    data.windSpeedMeasurements[windSpeedMeasurementNum++] = data.windSpeedMPH;
    if (windSpeedMeasurementNum >= NUM_WIND_SPEED_MEASUREMENTS) {
        windSpeedMeasurementNum = 0;
    }

    for (byte i = 0; i < NUM_WIND_SPEED_MEASUREMENTS; i++) {
        data.windSpeedAvg += data.windSpeedMeasurements[i];
    }
    data.windSpeedAvg /= NUM_WIND_SPEED_MEASUREMENTS;

    // Max Wind
    if (data.windSpeedMPH > data.windSpeedMax) {
        data.windSpeedMax = data.windSpeedMPH;
    }

    update();

    return &data;
}

char *WeatherShield::windDirToCompasPoint(int dir) {
    char *cp = "??";

    switch (dir) {
    case 0:
        cp = "N";
        break;
    case 45:
        cp = "NE";
        break;
    case 90:
        cp = "E";
        break;
    case 135:
        cp = "SE";
        break;
    case 180:
        cp = "S";
        break;
    case 225:
        cp = "SW";
        break;
    case 270:
        cp = "W";
        break;
    case 315:
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
    // the reed switch closes
    if (millis() - lastWindIRQ > 10) {
        lastWindIRQ = millis(); // Grab the current time
        windClicks++;           // There is 1.492MPH for each click per second.
    }
}

//---------------------------------------------------------------
// Read the wind direction sensor
// Currently only support 8 compass directions
int WeatherShield::getWindDirection() {
    unsigned int analogRaw;

    // get the current reading from the sensor
    analogRaw = analogRead(WIND_DIR);

    if (analogRaw >= 3570 && analogRaw < 3700)
        return (0); // North
    if (analogRaw >= 2750 && analogRaw < 2850)
        return (45); // NE
    if (analogRaw >= 1580 && analogRaw < 1650)
        return (90); // East
    if (analogRaw >= 1900 && analogRaw < 2000)
        return (135); // SE
    if (analogRaw >= 2200 && analogRaw < 2400)
        return (180); // South
    if (analogRaw >= 3200 && analogRaw < 3299)
        return (225); // SW
    if (analogRaw >= 3890 && analogRaw < 3999)
        return (270); // West
    if (analogRaw >= 3780 && analogRaw < 3890)
        return (315); // NW

    // if (analogRaw >= 2100 && analogRaw < 2200)
    //   return (3.53); // SSW
    // if (analogRaw >= 3100 && analogRaw < 3200)
    //   return (4.32); // WSW
    // if (analogRaw >= 3700 && analogRaw < 3780)
    //   return (5.11); // WNW
    // if (analogRaw >= 3400 && analogRaw < 3500)
    //   return (5.89); // NNW
    // if (analogRaw >= 2600 && analogRaw < 2700)
    //   return (0.39); // NNE
    // if (analogRaw >= 1510 && analogRaw < 1580)
    //   return (1.18); // ENE
    // if (analogRaw >= 1470 && analogRaw < 1510)
    //   return (1.96); // ESE
    // if (analogRaw >= 1700 && analogRaw < 1750)
    //   return (2.74); // SSE

    // Bad Reading
    if (analogRaw > 4000)
        return (-1);
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
        // Zero's out around midnight everyday
        data.rainPerDay += RAIN_PER_DUMP;

        // Increase this minute's amount of rain
        data.rainByMinute[currMinute] += RAIN_PER_DUMP;

        rainLastMeasure = now;
    }
}

float WeatherShield::getRainPerMinute() {
    return data.rainByMinute[currMinute];
}

float WeatherShield::getRainPerHour() {
    return data.rainPerHour;
}

float WeatherShield::getRainPerDay() {
    return data.rainPerDay;
}
