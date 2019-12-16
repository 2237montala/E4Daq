#include "UserTypes.h"
#include <Wire.h>
// User data functions.  Modify these functions for your data items.

// Start time for data
static uint32_t startMicros;

//RPM Sensor addresses
const uint8_t RPM_SENSOR_BYTES = 2;
uint8_t rpmSensorsAdd[RPM_DIM] = {8};

// Acquire a data record.
void acquireData(data_t* data) {
  data->time = micros();
  
    for(int i = 0; i < RPM_DIM; i++)
    {
        data->rpm[i] = getRPMSensorData(rpmSensorsAdd[i]);
    }
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
  pr->println();
}

// Sensor setup
void userSetup() {
    Wire.begin();
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