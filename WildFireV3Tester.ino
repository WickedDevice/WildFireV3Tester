#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"
#include <WildFire.h>
#include <SPI.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
WildFire wf;
 
typedef struct {
  uint16_t task_timer;
  uint16_t task_period;
  void (*task)(void);
} task_t;
 
////////////////////////////////////////////////////////////////////
//                                                                //
// define NUM_TASKS to be >= the actual number of tasks defined!  //
//                                                                //
////////////////////////////////////////////////////////////////////
 
#define NUM_TASKS (8)
task_t tasks[NUM_TASKS] = {0};

void setupCC3000(void);
void testCC3000(void); 
void firmwareUpdateCC3000(void);
boolean testCC3000_enabled = false;
boolean firmwareUpdateCC3000_enabled = false;

void setupSdCard(void);
void testSdCard(void);
boolean testSdCard_enabled = false;

void setupRfm69(void);
void testRfm69(void); 
boolean testRfm69_enabled = false;

void setupSpiFlash(void);
void testSpiFlash(void); 
boolean testSpiFlash_enabled = false;

boolean testAllOutputs_enabled = false;
void testAllOutputs(void){
  static uint8_t pin = 0;
  if(testAllOutputs_enabled){
    int8_t previous_pin = ((int8_t) pin) - 1;
    if(previous_pin < 0){
      previous_pin = 31;  
    }
    
    pinMode(previous_pin, INPUT_PULLUP);    
    
    if(pin == 0 || pin == 1){
      Serial.end();
    }    
    else{
      Serial.begin(115200); 
    }
    
    // on a fully populated WildFire v3
    // Dig22 (CC3000 interrupt) and Dig2 (RFM69 interrupt)
    // are connected to buffer outputs
    // and should be treated as "Input Only"
    if(pin != 2 && pin != 22){
      digitalWrite(pin, LOW);    
      pinMode(pin, OUTPUT);        
    }
    
    
    pin++;
    if(pin >= 32){
      pin = 0; 
    }
  }
}

void setupBitlash(void);
void bitlashTask(void);

boolean terminateTests_enabled = false;
void terminateTests(void){
  if(terminateTests_enabled){
    Serial.println(F("Tests Terminated"));
    testCC3000_enabled = false;
    firmwareUpdateCC3000_enabled = false;
    testSdCard_enabled = false;
    testRfm69_enabled = false;
    testSpiFlash_enabled = false;
    testAllOutputs_enabled = false;
    
    terminateTests_enabled = false;
  }
}

void initializeScheduler(){
  /////////////////////////////////////////////////////////////////////////////
  //                                                                         //
  // statically set up the tasks here, e.g.:                                 //
  // tasks[0].task_period = 500;   // execute once every 500ms (technically  //
  //                               // "no more frequantly than every 500ms") //
  // tasks[0].task = &exampleTask; // call the function exampleTask          //
  //                                                                         //
  /////////////////////////////////////////////////////////////////////////////
  tasks[0].task_period = 1;
  tasks[0].task = &testAllOutputs;
  
  tasks[1].task_period = 1;
  tasks[1].task = &terminateTests;
  
  tasks[2].task_period = 100;
  tasks[2].task = &testCC3000;

  tasks[3].task_period = 100;
  tasks[3].task = &firmwareUpdateCC3000;
  
  tasks[4].task_period = 100;
  tasks[4].task = &testSdCard;

  tasks[5].task_period = 100;
  tasks[5].task = &testRfm69;

  tasks[6].task_period = 100;
  tasks[6].task = &testSpiFlash;
  
  tasks[7].task_period = 1;
  tasks[7].task = &bitlashTask;
  
  TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30); // prescaler=64, enable CTC mode
  OCR3A = 250;                                 // compare match every 250 ticks
  TIMSK3 = _BV(OCIE3A);                        // enable compare match ISR
}
 
// initializeScheduler sets this ISR up to fire once per millisecond
ISR(TIMER3_COMPA_vect){
  for(uint8_t ii = 0; ii < NUM_TASKS; ii++){
    if(tasks[ii].task_timer > 0) tasks[ii].task_timer--;
  }
}
 
// call this from loop()
void executeTasks(){
  for(uint8_t ii = 0; ii < NUM_TASKS; ii++){
    if(tasks[ii].task != 0){
      if(tasks[ii].task_timer == 0){
        tasks[ii].task();
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
          tasks[ii].task_timer = tasks[ii].task_period;
        }
      }
    }
  }
}

// Arduino structures
void setup(){
  wf.begin();
  setupBitlash();
  setupSdCard();  
  setupSpiFlash();
  initializeScheduler();
}
 
void loop(){
  executeTasks();
}