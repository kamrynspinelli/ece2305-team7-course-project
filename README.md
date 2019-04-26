# ece2305-team7-course-project
Team 7 submission for ECE 2305: reverse-engineering and communicating with transceivers planted around Atwater Kent

# How to use
## Manual mode
Sending text to the Arduino through the serial connection will pass that on to the HC-12. The serial monitor should be configured with newline as the line ending, since our program removes whitespace from all serial messages in both directions.
## Automatic mode (`solve`)
At the serial monitor, you can send to the Arduino the message `solve(n)` where n is one of 0..3. This will tell the Arduino to run the programmed `solve()` function on the `n`th node. This function is an automatic solver which scans to find the right node, then associates, authenticates, and extracts the flag.
## Setting options
The program currently allows users to set two options at the serial prompt: channel access style and scan speed. The following commands at the serial monitor will produce the described effect.
- `accessstyle=polite`: polite channel access
- `accessstyle=impatient`: impatient channel access
- `accessstyle=aggressive`: aggressive channel access
- `scanspeed=slow`: slow scan speed
- `scanspeed=medium`: medium scan speed
- `scanspeed=fast`: fast scan speed

# To-do
- Packet filtering
- Get rid of remote command execution, once and for all
- Implement scanning for nodes with different baud rates

# To figure out
- How to filter packets when the nodes don't address them

# Be careful with
- If you simply wait for a certain amount of time to let the serial buffer fill, then read out of it, you'll get garbled text. Instead, the right way to handle this is to use a while loop as follows:
```int starttime = millis();
while(millis() < starttime+2000){
  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    if (!isspace(HC12ByteIn) || HC12ByteIn == '\n') { // If the incoming character is a non-newline whitespace character,
      text += char(HC12ByteIn);         // Write that character of byteIn to HC12ReadBuffer
    }
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }
}
```
- Once roughly 75% of RAM is used for global variables and constant strings, undefined behavior may occur. We need to keep an eye on this and make optimizations if necessary.
