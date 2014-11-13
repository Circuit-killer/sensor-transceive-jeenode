// receive data from a jeenode with some sensors on voltage dividers
#define RF69_COMPAT 1
#include <PortsLCD.h>
#include <JeeLib.h>

#define SERIAL_DEBUG 1
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
  #ifdef debug
  Serial.print(buf[0]); // sequence number
  Serial.print(buf[1]); // hall effect
  Serial.print("   ");
  Serial.print(buf[2]); //temperature
  Serial.print("   ");
  Serial.println(buf[3]); // voltage
  #endif
  float HE = ADCtoVoltage(buf[1], 5.7); // Convert to volts
  float TMP = CtoF(voltsToTemp(ADCtoVoltage((float) buf[2] / 100, 2.5))); // Convert the temperature in ADC units to C, then to archaic imperial units
  float Volts = buf[3] * 0.00117043121; // coefficient determined experimentally
  float output[] = {HE, TMP, Volts};
  lcd.print(output[0], 2); // 2 digits for the hall effect
  lcd.print(" ");
  lcd.print(output[1], 1); // 1 digit for the temperature
  lcd.print("  ");
  lcd.print(output[2], 2); // 2 digits for the voltage
  if (output[2] < 4.80){   // end of coin cell discharge curve
                           // there should still be ~4% in the batt's at the point
           lcd.setCursor(0, 0);
             lcd.print("Chng batt's now!");
 }
  delay(100);
}

void checkForDeadBattery() {
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    if (millis() - lastRX > TRANSMIT_DELAY + 10000) { // wait 10 seconds after last receive should have been received
      //lcd.clear(); leave last result on bottom of screen by not clearing
      lcd.setCursor(0, 0);
      lcd.print("                "); // dirty trick to prevent undercharacters
      lcd.setCursor(0, 0); //print on first line
      lcd.print("No RX for ");
      if ((millis() - lastRX) > 60L * 60L * 1000L) { // hr
        lcd.print((millis() - lastRX) / (60L * 60L * 1000L));
        lcd.print("  hr");
      }
      else if ((millis() - lastRX) > 60000L) {
        lcd.print((millis() - lastRX) / (60L * 1000L));
        lcd.print(" min");
      }
      else {
        lcd.print((millis() - lastRX) / 1000);
        lcd.print(" sec");
      }
    }
  }
}

float ADCtoVoltage(int rawValue, float ratio) {
  float ADCunits = 1.1 / 1024; // Using the internal 1.1V ADC
  float voltageDivRatio = ratio; // We have a voltage divider, we want to scale as if it isn't there. 1 if you don't have one.
  return rawValue * ADCunits * voltageDivRatio;
}

float voltsToTemp(float volts) {
  float degreesPerVolt = 0.02;
  return volts / degreesPerVolt;
}

float CtoF(float C) {
  return (C * 9.0 / 5) + 32;
}
