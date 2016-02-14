#include "blynk.h"
#include "SparkFun_Photon_Weather_Shield_Library.h"

// Blynk Setup
char auth[] = "507a6402f7f44ba68919f42e2d38cc65";

// Weather Shield Setup
double humidity   = 0;
double tempF      = 0;
double pressurePa = 0;
double baroTempF  = 0;
float  altFeet    = 0;
double uptime     = 0.0;
char   uptimeUnit = 's';

int wind_dir = 0; // [0-360 instantaneous wind direction]
float wind_speed_mpg = 0; // [mph instantaneous wind speed]
long last_wind_check = 0;
long lastPublish = 0;

int WDIR = A0;
/*int RAIN = D2;*/
int WSPEED = D3;

//Create Instance of HTU21D or SI7021 temp and humidity sensor and MPL3115A2 barometric sensor
Weather sensor;

//---------------------------------------------------------------
void setup()
{
    delay(5000);
    Blynk.begin(auth);

    // Create Particle.variables for each piece of data for easy access
    Particle.variable("humidity", humidity);
    Particle.variable("tempF", tempF);
    Particle.variable("pressurePa", pressurePa);
    Particle.variable("baroTempF", baroTempF);

    pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
    /*pinMode(RAIN, INPUT_PULLUP); // input from wind meters rain gauge sensor*/

    //Initialize the I2C sensors and ping them
    sensor.begin();

    //These are additional MPL3115A2 functions that MUST be called for the sensor to work.
    sensor.setOversampleRate(7); // Set Oversample rate
    //Call with a rate from 0 to 7. See page 33 for table of ratios.
    //Sets the over sample rate. Datasheet calls for 128 but you can set it
    //from 1 to 128 samples. The higher the oversample rate the greater
    //the time between data samples.

    sensor.enableEventFlags(); //Necessary register calls to enble temp, baro and alt

    // attach external interrupt pins to IRQ functions
    // attachInterrupt(RAIN, rainIRQ, FALLING);
    attachInterrupt(WSPEED, wspeedIRQ, FALLING);

    // turn on interrupts
    interrupts();
}
//---------------------------------------------------------------
void loop()
{
    //Get readings from all sensors
    getWeather();
    Blynk.run();

    // This math looks at the current time vs the last time a publish happened
    if(millis() - lastPublish > 5000) //Publishes every 5000 milliseconds, or 5 seconds
    {
        // Record when you published
        lastPublish = millis();

        //uptime - seconds
        uptime = (millis() / 1000);
        // uptime - minutes
        if (uptime >= 60) {
            uptime /= 60;
            uptimeUnit = 'm';
        }
        //uptime - hours
        if (uptime >= 60) {
            uptime /= 60;
            uptimeUnit = 'h';
        }

        // Choose which values you actually want to publish- remember, if you're
        // publishing more than once per second on average, you'll be throttled!
        Particle.publish("humidity", String(humidity));
        Particle.publish("tempF", String(tempF));
        Particle.publish("pressurePa", String(pressurePa));
        Particle.publish("baroTempF", String(baroTempF));

        Blynk.virtualWrite(1, tempF);
        Blynk.virtualWrite(2, humidity);
        Blynk.virtualWrite(3, pressurePa);
        Blynk.virtualWrite(4, baroTempF);
        Blynk.virtualWrite(5, altFeet);
        Blynk.virtualWrite(6, wind_dir_compass_point(wind_dir));
        Blynk.virtualWrite(7, wind_speed_mpg);

        char uptimeString[10];
        sprintf(uptimeString, "%.1f%c", uptime, uptimeUnit);
        Blynk.virtualWrite(10, uptimeString);
    }
}

char* wind_dir_compass_point(int dir) {
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
//------------------------------------------------------------------------------
void getWeather()
{
    // Measure Relative Humidity from the HTU21D or Si7021
    humidity = sensor.getRH();

    // Measure Temperature from the HTU21D or Si7021
    // Temperature is measured every time RH is requested.
    // It is faster, therefore, to read it from previous RH
    // measurement with getTemp() instead with readTemp()
    tempF = sensor.getTempF();

    readBarometer();
    readAltimeter();

    //Calc wind_dir
    wind_dir = get_wind_direction();

    //Calc windspeed
    wind_speed_mpg = get_wind_speed();
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
    baroTempF  = sensor.readBaroTempF();
    pressurePa = sensor.readPressure();
}
//------------------------------------------------------------------------------
void readAltimeter() {
    sensor.setModeAltimeter();
    altFeet = sensor.readAltitudeFt();
}

volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
void wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
{
  if (millis() - lastWindIRQ > 10) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
  }
}

//---------------------------------------------------------------
//Read the wind direction sensor, return heading in degrees
int get_wind_direction()
{
  unsigned int adc;

  adc = analogRead(WDIR); // get the current reading from the sensor

  // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
  // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  // Note that these are not in compass degree order! See Weather Meters datasheet for more information.

  //Wind Vains may vary in the values they return. To get exact wind direction,
  //it is recomended that you AnalogRead the Wind Vain to make sure the values
  //your wind vain output fall within the values listed below.
  if(adc > 2270 && adc < 2290) return (0);//North
  if(adc > 3220 && adc < 3299) return (1);//NE
  if(adc > 3890 && adc < 3999) return (2);//East
  if(adc > 3780 && adc < 3850) return (3);//SE

  if(adc > 3570 && adc < 3650) return (4);//South
  if(adc > 2790 && adc < 2850) return (5);//SW
  if(adc > 1580 && adc < 1610) return (6);//West
  if(adc > 1930 && adc < 1950) return (7);//NW

  return (-1); // error, disconnected?
}
//---------------------------------------------------------------
//Returns the instataneous wind speed
float get_wind_speed()
{
  float deltaTime = millis() - last_wind_check; //750ms

  deltaTime /= 1000.0; //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

  windClicks = 0; //Reset and start watching for new wind
  last_wind_check = millis();

  windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

  /* Serial.println();
   Serial.print("Windspeed:");
   Serial.println(windSpeed);*/

  return(windSpeed);
}
