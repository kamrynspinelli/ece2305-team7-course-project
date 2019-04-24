/*  This code is based off of a starter code posted at:
 *  https://www.allaboutcircuits.com/projects/understanding-and-implementing-the-hc-12-wireless-transceiver-module/
 *  HC12 Send/Receive Example Program 2
 *  By Mark J. Hughes
 *  for AllAboutCircuits.com
 */

#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <string.h>
#include <stdio.h>

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

// MAC and password for our team
String mac = "c6:21:f4:2f:1b:fa";
String password = "jYygwBQ90QiVQ85P";

// IP addresses for the mystery nodes (excluding the test node outside Wyglinski's office
String ips[4] = { "63.180.94.100",
                  "97.121.215.176",
                  "166.248.98.36",
                  "251.236.64.145" };

// Flags found through manual interaction with the nodes
String flags[4] = { "cmd:X9yFFij1doeuTjTg",
                    "cmd:Denh5l4vjdwFbJdp",
                    "cmd:0UrKhRPNS0kBE4Db",
                    "cmd:eubOFZcSxM8UnpOr" };

// Mystery nodes wait 1.3s or .7 s (alternating) between successive IP broadcasts

// Software Serial ports Rx and Tx are opposite the HC12 Rx and Tx
// Create Software Serial Port for HC12
// SoftwareSerial HC12(HC12TxdPin, HC12RxdPin);
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

  bool readSerial = false; // at this point in the loop, we haven't read anything from the serial connection to the PC
  while (Serial.available()) {                  // If Arduino's computer rx buffer has data
    SerialByteIn = Serial.read();               // Store each character in byteIn
    if (!isspace(SerialByteIn)) { // If the incoming character is a non-newline whitespace character,
      SerialReadBuffer += char(SerialByteIn);     // Write that character of byteIn to SerialReadBuffer
    }
    if (SerialByteIn == '\n') {                 // Check to see if at the end of the line
      SerialEnd = true;                         // Set SerialEnd flag to indicate end of line
    }
  }

  if (SerialEnd) {                              // Check to see if SerialEnd flag is true
    if (SerialReadBuffer.startsWith("AT")) {    // Has a command been sent from local computer
      digitalWrite(HC12SetPin, LOW);            // Enter command mode
      delay(100);                               // Allow chip time to enter command mode
      Serial.print(SerialReadBuffer);           // Echo command to serial
      HC12.print(SerialReadBuffer);             // Send command to local HC12
      delay(500);                               // Wait 0.5s for a response
      digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
      delay(100);                               // Delay before proceeding
    } else if (SerialReadBuffer.startsWith("solve(w)")) { // if the solve(w) command was issued, then solve for Wyglinski's office for debugging purposes
      Serial.println("Solving for Wyglinski's office node...");
      solvew();
    } else if (SerialReadBuffer.startsWith("solve(")) { // if the general solve command was issued
      // syntax: solve(n) where n is 0..3 (the four nodes)
      int ipindex = SerialReadBuffer.charAt(6) - '0'; // convert the given parameter as a character to an int
      Serial.print("Solving for the node with index "); // for debugging purposes
      Serial.print(ipindex);
      Serial.print(" corresponding to IP address ");
      Serial.println(ips[ipindex]);
      solve(ips[ipindex]); // run the solve() function on that IP address
    } else {
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
  flush_hc12();
  HC12.print(msg);
  Serial.print("[CSMA/CA sent] ");
  Serial.println(msg);
}

// STUB
void _csma_polite(String msg) { // CSMA/CA send function which transmits politely (following the flowchart given in the design proposal)
  
}

// STUB
void _csma_impatient(String msg) { // CSMA/CA send function which transmits impatiently, using a fixed backoff scheme
  
}

void _csma_aggressive(String msg) { // CSMA/CA send function which transmits aggressively, transmitting whenever the channel is idle
  while(!channel_idle()) {
    // wait until the channel stops being busy
  }
  HC12.print(msg); // then send the message to the HC-12
  Serial.print("[CSMA/CA sent] ");
  Serial.println(msg);
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

// finds the new channel of the node with the given IP address, leaves the HC-12 set to that channel, and returns the channel number
// a return value of -1 indicates that the node with the given IP was not found
// WARNING: may run for a long time
int track_channel(String ip) {
  Serial.print("Looking for node with IP ");
  Serial.print(ip);
  Serial.println("...");
  String incoming; // declare a string to be used later for incoming data
  int starttime;
  for (int attempt = 1; attempt <= 3; attempt++) { // we'll scan through all the channels three times to try and find the node
    // for (int channel = 0; channel <= 127; channel++) { // scan through each channel
    for (int channel = 1; channel <= 127; channel++) { // scan through each channel
      Serial.print("Scanning channel ");
      Serial.println(channel);
      flush_hc12();
      // set the HC-12 to the specified channel
      digitalWrite(HC12SetPin, LOW);            // Enter command mode
      delay(100);                               // Allow chip time to enter command mode
      // the following if statements are messy but that's life
      // we can make this neater using sprintf
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
      digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
      // print the OK+C___ message before listening for a packet
      delay(500);
      flush_hc12(); // flush the buffer
      delay(1400); // collect data from the HC-12 for 1.4s
      if (HC12.available()) { // if there's data in the HC-12's buffer, then we should record again and inspect what we received.
        Serial.print("Found signal on channel ");
        Serial.println(channel);
        starttime = millis(); // and collect data again for 1.4 seconds
        while (millis() < starttime+4000){
          while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
            HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
            if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
              incoming += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
            }
          }
        }
        Serial.println("Printing received data...");
        Serial.println(incoming);
        Serial.println("Finished printing received data. About to decide if we found the right IP.");
        Serial.println(incoming.indexOf(ip)); // test print
        if (incoming.indexOf(ip) >= 0) {// and it's broadcasting the right IP, ie. if the IP string occurs somewhere in the incoming string
          Serial.print("Found node with IP ");
          Serial.print(ip);
          Serial.print(" on channel ");
          Serial.println(channel);
          return channel; // then we're on the right channel
        } else {
          Serial.println("Wrong IP");
        }
      }
      incoming = "";
    }
  }
  return -1; // if we didn't find the right node after all that searching, then let the calling function know
}

