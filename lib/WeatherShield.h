#include "application.h"

#ifndef WeatherShield_h
#define WeatherShield_h

// ----- Si7021 & HTU21D Definitions -----
#define ADDRESS 0x40

#define TEMP_MEASURE_HOLD 0xE3
#define HUMD_MEASURE_HOLD 0xE5
#define TEMP_MEASURE_NOHOLD 0xF3
#define HUMD_MEASURE_NOHOLD 0xF5
#define TEMP_PREV 0xE0

#define WRITE_USER_REG 0xE6
#define READ_USER_REG 0xE7
#define SOFT_RESET 0xFE

#define HTRE 0x02
#define _BV(bit) (1 << (bit))

// Shifted Polynomial for CRC check
#define CRC_POLY 0x988000

// Error codes
#define I2C_TIMEOUT 998
#define BAD_CRC 999

// ----- MPL3115A2 Definitions -----
// Unshifted 7-bit I2C address for sensor
#define MPL3115A2_ADDRESS 0x60

#define STATUS 0x00
#define OUT_P_MSB 0x01
#define OUT_P_CSB 0x02
#define OUT_P_LSB 0x03
#define OUT_T_MSB 0x04
#define OUT_T_LSB 0x05
#define DR_STATUS 0x06
#define OUT_P_DELTA_MSB 0x07
#define OUT_P_DELTA_CSB 0x08
#define OUT_P_DELTA_LSB 0x09
#define OUT_T_DELTA_MSB 0x0A
#define OUT_T_DELTA_LSB 0x0B
#define WHO_AM_I 0x0C
#define F_STATUS 0x0D
#define F_DATA 0x0E
#define F_SETUP 0x0F
#define TIME_DLY 0x10
#define SYSMOD 0x11
#define INT_SOURCE 0x12
#define PT_DATA_CFG 0x13
#define BAR_IN_MSB 0x14
#define BAR_IN_LSB 0x15
#define P_TGT_MSB 0x16
#define P_TGT_LSB 0x17
#define T_TGT 0x18
#define P_WND_MSB 0x19
#define P_WND_LSB 0x1A
#define T_WND 0x1B
#define P_MIN_MSB 0x1C
#define P_MIN_CSB 0x1D
#define P_MIN_LSB 0x1E
#define T_MIN_MSB 0x1F
#define T_MIN_LSB 0x20
#define P_MAX_MSB 0x21
#define P_MAX_CSB 0x22
#define P_MAX_LSB 0x23
#define T_MAX_MSB 0x24
#define T_MAX_LSB 0x25
#define CTRL_REG1 0x26
#define CTRL_REG2 0x27
#define CTRL_REG3 0x28
#define CTRL_REG4 0x29
#define CTRL_REG5 0x2A
#define OFF_P 0x2B
#define OFF_T 0x2C
#define OFF_H 0x2D

// Wind and Rain
#define WIND_DIR A0
#define WIND_SPEED D3

#define RAIN D2
#define RAIN_PER_DUMP 0.011

struct WeatherData {
  // Temp, Humidity, etc.
  double humidity = 0.0;
  double tempF = 0.0;
  double pressurePa = 0.0;
  double baroTempF = 0.0;
  float altFeet = 0.0;

  // Wind
  int windDirection = 0;            // 0-360 instantaneous wind direction
  float windSpeedMPH = 0.0;         // MPH instantaneous wind speed
  float windSpeedAvg = 0.0;         // Average wind speed over 100 measurements
  float windSpeedMeasurements[100]; // Last 100 wind speed measurements
  float windSpeedMax = 0.0;         // Max wind speed per minute

  // Rain
  float rainByMinute[60];
  float rainPerHour = 0.0;
  float rainPerDay = 0.0;
};

class WeatherShield {
public:
  // Constructor
  WeatherShield();

  void begin(byte);
  void update();

  // Si7021 & HTU21D Public Functions
  float getRH();
  float readTemp();
  float getTemp();
  float readTempF();
  float getTempF();
  void heaterOn();
  void heaterOff();
  void changeResolution(uint8_t i);
  void reset();
  uint8_t checkID();

  // MPL3115A2 Public Functions
  // Returns float with meters above sealevel. Ex: 1638.94
  float readAltitude();
  // Returns float with feet above sealevel. Ex: 5376.68
  float readAltitudeFt();
  // Returns float with barometric pressure in Pa. Ex: 83351.25
  float readPressure();
  // Returns float with current temperature in Celsius. Ex: 23.37
  float readBaroTemp();
  // Returns float with current temperature in Fahrenheit. Ex: 73.96
  float readBaroTempF();
  // Puts the sensor into Pascal measurement mode.
  void setModeBarometer();
  // Puts the sensor into altimetery mode.
  void setModeAltimeter();
  // Puts the sensor into Standby mode. Required when changing CTRL1 register.
  void setModeStandby();
  // Start taking measurements!
  void setModeActive();
  // Sets the # of samples from 1 to 128. See datasheet.
  void setOversampleRate(byte);
  // Sets the fundamental event flags. Required during setup.
  void enableEventFlags();

  void readBarometer();
  void readAltimeter();

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

  uint16_t makeMeasurment(uint8_t command);
  // Si7021 & HTU21D Private Functions
  void writeReg(uint8_t value);
  uint8_t readReg();

  // MPL3115A2 Private Functions
  void toggleOneShot();
  byte IICRead(byte regAddr);
  void IICWrite(byte regAddr, byte value);
};
#endif
