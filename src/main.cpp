#include <Arduino.h>

// Setup TCC0 to capture pulse-width and period
volatile boolean periodComplete;
volatile uint16_t isrPeriod;
volatile uint16_t isrPulsewidth;
volatile uint32_t isrCount;
uint16_t period;
uint16_t pulsewidth;
uint32_t count;
uint32_t divisor = 12000000;

uint8_t muxA = 7;
uint8_t muxB = 9;
uint8_t muxEnable = 10;

const uint8_t numSensors = 4;
uint8_t currChannel = 0;
bool scanComplete = false;
uint32_t hzValsRec[numSensors];
uint32_t timeElapsed=0;


void stopTimer();
void startTimer();
void clearTimerVal();
void changeMuxInput(bool A, bool B);
void setMuxChannel(int channel);
void scanAllChannels();


void callback(){}                               // Dummy callback function

void setup()
{
  Serial.begin(115200);                  // Send data back on the Zero's native port
  while(!Serial);                        // Wait for the Serial port to be ready

  REG_PM_APBCMASK |= PM_APBCMASK_EVSYS;     // Switch on the event system peripheral
 
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |    // Divide the 48MHz system clock by 1 = 48MHz
                    GCLK_GENDIV_ID(5);      // Set division on Generic Clock Generator (GCLK) 5
  while (GCLK->STATUS.bit.SYNCBUSY);        // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |         // Enable GCLK 5
                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the clock source to 48MHz
                     GCLK_GENCTRL_ID(5);          // Set clock source on GCLK 5
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization*/

  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable the generic clock...
                     GCLK_CLKCTRL_GEN_GCLK5 |     // ....on GCLK5
                     GCLK_CLKCTRL_ID_TCC0_TCC1;   // Feed the GCLK5 to TCC0 and TCC1
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  REG_EIC_EVCTRL |= EIC_EVCTRL_EXTINTEO3;                                     // Enable event output on external interrupt 3
  attachInterrupt(12, callback, HIGH);                                        // Attach interrupts to digital pin 12 (external interrupt 3)
 
  REG_EVSYS_USER = EVSYS_USER_CHANNEL(1) |                                // Attach the event user (receiver) to channel 0 (n + 1)
                   EVSYS_USER_USER(EVSYS_ID_USER_TCC0_EV_1);              // Set the event user (receiver) as timer TCC0, event 1

  REG_EVSYS_CHANNEL = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |                // No event edge detection
                      EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                   // Set event path as asynchronous
                      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_3) |    // Set event generator (sender) as external interrupt 3
                      EVSYS_CHANNEL_CHANNEL(0);                           // Attach the generator (sender) to channel 0

  REG_TCC0_EVCTRL |= TCC_EVCTRL_MCEI1 |           // Enable the match or capture channel 1 event input
                     TCC_EVCTRL_MCEI0 |           //.Enable the match or capture channel 0 event input
                     TCC_EVCTRL_TCEI1 |           // Enable the TCC event 1 input
                     /*TCC_EVCTRL_TCINV1 |*/      // Invert the event 1 input         
                     TCC_EVCTRL_EVACT1_PPW;       // Set up the timer for capture: CC0 period, CC1 pulsewidth
                                       
  //NVIC_DisableIRQ(TCC0_IRQn);
  //NVIC_ClearPendingIRQ(TCC0_IRQn);
  NVIC_SetPriority(TCC0_IRQn, 0);      // Set the Nested Vector Interrupt Controller (NVIC) priority for TCC0 to 0 (highest)
  NVIC_EnableIRQ(TCC0_IRQn);           // Connect the TCC0 timer to the Nested Vector Interrupt Controller (NVIC)
 
  REG_TCC0_INTENSET = TCC_INTENSET_MC1 |            // Enable compare channel 1 (CC1) interrupts
                      TCC_INTENSET_MC0;             // Enable compare channel 0 (CC0) interrupts
 
  REG_TCC0_CTRLA |= TCC_CTRLA_CPTEN1 |              // Enable capture on CC1
                    TCC_CTRLA_CPTEN0 |              // Enable capture on CC0
                    TCC_CTRLA_PRESCALER_DIV4 |      // Set prescaler to 1, 48MHz/1 = 48MHz
                    TCC_CTRLA_ENABLE;               // Enable TCC0
  while (TCC0->SYNCBUSY.bit.ENABLE);                // Wait for synchronization

  pinMode(muxA,OUTPUT);
  pinMode(muxB,OUTPUT);
  setMuxChannel(0);

  Serial.println("Starting");
}

