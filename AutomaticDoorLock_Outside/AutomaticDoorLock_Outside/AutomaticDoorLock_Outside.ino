//Version 3
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

const int SS_PIN    = 10;
const int RST_PIN   = 9;
const int TRIG_PIN  = 7;
const int ECHO_PIN  = 6;

const int PERSON_THRESHOLD = 30;

const char MSTADDR[]="_._MASTER+ADDR:14:3:50b06";

bool doorClosed=true;

SoftwareSerial BT(2, 3);

MFRC522 rfid(SS_PIN, RST_PIN);

struct approvedID {
  const char* name;
  byte ID[4];
};

approvedID idList[] = {
  // Add your approved UIDs here
};
const size_t NUM_IDS = sizeof(idList) / sizeof(idList[0]);

bool checkID(byte scannedID[4]) {
  for (size_t i = 0; i < NUM_IDS; i++) {
    bool matchID = true;
    for (int j = 0; j < 4; j++) {
      if (scannedID[j] != idList[i].ID[j]) {
        matchID = false;
        break;
      }
    }
    if (matchID) {
      Serial.print(MSTADDR);
      Serial.print(": Access Granted: ");
      Serial.println(idList[i].name);
      return true;
    }
  }
  Serial.print(MSTADDR);
  Serial.println(": Access Denied...");
  return false;
}

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 100000UL);
  if (duration == 0) return -1;
  long dist = (duration * 0.0344) / 2;
  return dist;
}

bool presenceDetected = false;
unsigned long lastUSCheck   = 0;
unsigned long lastRFIDCheck = 0;



