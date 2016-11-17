#include "WeatherShield.h"

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

    if (regParticleVars) {
        registerParticleVars();
    }
}

void WeatherShield::registerParticleVars() {
    Particle.variable("tempF", data.tempF);
    Particle.variable("rainPerHour", data.rainPerHour);
    Particle.variable("moonIllume", data.moonIllumination);
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
        // Zero's out around midnight everyday
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
