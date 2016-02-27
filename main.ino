#include "WeatherShield.h"
#include "blynk.h"

// Blynk Setup
char auth[] = "507a6402f7f44ba68919f42e2d38cc65";

// Weather Shield Setup
double humidity = 0;
double tempF = 0;
double pressurePa = 0;
double baroTempF = 0;
float altFeet = 0;
double uptime = 0.0;
char uptimeUnit = 's';

int windDirection = 0;  // [0-360 instantaneous wind direction]
float windSpeedMPH = 0; // [mph instantaneous wind speed]
long lastPublish = 0;

// Create Instance of HTU21D or SI7021 temp and humidity sensor and MPL3115A2
// barometric sensor
WeatherShield sensor;

//---------------------------------------------------------------
void setup() {
  delay(5000);
  Blynk.begin(auth);

  // Create Particle.variables for each piece of data for easy access
  // Particle.variable("humidity", humidity);
  // Particle.variable("tempF", tempF);
  // Particle.variable("pressurePa", pressurePa);
  // Particle.variable("baroTempF", baroTempF);

  pinMode(WIND_SPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
  /*pinMode(RAIN, INPUT_PULLUP); // input from wind meters rain gauge sensor*/

  // Initialize the I2C sensors and ping them
  sensor.begin();

  // These are additional MPL3115A2 functions that MUST be called for the sensor
  // to work.
  sensor.setOversampleRate(7); // Set Oversample rate
  // Call with a rate from 0 to 7. See page 33 for table of ratios.
  // Sets the over sample rate. Datasheet calls for 128 but you can set it
  // from 1 to 128 samples. The higher the oversample rate the greater
  // the time between data samples.

  sensor.enableEventFlags(); // Necessary register calls to enble temp, baro and
                             // alt

  // attach external interrupt pins to IRQ functions
  // attachInterrupt(RAIN, rainIRQ, FALLING);
  attachInterrupt(WIND_SPEED, windSpeedIRQ, FALLING);

  // turn on interrupts
  interrupts();
}
//---------------------------------------------------------------
void loop() {

  getWeather();
  Blynk.run();

  // This math looks at the current time vs the last time a publish happened
  // Publishes every 5000 milliseconds, or 5 seconds
  if (millis() - lastPublish > 5000) {
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
    // Particle.publish("tempF", String(tempF));
    // Particle.publish("pressurePa", String(pressurePa));
    // Particle.publish("baroTempF", String(baroTempF));

    Blynk.virtualWrite(1, tempF);
    Blynk.virtualWrite(2, humidity);
    Blynk.virtualWrite(3, pressurePa);
    Blynk.virtualWrite(4, baroTempF);
    Blynk.virtualWrite(5, altFeet);
    Blynk.virtualWrite(6, WeatherShield::windDirToCompasPoint(windDirection));
    // Blynk.virtualWrite(6, windDirection);
    Blynk.virtualWrite(7, windSpeedMPH);

    char uptimeString[10];
    sprintf(uptimeString, "%.1f%c", uptime, uptimeUnit);
    Blynk.virtualWrite(10, uptimeString);
  }
}

//------------------------------------------------------------------------------
void getWeather() {
  // Measure Relative Humidity from the HTU21D or Si7021
  humidity = sensor.getRH();

  // Measure Temperature from the HTU21D or Si7021
  // Temperature is measured every time RH is requested.
  // It is faster, therefore, to read it from previous RH
  // measurement with getTemp() instead with readTemp()
  tempF = sensor.getTempF();

  readBarometer();
  readAltimeter();

  // Calc windDirection
  windDirection = sensor.getWindDirection();

  // Calc windspeed
  windSpeedMPH = sensor.getWindSpeed();
}

/*You can only receive accurate barometric readings or accurate altitude
readings at a given time, not both at the same time. The following two lines
tell the sensor what mode to use. You could easily write a function that
takes a reading in one made and then switches to the other mode to grab that
reading, resulting in data that contains both accurate altitude and barometric
readings. For this example, we will only be using the barometer mode. Be sure
to only uncomment one line at a time. */
//------------------------------------------------------------------------------
// Measure pressure and temperature from MPL3115A2
void readBarometer() {
  sensor.setModeBarometer();
  baroTempF = sensor.readBaroTempF();
  pressurePa = sensor.readPressure();
}
//------------------------------------------------------------------------------
void readAltimeter() {
  sensor.setModeAltimeter();
  altFeet = sensor.readAltitudeFt();
}

void windSpeedIRQ() { sensor.windSpeedIRQ(); }
