/* This program sends RC instructions to the Seatalk network.
*/

#include <RCSwitch.h>
#include <SoftwareSerial.h>

// Pin-definitions
const int receiverPin = 14; // RC-receiver
const int buzzerPin = 27;   // Beeper
#define TX_PIN 32           // TX pin for UART communication to ST 
#define RX_PIN 33           // RX pin for UART communication from ST 
#define TX_LED 25           // LED that is ON during TX

#define MAX_BUF_SIZE 20

// RC and UART objects
RCSwitch mySwitch = RCSwitch();
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// Codes of the RC-buttons (A: big yellow RC, B: Small RC)
const long long buttonCodeA[] = {8049169, 8049170, 8049172, 8049176}; 
const long long buttonCodeB[] = {753832, 753828, 753826, 753825};

// Threshold values (x 0.1 sec) that determine short, middle, long and very-long 
int thresholdsPushButton[] = {5, 15, 30};

// Buffer-array for SeaTalk commands
uint8_t stCmd [4];

// ---------- Functie-declarations ----------
// int  measurePushDuration();
// void waitOrBeep(int timeStep, int count);
// void beep(int duration);
// void handleButton(long long value, int lengthPush);
// int  pushDuration(int lengthPush);
// void send2ST(uint8_t* data);
// void CheckBus();



// ---------- Setup ----------
void setup() {
  // Create a serial monitor for debugging
  Serial.begin(115200);         
  delay(500);

  // Create the UART port using SoftwareSerial
  mySerial.begin(4800, SWSERIAL_8S1, RX_PIN, TX_PIN, true); 
  delay(500);

  // Create RC receiver 
  mySwitch.enableReceive(digitalPinToInterrupt(receiverPin)); 
  
  // set some pins to output
  pinMode(buzzerPin, OUTPUT);                
  pinMode(TX_LED, OUTPUT);                
  
  Serial.println("Ontvanger staat aan. Wachten op RC signaal...");
}

// ---------- Loop ----------
void loop() {
  if (mySwitch.available()) {
    long long value   = mySwitch.getReceivedValue();
    int lengthPush    = measurePushDuration();
    handleButton(value, lengthPush);
    mySwitch.resetAvailable();
  }
  delay(100);
}



// -------------Measures how long the RC-button is pushed-------------
int measurePushDuration() {
  int count = 0;
  // unsigned long startTime = millis();

  while (mySwitch.available()) {
    // if (millis() - startTime > 5000) return 5;  // max 5 seconden
    count++;
    mySwitch.resetAvailable();
    waitOrBeep(100, count);
  }
  return count;
}

//---------Beep on threshold values -------------
void waitOrBeep(int timeStep, int count){
  if       (count == thresholdsPushButton[0]){
      delay(timeStep*0.50);
      beep(timeStep*0.50);
    }
  else if  (count == thresholdsPushButton[1]){
      beep(timeStep*0.33);
      delay(timeStep*0.33);
      beep(timeStep*0.33);
    }
  else if  (count == thresholdsPushButton[2]){
      beep(timeStep*2);
    }
    delay(100);
}

// -------------beeper-------------
void beep(int duration) {
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}

// -------------Processes button actions-------------
void handleButton(long long value, int lengthPush) {
  int dur = pushDuration(lengthPush);

  // STANDBY
  if (value == buttonCodeA[1] || value == buttonCodeB[1]) {
    Serial.println("STANDBY");
    stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x02; stCmd[3] = 0xfd;
  }
  // AUTO / WIND / TRACK (zelfde knop, verschillende drukduur)
  else if (value == buttonCodeA[0] || value == buttonCodeB[0]) {
    if      (dur == 1) { Serial.println("AUTO");  stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x01; stCmd[3] = 0xfe; }
    else if (dur == 2) { Serial.println("WIND");  stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x23; stCmd[3] = 0xdc; }
    else if (dur == 3) { Serial.println("TRACK"); stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x03; stCmd[3] = 0xfc; }
    else { Serial.println("Onbekende knop"); return; }
  }
  // BB (port) button
  else if (value == buttonCodeA[2] || value == buttonCodeB[2]) {
    if      (dur == 0) { Serial.println("BB 1 degree");  stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x05; stCmd[3] = 0xfa; }
    else if (dur == 1) { Serial.println("BB 5 degrees"); stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x05; stCmd[3] = 0xfa; 
      for (int i = 0; i < 4; i++) send2ST(stCmd);
    }
    else if (dur == 2) { Serial.println("BB 10 degrees");stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x06; stCmd[3] = 0xf9; }
    else if (dur == 3) { Serial.println("BB tack");      stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x21; stCmd[3] = 0xde; }
    else { Serial.println("Onbekende knop"); return; }
  }
  // SB (starboard) button
  else if (value == buttonCodeA[3] || value == buttonCodeB[3]) {
    if      (dur == 0) { Serial.println("SB 1 degree");  stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x07; stCmd[3] = 0xf8; }
    else if (dur == 1) { Serial.println("SB 5 degrees"); stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x07; stCmd[3] = 0xf8; 
      for (int i = 0; i < 4; i++) send2ST(stCmd);
    }
    else if (dur == 2) { Serial.println("SB 10 degrees");stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x08; stCmd[3] = 0xf7; }
    else if (dur == 3) { Serial.println("SB tack");      stCmd[0] = 0x86; stCmd[1] = 0x21; stCmd[2] = 0x22; stCmd[3] = 0xdd; }
    else { Serial.println("Onbekende knop"); return; }
  }
  else {
    Serial.println("Onbekende knop");
    return;
  }

  send2ST(stCmd);
}

// ------------Determine the duration of the push-button-----------------
int pushDuration(int lengthPush) {
  if      (lengthPush > 0                       && lengthPush <= thresholdsPushButton[0]) return 0; 
  else if (lengthPush > thresholdsPushButton[0] && lengthPush <= thresholdsPushButton[1]) return 1; 
  else if (lengthPush > thresholdsPushButton[1] && lengthPush <= thresholdsPushButton[2]) return 2; 
  else                                                                                    return 3;
}

// -------- send SeaTalk message ------------------
void send2ST(uint8_t* data){
  CheckBus();
  digitalWrite(TX_LED, HIGH );

  for (int i = 0; i < 4; i++) {
    if (i == 0) mySerial.begin(4800, SWSERIAL_8M1, RX_PIN, TX_PIN, true); // MARK parity
    else        mySerial.begin(4800, SWSERIAL_8S1, RX_PIN, TX_PIN, true); // SPACE parity
    mySerial.write(data[i]);
  }

  delay(100);
  digitalWrite(TX_LED, LOW);
}


// -----------wait line idle ------------------ 
void CheckBus ( void ){
  unsigned long startTime = millis();

  for(int cX = 0; cX < 255; cX++ ){
    if (digitalRead(RX_PIN) == 1) cX = 0;  // reset als bus bezet
    delayMicroseconds(20);           // so, 20*0.250 ms = 5 ms
     
    // give up after 500 msec
    if(millis() - startTime > 500) {
      Serial.println("CheckBus timeout!");
      break;
    }
  }
}