// associates with a node having the specified IP on the current channel by sending a packet of the following format
// ip|mac
void associate(String ip) {
  String packet = ip + "|" + mac; // construct the packet
  csma_send(packet); // send the packet
}

// authenticates with a node having the specified IP on the current channel by sending a packet of the following format
// ip|mac|password
void authenticate(String ip) {
  String packet = ip + "|" + mac + "|" + password; // construct the packet
  csma_send(packet); // send the packet
}

// extracts the flag from a node having the specified IP on the current channel by sending a packet of the following format
// ip|mac|get_flag
// puts into &response all the text received all over the serial connection from the HC-12
void extract_flag(String ip, String &response) {
  String packet = ip + "|" + mac + "|" + "get_flag"; // construct the packet
  csma_send(packet); // send the packet
  /*delay(2000); // wait 2s for the node to respond
  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      response += char(HC12ByteIn);         // Write that character of byteIn to response
    }
  }
  Serial.print(response);*/
}

// solves the challenge by scanning for the node with the specified IP, then associating, authenticating, and extracting the flag
String solve(String ip) {
  track_channel(ip); // switch to the right channel for the desired node
  String received;
  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }
  Serial.println(received);
  received = "";
  // for (int i = 1; i <= 3; i++) {
  for (int i = 1; i <= 1; i++) {
    flush_hc12();
    associate(ip); // try associating three times
    /*int starttime = millis();
    while (millis() < starttime+2000) {
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    Serial.println(received);
    received = "";*/
    delay(4000);
    flush_hc12();
  }
  // for (int i = 1; i <= 3; i++) {
  for (int i = 1; i <= 1; i++) {
    flush_hc12();
    authenticate(ip); // try authenticating times
    /*int starttime = millis();
    while (millis() < starttime+2000) {
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    Serial.println(received);
    received = "";*/
    delay(4000);
    flush_hc12();
  }
  // for (int i = 1; i <= 3; i++) { // finally, we will try thrice to obtain the flag itself
  for (int i = 1; i <= 1; i++) {
    String maybeflag; // declare a string to hold the possible flag
    flush_hc12();
    extract_flag(ip, maybeflag); // get a possible flag
    int starttime = millis();
    while (millis() < starttime+4000){
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          maybeflag += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    /*Serial.print("[maybeflag begin] ");
    Serial.println(maybeflag);
    Serial.println("[maybleflag end]");*/
    int poscmd = maybeflag.indexOf("cmd:"); // look for the index of the substring "cmd:" in the possible flag
    if (poscmd >= 0 && poscmd+20 < maybeflag.length()) { // if this position is greater than 0 , we found this substring in the string
      // and if the end ofthe flag wasn't cut off, we can return the flag which is 20 characters long in total
      String flag = maybeflag.substring(poscmd, poscmd+20); // extract the flag itself from the string
      Serial.println("The flag is:");
      Serial.println(flag); // print it to the serial monitor
      return flag; // and return it!
    }
    delay(2000); // wait a bit
  }
  /*for (int i = 1; i <= 3; i++) {
    associate(ip); // try associating three times
    delay(1000); // wait a bit
  }
  for (int i = 1; i <= 3; i++) {
    authenticate(ip); // try authenticating times
    delay(1000); // wait a bit
  }
  for (int i = 1; i <= 3; i++) { // finally, we will try thrice to obtain the flag itself
    String maybeflag; // declare a string to hold the possible flag
    extract_flag(ip, maybeflag); // get a possible flag
    int poscmd = maybeflag.indexOf("cmd:"); // look for the index of the substring "cmd:" in the possible flag
    if (poscmd >= 0 && poscmd+20 < maybeflag.length()) { // if this position is greater than 0 , we found this substring in the string
      // and if the end ofthe flag wasn't cut off, we can return the flag which is 20 characters long in total
      String flag = maybeflag.substring(poscmd, poscmd+20); // extract the flag itself from the string
      Serial.println("The flag is:");
      Serial.println(flag); // print it to the serial monitor
      return flag; // and return it!
    }
    delay(1000); // wait a bit
  }*/
}

