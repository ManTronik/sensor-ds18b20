
/*
 * Board       :ATtinyMicroControllers - 2 choix Attiny25/45/85 
 * Processor  :Attiny85
 * Clock      : 1Mhz
 * 
 * 
 * 
 */

/**************
 *   config   *
 **************/

// time until the watchdog wakes the mc in seconds
#define WATCHDOG_TIME 2 // 1, 2, 4 or 8

// after how many watchdog wakeups we should collect and send the data
#define WATCHDOG_WAKEUPS_TARGET 8 // 8 * 8 = 64 seconds between each data collection

// after how many data collections we should get the battery status
#define BAT_CHECK_INTERVAL 30

// min max values for the ADC to calculate the battery percent
#define BAT_ADC_MIN 40  // ~0,78V
#define BAT_ADC_MAX 225 // ~4,41V

#define ONEWIRE_BUSS 4
// pin with the LED connected
//#define LED_PIN 0  //  attiny85 - pin 5 - port 0




/**************
 * end config *
 **************/

#include <OneWire.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>


#if BAT_ADC_MIN >= BAT_ADC_MAX
  #error BAT_ADC_MAX must be greater than BAT_ADC_MIN!
#endif

// some mcs (e.g atmega328) have WDTCSR instead of WTDCR
#if !defined WDTCR and defined WDTCSR
  #define WDTCR WDTCSR
#endif

// counter for the battery check, starting at BAT_CHECK_INTERVAL to transmit the battery status on first loop
uint8_t bat_check_count = BAT_CHECK_INTERVAL;

float tempMax = -100  ;       //store Maximum Temperature level Sonde-1: Intérieure
float tempMin = 20   ;       //store Maximum temperature level

 
OneWire TemperatureSensor(ONEWIRE_BUSS); 

#include <LCD5110_Basic.h>


//                     __________ SCK (CLK)
//                    /  ________ MOSI (DIN)
//                   /  /  ______ DC (register select)
//                  /  /  /  ____ RST
//                 /  /  /  /  __ CS (CE)
//                /  /  /  /  /
LCD5110 myGLCD(0, 1, 2, 3, 6); //D6 don't exist - conect CS to GND

extern uint8_t BigNumbers[];
extern uint8_t SmallFont[];


void enableWatchdog()
{
  cli();
  
  // clear the reset flag
  MCUSR &= ~(1<<WDRF);
  
  // set WDCE to be able to change/set WDE
  WDTCR |= (1<<WDCE) | (1<<WDE);

  // set new watchdog timeout prescaler value
  #if WATCHDOG_TIME == 1
    WDTCR = 1<<WDP1 | 1<<WDP2;
  #elif WATCHDOG_TIME == 2
    WDTCR = 1<<WDP0 | 1<<WDP1 | 1<<WDP2;
  #elif WATCHDOG_TIME == 4
    WDTCR = 1<<WDP3;
  #elif WATCHDOG_TIME == 8
    WDTCR = 1<<WDP0 | 1<<WDP3;
  #else
    #error WATCHDOG_TIME must be 1, 2, 4 or 8!
  #endif
  
  // enable the WD interrupt to get an interrupt instead of a reset
  WDTCR |= (1<<WDIE);
  
  sei();
}

// function to go to sleep

void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
}






void setup() {

  
   myGLCD.InitLCD();
   myGLCD.clrScr();
  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, LOW);
  
   // setup the ADC
  ADMUX =
    (1 << ADLAR) | // left shift result
    (0 << REFS1) | // Sets ref. voltage to VCC, bit 1
    (0 << REFS0) | // Sets ref. voltage to VCC, bit 0
    (0 << MUX3)  | // use ADC1 for input (PB2), MUX bit 3
    (0 << MUX2)  | // use ADC1 for input (PB2), MUX bit 2
    (0 << MUX1)  | // use ADC1 for input (PB2), MUX bit 1
    (1 << MUX0);   // use ADC1 for input (PB2), MUX bit 0
  ADCSRA =
    (1 << ADEN)  | // enable ADC
    (1 << ADPS2) | // set prescaler to 64, bit 2
    (1 << ADPS1) | // set prescaler to 64, bit 1
    (0 << ADPS0);  // set prescaler to 64, bit 0

  // disable ADC for powersaving
  ADCSRA &= ~(1<<ADEN);

  // disable analog comperator for powersaving
  ACSR |= (1<<ACD);

  
  //enterSleep();

  
  // enable the watchdog
  enableWatchdog();
  
}

void loop(void) {


       
  
// myGLCD.setFont(BigNumbers);
   myGLCD.setFont(SmallFont);


 
    byte i;
    byte data[12];
    int16_t raw;
    float t;
 
    TemperatureSensor.reset();       // reset one wire buss
    TemperatureSensor.skip();        // select only device
    TemperatureSensor.write(0x44);   // start conversion
 
    delay(1000);                     // wait for the conversion
 
    TemperatureSensor.reset();
    TemperatureSensor.skip();
    TemperatureSensor.write(0xBE);   // Read Scratchpad
    
    for ( i = 0; i < 9; i++) {       // 9 bytes
      data[i] = TemperatureSensor.read();
    }
 
    raw = (data[1] << 8) | data[0];
    t = (float)raw / 16.0;


       //calculate min and max
         if (t < tempMin) {    // here is the problem... minimum won't store
          tempMin = t;   //compare old temperature with old minimum
          //digitalWrite(LED_PIN, HIGH);
          
          }
   
          if (t >= tempMax) {    //if higher then current temperature, store current temperatute as MAX Temperature
           tempMax = t;
         } 
 myGLCD.clrScr();
 myGLCD.setFont(BigNumbers);
 //myGLCD.setFont(SmallFont);
 myGLCD.printNumF(t, 1, CENTER, 14 );


//  myGLCD.print(num, 3, CENTER,0); // Print the value of “num” with 3 fractional digits top centered

 myGLCD.setFont(SmallFont);
 myGLCD.printNumF(tempMin, 1, LEFT , 40);
 myGLCD.printNumF(tempMax, 1, RIGHT, 40  );

 // deep sleep
 for(uint8_t i=0;i < WATCHDOG_WAKEUPS_TARGET;i++){
    enterSleep();
 }

  
} // end loop 





// watchdog ISR
ISR(WDT_vect){
  // nothing to do here, just wake up
}

  

 
