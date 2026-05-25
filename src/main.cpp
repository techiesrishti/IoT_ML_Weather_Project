/*code for 1)connecting to wifi 2)connecting to thingsboard 3)displaying data on display
  4)displaying data on thingsboard 5)sending data on excel*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <Streaming.h>
#include <Adafruit_BME680.h>
#include "bsec.h"
#include "variables.h"
#include "protos.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <AS5600.h>


AS5600L as5600;  

// Wi-Fi Credentials
const char* ssid = "tapse";        
const char* pass = "haikyuuu";    

// ThingsBoard Cloud Configuration
const char* thingsboardServer = "www.iotrendz.com"; 
const int mqttPort = 1883;  
const char* accessToken = "rhWg5IWBbsETBQU0zBk8";        

WiFiClient wifiClient;
PubSubClient client(wifiClient);

int attempts = 0; // Counter for Wi-Fi connection attempts
// Function to connect to Wi-Fi
void initWiFi() {
  Serial.println("Starting WiFi connection...");
  WiFi.mode(WIFI_STA);

  while ((WiFi.status() != WL_CONNECTED) && (attempts < 36)) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(5000);
    attempts++;
    Serial.print(".");
    if (attempts == 35) {
      Serial.println("NO WIFI. Restarting...");
      ESP.restart();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WIFI.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

// Function to connect to ThingsBoard
void initThingsBoard() {
  Serial.println("Connecting to ThingsBoard...");
  client.setServer(thingsboardServer, 1883); // ThingsBoard MQTT broker on port 1883

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection to ThingsBoard...");
    if (client.connect("NodeMCU", accessToken, NULL)) {
      Serial.println("Connected to ThingsBoard!");
    } else {
      Serial.print("Failed to connect. MQTT Error code: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
   sendTelemetry();
}

// Create an object of the class Bsec
Bsec iaqSensor;
// Adafruit_SSD1306 display(128, 64, &Wire, -1); // Adjust reset pin if necessary
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display1(OLED_RESET);

// Task to be executed inside the Rainfall interrupt
void IRAM_ATTR handleRainfallInterrupt() {
  static unsigned long currenttime=0;
  static unsigned long lastIntTime=0;
  currenttime = millis();
  if((currenttime -lastIntTime) > 250 ) { // debounce
     rainfallPulseCounter++;
     lastIntTime = currenttime;
  }
}
// Task to be executed inside the button interrupt
void IRAM_ATTR handleButtonInterrupt() {
  static unsigned long currenttime=0;
  static unsigned long lastIntTime=0;
  currenttime = millis();
  if((currenttime -lastIntTime) > 200 ) { // debounce
     buttonPulseCounter++;
     lastIntTime = currenttime;
  }
    
}
// Task to be executed inside the wind speed interrupt
void IRAM_ATTR handleWindSpeedInterrupt() {
  static unsigned long currenttime=0;
  static unsigned long lastIntTime=0;
  currenttime = millis();
  if((currenttime -lastIntTime) > 200 ) { // debounce
     windSpeedPulseCounter++;
     lastIntTime = currenttime;
     
  }
    
    
}
// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.bsecStatus != BSEC_OK) {
    if (iaqSensor.bsecStatus < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme68xStatus != BME68X_OK) {
    if (iaqSensor.bme68xStatus < BME68X_OK) {
      output = "BME68X error code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
    }
  }
}
void errLeds(void)
{
  // pinMode(PC13, OUTPUT);
  // digitalWrite(PC13, HIGH);
  // delay(100);
  // digitalWrite(PC13, LOW);
  // delay(100);
  Serial << "Error BME" << endl;
  display1.clearDisplay();
  display1.print(F("BME ERROR"));
  display1.display();
  delay(2000);


}
uint16_t readAngle(uint8_t pin) {
    const float ADC_RESOLUTION = 4095.0; // 12-bit ADC resolution (0-4095)
    const float MAX_VOLTAGE = 3.3;       // Max ADC voltage reference (in volts)
    const float MAX_ANGLE = 360.0;       // Maximum angle (degrees)

    // Read the analog value from the pin
    uint16_t analogValue = analogRead(pin);

    // Convert the ADC value to voltage
    float voltage = (analogValue * MAX_VOLTAGE) / ADC_RESOLUTION;

    // Convert the voltage to angle
    float angle = (voltage * MAX_ANGLE) / MAX_VOLTAGE;

    // Return the angle as an integer
    return (uint16_t)angle;
}

void initAnalog(void)
{
   // Configure analog input pin
    pinMode(WIND_DIR_PIN, INPUT);
    analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
    analogSetAttenuation(ADC_11db); // Set attenuation to handle higher voltage (max ~3.6V)
  
}

void initAS5600(){

  as5600.begin();  //  set direction pin.
  as5600.setAddress(0x36);
  as5600.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
  //int b = as5600.isConnected();
  //Serial.print("Connect: ");
  //Serial.println(b);
}

void initIqSensor(){
  iaqSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
  //output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  //Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[13] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE};

  iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

}
void initI2C(){
  Wire.begin();
}
void updateAllInterrupts(void)
{
  static uint32_t lastButtonCount = 0;
  static uint32_t lastWindSpeedCount = 0;
  static uint32_t lastRainfallCount = 0;

  // Check for button interrupt updates
  if (buttonPulseCounter != lastButtonCount)
  {
    lastButtonCount = buttonPulseCounter;
    Serial.print("Button Interrupt Triggered! Count: ");
    Serial.println(buttonPulseCounter);
  }

  // Check for wind speed interrupt updates
  if (windSpeedPulseCounter != lastWindSpeedCount)
  {
    lastWindSpeedCount = windSpeedPulseCounter; 
    //Serial.print("Wind Speed Pulses: ");
    //Serial.println(windSpeedPulseCounter);
  }

  // Check for wind speed interrupt updates
  if (rainfallPulseCounter != lastRainfallCount)
  {
    lastRainfallCount = rainfallPulseCounter;
    //Serial.print("Rain Fall Pulses: ");
    //Serial.println(rainfallPulseCounter);
  }
}
void initInterrupts(void)
{
   // Attach interrupt to the pin
    pinMode(RAINFALL_INTERRUPT_PIN, INPUT_PULLUP);    // Configure the interrupt pin as input
    attachInterrupt(digitalPinToInterrupt(RAINFALL_INTERRUPT_PIN), handleRainfallInterrupt, FALLING);
    Serial.println("Interrupt configured. Waiting for triggers...");

    // Configure the button interrupt pin
    pinMode(BUTTON_INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_INTERRUPT_PIN), handleButtonInterrupt, FALLING);

    // Configure the wind speed interrupt pin
    pinMode(WINDSPEED_INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(WINDSPEED_INTERRUPT_PIN), handleWindSpeedInterrupt, FALLING);

    Serial.println("Interrupts configured. Waiting for triggers..."); 
}
void initDisplay(void)
{
 // Initialize OLED display
 if(!display1.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display1.display();
  display1.clearDisplay();
  display1.setTextSize(2);
  display1.setTextColor(SSD1306_WHITE);
  delay(1000);
    // Display data on OLED
  display1.clearDisplay();
  delay(2000);
  display1.setCursor(0, 0);
  displaytext = "Weather Station";
  oledDisplayLine(displaytext,1);
  display1.display(); // updates display only if this function is called
  delay(2000);
}
void oledDisplayLine(String s,int16_t lineno) {
  int16_t y = (lineno -1) * 16;
 // display1.clearDisplay();
  display1.setTextSize(2); // Draw 2X-scale text
  display1.setTextColor(SSD1306_WHITE);
  display1.setCursor((128 - (s.length() * 12)) / 2, y); // Adjusted for text size 2
  display1.println(s);
  display1.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
 // display1.startscrollleft(0x00, 0x0F);
   
}
void oledBotLine(String s) {
  display1.clearDisplay();
  display1.setTextSize(2); // Draw 2X-scale text
  display1.setTextColor(SSD1306_WHITE);
  display1.setCursor((128 - (s.length() * 12)) / 2, 16); // Adjusted for text size 2
  display1.println(s);
  display1.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
 // display1.startscrollleft(0x00, 0x0F);
   
}
// Function to  sequential display data on the OLED
void sequentialScroll()
{
#define INTERVAL 3000
#define MESSAGECOUNTS 11
  // List of strings to scroll
  String msgTopLine[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
  String msgBotLine[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};

  static unsigned long previousMillis = 0;
  static uint8_t msgno = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL)
  {
    msgTopLine[0] = "IAQ";
    msgBotLine[0] = String(weather.iaq);
    msgTopLine[1] = "Temp [C]";
    msgBotLine[1] = String(weather.ambientTemp);
    msgTopLine[2] = "Humidity";
    msgBotLine[2] = String(weather.relHumidity);
    msgTopLine[3] = "Gas %";
    msgBotLine[3] = String(weather.gasPercentage);
    msgTopLine[4] = "CO2 Equ";
    msgBotLine[4] = String(weather.CO2Equivalent);
    msgTopLine[5] = "Air Press";
    msgBotLine[5] = String(weather.airPressure);
    msgTopLine[6] = "Altitude";
    msgBotLine[6] = String(weather.altitude);
    msgTopLine[7] = "WindSpeed";
    msgBotLine[7] = String(weather.windSpeed);
    msgTopLine[8] = "WindDir";
    msgBotLine[8] = String(weather.windDir);
    msgTopLine[9] = "Rainfall";
    msgBotLine[9] = String(weather.rainfall_mm) + String(" mm");
    msgTopLine[10] = "BeathVoc Eq";
    msgBotLine[10] = String(weather.breathVocEqu);

    previousMillis = currentMillis;
 
    display1.clearDisplay();
    oledDisplayLine(msgTopLine[msgno], 1); // Scroll the current message
    oledDisplayLine(msgBotLine[msgno], 2); // Scroll the current message
    msgno++;                               // Move to the next message

    if (msgno >= MESSAGECOUNTS)
    {
      msgno = 0; // Loop back to the first message
    }
  }
}
//======== Rain sensor
void updateData(WeatherData &data) {
  // Update weather data fields
  data.CO2Equivalent = iaqSensor.co2Equivalent;
  data.ambientTemp = iaqSensor.temperature;
  data.relHumidity = iaqSensor.humidity;
  data.airPressure = iaqSensor.pressure/100;
  data.altitude = weather.altitude;
  data.iaq = iaqSensor.iaq;
  data.windSpeed = WINDSPEED_CAL_CONST * windSpeedPulseCounter; //  
  data.windDir = (uint16_t)as5600WindDir;   // xxx to define
  data.rainfall_mm = RAINFALL_MM_CAL_CONST * rainfallPulseCounter; // xxx to define
  data.breathVocEqu = iaqSensor.breathVocEquivalent;
  data.gasPercentage = iaqSensor.gasPercentage;
}
void Timer(void) {
const unsigned long ONE_SECOND = 1000;
const unsigned long ONE_MINUTE = 60000;
const unsigned long ONE_HOUR = 3600000;
const unsigned long TWENTY_FOUR_HOURS = 86400000;

// Variables to track the last time each timer triggered
static unsigned long lastSecondTimer = 0;
static unsigned long lastMinuteTimer = 0;
static unsigned long lastHourTimer = 0;
static unsigned long lastDayTimer = 0;
  // Get the current time
  unsigned long currentMillis = millis();

  // 1 Second Timer
  if (currentMillis - lastSecondTimer >= ONE_SECOND) {
    lastSecondTimer += ONE_SECOND;
    // // your update functions for  every oneSecondRoutine
    //Serial.println(as5600.getCumulativePosition());
  }

  // 1 Minute Timer
  if (currentMillis - lastMinuteTimer >= ONE_MINUTE) {
    lastMinuteTimer += ONE_MINUTE;
     // your update functions for  every oneMinute
     windSpeedPulseCounter = 0;     // reset every minute
     //Serial.println("one minute over") ;
  }

  // 1 Hour Timer
  if (currentMillis - lastHourTimer >= ONE_HOUR) {
    lastHourTimer += ONE_HOUR;
    // your update functions for every oneHour
  }

  // 24 Hours Timer
  if (currentMillis - lastDayTimer >= TWENTY_FOUR_HOURS) {
    lastDayTimer += TWENTY_FOUR_HOURS;
    // your update functions for every twentyFourHourRoutine
    rainfallPulseCounter =0;        // reset every day 
  }
}
// ===============================================
float calculateAltitude(float pressure ) {

    // Constants for the barometric formula
    const float R = 8.31432;         // Universal gas constant (J/(mol*K))
    const float M = 0.0289644;       // Molar mass of Earth's air (kg/mol)
    const float g = 9.80665;         // Gravitational acceleration (m/s^2)
    const float T0 = 288.15;         // Standard temperature at sea level (K)
    const float L = -0.0065;         // Temperature lapse rate (K/m)
    float seaLevelPressure = 101325.0;

    // Check for invalid pressure values
    if (pressure <= 0 || seaLevelPressure <= 0) {
        return -1.0; // Return -1 to indicate error
    }

    // Barometric formula
    float altitude = (T0 / L) * (1.0 - pow(pressure / seaLevelPressure, (R * L) / (g * M)));
    
    return altitude; // Altitude in meters
}
void calcWindDirection(){

  uint16_t rAngle = as5600.rawAngle();
  uint16_t deg =(uint16_t) rAngle * AS5600_RAW_TO_DEGREES;
  weather.windDir=deg;
  //Serial << as5600.readAngle() << "\t" << rAngle << "\t" << deg << endl; 
/*
  weather.windDir=(float)deg;
  if(weather.windDir>=0 && weather.windDir<=11.25){
    rawWindDir='N';
  }
  else if(weather.windDir>11.25 && weather.windDir<=33.75){
    rawWindDir="NNE";
  }
  else if(weather.windDir>33.75 && weather.windDir<=56.25){
    rawWindDir="NE";
  }
  else if(weather.windDir>56.25 && weather.windDir<=78.75){
    rawWindDir="ENE";
  }
  else if(weather.windDir>78.75 && weather.windDir<=101.75){
    rawWindDir='E';
  }
  else if(weather.windDir>101.75 && weather.windDir<=123.75){
    rawWindDir="ESE";
  }
  else if(weather.windDir>123.75 && weather.windDir<=146.25){
    rawWindDir="SE";
  }
  else if(weather.windDir>146.25 && weather.windDir<=168.75){
    rawWindDir="SSE";
  }
  else if(weather.windDir>168.75 && weather.windDir<=191.25){
    rawWindDir='S';
  }
  else if(weather.windDir>191.25 && weather.windDir<=213.7){
    rawWindDir="SSW";
  }
  else if(weather.windDir>213.7 && weather.windDir<=236.25){
    rawWindDir="SW";
  }
  else if(weather.windDir>236.25 && weather.windDir<=258.75){
    rawWindDir="WSW";
  }
  else if(weather.windDir>258.75 && weather.windDir<=281.25){
    rawWindDir='W';
  }
  else if(weather.windDir>281.25 && weather.windDir<=303.75){
    rawWindDir="WNW";
  }
  else if(weather.windDir>303.75 && weather.windDir<=326.25){
    rawWindDir="NW";
  }
  else if(weather.windDir>326.25 && weather.windDir<=348.75){
    rawWindDir="NNW";
  }
  else{
    rawWindDir="error";
  }
  */
}
void sendToExcel(void)
{
    Serial<<weather.ambientTemp<<","<<weather.relHumidity<<","<<weather.iaq<<","<<weather.airPressure<<","<<weather.rainfall_mm<<","<<weather.windSpeed<<","<<weather.windDir<<endl;
}
// Function to send telemetry data to ThingsBoard
void sendTelemetry() {
  // Create telemetry payload in JSON format
  String payload = "{\"temperature\":";
  payload += weather.ambientTemp;
  payload += ",\"humidity\":";
  payload += weather.relHumidity;
  payload += ",\"IAQ\":";
  payload += weather.iaq;
  payload += ",\"Air Pressure\":";
  payload += weather.airPressure;
  payload += ",\"Rainfall\":";
  payload += weather.rainfall_mm;
  payload += ",\"Wind Direction\":";
  payload += weather.windDir;
  payload += ",\"Wind Speed\":";
  payload += weather.windSpeed;
  payload += ",\"Altitude\":";
  payload += weather.altitude;
  payload += ",\"CO2\":";
  payload += weather.CO2Equivalent;
  payload += ",\"Gas Percentage\":";
  payload += weather.gasPercentage; 
  payload += "}";
  client.publish("v1/devices/me/telemetry", payload.c_str());
}
void setup() {
  Serial.begin(115200);
  delay(100);

  initWiFi();        
  initThingsBoard(); 

  initAnalog();
  initInterrupts();

  initI2C();
  initDisplay();

  initIqSensor();
  initAS5600();
  //as5600WindDir=as5600.getCumulativePosition(true);
}

void loop() {
  

  if (!client.connected()) {
    initThingsBoard(); // Reconnect to ThingsBoard if disconnected
  }
  client.loop(); // Keep MQTT connection alive

  time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available
  } else {
    checkIaqSensorStatus();
  }
  //  set initial position
  


  updateAllInterrupts();
  weather.altitude =  calculateAltitude(iaqSensor.pressure);
  
  updateData(weather);      // updates the required output parameters        
  
  Timer();
  calcWindDirection();
  sendToExcel();
  sequentialScroll();
  sendTelemetry();
  delay(5000);
  
}
