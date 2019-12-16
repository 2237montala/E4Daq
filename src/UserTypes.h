#ifndef UserTypes_h
#define UserTypes_h
#include <Arduino.h>
// User data types.  Modify for your data items.
#define FILE_BASE_NAME "data"
const uint8_t RPM_DIM = 1;
struct data_t {
  uint32_t time;
  uint16_t rpm[RPM_DIM];
};
void acquireData(data_t* data);
void printData(Print* pr, data_t* data);
void printHeader(Print* pr);
void userSetup();
uint16_t getRPMSensorData(uint8_t address);
#endif  // UserTypes_h