// solves the challenge for Wyglinski's node by switching to channel 1, then associating, authenticating, and extracting the flag
String solvew() {
  String ip = "167.251.78.6";
  digitalWrite(HC12SetPin, LOW);            // Enter command mode
  delay(100);                               // Allow chip time to enter command mode
  HC12.print("AT+C001");             // Send command to local HC12
  delay(500);                               // Wait 0.5s for a response
  digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
  // print the OK+C___ message before listening for a packet
  delay(500);
  String received;
  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }
  Serial.println(received);
  received = "";
  // for (int i = 1; i <= 3; i++) {
  for (int i = 1; i <= 1; i++) {
    flush_hc12();
    associate(ip); // try associating three times
    /*int starttime = millis();
    while (millis() < starttime+2000) {
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    Serial.println(received);
    received = "";*/
    delay(4000);
    flush_hc12();
  }
  // for (int i = 1; i <= 3; i++) {
  for (int i = 1; i <= 1; i++) {
    flush_hc12();
    authenticate(ip); // try authenticating times
    /*int starttime = millis();
    while (millis() < starttime+2000) {
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          received += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    Serial.println(received);
    received = "";*/
    delay(4000);
    flush_hc12();
  }
  // for (int i = 1; i <= 3; i++) { // finally, we will try thrice to obtain the flag itself
  for (int i = 1; i <= 1; i++) {
    String maybeflag; // declare a string to hold the possible flag
    flush_hc12();
    extract_flag(ip, maybeflag); // get a possible flag
    int starttime = millis();
    while (millis() < starttime+4000){
      while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
        HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
        if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
          maybeflag += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
        }
        if (HC12ByteIn == '\n') {                   // At the end of the line
          HC12End = true;                           // Set HC12End flag to true
        }
      }
    }
    /*Serial.print("[maybeflag begin] ");
    Serial.println(maybeflag);
    Serial.println("[maybleflag end]");*/
    int poscmd = maybeflag.indexOf("cmd:"); // look for the index of the substring "cmd:" in the possible flag
    if (poscmd >= 0 && poscmd+20 < maybeflag.length()) { // if this position is greater than 0 , we found this substring in the string
      // and if the end ofthe flag wasn't cut off, we can return the flag which is 20 characters long in total
      String flag = maybeflag.substring(poscmd, poscmd+20); // extract the flag itself from the string
      Serial.println("The flag is:");
      Serial.println(flag); // print it to the serial monitor
      return flag; // and return it!
    }
    delay(2000); // wait a bit
  }
}

void flush_hc12() {
  while (HC12.available()) {
    HC12.read();
  }
}