void loop()
{
  if (scanComplete && millis()-timeElapsed>=200)                             // Check if the period is complete
  {
    timeElapsed=millis();

    //Stop timer
    //stopTimer();

    for(int i = 0; i < numSensors;i++)
    {
      Serial.print(hzValsRec[i]);                      // Output the results
      Serial.print("   ");
    }
    Serial.println();
    scanComplete=false;

    //Start timer
    //startTimer();
  }
  else
  {
    scanAllChannels();
  }
  Serial.println("Fuck this shit");
}

void TCC0_Handler()                              // Interrupt Service Routine (ISR) for timer TCC0
{     
  // Check for match counter 0 (MC0) interrupt
  if (TCC0->INTFLAG.bit.MC0)             
  {   
    isrPeriod = REG_TCC0_CC0;                   // Copy the period
    periodComplete = true;                       // Indicate that the period is complete
  }

  // Check for match counter 1 (MC1) interrupt
  if (TCC0->INTFLAG.bit.MC1)           
  {
    isrPulsewidth = REG_TCC0_CC1;               // Copy the pulse-width
    isrCount++;
  }
}

void stopTimer()
{
  REG_TCC0_CTRLBSET = TCC_CTRLBSET_CMD_STOP;
  while(TCC0->SYNCBUSY.bit.CTRLB);
}

void startTimer()
{
  REG_TCC0_CTRLBSET = TCC_CTRLBSET_CMD_RETRIGGER;
  while(TCC0->SYNCBUSY.bit.CTRLB);
}

void clearTimerVal()
{
  REG_TCC0_COUNT = 0x0;
  while(TCC0->SYNCBUSY.bit.COUNT);
  //REG_TCC0_CTRLA |= TCC_CTRLA_PRESCSYNC_PRESC;
  //REG_TCC0_CC0 = 0x0;
  //REG_TCC0_CC1 = 0x0;
  //while(TCC0->SYNCBUSY.bit.CC0 && TCC0->SYNCBUSY.bit.CC1);
  //while(TCC0->SYNCBUSY.);
  //REG_TC3_CTRLA |= TC_CTRLA_PRESCSYNC_GCLK;    // Reload or reset the counter on next generic clock. Reset the prescaler counter

}

void setMuxChannel(int channel)
{
  switch (channel)
  {
  case 0:
    changeMuxInput(false,false);
    break;
  
  case 1:
    changeMuxInput(false,true);
    break;

  case 2:
    changeMuxInput(true,false);
    break;

  case 3:
    changeMuxInput(true,true);
    break;
  }
}

void changeMuxInput(bool A, bool B)
{
  digitalWrite(muxA,A);
  digitalWrite(muxB,B);
}

void scanAllChannels()
{
  int count = 0;
  while(count < numSensors)
  {
    if(periodComplete)
    {
      noInterrupts();                               // Read the new period and pulse-width
      period = isrPeriod;                   
      interrupts();

      //Save value in array
      hzValsRec[count] = divisor/period;

      //Move channels
      count++;
      //If channel is over the sensor number then reset it
      if(count > numSensors){
        //currChannel = 0;
        scanComplete = true;
      }
      setMuxChannel(count);

      
      // //Disable timer
      // stopTimer();

      // //Clear timer count
      // clearTimerVal();

      // //Start timer 
      // startTimer();
      // delayMicroseconds(50);
      periodComplete=false;
    }
  }
}