void setup() {
  Serial.begin(9600);
  BT.begin(38400);    
  Serial.print(MSTADDR);
  Serial.println(": Booting...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  SPI.begin();
  rfid.PCD_Init();
  Serial.print(MSTADDR);
  Serial.println(": SPI & RFID initialized");
}

void loop() {
  unsigned long now = millis();

  if(BT.available()){
    doorClosed=BT.read();
  }
  if(now - lastUSCheck >= 200 && doorClosed){
    lastUSCheck = now;
    long dist = readDistanceCM();

    if(dist > 0){
      presenceDetected = (dist < PERSON_THRESHOLD);
      Serial.print(MSTADDR);
      Serial.print(": Distance = ");
      Serial.print(dist);
      Serial.print(" cm, presenceDetected = ");
      Serial.println(presenceDetected ? "true" : "false");
    } else {
      Serial.print(MSTADDR);
      Serial.println(": Distance invalid");
      presenceDetected = false;
    }
  }

  if(now - lastRFIDCheck >= 100) {
    lastRFIDCheck = now;

    if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      Serial.print(MSTADDR);
      Serial.println(": Card detected");
      byte scannedID[4];
      for(int i = 0; i < 4; i++) {
        scannedID[i] = rfid.uid.uidByte[i];
      }

      bool accessOK = checkID(scannedID);

      if(accessOK && presenceDetected) {
        Serial.print(MSTADDR);
        Serial.println(": Access OK + presence -> send '0' (UNLOCK)");
        BT.write('0');
      } else if(!accessOK) {
        Serial.print(MSTADDR);
        Serial.println(": Access DENIED -> send '1' (LOCK)");
        BT.write('1');
      } else {
        Serial.print(MSTADDR);
        Serial.println(": Valid ID but no presence -> no command sent");
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}

// Version 1
// #include <Arduino.h>
// #include <Arduino_FreeRTOS.h>
// #include <SoftwareSerial.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <semphr.h>

// const int SS_PIN=10;
// const int RST_PIN=9;
// const int TRIG_PIN=7;
// const int ECHO_PIN=6;

// const int PERSON_THRESHOLD=30;

// SoftwareSerial BT(2,3);
// MFRC522 rfid(SS_PIN,RST_PIN);

// struct approvedID
// {
//   const char* name;
//   byte ID[4];
// };

// approvedID idList[] = 
// {
//   {"Ryan Antcliff",{0xB9,0x95,0x24,0x12}}
// };
// const size_t NUM_IDS=sizeof(idList)/sizeof(idList[0]);

// bool checkID(byte scannedID[4])
// { 
//   for(size_t i=0;i<NUM_IDS;i++)
//   {
//     bool matchID=true;
//     for(int j=0;j<4;j++)
//     {
//       if(scannedID[j]!=idList[i].ID[j])
//       {
//         matchID=false;
//         break;
//       }
//     }
//     if(matchID)
//     {
//       Serial.print("Access Granted: "); Serial.println(idList[i].name);
//       return true;
//     }
//   }
//   Serial.println("Access Denied...");
//   return false;
// }

// long readDistanceCM()
// {
//   digitalWrite(TRIG_PIN, LOW);
//   delayMicroseconds(2);
//   digitalWrite(TRIG_PIN, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(TRIG_PIN, LOW);

//   long duration=pulseIn(ECHO_PIN, HIGH, 30000UL);
//   if(duration==0) return -1;
//   long dist=(duration*0.0344)/2;
//   return dist;
// }

// void TaskRFID(void *pvParameters);
// void TaskUltrasonic(void *pvParameters);
// volatile bool presenceDetected=false;
// SemaphoreHandle_t xPresenceDetectedMutex;

// void setup()
// {
//   Serial.begin(9600);
//   BT.begin(38400);
//   SPI.begin();
//   rfid.PCD_Init();

//   pinMode(TRIG_PIN, OUTPUT);
//   pinMode(ECHO_PIN, INPUT);
//   xPresenceDetectedMutex=xSemaphoreCreateMutex();
//   xTaskCreate(TaskRFID, "RFID", 1024, NULL, 2, NULL);
//   xTaskCreate(TaskUltrasonic, "US", 1024, NULL, 1, NULL);
//   bool ok1 = xTaskCreate(TaskRFID, "RFID", 1024, NULL, 2, NULL);
//   bool ok2 = xTaskCreate(TaskUltrasonic, "US", 1024, NULL, 1, NULL);

//   Serial.print("RFID task create = ");
//   Serial.println(ok1);
//   Serial.print("US task create = ");
//   Serial.println(ok2);
  
//   vTaskStartScheduler();
// }
// // void setup() {
// //   Serial.begin(38400);
// //   Serial.println("BOOTING...");

// //   SPI.begin();
// //   Serial.println("SPI OK");

// //   rfid.PCD_Init();
// //   Serial.println("RFID OK");

// //   Serial.println("CREATING TASKS...");
// // }

// void loop(){}

// void TaskRFID(void *pvParameters)
// {
//   (void) pvParameters;
//   for(;;)
//   {
//     if (rfid.PICC_IsNewCardPresent()&&rfid.PICC_ReadCardSerial()) {
//       byte scannedID[4];
//       for(int i=0;i<4;i++)
//       {
//         scannedID[i]=rfid.uid.uidByte[i];
//       }
//       xSemaphoreTake(xPresenceDetectedMutex, portMAX_DELAY);
//       bool detected=presenceDetected;
//       xSemaphoreGive(xPresenceDetectedMutex);
//       if(checkID(scannedID)&&detected){
//         Serial.println("Master: sending 0");
//         BT.write('0');
//       }
//       else if(!checkID(scannedID)){
//         Serial.println("Master: sending 1");
//         BT.write('1');
//       }
//       rfid.PICC_HaltA();
//       rfid.PCD_StopCrypto1();
//     }
//     vTaskDelay(pdMS_TO_TICKS(100));
//   }
// }

// void TaskUltrasonic(void *pvParameters)
// {
//   (void) pvParameters;
//   static bool lastState=false;
//   for(;;)
//   {
//     long dist=readDistanceCM();

//     Serial.print("Distance: ");
//     Serial.println(dist);

//     bool current=(dist>0&&dist<PERSON_THRESHOLD);
//     xSemaphoreTake(xPresenceDetectedMutex, portMAX_DELAY);
//     presenceDetected=current;
//     xSemaphoreGive(xPresenceDetectedMutex);
//     if(current!=lastState)
//     {
//       lastState=current;
//     }
//     vTaskDelay(pdMS_TO_TICKS(200));
//     Serial.print("Presence: ");
//     Serial.println(current);
//   }
// }


// Version 2
// #include <Arduino.h>
// #include <SoftwareSerial.h>
// #include <SPI.h>
// #include <MFRC522.h>

// const int SS_PIN    = 10;
// const int RST_PIN   = 9;
// const int TRIG_PIN  = 7;
// const int ECHO_PIN  = 6;

// const int PERSON_THRESHOLD = 30; // cm

// // HC-05 on pins 2 (RX) and 3 (TX)
// SoftwareSerial BT(2, 3); // RX, TX

// MFRC522 rfid(SS_PIN, RST_PIN);

// struct approvedID {
//   const char *name;
//   byte ID[4];
// };

// approvedID idList[] = {
//   {"Ryan Antcliff", {0xB9, 0x95, 0x24, 0x12}}
// };
// const size_t NUM_IDS = sizeof(idList) / sizeof(idList[0]);

// bool checkID(byte scannedID[4]) {
//   for (size_t i = 0; i < NUM_IDS; i++) {
//     bool matchID = true;
//     for (int j = 0; j < 4; j++) {
//       if (scannedID[j] != idList[i].ID[j]) {
//         matchID = false;
//         break;
//       }
//     }
//     if (matchID) {
//       Serial.print("Access Granted: ");
//       Serial.println(idList[i].name);
//       return true;
//     }
//   }
//   Serial.println("Access Denied...");
//   return false;
// }

// long readDistanceCM() {
//   digitalWrite(TRIG_PIN, LOW);
//   delayMicroseconds(5);
//   digitalWrite(TRIG_PIN, HIGH);
//   delayMicroseconds(20);
//   digitalWrite(TRIG_PIN, LOW);

//   long duration = pulseIn(ECHO_PIN, HIGH, 100000UL);
//   if (duration == 0) return -1;
//   long dist = (duration * 0.0344) / 2;
//   return dist;
// }

// bool presenceDetected = false;
// unsigned long lastUSCheck = 0;
// unsigned long lastRFIDCheck = 0;

// void setup() {
//   Serial.begin(9600);
//   BT.begin(38400);

//   Serial.println("MASTER BOOTING...");

//   pinMode(TRIG_PIN, OUTPUT);
//   pinMode(ECHO_PIN, INPUT);

//   SPI.begin();
//   rfid.PCD_Init();

//   Serial.println("SPI OK");
//   Serial.println("RFID OK");
//   Serial.println("MASTER READY");
// }

// void loop() {
//   unsigned long now = millis();

//   if (now - lastUSCheck >= 200) {
//     lastUSCheck = now;
//     long dist = readDistanceCM();
//     if (dist > 0) {
//       presenceDetected = (dist < PERSON_THRESHOLD);
//       Serial.print("Distance: ");
//       Serial.print(dist);
//       Serial.print(" cm, presenceDetected = ");
//       Serial.println(presenceDetected ? "true" : "false");
//     } else {
//       Serial.println("Distance: invalid");
//       presenceDetected = false;
//     }
//   }

//   if (now - lastRFIDCheck >= 100) {
//     lastRFIDCheck = now;

//     if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
//       Serial.println("Card detected");
//       byte scannedID[4];
//       for (int i = 0; i < 4; i++) {
//         scannedID[i] = rfid.uid.uidByte[i];
//       }

//       bool goodID = checkID(scannedID);

//       if (goodID && presenceDetected) {
//         Serial.println("MASTER: Sending 0 (unlock)");
//         BT.write('0'); // unlock request
//       } else if (!goodID) {
//         Serial.println("MASTER: Sending 1 (lock)");
//         BT.write('1'); // lock request
//       } else {
//         Serial.println("Valid ID but no presence; not sending.");
//       }

//       rfid.PICC_HaltA();
//       rfid.PCD_StopCrypto1();
//     }
//   }

//   if (BT.available()) {
//     char c = BT.read();
//     Serial.print("From SLAVE: ");
//     Serial.println(c);
//   }
// }
