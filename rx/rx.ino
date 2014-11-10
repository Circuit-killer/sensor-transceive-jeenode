
// JN_SimpleReceive.pde   Illustrates receiving data from JN_SimpleSend.pde
// updated 6-4-2012 by paul badger from code by <jcw@equi4.com>
// http://opensource.org/licenses/mit-license.php
// Sketch to receive data from SimpleSend
#define RF69_COMPAT 1
#include <PortsLCD.h>
#include <JeeLib.h>

#define SERIAL_DEBUG 1

/************************ important note ***************************/
#define TRANSMIT_DELAY 9000   //  This must be 2.15 Transmit Delay in transmitter sketch
#define screen_width 16
#define screen_height 2

PortI2C myI2C (1);
LiquidCrystalI2C lcd (myI2C);

MilliTimer timer;
unsigned long lastRX, lastCheck;

void setup() {
  Serial.begin(57600);
  lcd.begin(screen_width, screen_height);


  lcd.clear();
  lcd.print("PropaneReceive");
  Serial.println("[PropaneReceive]");
  delay(1000);
  rf12_initialize(18, RF12_915MHZ, 212); // params are byte nodeId, byte freqBand, byte netGroup
  // freqBands  should be RF12_915MHZ, or RF12_433MHZ
  // nodeId parameter should be in range of to 1-26
  // netGroup parameter should be in range of to 1-212
  // red dots on radios are 915Mhz, green dots are 434 Mhz
  //lcd.clear();
  
 lcd.setCursor(0, 0);
   lcd.clear();
 lcd.print("Init Done");
 delay(1000);
  lastRX = 0xFFFFFFFF;
}

void loop() {
  checkForDeadBattery();
  // rf12_recvDone() is true if new information has been received.
  // re12_crc == 0 indicates good transmission, checks validity of data
  if (rf12_recvDone() && rf12_crc == 0) {  // received good data if true
    lastRX = millis();
    processData();

  }
}

void processData() {
  int* buf = (int*) rf12_data;       // buf is a pointer to our data
  //int* CurrentRecv = buf;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hall, Temp, BattV");
  lcd.setCursor(0, 1);
  //Serial.print(buf[0]);          // sequence number
  // lcd.print(buf[0]);
  // Serial.print("  ");
  // lcd.print(" ");
  float HE = buf[1]; // No calculation here, yet
  float TMP = CtoF(ADCtoDegC((float) buf[2] / 100)); // Convert the temperature in ADC units to C, then to archaic imperial units
  float Volts = buf[3] * 0.00117043121; // coefficient determined experimentally
  float output[] = {HE, TMP, Volts};
  Print(output, SERIAL_DEBUG);
  delay(100);
}

void checkForDeadBattery() {
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    if (millis() - lastRX > TRANSMIT_DELAY + 10000) { // wait 10 seconds after last receive should have been received
      lcd.clear();
      lcd.setCursor(0, 0); //print on first line
      lcd.print("No RX for ");
      if ((millis() - lastRX) > 60L*60L*1000L){  // hr
       lcd.print((millis() - lastRX)/ (60L*60L*1000L));
       lcd.print("hr");                
      }
      if ((millis() - lastRX) > 60000L){
       lcd.print((millis() - lastRX)/60000L);
       lcd.print("min");                
      }
      
      else {
      lcd.print((millis() - lastRX)/1000);
      lcd.print("sec");
      }
    }

  }
}

void Print(float *buf, bool serialDebug) {
  int bufsize = (sizeof buf) + 1;
  //prints using the LCD, space length " "
  for (int i = 0; i < bufsize; i++) {
    lcd.print(buf[i], 1);
    lcd.print(" ");
    if (serialDebug) {
      Serial.print(buf[i]);
      Serial.print("   ");
    }
  }
  if (serialDebug) {
    Serial.println();
  }
}

float ADCtoDegC(int rawTemp) {
  float degreesPerVolt = 0.02;
  float ADCunits = 1.1 / 1024; // Using the internal 1.1V ADC
  float voltageDivRatio = 2.5; // We have a voltage divider, 100k on one side, 10k on the other, so it divides the voltage by 2.5
  return ((rawTemp * ADCunits * voltageDivRatio) / degreesPerVolt);
}

float CtoF(float C) {
  return (C * 9.0 / 5) + 32;
}
