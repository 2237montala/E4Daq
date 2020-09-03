#include <Arduino.h>

//Almost all the code comes from https://forum.arduino.cc/index.php?topic=396804.0 which is in SAMD21
//Converting to SAMD51 was done with examples from https://forum.arduino.cc/index.php?topic=589655.0

// NEED TO CROSS REFERENCE THIS ARDUINO FORUM LINK TO CORRECT THIS CODE
// I LOST THE FINAL VERSION SO THIS VERSION IS BEFORE I MADE THE FORUM POST
// https://forum.arduino.cc/index.php?topic=673692.0


//#define timerClkDiv TCC_CTRLA_PRESCALER_DIV4

// Setup TCC2 to capture pulse-width and period with DMAC transfer
volatile uint32_t dmacPeriod1; //TCC0 period
volatile uint32_t dmacPeriod2; //TCC1 period

typedef struct                              // DMAC descriptor structure
{
  uint16_t btctrl;
  uint16_t btcnt;
  uint32_t srcaddr;
  uint32_t dstaddr;
  uint32_t descaddr;
} dmacdescriptor ;

volatile dmacdescriptor wrb[12] __attribute__ ((aligned (16)));               // Write-back DMAC descriptors
dmacdescriptor descriptor_section[12] __attribute__ ((aligned (16)));         // DMAC channel descriptors
dmacdescriptor descriptor __attribute__ ((aligned (16)));                     

//Loop variables
double divisor = 120000000.0 / 4; // Value is divide cc value to convert to seconds
uint32_t prevTime;                // Print if statement last ran time
#define interval 100               // in mS

