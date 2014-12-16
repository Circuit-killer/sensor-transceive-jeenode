#define RF69_COMPAT 1    // define RF69_COMPAT 1  MUST BE FIRST
#include <JeeLib.h>
#include <avr/sleep.h>
#define DEBUG 0
#define GRP 212
#define NODE 3
//#define TRANSMIT_DELAY 360000 // Transmit delay in JC units, 1 JC = ~2.5milliseconds
//#define TRANSMIT_DELAY 1000
#define TRANSMIT_DELAY 10 // in minutes
#define sensorPowerPin 4
#define tempPin A3
#define voltagePin A1
#define hallEffectPin A0


// old pin definitions
/*#define sensorPowerPin 5
#define tempPin A0
#define voltagePin A3
#define hallEffectPin A1
*/


// A struct with the sort of data we're going to send.
struct {
  int sequence; // a counter
  int hallEffectValue; // light sensor
  int temperature; // from the TMP37
  int battVoltage;  // battery voltage
} payload;

MilliTimer timer;

EMPTY_INTERRUPT(WDT_vect); // just wakes us up to resume

static void watchdogInterrupts (char mode) {
  MCUSR &= ~(1 << WDRF); // only generate interrupts, no reset
  cli();
  WDTCSR |= (1 << WDCE) | (1 << WDE); // start timed sequence
  WDTCSR = mode >= 0 ? bit(WDIE) | mode : 0;
  sei();
}


static void lowPower (byte mode) {
  // prepare to go into power down mode
  set_sleep_mode(mode);
  // disable the ADC
  byte prrSave = PRR, adcsraSave = ADCSRA;
  ADCSRA &= ~ bit(ADEN);
  PRR |=  bit(PRADC);
  // zzzzz...
  sleep_mode();
  // re-enable the ADC
  PRR = prrSave;
  ADCSRA = adcsraSave;
}

// End of power-save code.

void setup() {
  #ifdef DEBUG 
  Serial.begin(57600);
  Serial.print("\n[propaneDemo]");
  #endif
  rf12_initialize(NODE, RF12_915MHZ, GRP); //node, frequency, net group
  analogReference(INTERNAL); // use the 1.1v internal reference voltage

  Sleepy::loseSomeTime(1000);  //uC to sleep
  //pinMode(sensorPowerPin, OUTPUT);
}

void loop() {
  // spend most of the waiting time in a low-power sleep mode
  // note: the node's sense of time is no longer 100% accurate after sleeping
 

  lowPower(SLEEP_MODE_IDLE);  // still not running at full power
  digitalWrite(sensorPowerPin, HIGH); // enable the power pin to read
  payload.sequence++;
  pinMode(tempPin, INPUT); // for temperature
  pinMode(voltagePin, INPUT); // for voltage
  pinMode(hallEffectPin, INPUT); // for hall effect sensor
  payload.battVoltage = analogRead(voltagePin) * 10;
  analogRead(hallEffectPin); // turn on the ADC
  // let the ADC get stable for a bit, may need to be much longer for a hall effect sensor
  // to become stable
  delay(20);
  payload.hallEffectValue = analogRead(hallEffectPin); // read hall effect
  payload.temperature = analogRead(tempPin)*100; // read temperature
  digitalWrite(sensorPowerPin, LOW); // got our result, turn off power

#ifdef DEBUG // Serial debug dump payload
  Serial.print((int) payload.hallEffectValue);
  Serial.print("   ");
  Serial.print((int) payload.temperature);
  Serial.print("   ");
  Serial.println((int) payload.battVoltage);
  Serial.flush();
#endif

  rf12_sleep(RF12_WAKEUP);         // turn radio back on at the last moment

  MilliTimer wait;                 // radio needs some time to power up, why?
  while (!wait.poll(3)) {    // was 5
    rf12_recvDone();
    lowPower(SLEEP_MODE_IDLE);
  }

  rf12_sendNow(0, &payload, sizeof payload);
  rf12_sendWait(1); // sync mode!
  rf12_sleep(RF12_SLEEP);          // turn the radio off
  
 for (int i = 0; i < TRANSMIT_DELAY; i++){
      while (!timer.poll(30000))  // delay is twice as long as its supposed to be
  // for some reason. We didn't bother getting to the bottom of this - may be processor speed
  // is wrong for some reason.
    Sleepy::loseSomeTime(timer.remaining()); // go into a (controlled) comatose state
 }

}
