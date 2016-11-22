#include "application.h"

#ifndef WeatherShield_h
#define WeatherShield_h

#include "SparkFunHTU21D.h"
#include "SparkFunMPL3115A2.h"

// Wind and Rain
#define WIND_DIR A0
#define WIND_SPEED D3

#define RAIN D2
#define RAIN_PER_DUMP 0.011

#define NUM_WIND_SPEED_MEASUREMENTS 120

#define MERCURY_FALLING -1
#define MERCURY_STEADY 0
#define MERCURY_RISING 1

#define MOON_UNKNOWN -1
#define MOON_WANING 0
#define MOON_WAXING 1

// -----------------------------------------------------------------------------
// NOTE: Any data point that you want to use as a Particle Variable needs to be
// INT, DOUBLE or STRING. "float" will not work.
// -----------------------------------------------------------------------------
struct WeatherData {
    // Temperature, Humidity, etc.
    double humidity = 0.0;
    double tempF = 0.0;

    double lastPressurePa = 0.0;
    double pressurePa = 0.0;
    int pressureDir = MERCURY_STEADY;

    // Wind
    // 0-360 instantaneous wind direction
    int windDirection = 0;

    // MPH instantaneous wind speed
    float windSpeedMPH = 0.0;

    // Average wind speed over NUM_WIND_SPEED_MEASUREMENTS measurements
    double windSpeedAvg = 0.0;

    // The last NUM_WIND_SPEED_MEASUREMENTS measurements
    float windSpeedMeasurements[NUM_WIND_SPEED_MEASUREMENTS];

    float windSpeedMax = 0.0;

    // Rain
    float rainByMinute[60];
    double rainPerHour = 0.0;
    double rainPerDay = 0.0;

    // Moon
    double lastMoonIllume = 0.0;
    double moonIllumination = 0.0;
    int moonWaxWane = MOON_UNKNOWN;
};

class WeatherShield {
  public:
    // Constructor
    WeatherShield();

    void begin(bool);
    void update();

    // Wind
    void windSpeedIRQ();
    int getWindDirection();
    float getWindSpeed();
    static char *windDirToCompasPoint(int);

    // Rain
    float getRainPerMinute();
    float getRainPerHour();
    float getRainPerDay();
    void rainIRQ();

    WeatherData *getWeather();

  private:
    HTU21D htu21d;
    MPL3115A2 mpl3115a2;

    int currMinute = 0;
    int currHour = 0;

    // Wind Measurement Vars
    int windSpeedMeasurementNum = 0;
    long lastWindCheck = 0;
    volatile long lastWindIRQ = 0;
    volatile byte windClicks = 0;

    // Rain Measurement Vars
    volatile unsigned long rainLastMeasure;

    // Misc
    long lastHistoricalDataCheck = 0;

    WeatherData data;

    void registerParticleVars();
};

#endif