void setup()
{
  Serial.begin(115200);                  // Send data back on the Zero's native port
  while(!Serial);                        // Wait for the Serial port to be ready
 
  // Enable DMAC /////////////////////////////////////////////////////////////////////////
  DMAC->CTRL.bit.DMAENABLE = 0;                                     // Disable DMA to edit it
  DMAC->BASEADDR.reg = (uint32_t)descriptor_section;                // Set the descriptor section base 
  DMAC->WRBADDR.reg = (uint32_t)wrb;                                // Set the write-back descriptor base adddress
  DMAC->CTRL.reg = DMAC_CTRL_LVLEN(0xF) | DMAC_CTRL_DMAENABLE;      // Enable all priority levels and enable DMA 
  
  // Set up DMAC Channel 0 /////////////////////////////////////////////////////////////////////
  DMAC->Channel[0].CHCTRLA.reg = DMAC_CHCTRLA_BURSTLEN_SINGLE |           // Set up burst length to single burst
                                 DMAC_CHCTRLA_TRIGACT_BURST   |           // Trigger a transfer on burst
                                 DMAC_CHCTRLA_TRIGSRC(TCC0_DMAC_ID_MC_0); // Source is TCC cc0
  DMAC->Channel[0].CHPRILVL.bit.PRILVL  = DMAC_PRICTRL0_LVLPRI0_Pos;      // Priority is 0 (lowest)

  // Set up DMAC Channel 0 transfer descriptior (what is being transfered to memory)
  descriptor.descaddr = (uint32_t)&descriptor_section[0];               // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&TCC0->CC[0].reg;                      // Take the contents of the TCC0 counter comapare 0 register
  descriptor.dstaddr = (uint32_t)&dmacPeriod1;                           // Copy it to the "dmacPeriod" variable
  descriptor.btcnt = 1;                                                 // Transfer only takes 1 beat
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD | DMAC_BTCTRL_VALID;   // Copy 16-bits (HWORD) and flag discriptor as valid
  memcpy(&descriptor_section[0], &descriptor, sizeof(dmacdescriptor));  // Copy to the channel 0 
  
  // Set up DMAC Channel 1 /////////////////////////////////////////////////////////////////////
  DMAC->Channel[1].CHCTRLA.reg = DMAC_CHCTRLA_BURSTLEN_SINGLE |           // Set up burst length to single burst
                                 DMAC_CHCTRLA_TRIGACT_BURST   |           // Trigger a transfer on burst
                                 DMAC_CHCTRLA_TRIGSRC(TCC1_DMAC_ID_MC_0); // Source is TCC cc0
  DMAC->Channel[1].CHPRILVL.bit.PRILVL  = DMAC_PRICTRL0_LVLPRI0_Pos;      // Priority is 0 (lowest)

  // // Set up DMAC Channel 1 transfer descriptior (what is being transfered to memory)
  descriptor.descaddr = (uint32_t)&descriptor_section[1];               // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&TCC1->CC[0].reg;                      // Take the contents of the TCC0 counter comapare 0 register
  descriptor.dstaddr = (uint32_t)&dmacPeriod2;                           // Copy it to the "dmacPeriod" variable
  descriptor.btcnt = 1;                                                 // Transfer only takes 1 beat
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD | DMAC_BTCTRL_VALID;   // Copy 16-bits (HWORD) and flag discriptor as valid
  memcpy(&descriptor_section[1], &descriptor, sizeof(dmacdescriptor));  // Copy to the channel 1 

  // Set up TCC0 clock /////////////////////////////////////////////////////////////////////////

  // GCLK->GENCTRL[5].reg = GCLK_GENCTRL_DIV(1) |       // Divide the 120MHz clock source by divisor 1: 120MHz/1 = 120MHz
  //                        GCLK_GENCTRL_IDC |          // Set the duty cycle to 50/50 HIGH/LOW
  //                        GCLK_GENCTRL_GENEN |        // Enable GCLK5
  //                        GCLK_GENCTRL_SRC_DFLL;      // Select 48MHz DFLL clock source
  // while (GCLK->SYNCBUSY.bit.GENCTRL5);               // Wait for synchronization

  // EIC Interrupt System ///////////////////////////////////////////////////////////////////////////////

  // Enable the port multiplexer on analog pin A4
  PORT->Group[g_APinDescription[A4].ulPort].PINCFG[g_APinDescription[A4].ulPin].bit.PMUXEN = 1;
 
  // Set-up the pin as an EIC (interrupt) peripheral on analog pin A4
  PORT->Group[g_APinDescription[A4].ulPort].PMUX[g_APinDescription[A4].ulPin >> 1].reg |= PORT_PMUX_PMUXE(0);

  // Set up A3 at an input and an external interrupt
  PORT->Group[g_APinDescription[A3].ulPort].PINCFG[g_APinDescription[A3].ulPin].bit.PMUXEN = 1;
  PORT->Group[g_APinDescription[A3].ulPort].PMUX[g_APinDescription[A3].ulPin >> 1].reg |= PORT_PMUX_PMUXE(0);
 
  EIC->CTRLA.bit.ENABLE = 0;                        // Disable the EIC peripheral
  while (EIC->SYNCBUSY.bit.ENABLE);                 // Wait for synchronization
  EIC->CONFIG[0].reg = EIC_CONFIG_SENSE4_HIGH;      // Set event on detecting a HIGH level
  EIC->EVCTRL.reg = 1 << 4;                         // Enable event output on external interrupt 4
  EIC->INTENCLR.reg = 1 << 4;                       // Clear interrupt on external interrupt 4
  EIC->ASYNCH.reg = 1 << 4;                         // Set-up interrupt as asynchronous input
  //EIC->CTRLA.bit.ENABLE = 1;                        // Enable the EIC peripheral

  EIC->CONFIG[0].reg |= EIC_CONFIG_SENSE5_HIGH;      // Set event on detecting a HIGH level
  EIC->EVCTRL.reg |= 1 << 5;                         // Enable event output on external interrupt 5
  EIC->INTENCLR.reg |= 1 << 5;                       // Clear interrupt on external interrupt 5
  EIC->ASYNCH.reg |= 1 << 5;                         // Set-up interrupt as asynchronous input
  EIC->CTRLA.bit.ENABLE = 1;                        // Enable the EIC peripheral

  while (EIC->SYNCBUSY.bit.ENABLE);                 // Wait for synchronization
 
  // Set up clock input for TCC0
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC0 perhipheral channel
                                    GCLK_PCHCTRL_GEN_GCLK0;    // Connect 48MHz generic clock 5 to TCC0

  // Set up clock input for TCC1
  GCLK->PCHCTRL[TCC1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC1 perhipheral channel
                                    GCLK_PCHCTRL_GEN_GCLK0;    // Connect 48MHz generic clock 5 to TCC1

  // Event System ///////////////////////////////////////////////////////////////////////////////

  MCLK->APBBMASK.reg |= MCLK_APBBMASK_EVSYS;         // Switch on the event system peripheral
 
  // Select the event system user on channel 0 (USER number = channel number + 1)
  EVSYS->USER[EVSYS_ID_USER_TCC0_EV_1].reg = EVSYS_USER_CHANNEL(1);         // Set the event user (receiver) as timer TCC0
  EVSYS->USER[EVSYS_ID_USER_TCC1_EV_1].reg = EVSYS_USER_CHANNEL(2);         // Set the event user (receiver) as timer TCC1

  // Select the event system generator on channel 0
  EVSYS->Channel[0].CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |              // No event edge detection
                                  EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                 // Set event path as asynchronous
                                  EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_4);   // Set event generator (sender) as external interrupt 4

  // Select the event system generator on channel 1
  EVSYS->Channel[1].CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |              // No event edge detection
                                  EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                 // Set event path as asynchronous
                                  EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_5);   // Set event generator (sender) as external interrupt 5      

  // Set up TCC0 as a pwp
  TCC0->EVCTRL.reg |= TCC_EVCTRL_MCEI1 |           // Enable the match or capture channel 1 event input
                      TCC_EVCTRL_MCEI0 |           //.Enable the match or capture channel 0 event input
                      TCC_EVCTRL_TCEI1 |           // Enable the TCC event 1 input
                      /*TCC_EVCTRL_TCINV1 |*/      // Invert the event 1 input         
                      TCC_EVCTRL_EVACT1_PPW;       // Set up the timer for capture: CC0 period, CC1 pulsewidth

  TCC0->INTENSET.reg = TCC_INTENSET_MC1 |               // Enable compare channel 1 (CC1) interrupts
                       TCC_INTENSET_MC0;                // Enable compare channel 0 (CC0) interrupts

  TCC0->CTRLA.reg |= TCC_CTRLA_CPTEN1 |              // Enable capture on CC1
                     TCC_CTRLA_CPTEN0 |              // Enable capture on CC0
                     TCC_CTRLA_PRESCALER_DIV4;      // Set prescaler to 1, 48MHz/1 = 48MHz

  // Set up TCC1 as a pwp
  TCC1->EVCTRL.reg |= TCC_EVCTRL_MCEI1 |           // Enable the match or capture channel 1 event input
                      TCC_EVCTRL_MCEI0 |           //.Enable the match or capture channel 0 event input
                      TCC_EVCTRL_TCEI1 |           // Enable the TCC event 1 input
                      /*TCC_EVCTRL_TCINV1 |*/      // Invert the event 1 input         
                      TCC_EVCTRL_EVACT1_PPW;       // Set up the timer for capture: CC0 period, CC1 pulsewidth

  TCC1->INTENSET.reg = TCC_INTENSET_MC1 |               // Enable compare channel 1 (CC1) interrupts
                       TCC_INTENSET_MC0;                // Enable compare channel 0 (CC0) interrupts

  TCC1->CTRLA.reg |= TCC_CTRLA_CPTEN1 |              // Enable capture on CC1
                     TCC_CTRLA_CPTEN0 |              // Enable capture on CC0
                     TCC_CTRLA_PRESCALER_DIV4;      // Set prescaler to 1, 48MHz/1 = 48MHz



  TCC0->CTRLA.bit.ENABLE = 0x1;
  while(TCC0->SYNCBUSY.bit.ENABLE); //Wait for TCC0 reg sync

  TCC1->CTRLA.bit.ENABLE = 0x1;
  while(TCC1->SYNCBUSY.bit.ENABLE); //Wait for TCC0 reg sync

  Serial.println("Starting PPW on TCC0");

  //Enabling DMAC channel 0
  DMAC->Channel[0].CHCTRLA.bit.ENABLE = 0x1;
  Serial.println("Starting DMAC channel 0");

  DMAC->Channel[1].CHCTRLA.bit.ENABLE = 0x1;
  Serial.println("Starting DMAC channel 1");

}

double periodToHz(uint32_t period, long clkDivisior) {
  if(period != 0) {
    return clkDivisior/period;
  } else {
    return 0;
  }
}

void loop()
{
  if(millis() > prevTime + interval) {
    prevTime = millis();
    double freq = periodToHz(dmacPeriod1,divisor);
    Serial.println(freq);                      // Output the results

    freq = periodToHz(dmacPeriod2,divisor);
    Serial.println(freq);
    Serial.println();

    dmacPeriod1 = 0;
    dmacPeriod2 = 0;
  }
}
