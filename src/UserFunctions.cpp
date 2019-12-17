#include "UserTypes.h"
#include <Wire.h>
#include "Adafruit_LIS3DH.h"
#include "Adafruit_Sensor.h"
// User data functions.  Modify these functions for your data items.

// Start time for data
static uint32_t startMicros;

//RPM Sensor addresses
uint8_t rpmSensorsAdd[RPM_DIM] = {0,1};

Adafruit_LIS3DH accel = Adafruit_LIS3DH();

// Acquire a data record.
void acquireData(data_t* data) 
{
  data->time = micros();

  accel.read();
  for(int i = 0; i < RPM_DIM; i++)
  {
    data->rpm[i] = getRPMSensorData(rpmSensorsAdd[i]);
    delayMicroseconds(75);
  }
  data->accel[0] = accel.x;
  data->accel[1] = accel.y;
  data->accel[2] = accel.z;
}

// Print a data record.
void printData(Print* pr, data_t* data) {
  if (startMicros == 0) {
    startMicros = data->time;
  }
  pr->print(data->time - startMicros);
  for (int i = 0; i < RPM_DIM; i++) {
    pr->write(',');
    pr->print(data->rpm[i]);
  }
  for (int i = 0; i < 3; i++) {
    pr->write(',');
    pr->print(data->accel[i]);
  }
  pr->println();
}

// Print data header.
void printHeader(Print* pr) {
  startMicros = 0;
  pr->print(F("micros"));
  for (int i = 0; i < RPM_DIM; i++) {
    pr->print(F(",adc"));
    pr->print(i);
  }
  pr->print(F(",x accel"));
  pr->print(F(",y accel"));
  pr->print(F(",z accel"));
  pr->println();
}

// Sensor setup
void userSetup()
{
    accel.begin(0x18);
    delay(10);
    accel.setRange(LIS3DH_RANGE_4_G);

    //Wire.begin();
    delay(10);
}

uint16_t getRPMSensorData(uint8_t address)
{
    Wire.requestFrom(address,RPM_SENSOR_BYTES);
    
    //Give time for sensor to respond
    while(!Wire.available());

    //Reaad in data
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    return (msb << 8) + lsb;
}