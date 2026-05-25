#ifndef VARIABLES_H
#define VARIABLES_H

#define OLED_I2C_ADDR 0x78

// Define analog pins
const int WIND_DIR_PIN = 34; // GPIO34  for analog input
const uint8_t RAINFALL_INTERRUPT_PIN = 25; // GPIO4 (can be any GPIO capable of interrupts)

// define variables
String output;
String displaytext;
uint16_t angleValue;
unsigned long time_trigger;


// Define GPIO pins for the interrupts
const uint8_t BUTTON_INTERRUPT_PIN = 13; // GPIO for button interrupt
const uint8_t WINDSPEED_INTERRUPT_PIN = 4; // GPIO for wind speed sensor

// Variables to track interrupt occurrences
volatile uint32_t buttonPulseCounter = 0;
volatile uint32_t windSpeedPulseCounter = 0;
volatile uint32_t rainfallPulseCounter = 0;     // Variable to track interrupt occurrences

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define RAINFALL_MM_CAL_CONST 1   //0.235
#define WINDSPEED_CAL_CONST     0.250

struct WeatherData {
  float CO2Equivalent;  // Carbon dioxide concentration in ppm
  float ambientTemp;    // Ambient temperature in °C
  float relHumidity;    // Relative humidity in %
  float airPressure;    // Air pressure in hPa
  float altitude;       // Altitude in meters
  float iaq;            // Indoor Air Quality index
  float windSpeed;      // Wind speed in m/s
  uint16_t windDir;        // Wind direction in degrees
  float rainfall_mm;    // Rainfall in mm
  float breathVocEqu;   // breath Voc equivalent
  float gasPercentage;  // gas percentage
};
WeatherData weather;    // weather data obj
String rawWindDir="   ";
int32_t as5600WindDir=0;

#endif