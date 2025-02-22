#include <Phpoc.h>

// Server instances for ROS and web communication
PhpocServer ros_server(23);  // ROS server on port 23
PhpocServer web_server(80);  // Web server on port 80

// Flag to track if a client was previously connected
bool alreadyConnected = false;

// Pin assignments using modern C++ constexpr for type safety and clarity
constexpr int DOOR_DIRECTION_PIN = 4;    // Pin controlling the door motor direction
constexpr int DOOR_ENABLE_PIN = 5;       // Pin to enable/disable the door motor
constexpr int PLATE_DIRECTION_PIN = 6;   // Pin controlling the landing plate motor direction
constexpr int PLATE_ENABLE_PIN = 7;      // Pin to enable/disable the landing plate motor
constexpr int DOOR_PHOTO_PIN = 8;        // Pin for door photo sensor
constexpr int PLATE_PHOTO_PIN = 9;       // Pin for landing plate photo sensor
constexpr int WPT_RELAY_PIN = 10;        // Pin for wireless power transfer relay

// Timing constants for door and plate operations
constexpr unsigned long DOOR_TIME = 25000;  // Time in milliseconds for door operation
constexpr unsigned long PLATE_TIME = 45000; // Time in milliseconds for plate operation

// Function prototypes for motor and relay control operations
void StopAllMotors();       // Stops all motors by disabling them
void CloseDoor();           // Starts closing the door
void OpenDoor();            // Starts opening the door
void RetractPlate();        // Starts retracting the landing plate (moves in)
void ExtendPlate();         // Starts extending the landing plate (moves out)
void TakeOffSequence();     // Executes the takeoff sequence: open door, then extend plate
void LandingSequence();     // Executes the landing sequence: retract plate, then close door
void EnableWirelessPower(); // Turns on wireless power
void DisableWirelessPower();// Turns off wireless power

// Variable to control wireless power state (0: on, 1: off)
int wirelessPowerState = 1; // Initially off

void setup() {
    // Initialize serial communication at 9600 baud for debugging
    Serial.begin(9600);
    // Wait for the serial port to connect (needed for some Arduino boards)
    while (!Serial);

    // Initialize PHPoC [WiFi] Shield with logging enabled for SPI and network
    Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);

    // Start WebSocket server for web clients
    web_server.beginWebSocket("remote_push");
    // Start ROS server for ROS clients
    ros_server.begin();

    // Print IP addresses for both servers to the serial monitor
    Serial.print("WebSocket server address : ");
    Serial.println(Phpoc.localIP());
    Serial.print("ROS server address : ");
    Serial.println(Phpoc.localIP());

    // Set pin modes for motor control outputs, sensor inputs, and relay
    pinMode(PLATE_DIRECTION_PIN, OUTPUT);
    pinMode(PLATE_ENABLE_PIN, OUTPUT);
    pinMode(DOOR_DIRECTION_PIN, OUTPUT);
    pinMode(DOOR_ENABLE_PIN, OUTPUT);
    pinMode(DOOR_PHOTO_PIN, INPUT);
    pinMode(PLATE_PHOTO_PIN, INPUT);
    pinMode(WPT_RELAY_PIN, OUTPUT);

    // Initially stop all motors and ensure wireless power is off
    StopAllMotors();
    DisableWirelessPower();
}

