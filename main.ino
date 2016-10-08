#include "MoonPhase.h"
#include "WeatherShield.h"
#include "blynk.h"
#include <math.h>

#define UPDATE_INTERVAL 5 // in seconds

// Blynk Setup
char auth[] = "24714b31d1d5415aab822e75614c7595";

// Devel Token
// char auth[] = "1c7a55f21f734ad4a0d0c035a50ee7c0";

double uptime = 0.0;
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
    shield.begin(false);

    // attach external interrupt pins to IRQ functions
    attachInterrupt(RAIN, rainIRQ, FALLING);
    attachInterrupt(WIND_SPEED, windSpeedIRQ, FALLING);

    // turn on interrupts
    interrupts();

    Time.zone(-4);
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

        // uptime - seconds
        uptime = (millis() / 1000);
        // uptime - minutes
        if (uptime >= 60) {
            uptime /= 60;
            uptimeUnit = 'm';
        }
        // uptime - hours
        if (uptime >= 60) {
            uptime /= 60;
            uptimeUnit = 'h';
        }
        // uptime - days
        if (uptimeUnit == 'h' && uptime >= 24) {
            uptime /= 24;
            uptimeUnit = 'd';
        }

        // Send data to Blynk App
        Blynk.virtualWrite(1, data->tempF);
        Blynk.virtualWrite(2, data->humidity);
        Blynk.virtualWrite(3, data->pressurePa);
        // Pressure in Inches of Mercury
        Blynk.virtualWrite(4, data->pressurePa * 0.0002953);

        Blynk.virtualWrite(
            6, WeatherShield::windDirToCompasPoint(data->windDirection));
        Blynk.virtualWrite(7, data->windSpeedMPH);
        Blynk.virtualWrite(8, data->windSpeedAvg);
        Blynk.virtualWrite(9, data->windSpeedMax);

        char uptimeString[10];
        sprintf(uptimeString, "%.1f%c", uptime, uptimeUnit);
        Blynk.virtualWrite(10, uptimeString);

        Blynk.virtualWrite(11, data->rainPerHour);

        // year, month, day, hour
        double phase = MoonPhase::moon_phase(Time.year(), Time.month(),
                                             Time.day(), Time.hour());
        phase = floor(phase * 1000 + 0.5) / 10;
        Blynk.virtualWrite(12, phase);

        Blynk.virtualWrite(14, data->rainPerDay);

        // Turn off Virtual LED
        Blynk.virtualWrite(25, 0);
    }
}
