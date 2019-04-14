/*  This code is based off of a starter code posted at:
 *  https://www.allaboutcircuits.com/projects/understanding-and-implementing-the-hc-12-wireless-transceiver-module/
 *  HC12 Send/Receive Example Program 2
 *  By Mark J. Hughes
 *  for AllAboutCircuits.com
 */

#include <SoftwareSerial.h>

// HC12 pin definitions
const byte HC12RxdPin = 4;                      // "RXD" Pin on HC12
const byte HC12TxdPin = 5;                      // "TXD" Pin on HC12
const byte HC12SetPin = 6;                      // "SET" Pin on HC12

// CSMA/CA timing constants (in ms)
// Will hit Tmax after 4 successive backoffs (assuming T is doubled each time we back off)
const int Tmin = 32;
const int Tmax = 512;

// Timer setup
unsigned long timer = millis();                 // Delay Timer

// Serial communication variables
char SerialByteIn;                              // Temporary variable
char HC12ByteIn;                                // Temporary variable
String HC12ReadBuffer = "";                     // Read/Write Buffer 1 for HC12
String SerialReadBuffer = "";                   // Read/Write Buffer 2 for Serial
boolean SerialEnd = false;                      // Flag to indicate End of Serial String
boolean HC12End = false;                        // Flag to indiacte End of HC12 String
boolean commandMode = false;                    // Send AT commands

// Software Serial ports Rx and Tx are opposite the HC12 Rx and Tx
// Create Software Serial Port for HC12
SoftwareSerial HC12(HC12TxdPin, HC12RxdPin);

void setup() {

  HC12ReadBuffer.reserve(64);                   // Reserve 64 bytes for Serial message input
  SerialReadBuffer.reserve(64);                 // Reserve 64 bytes for HC12 message input

  pinMode(HC12SetPin, OUTPUT);                  // Output High for Transparent / Low for Command
  digitalWrite(HC12SetPin, HIGH);               // Enter Transparent mode
  delay(80);                                    // 80 ms delay before operation per datasheet
  Serial.begin(9600);                           // Open serial port to computer
  HC12.begin(9600);                             // Open software serial port to HC12
}

void loop() {

  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      HC12ReadBuffer += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }

  while (Serial.available()) {                  // If Arduino's computer rx buffer has data
    SerialByteIn = Serial.read();               // Store each character in byteIn
    SerialReadBuffer += char(SerialByteIn);     // Write each character of byteIn to SerialReadBuffer
    if (SerialByteIn == '\n') {                 // Check to see if at the end of the line
      SerialEnd = true;                         // Set SerialEnd flag to indicate end of line
    }
  }

  if (SerialEnd) {                              // Check to see if SerialEnd flag is true

    if (SerialReadBuffer.startsWith("AT")) {    // Has a command been sent from local computer
      HC12.print(SerialReadBuffer);             // Send local command to remote HC12 before changing settings
      delay(100);                               //
      digitalWrite(HC12SetPin, LOW);            // Enter command mode
      delay(100);                               // Allow chip time to enter command mode
      Serial.print(SerialReadBuffer);           // Echo command to serial
      HC12.print(SerialReadBuffer);             // Send command to local HC12
      delay(500);                               // Wait 0.5s for a response
      digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
      delay(100);                               // Delay before proceeding
    } else {
      // HC12.print(SerialReadBuffer);             // Transmit non-command message
      csma_send(SerialReadBuffer);
    }
    SerialReadBuffer = "";                      // Clear SerialReadBuffer
    SerialEnd = false;                          // Reset serial end of line flag
  }

  if (HC12End) {                                // If HC12End flag is true
    if (HC12ReadBuffer.startsWith("AT")) {      // Check to see if a command is received from remote
      digitalWrite(HC12SetPin, LOW);            // Enter command mode
      delay(100);                               // Delay before sending command
      Serial.print(SerialReadBuffer);           // Echo command to serial.
      HC12.print(HC12ReadBuffer);               // Write command to local HC12
      delay(500);                               // Wait 0.5 s for reply
      digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
      delay(100);                               // Delay before proceeding
      HC12.println("Remote Command Executed");  // Acknowledge execution
    } else {
      Serial.print(HC12ReadBuffer);             // Send message to screen
    }
    HC12ReadBuffer = "";                        // Empty buffer
    HC12End = false;                            // Reset flag
  }
}

void csma_send(String msg) { // accepts a message to send as a string and transmits it politely using CSMA/CA
  int T = Tmin; 
  int startTime = millis();
  
  HC12.print(msg);
  Serial.print("[CSMA/CA sent] ");
  Serial.print(msg);
}

bool _csma_channel_idle() {
  return true;
}

bool _csma_collision() { // returns true if there was a collision
  return false;
}
