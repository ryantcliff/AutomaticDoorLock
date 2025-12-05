# Automatic Door Lock #
Real-Time door locking system

## COMPONENT LIST ##
  - 5 1kΩ Resistors
  - 3 2kΩ Resistors
  - 2 Arduino UNO R3 (or equivalent)
  - 2 HC-05 Bluetooth Modules
  - 1 RFID-RC522 Module
  - 2 HC-SR04 Ultrasonic Sensor Modules
  - 1 High Torque Servo
  - 1 Magnetic Door Switch
  - 1 Green LED
  - 1 Red LED
  - 1 5V External Power Supply
  - Jumper Wires

## REQUIRED SOFTWARE ##
  - Arduino IDE

## INSTRUCTIONS ##
  
  Wire the `Inside Controller Unit` and `Outside Controller Unit` modules as shown in the diagrams below:
  
  ![InsideUnitController_WiringDiagram](https://github.com/user-attachments/assets/ddcec90c-b41e-4d01-a3e4-7d61aa804d34)
  ![OutsideUnitController_WiringDiagram](https://github.com/user-attachments/assets/49b6eb8a-7373-4180-92a5-bb199a227ccd)
  
  ---
  Clone the repository
  ```bash
  git clone https://github.com/ryantcliff/AutomaticDoorLock.git
  ```
  ---
  Upload the following code to Outside Controller Unit to find the UID of RFID tags you wish to add as approved users:
  ```.ino
  #include <SPI.h>
  #include <MFRC522.h>

  #define RST_PIN         9          // Configurable, see typical pin layout
  #define SS_PIN          10         // Configurable, see typical pin layout

  MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

  void setup() {
    Serial.begin(9600);		// Initialize serial communications with the PC
    while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();			// Init SPI bus
    mfrc522.PCD_Init();		// Init MFRC522
    mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  }

  void loop() {
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    // Dump debug info about the card; PICC_HaltA() is automatically called
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
  
    mfrc522.PICC_HaltA(); // Halt PICC
    mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
  }
  ```
  ---
  Add UIDs to `approvedID idList[]` in `AutomaticDoorLock_Outside.ino` in the format:
  ```.ino
  approvedID idList[] = {
    {"User1", {0xXX, 0xXX, 0xXX, 0xXX}},
    {"User2", {0xXX, 0xXX, 0xXX, 0xXX}},
    {"User3", {0xXX, 0xXX, 0xXX, 0xXX}},
    .
    .
    .
  };
  ```
  ---
  Upload the following code to the `Inside Controller Unit` and the `Outside Controller Unit`:
  ```.ino
  #include <SoftwareSerial.h>

  SoftwareSerial mySerial(2, 3); // RX, TX

  void setup() {
    Serial.begin(9600);
    Serial.println("Enter AT commands:");
    mySerial.begin(38400);
  }

  void loop() {
    if (mySerial.available()) {
      Serial.write(mySerial.read());
    }
    if (Serial.available()) {
      mySerial.write(Serial.read());
    }
  }
  ```
  This is how we will begin the process of pairing the HC-05 Bluetooth modules to each other.

  ---
  
  ### SLAVE CONFIGURATION ###
  - Hold down EN button on HC-05 while powering on to enter AT Mode.

    HC-05 LED will change from a rapid blink to a slow blink if this is successful.
  - In Serial Monitor, enter command

    ```Serial
      AT
    ```
    If HC-05 is in AT Mode,
    
    Output:
    >OK
  - To clear any existing paired devices, enter command
    ```Serial
      AT+RMAAD
    ```
  - To get HC-05 name, enter command
    ```Serial
      AT+NAME?
    ```
    You may change the name by entering command
    ```Serial
      AT+NAME=<Enter Name Here>
    ```
  - Set HC-05 role to '0' to signal that it will be functioning as the ***slave*** by entering command
    ```Serial
      AT+ROLE=0
    ```
  - To get HC-05 address, enter command
    ```Serial
      AT+ADDR?
    ```
    Output:
    
    >+ADDR:XXX:XXX:XXX
    
    The address will appear as set of hex values separated by colons. You will use this value to bind the master to the slave.
    You may also use this value to fill in `const char SLVADDR[]` in `AutomaticDoorLock_Inside.ino`.
    
  - Change UART to 38400 by entering command
    ```Serial
      AT+UART=38400,0,0
    ```
  ### MASTER CONFIGURATION ###
  - Hold down EN button on HC-05 while powering on to enter AT Mode.

    HC-05 LED will change from a rapid blink to a slow blink if this is successful.
  - In Serial Monitor, enter command

    ```Serial
      AT
    ```
    If HC-05 is in AT Mode,
    
    Output:
    >OK
    
  - To clear any existing paired devices, enter command
    ```Serial
      AT+RMAAD
    ```
  - To get HC-05 name, enter command
    ```Serial
      AT+NAME?
    ```
    You may change the name by entering command
    ```Serial
      AT+NAME=<Enter Name Here>
    ```
  - Set HC-05 role to '0' to signal that it will be functioning as the ***master*** by entering command
    ```Serial
      AT+ROLE=1
    ```
  - Set HC-05 cmode to 
  - To get HC-05 address, enter command
    ```Serial
      AT+ADDR?
    ```
    Output:
    >+ADDR:XXX:XXX:XXX
    
    The address will appear as set of hex values separated by colons.
    You may also use this value to fill in `const char MSTADDR[]` in `AutomaticDoorLock_Outside.ino`.
  - Change UART to 38400 by entering command
    ```Serial
      AT+UART=38400,0,0
    ```
    

      
