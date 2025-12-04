//Version 2
#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

const int SERVO_PIN       = 6;
const int MAGNET_PIN      = 7;
const int TRIG_PIN        = 9;
const int ECHO_PIN        = 8;
const int LOCK_LED_PIN    = 5;
const int UNLOCK_LED_PIN  = 4;

const int PERSON_THRESHOLD = 30;
const int LOCK_ANGLE       = 10;
const int UNLOCK_ANGLE     = 90;

const char SLVADDR[]="_.._SLAVE+ADDR:14:3:50a37";

const unsigned long UNLOCK_HOLD = 5000;
unsigned long lastUnlockRequest = 0;

#define LOCKED   true
#define UNLOCKED false

SoftwareSerial BT(2, 3);
Servo lockServo;

bool isLocked     = true;
bool doorClosed   = false;
bool presenceDetected   = false;

unsigned long lastUSCheck     = 0;
unsigned long lastMagnetCheck = 0;

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 100000UL);
  if(duration == 0) return -1;
  long dist = (duration * 0.0344) / 2;
  return dist;
}

void setLock(bool lockState) {
  if(lockState == isLocked) return;

  isLocked = lockState;
  lockServo.attach(SERVO_PIN);

  if(lockState == LOCKED) {
    lockServo.write(LOCK_ANGLE);
  } else {
    lockServo.write(UNLOCK_ANGLE);
  }

  delay(300);
  lockServo.detach();

  if(lockState == LOCKED) {
    Serial.print(SLVADDR);
    Serial.println(": locked");
    digitalWrite(LOCK_LED_PIN, HIGH);
    digitalWrite(UNLOCK_LED_PIN, LOW);
  } else {
    Serial.print(SLVADDR);
    Serial.println(": unlocked");
    digitalWrite(LOCK_LED_PIN, LOW);
    digitalWrite(UNLOCK_LED_PIN, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  BT.begin(38400);
  Serial.println("SLAVE: Booting...");

  pinMode(MAGNET_PIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LOCK_LED_PIN, OUTPUT);
  pinMode(UNLOCK_LED_PIN, OUTPUT);

  setLock(LOCKED);
  Serial.println("SLAVE: Ready");
}

void loop() {
  unsigned long now = millis();

  if(BT.available()) {
    char requestVal = BT.read();
    Serial.print(SLVADDR);
    Serial.print(": RECEIVED: ");
    Serial.print(requestVal);

    if(requestVal == '0') {
      Serial.println(" (UNLOCK)");
      setLock(UNLOCKED);
      lastUnlockRequest = now;
    } else if(requestVal == '1') {
      Serial.println(" (LOCK)");
      if(doorClosed){
        setLock(LOCKED);
        lastUnlockRequest = 0;
      }else{
        Serial.println(" (LOCK IGNORED - door open)");
      }
    }
    
  }

  if(now - lastMagnetCheck >= 100) {
    lastMagnetCheck = now;
    int magnetState = digitalRead(MAGNET_PIN);
    doorClosed = (magnetState == LOW);
    
  }
  BT.write(doorClosed);
  static int stableCount = 0;
  if(now - lastUSCheck >= 150 && doorClosed) {
    lastUSCheck = now;

    long dist = readDistanceCM();
    bool detectedNow = (dist > 0 && dist < PERSON_THRESHOLD);

    if (detectedNow) {
      stableCount++;
    } else {
      stableCount = 0;
    }
    presenceDetected = (stableCount >= 2);

    Serial.print(SLVADDR);
    Serial.print(": Distance = ");
    Serial.print(dist);
    Serial.print(" cm, presenceDetected = ");
    Serial.println(presenceDetected ? "true" : "false");
  }

  if(presenceDetected && !isLocked) {} 
  else if(presenceDetected && isLocked) {
    setLock(UNLOCKED);
    lastUnlockRequest = now;
  } else if(!presenceDetected && 
              doorClosed && 
              !isLocked && 
              lastUnlockRequest!=0 &&
              (now-lastUnlockRequest>=UNLOCK_HOLD)) {
    setLock(LOCKED);
    lastUnlockRequest = 0;
  }
  delay(10);
}

// Version 1
// #include <Arduino.h>
// #include <Servo.h>
// #include <Arduino_FreeRTOS.h>
// #include <SoftwareSerial.h>

// const int SERVO_PIN= 6;
// const int MAGNET_PIN= 7;
// const int TRIG_PIN= 9;
// const int ECHO_PIN= 8;
// const int LOCK_LED_PIN=5;
// const int UNLOCK_LED_PIN=4;

// const int PERSON_THRESHOLD=30;
// const int LOCK_ANGLE=10;
// const int UNLOCK_ANGLE=90;

// #define LOCKED true
// #define UNLOCKED false

// volatile bool isLocked=true;
// volatile bool doorClosed=false;
// volatile bool personNear=false;

// int rqVal=2;
// SoftwareSerial BT(2,3);
// Servo lockServo;

// void TaskBluetooth(void *pvParameters);
// void TaskSensors(void *pvParameters);

// long readDistanceCM()
// {
//   digitalWrite(TRIG_PIN, LOW);
//   delayMicroseconds(2);
//   digitalWrite(TRIG_PIN, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(TRIG_PIN, LOW);

//   long duration=pulseInLong(ECHO_PIN, HIGH, 30000UL);
//   if(duration==0) return -1;
//   long dist=(duration*0.0344)/2;
//   return dist;
// }

// void setLock(bool lockState)
// {
//   if(lockState==isLocked) return;

//   isLocked=lockState;
//   lockServo.attach(SERVO_PIN);
//   if(lockState==LOCKED)
//   {
//     lockServo.write(LOCK_ANGLE);
//   }
//   else
//   {
//     lockServo.write(UNLOCK_ANGLE);
//   }

//   vTaskDelay(pdMS_TO_TICKS(50));
//   lockServo.detach();

//   if(lockState==LOCKED)
//   {
//     Serial.println("locked");
//     digitalWrite(LOCK_LED_PIN, HIGH);
//     digitalWrite(UNLOCK_LED_PIN, LOW);
//   }
//   else
//   {
//     Serial.println("unlocked");
//     digitalWrite(LOCK_LED_PIN, LOW);
//     digitalWrite(UNLOCK_LED_PIN, HIGH);
//   }
// }

// void setup()
// {
//   pinMode(MAGNET_PIN, INPUT_PULLUP);
//   pinMode(TRIG_PIN,OUTPUT);
//   pinMode(ECHO_PIN,INPUT);
//   pinMode(LOCK_LED_PIN, OUTPUT);
//   pinMode(UNLOCK_LED_PIN, OUTPUT);

//   setLock(LOCKED);

//   Serial.begin(38400);
//   BT.begin(9600);
//   xTaskCreate(TaskBluetooth, "BT", 1024, NULL, 2, NULL);
//   xTaskCreate(TaskSensors, "UM", 256, NULL, 1, NULL);
  

//   vTaskStartScheduler();
  
// }

// void loop(){}

// void TaskBluetooth(void *pvParameters)
// {
//   (void) pvParameters;

//   for(;;)
//   {
//     if(BT.available()>0)
//     {
//       rqVal=BT.read();
//       Serial.print("SLAVE RECEIVED: ");
//       Serial.println((char)rqVal);
//       if(rqVal=='0')
//       {
//         setLock(UNLOCKED);
//       }
//       else if(rqVal=='1')
//       {
//         setLock(LOCKED);
//       }
//     }
//     vTaskDelay(pdMS_TO_TICKS(20));
//   }
// }

// void TaskSensors(void *pvParameters)
// {
//   (void) pvParameters;
//   static int stableCount=0;
//   for(;;)
//   {
//     int magnetState=digitalRead(MAGNET_PIN);
//     doorClosed=(magnetState==LOW);
//     long dist=readDistanceCM();
//     bool detectedNow=(dist>0&&dist<PERSON_THRESHOLD);
//     if(detectedNow)
//     {
//       stableCount++;
//     }
//     else
//     {
//       stableCount=0;
//     }
//     personNear=(stableCount>=2);

//     if(personNear)
//     {
//       setLock(UNLOCKED);
//     }
//     else if(doorClosed&&!personNear)
//     {
//       setLock(LOCKED);
//     }
//     vTaskDelay(100);
//   }
// }

// Bluetooth test version
// #include <SoftwareSerial.h>
// SoftwareSerial BT(2, 3);  // RX=2, TX=3

// void setup() {
//   Serial.begin(9600);
//   BT.begin(38400);
//   Serial.println("SLAVE TEST READY");
// }

// void loop() {
//   if (BT.available()) {
//     char c = BT.read();
//     Serial.print("RECEIVED: ");
//     Serial.println(c);
//   }
// }