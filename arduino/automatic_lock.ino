/*
  automatic_lock.ino
  Arduino Uno inner-unit for AutomaticDoorLock

  Features:
  - Controls MG996R servo (locking deadbolt)
  - Reads HC-SR04 ultrasonic for proximity/diagnostics
  - Reads magnetic reed switch to know door closed/open
  - Exposes simple serial text protocol over USB for dev and over HC-05 Bluetooth (SoftwareSerial)

  Protocol (newline-terminated):
    LOCK     -> attempt to lock (succeeds only if door closed)
    UNLOCK   -> unlock immediately
    STATUS   -> report sensors and lock state
    PING     -> reply PONG

  Responses:
    OK LOCKED
    OK UNLOCKED
    ERR <message>
    STATUS DIST:xx MAG:0/1 LOCKED:0/1

  Wiring (suggested defaults):
    Servo signal -> D9 (use separate 5V supply for servo power; common GND)
    HC-SR04 TRIG -> D7
    HC-SR04 ECHO -> D6
    Magnetic reed switch -> D5 (pull-up)
    HC-05 RX/TX -> D10 (RX), D11 (TX) via SoftwareSerial (HC-05 TX -> D10, HC-05 RX <- D11) 
      Note: put a 1 or 2k resistor voltage divider from Arduino TX (D11) to HC-05 RX if needed.

  Notes:
    - MG996R is power hungry; do NOT power it from the Uno 5V regulator when under load.
    - This sketch favors clarity and safety checks; adapt timings/angles to your deadbolt geometry.
*/

#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// --- Pin configuration (change as needed) ---
const uint8_t PIN_SERVO = 9;
const uint8_t PIN_TRIG = 7;
const uint8_t PIN_ECHO = 6;
const uint8_t PIN_MAG = 5; // reed switch

// HC-05 via SoftwareSerial
const uint8_t PIN_BT_RX = 10; // Arduino RX <- HC-05 TX
const uint8_t PIN_BT_TX = 11; // Arduino TX -> HC-05 RX

// --- Servo angles ---
const int SERVO_LOCK_ANGLE = 20;   // adjust for your lock (mechanical)
const int SERVO_UNLOCK_ANGLE = 120; // adjust for your lock

// Safety/behavior tuning
const unsigned long MAG_DEBOUNCE_MS = 50;
const unsigned long SERVO_MOVE_MS = 600; // how long to wait for servo to move
const unsigned long HC_SR04_TIMEOUT_US = 30000UL; // 30ms -> ~5m (we'll measure smaller ranges)

// Serial speeds
const unsigned long SERIAL_BAUD = 115200UL; // USB serial for debug
const unsigned long BT_BAUD = 9600UL;       // common default for HC-05

// Globals
Servo lockServo;
SoftwareSerial btSerial(PIN_BT_RX, PIN_BT_TX);

bool lockedState = false; // logical state
int lastMagState = HIGH; // using INPUT_PULLUP
unsigned long lastMagChangeMs = 0;

// Simple helpers
void sendAll(const String &line) {
  Serial.println(line);
  btSerial.println(line);
}

long readDistanceCm() {
  // Trigger pulse
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Read echo
  unsigned long duration = pulseIn(PIN_ECHO, HIGH, HC_SR04_TIMEOUT_US);
  if (duration == 0) return -1; // timeout
  long distanceCm = duration / 58; // approximate conversion
  return distanceCm;
}

bool readMagDebounced() {
  int raw = digitalRead(PIN_MAG);
  unsigned long now = millis();
  if (raw != lastMagState) {
    lastMagChangeMs = now;
    lastMagState = raw;
  }
  // only accept stable after debounce window
  if (now - lastMagChangeMs < MAG_DEBOUNCE_MS) return (lastMagState == LOW);
  return (lastMagState == LOW);
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  lockServo.write(angle);
}

void doLock() {
  bool doorClosed = readMagDebounced();
  if (!doorClosed) {
    sendAll(String("ERR Door not closed - cannot lock"));
    return;
  }
  setServoAngle(SERVO_LOCK_ANGLE);
  delay(SERVO_MOVE_MS);
  lockedState = true;
  sendAll(String("OK LOCKED"));
}

void doUnlock() {
  setServoAngle(SERVO_UNLOCK_ANGLE);
  delay(SERVO_MOVE_MS);
  lockedState = false;
  sendAll(String("OK UNLOCKED"));
}

void reportStatus() {
  long dist = readDistanceCm();
  bool mag = readMagDebounced();
  String line = "STATUS ";
  if (dist < 0) line += "DIST:-1"; else line += "DIST:" + String(dist);
  line += ",MAG:" + String(mag ? 1 : 0);
  line += ",LOCKED:" + String(lockedState ? 1 : 0);
  sendAll(line);
}

void processLine(String s) {
  s.trim();
  s.toUpperCase();
  if (s == "LOCK") {
    doLock();
  } else if (s == "UNLOCK") {
    doUnlock();
  } else if (s == "STATUS") {
    reportStatus();
  } else if (s == "PING") {
    sendAll(String("PONG"));
  } else if (s.length() == 0) {
    // ignore
  } else {
    sendAll(String("ERR Unknown command: ") + s);
  }
}

String readIncomingLine(Stream &s) {
  static String buf;
  while (s.available()) {
    char c = s.read();
    if (c == '\n') {
      String out = buf;
      buf = "";
      return out;
    } else if (c >= 32 || c == '\r' || c == '\t') {
      buf += c;
      // Limit buffer length to avoid memory explosion
      if (buf.length() > 256) buf = buf.substring(buf.length() - 256);
    }
  }
  return String(); // empty means no complete line
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  btSerial.begin(BT_BAUD);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_MAG, INPUT_PULLUP);

  // attach servo
  lockServo.attach(PIN_SERVO);
  // start in unlocked for safety
  setServoAngle(SERVO_UNLOCK_ANGLE);
  lockedState = false;

  // initial messages
  Serial.println("AutomaticDoorLock: Arduino unit up");
  btSerial.println("AutomaticDoorLock: Arduino unit up");
  reportStatus();
}

void loop() {
  // Read from USB serial (developer) first
  String line = readIncomingLine(Serial);
  if (line.length()) {
    processLine(line);
  }

  // Read from Bluetooth serial
  String btline = readIncomingLine(btSerial);
  if (btline.length()) {
    processLine(btline);
  }

  // Periodically report a small heartbeat on USB (not on BT to avoid noise)
  static unsigned long lastHeartbeat = 0;
  unsigned long now = millis();
  if (now - lastHeartbeat > 60000UL) { // every 60s
    Serial.println(String("HB LOCKED:") + (lockedState ? "1" : "0"));
    lastHeartbeat = now;
  }
}
