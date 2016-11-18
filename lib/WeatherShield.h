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

// NOTE: Any data point that you want to use as a Particle Variable needs to be
// INT, DOUBLE or STRING. "float" will not work.
struct WeatherData {
    // Temperature, Humidity, etc.
    double humidity = 0.0;
    double tempF = 0.0;
    double pressurePa = 0.0;

    // Wind
    int windDirection = 0;    // 0-360 instantaneous wind direction
    float windSpeedMPH = 0.0; // MPH instantaneous wind speed
    float windSpeedAvg = 0.0; // Average wind speed over 100 measurements
    float windSpeedMeasurements[100]; // Last 100 wind speed measurements
    float windSpeedMax = 0.0;         // Max wind speed per minute

    // Rain
    float rainByMinute[60];
    double rainPerHour = 0.0;
    double rainPerDay = 0.0;

    // Moon
    double moonIllumination = 0.0;
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

    int updateInterval = 1; // in seconds
    unsigned long seconds;
    unsigned long minutes;

    // Wind Measurement Vars
    long lastWindCheck = 0;
    volatile long lastWindIRQ = 0;
    volatile byte windClicks = 0;

    // Rain Measurement Vars
    volatile unsigned long rainLastMeasure;

    WeatherData data;

    void registerParticleVars();
};

#endif
