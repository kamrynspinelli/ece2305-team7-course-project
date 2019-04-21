/*  This code is based off of a starter code posted at:
 *  https://www.allaboutcircuits.com/projects/understanding-and-implementing-the-hc-12-wireless-transceiver-module/
 *  HC12 Send/Receive Example Program 2
 *  By Mark J. Hughes
 *  for AllAboutCircuits.com
 */

#include <SoftwareSerial.h>
#include <string.h>
#include <stdio.h>

// HC12 pin definitions
const byte HC12RxdPin = 4;                      // "RXD" Pin on HC12
const byte HC12TxdPin = 5;                      // "TXD" Pin on HC12
const byte HC12SetPin = 6;                      // "SET" Pin on HC12

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

int channel; // holds the current channel number

void setup() {

  HC12ReadBuffer.reserve(64);                   // Reserve 64 bytes for Serial message input
  SerialReadBuffer.reserve(64);                 // Reserve 64 bytes for HC12 message input

  pinMode(HC12SetPin, OUTPUT);                  // Output High for Transparent / Low for Command
  digitalWrite(HC12SetPin, HIGH);               // Enter Transparent mode
  delay(80);                                    // 80 ms delay before operation per datasheet
  Serial.begin(9600);                           // Open serial port to computer
  HC12.begin(9600);                             // Open software serial port to HC12

  channel = 1;
  Serial.println("[Channel 1]");
}

void loop() {
  digitalWrite(HC12SetPin, LOW);            // Enter command mode
  delay(100);                               // Allow chip time to enter command mode
  if (channel >= 1 && channel <= 9) {
    HC12.print("AT+C00");             // Send command to local HC12
    HC12.print(channel);
  }
  if (channel >= 10 && channel <= 99) {
    HC12.print("AT+C0");             // Send command to local HC12
    HC12.print(channel);
  }
  if (channel >= 100 && channel <= 127) {
    HC12.print("AT+C");             // Send command to local HC12
    HC12.print(channel);
  }
  delay(500);                               // Wait 0.5s for a response
  digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
  // print the OK+C___ message before listening for a packet
  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      HC12ReadBuffer += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }
  if (HC12End) {                                // If HC12End flag is true
    Serial.print(HC12ReadBuffer);             // Send message to screen
    HC12ReadBuffer = "";                        // Empty buffer
    HC12End = false;                            // Reset flag
  }
  
  delay(2000);                               // Wait 2s to listen for a packet

  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      HC12ReadBuffer += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }

  if (HC12End) {                                // If HC12End flag is true
    Serial.print(HC12ReadBuffer);             // Send message to screen
    HC12ReadBuffer = "";                        // Empty buffer
    HC12End = false;                            // Reset flag
  }

  channel++; // move to the next channel
  if (channel > 127) { // if we're past the 127th channel,
    channel = 1; // go back to the first
  }
  Serial.print("[Channel "); // display over serial to PC that we're changing channels
  Serial.print(channel);
  Serial.println("]");
}
