# ece2305-team7-course-project
Team 7 submission for ECE 2305: reverse-engineering and communicating with transceivers planted around Atwater Kent

# How to use
## Manual mode
Sending text to the Arduino through the serial connection will pass that on to the HC-12. The serial monitor should be configured with newline as the line ending, since our program removes whitespace from all serial messages in both directions.
## Automatic mode (`solve`)
At the serial monitor, you can send to the Arduino the message `solve(n)` where n is one of 0..3. This will tell the Arduino to run the programmed `solve()` function on the `n`th node. This function is an automatic solver which scans to find the right node, then associates, authenticates, and extracts the flag.

# To-do
- Finish implementation of CSMA/CA (specifically collision detection)
- Set up options
    - Channel access style (polite, impatient, aggressive)
    - Scan speed
- Packet filtering
- Get rid of remote command execution, once and for all
- Implement scanning for nodes with different baud rates

# To figure out
- How to filter packets when the nodes don't address them
- How to detect collisions
