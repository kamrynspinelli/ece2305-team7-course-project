# ece2305-team7-course-project
Team 7 submission for ECE 2305: reverse-engineering and communicating with transceivers planted around Atwater Kent

# How to use
Sending text to the Arduino through the serial connection will pass that on to the HC-12. The serial monitor should be configured with newline as the line ending, since our program removes whitespace from all serial messages in both directions.

# To-do
- Finish implementation of CSMA/CA (specifically collision detection)
- Set up options
    - Channel access style (polite, impatient, aggressive)
    - Scan speed
- Packet filtering
- Get rid of remote command execution, once and for all

# To figure out
- How to filter packets when the nodes don't address them
- How to detect collisions
