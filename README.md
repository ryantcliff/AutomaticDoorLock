# Automatic Door Lock #
Real-time automatic door-locking system

<a name="top"></a>
## Table of Contents
- [Components](#components)
- [Required Software](#required-software)
- [Instructions](#instructions)
  - [Hardware Wiring](#hardware-wiring)
  - [Cloning the Repository](#cloning-the-repository)
  - [Adding Valid RFID Tags](#adding-valid-rfid-tags)
  - [Pairing HC-05 Modules](#pairing-hc-05-modules)
    - [Slave Configuration](#slave-configuration)
    - [Master Configuration](#master-configuration)
  - [Serial Environment Setup](#serial-environment-setup)
  - [Initializing the System](#initializing-the-system)
  - [Operation](#operation)
    - [Operation State Table](#operation-state-table)
 
---

[Back to Top](#top)
## Components ## 

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

---

[Back to Top](#top)
## Required Software ##
  - Arduino IDE

---

[Back to Top](#top)
## Instructions ##
  ### Hardware Wiring ###
  Wire the `Inside Controller Unit` and `Outside Controller Unit` modules as shown in the diagrams below:
  
  ![InsideUnitController_WiringDiagram](https://github.com/user-attachments/assets/e13c7240-64f8-4881-9ef8-531e54a75543)
  ![OutsideUnitController_WiringDiagram](https://github.com/user-attachments/assets/8c17d888-fb0c-4c63-8ea3-af288f17e96c)

  ---

  ### Cloning the Repository ###
  [Back to Top](#top)
  
  Clone the repository
  ```bash
  git clone https://github.com/ryantcliff/AutomaticDoorLock.git
  ```
  
  ---

  ### Adding Valid RFID Tags ###
  [Back to Top](#top)
  
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

  ### Pairing HC-05 Modules ###
  [Back to Top](#top)
  
  Upload the following code to the `Inside Controller Unit` and the `Outside Controller Unit`:
  ```.ino
  #include <SoftwareSerial.h>

  SoftwareSerial BT(2, 3); // RX, TX

  void setup() {
    Serial.begin(9600);
    Serial.println("Enter AT commands:");
    BT.begin(38400);
  }

  void loop() {
    if (BT.available()) {
      Serial.write(BT.read());
    }
    if (Serial.available()) {
      BT.write(Serial.read());
    }
  }
  ```
  This is how we will begin the process of pairing the HC-05 Bluetooth modules to each other.

  >[!IMPORTANT]
  >To see any output in the Serial Monitor, you **MUST** set the baud rate in the Serial Monitor to match the baud rate we set above for the Bluetooth modules, **38400**.  
  >
  >If you are seeing garbled output in the Serial Monitor, this is most certainly the reason why.
  
  ---

  #### Slave Configuration ####
  [Back to Top](#top)
  
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

  >[!TIP]
  >You may use this address to assign a value to `const char SLVADDR[]` in `AutomaticDoorLock_Inside.ino` for clearer formatting in the Serial Monitor output.
    
  - Change UART to 38400 by entering command
    ```Serial
      AT+UART=38400,0,0
    ```

  #### Master Configuration ####
  [Back to Top](#top)
  
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
  - Set HC-05 role to '1' to signal that it will be functioning as the ***master*** by entering command
    ```Serial
      AT+ROLE=1
    ```
    
  - Set HC-05 cmode to '0' to allow discovery of other Bluetooth devices by entering command
    ```Serial
      AT+CMODE=0
    ```
    
  - To bind the ***master*** to the ***slave***, enter command
    ```Serial
      AT+BIND=XXX,XXX,XXX
    ```
    replacing `XXX,XXX,XXX` with the address of the ***Slave HC-05***.

  >[!IMPORTANT]
  >The address you input with `AT+BIND=` will not include the `'+ADDR:'` that appears in the original address, and the colons from the original address are replaced with commas.
    
  - To get HC-05 address, enter command
    ```Serial
      AT+ADDR?
    ```
    Output:
    >+ADDR:XXX:XXX:XXX
    
    The address will appear as set of hex values separated by colons.
    
  > [!TIP]
  > You may use this address to assign a value to `const char MSTADDR[]` in `AutomaticDoorLock_Outside.ino` for clearer formatting in the Serial Monitor output.
    
  - Change UART to 38400 by entering command
    ```Serial
      AT+UART=38400,0,0
    ```
    
  >[!NOTE]
  >If the HC-05 modules are paired successfully, they will both exhibit a short double-blink.  
  >
  >If either modules are exhibiting a fast blink or a slow single-blink, perform a power cycle on both of them to attempt a connection.  
  >
  >If this does not work, begin pairing steps again starting at the top of [Pairing HC-05 Modules](#pairing-hc-05-modules).

  ---

  ### Serial Environment Setup ###
  [Back to Top](#top)
  
  Once the HC-05 modules have been successfully paired, upload `AutomaticDoorLock_Inside.ino` and `AutomaticDoorLock_Outside.ino` to the `Inside Controller Unit` and `Outside Controller Unit`, respectively.  

  >[!IMPORTANT]
  >In the Arduino IDE, the baud rate for the Serial Monitor **MUST** be changed back to **9600** so the debug info printed to Serial can be seen. If you keep the baud rate at 38400, you will only see the unreadable garbled output being sent between the HC-05 modules.

  ---

  ### Initializing the System ###
  [Back to Top](#top)

  When the `Inside Controller Unit` and the `Outside Controller Unit` are powered on, the system is ready to be initialized when ***both*** indicator LEDs are lit on the `Inside Controller Unit`.

  To initialize and put the system in active mode:
  
  1. Ensure the `Magnetic Door Switch` is closed
  2. Trigger the `Ultrasonic Sensor` on the `Inside Controller Unit` by putting anything in front of it within its `PERSON_THRESHOLD`.

  The Red LED will turn off and the Servo will turn to the `UNLOCK_ANGLE` for `UNLOCK_HOLD` milliseconds, then the Green LED will turn off, the Red LED will turn on, and the Servo will turn to the `LOCK_ANGLE`.

  Now the system is initialized and ready for operation.

  ---

  ### Operation ###
  [Back to Top](#top)

  #### Inside Operation ####
  - To unlock the door from the inside, simply trigger the Ultrasonic Sensor by putting anything in front of it within its `PERSON_THRESHOLD`.  
  - The lock state LEDs will go from **Red *(LOCKED)*** --> **Green *(UNLOCKED)*** and the Servo will move from `LOCK_ANGLE` --> `UNLOCK_ANGLE` for `UNLOCK_HOLD` milliseconds.  
  - If the Magnetic Door Switch is then opened before `UNLOCK_HOLD` milliseconds has elapsed, the system will remain in the ***UNLOCKED*** state and both units will pause ultrasonic readings until the Magnetic Door Switch is closed, which will then change the lock state LEDs from **Green *(UNLOCKED)*** --> **Red (LOCKED)*** and the Servo from `UNLOCK_ANGLE` --> `LOCK_ANGLE`.

  #### Outside Operation ####
  - To unlock the door from the outside, two conditions must be met at the same time:
    1. The outside Ultrasonic Sensor must be triggered
    2. A ***valid*** UID must be read by the RFID module

  - When both conditions have been met, the Master HC-05 on the `Outside Controller Unit` will send a '0' to the Slave HC-05 on the `Inside Controller Unit`, signaling an 'Unlock Request' that will cause the lock state LEDs and Servo to transition states in the same way stated in [Inside Operation](#inside-operation).

  - If the Ultrasonic Sensor is triggered and an ***invalid*** UID is read by the RFID module, the Master HC-05 on the `Outside Controller Unit` will send a '1' to the Slave HC-05 on the `Inside Controller Unit`, signaling a 'Lock Request'. If the system is in the ***UNLOCKED*** state, this will change the lock state LEDs from **Green *(UNLOCKED)*** --> **Red (LOCKED)*** Servo to move from `UNLOCK_ANGLE` --> `LOCK_ANGLE`. If the system is already in the ***LOCKED*** state, nothing will happen.

  - If the Ultrasonic Sensor is triggered but no UIDs are read by the RFID module, nothing will happen.

  - If the RFID module reads a UID, but the Ultrasonic Sensor is not triggered, the `Outside Controller Unit` will print to the Serial Monitor whether the UID is accepted or denied, but it ***will <ins>NOT</ins>*** send any 'Unlock/Lock Requests' to the `Inside Controller Unit`.

  #### Operation State Table ####
  [Back to Top](#top)

  | Current State | Next State | Door Open | Inside Ultrasonic Sensor Triggered | Outside Ultrasonic Sensor Triggered | RFID Valid UID Read | RFID Invalid UID Read | (now - lastUnlockRequest) >= UNLOCK_HOLD |
  |---------------|------------|-----------|------------------------------------|-------------------------------------|---------------------|-----------------------|------------------------------------------|
  | Locked        | Unlocked   | X | Yes | X | X | X | X |
  | Locked        | Unlocked   | X | X | Yes | Yes | No | X |
  | Locked        | Locked     | X | No | Yes | X | Yes | X |
  | Unlocked      | Locked     | No | No | X | No | X | Yes |
  | Unlocked      | Locked     | No | No | No | X | X | Yes |
  | Unlocked      | Locked     | No | No | Yes | X | Yes | X |
  | Unlocked      | Unlocked   | Yes | X | X | X | X | X |
  
  ---
