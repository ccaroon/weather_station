#include "WeatherShield.h"
#include "blynk.h"

#define UPDATE_INTERVAL 5 // in seconds

// Blynk Setup
char auth[] = "507a6402f7f44ba68919f42e2d38cc65";

double uptime = 0.0;
char uptimeUnit = 's';
long lastPublish = 0;

WeatherShield shield;

//------------------------------------------------------------------------------
void windSpeedIRQ() { shield.windSpeedIRQ(); }
void rainIRQ() { shield.rainIRQ(); }

//------------------------------------------------------------------------------
void setup() {
  delay(5000);
  Blynk.begin(auth);

  // Create Particle.variables for each piece of data for easy access
  // Particle.variable("humidity", humidity);

  // Initialize the WeatherShield
  shield.begin(7);

  // TODO: can this be moved to WS::begin() ?
  // attach external interrupt pins to IRQ functions
  attachInterrupt(RAIN, rainIRQ, FALLING);
  attachInterrupt(WIND_SPEED, windSpeedIRQ, FALLING);

  // turn on interrupts
  interrupts();
}
//---------------------------------------------------------------
void loop() {

  // TODO: move this function to the WeatherShield class
  WeatherData *data = shield.getWeather();
  Blynk.run();

  // Publishes every UPDATE_INTERVAL seconds
  if (millis() - lastPublish > UPDATE_INTERVAL * 1000) {
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

    // Choose which values you actually want to publish- remember, if you're
    // publishing more than once per second on average, you'll be throttled!
    // Particle.publish("humidity", String(humidity));

    // Send data to Blynk App
    Blynk.virtualWrite(1, data->tempF);
    Blynk.virtualWrite(2, data->humidity);
    Blynk.virtualWrite(3, data->pressurePa);
    Blynk.virtualWrite(4, data->baroTempF);
    Blynk.virtualWrite(5, data->altFeet);
    Blynk.virtualWrite(
        6, WeatherShield::windDirToCompasPoint(data->windDirection));
    Blynk.virtualWrite(7, data->windSpeedMPH);
    Blynk.virtualWrite(8, data->windSpeedAvg);
    Blynk.virtualWrite(9, data->windSpeedMax);

    char uptimeString[10];
    sprintf(uptimeString, "%.1f%c", uptime, uptimeUnit);
    Blynk.virtualWrite(10, uptimeString);
  }
}
