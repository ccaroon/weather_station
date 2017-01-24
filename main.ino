#include "MoonPhase.h"
#include "WeatherShield.h"
#include "blynk.h"
#include <math.h>

#define UPDATE_INTERVAL 5 // in seconds

// Blynk Setup
char auth[] = "24714b31d1d5415aab822e75614c7595";

// Devel Token
// char auth[] = "1c7a55f21f734ad4a0d0c035a50ee7c0";

long startUpTime = 0;
char uptimeUnit = 's';
long lastPublish = 0;

WeatherShield shield;

//------------------------------------------------------------------------------
void windSpeedIRQ() {
    shield.windSpeedIRQ();
}
void rainIRQ() {
    shield.rainIRQ();
}

//------------------------------------------------------------------------------
void setup() {
    delay(5000);
    Blynk.begin(auth);

    // Initialize the WeatherShield
    shield.begin(true);

    // attach external interrupt pins to IRQ functions
    attachInterrupt(RAIN, rainIRQ, FALLING);
    attachInterrupt(WIND_SPEED, windSpeedIRQ, FALLING);

    // turn on interrupts
    interrupts();

    Time.zone(-4);
    startUpTime = Time.now();
}

//------------------------------------------------------------------------------
String durationToString(long start, long end) {

    // duration between start & end in seconds
    long duration = end - start;

    int days = duration / 86400;
    duration = duration % 86400;

    int hours = duration / 3600;
    duration = duration % 3600;

    int mins = duration / 60;
    int secs = duration % 60;

    char dStr[12];
    sprintf(dStr, "%02d:%02d:%02d:%02d", days, hours, mins, secs);

    return (String(dStr));
}
//------------------------------------------------------------------------------
void loop() {
    Blynk.run();

    // Publishes every UPDATE_INTERVAL seconds
    if (millis() - lastPublish > UPDATE_INTERVAL * 1000) {
        // Turn on Virtual LED
        Blynk.virtualWrite(25, 255);

        // Get Weather
        WeatherData *data = shield.getWeather();

        // Record when you published
        lastPublish = millis();

        // Send data to Blynk App
        Blynk.virtualWrite(1, data->tempF);
        Blynk.virtualWrite(2, data->humidity);
        Blynk.virtualWrite(3, data->pressurePa);
        // Pressure in Inches of Mercury
        // const char *arrow;
        // if (data->pressureDir == MERCURY_FALLING) {
        //     arrow = "↓";
        // } else if (data->pressureDir == MERCURY_RISING) {
        //     arrow = "↑";
        // } else if (data->pressureDir == MERCURY_STEADY) {
        //     arrow = "⇕";
        //     // arrow = "↕";
        // }
        String pressure = String::format("%0.2f", data->pressurePa * 0.0002953);
        Blynk.virtualWrite(4, pressure);

        Blynk.virtualWrite(
            6, WeatherShield::windDirToCompasPoint(data->windDirection));
        Blynk.virtualWrite(7, data->windSpeedMPH);
        Blynk.virtualWrite(8, data->windSpeedAvg);
        Blynk.virtualWrite(9, data->windSpeedMax);

        Blynk.virtualWrite(10, durationToString(startUpTime, Time.now()));

        Blynk.virtualWrite(11, data->rainPerHour);

        // year, month, day, hour
        double phase = MoonPhase::moon_phase(Time.year(), Time.month(),
                                             Time.day(), Time.hour());
        data->moonIllumination = floor(phase * 1000 + 0.5) / 10;
        Blynk.virtualWrite(12, data->moonIllumination);

        Blynk.virtualWrite(14, data->rainPerDay);

        // Turn off Virtual LED
        Blynk.virtualWrite(25, 0);
    }
}
