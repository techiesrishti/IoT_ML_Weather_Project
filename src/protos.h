#ifndef PROTOS_H
#define PROTOS_H

// Helper functions declarations

void checkIaqSensorStatus(void);
void errLeds(void);
void sendToExcel(void);
uint16_t readAngle(uint8_t pin);
void initAnalog(void);
void updateAllInterrupts(void);
void initInterrupts(void);
void initDisplay(void);
void oledDisplayLine(String s, int16_t lineno);
void sequentialScroll();
void updateData(WeatherData &data);
void Timer(void);
float calculateAltitude(float pressure);
void sendTelemetry();
#endif