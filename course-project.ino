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

  String *fields = split_packet("192.146.2.45|c4:f2:33:45:1e:eb|blahblahblah");
  Serial.print(fields[0]);
  Serial.print(fields[1]);
  Serial.print(fields[2]);
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
    csma_send(SerialReadBuffer);
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
  int T = Tmin; // upper bound on wait time
  int waitTime = random(0, T); // pick a wait time randomly between 0 and the upper limit
  int currentTime = 0; // initialize the amount of idle time we've waited to 0
  int endTimePrevIteration = millis(); // in order to get the wait loop going
  while (currentTime < waitTime) { // if we haven't waited enough time yet,
    // the idea here is that millis() - endTimePrevIteration should always equal the time that elapsed in this iteration of the look
    if (channel_idle()) { // and if the channel was idle since the last iteration,
      currentTime += millis() - endTimePrevIteration; // update the current time accordingly
    }
    endTimePrevIteration = millis(); // set the current time to be the end time of this iteration, so when the loop restarts we'll be able to keep time
  }
  while (_csma_collision()) { // if there was a collision, we need to back off
    T = min(T*2, Tmax); // increase the possible wait time exponentially (up to the maximum amount)
    waitTime = random(0, T); // pick a wait time randomly between 0 and the upper limit
    currentTime = 0; // initialize the amount of idle time we've waited to 0
    endTimePrevIteration = millis(); // in order to get the wait loop going
    while (currentTime < waitTime) { // if we haven't waited enough time yet,
      // the idea here is that millis() - endTimePrevIteration should always equal the time that elapsed in this iteration of the look
      if (channel_idle()) { // and if the channel was idle since the last iteration,
        currentTime += millis() - endTimePrevIteration; // update the current time accordingly
      }
      endTimePrevIteration = millis(); // set the current time to be the end time of this iteration, so when the loop restarts we'll be able to keep time
    }
  }
  HC12.print(msg);
  Serial.print("[CSMA/CA sent] ");
  Serial.print(msg);
}

bool channel_idle() { // returns true if the channel is idle
  if (!HC12.available()) { // if nothing is being received on the HC-12,
    return true; // then the channel is idle
  } else { // otherwise, we should accept what is being received and print it to the serial console
    // duplicated code from above
    while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
      HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
      if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
        HC12ReadBuffer += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
      }
      if (HC12ByteIn == '\n') {                   // At the end of the line
        HC12End = true;                           // Set HC12End flag to true
      }
    }
    Serial.print(HC12ReadBuffer);
    return false; // then let the calling function know that the channel was busy
  }
}

// STUB
bool _csma_collision() { // returns true if there was a collision
  return false;
}

// TODO: fix memory problems
String * split_packet(String packet) { // consumes a packet string and returns an array containing each field of the packet
  int numpipes = 0; // a variable to count the number of pipe characters in the packet
  for (int letter = 0; letter < packet.length(); letter++) { // loop through the letters in packet
    if (packet[letter] = '|') {
      numpipes++;
    }
  }
  String fields[numpipes+1]; // if the packet has n pipes, then it will have n+1 fields
  int packetPosition = 0; // start at the first character of the packet
  for (int field = 0; field < numpipes+1; field++) { // for each field,
    while (packet[packetPosition] != '|' && packetPosition != packet.length()) { // as long as the current character is not a pipe character and we aren't past the end of the string,
      fields[field] += packet[packetPosition]; // add this character to the current field,
      packetPosition++; // and move to the next character
    }
    packetPosition++; // in the case that this character was a pipe, move to the next character
  }
  return fields;
}

// STUB
void track_channel(String ip, int channel) { // finds the new channel of the node with the given IP address, and stores it in channel
  
}
