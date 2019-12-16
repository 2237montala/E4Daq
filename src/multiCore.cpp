// #include <Arduino.h>

// TaskHandle_t displayWebPage, recordData;

// void setup() {
//   Serial.begin(115200); 

//   //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
//   xTaskCreatePinnedToCore(
//                     displayWebPageCode,   /* Task function. */
//                     "Webpage",     /* name of task. */
//                     10000,       /* Stack size of task */
//                     NULL,        /* parameter of the task */
//                     1,           /* priority of the task */
//                     &displayWebPage,      /* Task handle to keep track of created task */
//                     0);          /* pin task to core 0 */                  
//   delay(500); 

//   //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
//   xTaskCreatePinnedToCore(
//                     recordDataCode,   /* Task function. */
//                     "Record",     /* name of task. */
//                     10000,       /* Stack size of task */
//                     NULL,        /* parameter of the task */
//                     1,           /* priority of the task */
//                     &recordData,      /* Task handle to keep track of created task */
//                     1);          /* pin task to core 1 */
//     delay(500); 
// }

// //Task1code: blinks an LED every 1000 ms
// void displayWebPageCode( void * pvParameters ){
  
// }

// //Task2code: blinks an LED every 700 ms
// void recordDataCode( void * pvParameters ){

// }

// void loop() {
  
// }