void loop() {
    // Wait for new clients from ROS and web servers
    PhpocClient ros_client = ros_server.available();
    PhpocClient web_client = web_server.available();

    // If either client is connected
    if (ros_client || web_client) {
        if (!alreadyConnected) {
            // Clear transmission buffers for new connections
            ros_client.flush();
            web_client.flush();
            Serial.println("New client connected");
            alreadyConnected = true;
        }

        // Handle incoming data from ROS client
        if (ros_client.available() > 0) {
            char command = ros_client.read();
            switch (command) {
                case 'a':
                    Serial.println("ROS: Extend Plate");
                    ExtendPlate();
                    ros_server.write('A');  // Acknowledge command
                    break;
                case 'b':
                    Serial.println("ROS: Retract Plate");
                    RetractPlate();
                    ros_server.write('B');
                    break;
                case 'c':
                    Serial.println("ROS: Open Door");
                    OpenDoor();
                    ros_server.write('C');
                    break;
                case 'd':
                    Serial.println("ROS: Close Door");
                    CloseDoor();
                    ros_server.write('D');
                    break;
                case 'e':
                    Serial.println("ROS: Wireless Power On");
                    wirelessPowerState = 0;  // Set state to on
                    ros_server.write('E');
                    break;
                case 'f':
                    Serial.println("ROS: Wireless Power Off");
                    wirelessPowerState = 1;  // Set state to off
                    ros_server.write('F');
                    break;
                case 'z':
                    Serial.println("ROS: Take Off Sequence");
                    TakeOffSequence();
                    ros_server.write('Z');
                    break;
                case 'x':
                    Serial.println("ROS: Landing Sequence");
                    LandingSequence();
                    ros_server.write('X');
                    break;
                case 'g':
                    Serial.println("ROS: Stop All");
                    StopAllMotors();
                    ros_server.write('G');
                    break;
                default:
                    Serial.println("Unknown ROS command");
                    break;
            }
        }

        // Handle incoming data from web client
        if (web_client.available() > 0) {
            char command = web_client.read();
            switch (command) {
                case 'A':
                    Serial.println("Web: Extend Plate");
                    ExtendPlate();
                    break;
                case 'D':
                    Serial.println("Web: Retract Plate");
                    RetractPlate();
                    break;
                case 'B':
                    Serial.println("Web: Open Door");
                    OpenDoor();
                    break;
                case 'E':
                    Serial.println("Web: Close Door");
                    CloseDoor();
                    break;
                case 'C':
                    Serial.println("Web: Wireless Power On");
                    wirelessPowerState = 0;  // Set state to on
                    break;
                case 'F':
                    Serial.println("Web: Wireless Power Off");
                    wirelessPowerState = 1;  // Set state to off
                    break;
                case 'G':
                    Serial.println("Web: Take Off Sequence");
                    TakeOffSequence();
                    break;
                case 'H':
                    Serial.println("Web: Landing Sequence");
                    LandingSequence();
                    break;
                case 'I':
                    Serial.println("Web: Stop All");
                    StopAllMotors();
                    break;
                default:
                    Serial.println("Unknown Web command");
                    break;
            }
        }
    }

    // Control wireless power based on the state variable
    if (wirelessPowerState == 0) {
        EnableWirelessPower();
    } else {
        DisableWirelessPower();
    }
}

// Function to stop all motors by disabling them
void StopAllMotors() {
    digitalWrite(DOOR_ENABLE_PIN, HIGH);   // Disable door motor
    digitalWrite(PLATE_ENABLE_PIN, HIGH);  // Disable plate motor
}

// Function to start closing the door
void CloseDoor() {
    digitalWrite(DOOR_DIRECTION_PIN, HIGH);  // Set direction to close
    digitalWrite(DOOR_ENABLE_PIN, LOW);      // Enable motor
}

// Function to start opening the door
void OpenDoor() {
    digitalWrite(DOOR_DIRECTION_PIN, LOW);   // Set direction to open
    digitalWrite(DOOR_ENABLE_PIN, LOW);      // Enable motor
}

// Function to start retracting the landing plate (move in)
void RetractPlate() {
    digitalWrite(PLATE_DIRECTION_PIN, HIGH); // Set direction to retract (in)
    digitalWrite(PLATE_ENABLE_PIN, LOW);     // Enable motor
}

// Function to start extending the landing plate (move out)
void ExtendPlate() {
    digitalWrite(PLATE_DIRECTION_PIN, LOW);  // Set direction to extend (out)
    digitalWrite(PLATE_ENABLE_PIN, LOW);     // Enable motor
}

// Function to execute the takeoff sequence: open door, wait, then extend plate
void TakeOffSequence() {
    OpenDoor();          // Start opening the door
    delay(DOOR_TIME);    // Wait for door to fully open
    ExtendPlate();       // Start extending the plate
    delay(PLATE_TIME);   // Wait for plate to fully extend
}

// Function to execute the landing sequence: retract plate, wait, then close door
void LandingSequence() {
    RetractPlate();      // Start retracting the plate
    delay(PLATE_TIME);   // Wait for plate to fully retract
    CloseDoor();         // Start closing the door
    delay(DOOR_TIME);    // Wait for door to fully close
}

// Function to enable wireless power
void EnableWirelessPower() {
    digitalWrite(WPT_RELAY_PIN, HIGH);  // Assuming HIGH activates the relay
}

// Function to disable wireless power
void DisableWirelessPower() {
    digitalWrite(WPT_RELAY_PIN, LOW);   // Assuming LOW deactivates the relay
}