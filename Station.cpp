#include <Phpoc.h>

// WebSocket server instance listening on port 80 for client connections
PhpocServer server(80);

// Pin assignments using modern C++ constexpr for type safety and clarity
constexpr int DOOR_DIRECTION_PIN = 4;    // Pin controlling the door motor direction
constexpr int DOOR_ENABLE_PIN = 5;       // Pin to enable/disable the door motor
constexpr int PLATE_DIRECTION_PIN = 6;   // Pin controlling the landing plate motor direction
constexpr int PLATE_ENABLE_PIN = 7;      // Pin to enable/disable the landing plate motor
constexpr int DOOR_PHOTO_PIN = 8;        // Pin for door photo sensor (LOW when door is closed)
constexpr int PLATE_PHOTO_PIN = 9;       // Pin for landing plate photo sensor (LOW when plate is retracted)

// Function prototypes for motor control operations
void StopAllMotors();       // Stops all motors by disabling them
void CloseDoor();           // Starts closing the door
void OpenDoor();            // Starts opening the door
void RetractPlate();        // Starts retracting the landing plate (moves in)
void ExtendPlate();         // Starts extending the landing plate (moves out)

void setup() {
    // Initialize serial communication at 9600 baud for debugging
    Serial.begin(9600);
    // Wait for the serial port to connect (needed for some Arduino boards)
    while (!Serial);

    // Initialize PHPoC [WiFi] Shield with logging enabled for SPI and network
    Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);

    // Start WebSocket server with the specified endpoint "remote_push"
    server.beginWebSocket("remote_push");

    // Print the IP address of the PHPoC [WiFi] Shield to the serial monitor
    Serial.print("WebSocket server address : ");
    Serial.println(Phpoc.localIP());

    // Set pin modes for motor control outputs and sensor inputs
    pinMode(PLATE_DIRECTION_PIN, OUTPUT);  // Plate direction control pin
    pinMode(PLATE_ENABLE_PIN, OUTPUT);     // Plate motor enable pin
    pinMode(DOOR_DIRECTION_PIN, OUTPUT);   // Door direction control pin
    pinMode(DOOR_ENABLE_PIN, OUTPUT);      // Door motor enable pin
    pinMode(DOOR_PHOTO_PIN, INPUT);        // Door photo sensor input pin
    pinMode(PLATE_PHOTO_PIN, INPUT);       // Plate photo sensor input pin

    // Initially stop all motors to prevent unintended movement
    StopAllMotors();
}

void loop() {
    // Wait for a new client connection from the WebSocket server
    PhpocClient client = server.available();

    // Check if a client is connected
    if (client) {
        // Check if there are bytes available to read from the client
        if (client.available() > 0) {
            // Read the incoming command character from the client
            char command = client.read();

            // Read current states of the photo sensors
            // Assuming DOOR_PHOTO_PIN is LOW when the door is closed
            bool isDoorClosed = digitalRead(DOOR_PHOTO_PIN) == LOW;
            // Assuming PLATE_PHOTO_PIN is LOW when the plate is retracted (in)
            bool isPlateIn = digitalRead(PLATE_PHOTO_PIN) == LOW;

            // Process the command received from the client using a switch statement
            switch (command) {
                case 'A':
                    // Command to extend the landing plate
                    Serial.println("Extend Plate");
                    // Extend plate only if the door is closed (sensor LOW)
                    // Note: This logic might need verification, as extending the plate
                    // typically requires the door to be open for physical clearance
                    if (isDoorClosed) {
                        ExtendPlate();
                    }
                    break;

                case 'D':
                    // Command to retract the landing plate
                    Serial.println("Retract Plate");
                    // Start retracting the plate
                    RetractPlate();
                    break;

                case 'B':
                    // Command to open the door
                    Serial.println("Open Door");
                    // Start opening the door
                    OpenDoor();
                    break;

                case 'E':
                    // Command to close the door
                    Serial.println("Close Door");
                    // Close door only if the plate is retracted (sensor LOW)
                    // This ensures clearance for door movement
                    if (isPlateIn) {
                        CloseDoor();
                    }
                    break;

                case 'G':
                    // Command for takeoff sequence: open door, then extend plate
                    Serial.println("Take Off Sequence");
                    // Start sequence only if the plate is retracted (sensor LOW)
                    if (isPlateIn) {
                        // Start opening the door
                        OpenDoor();
                        // Check if door is closed (sensor LOW) after starting to open
                        // Note: Immediate check might be premature; door may not be open yet
                        // Client might need to send separate commands after door opens
                        if (isDoorClosed) {
                            ExtendPlate();
                        }
                    }
                    break;

                case 'H':
                    // Command for landing sequence: retract plate, then close door
                    Serial.println("Landing Sequence");
                    // Start retracting the plate
                    RetractPlate();
                    // Check if plate is retracted (sensor LOW) after starting to retract
                    // Note: Immediate check might be premature; plate may not be in yet
                    // Client might need to send separate commands after plate retracts
                    if (isPlateIn) {
                        // Start closing the door
                        CloseDoor();
                        // Check if door is closed (sensor LOW) after starting to close
                        // If door is closed, stop all motors
                        if (isDoorClosed) {
                            StopAllMotors();
                        }
                    }
                    break;

                case 'I':
                    // Command to stop all motor movements
                    Serial.println("Stop All");
                    // Stop all motors
                    StopAllMotors();
                    break;

                default:
                    // Handle unrecognized commands
                    Serial.println("Unknown command");
                    break;
            }
        }
    }
}

// Function to stop all motors by disabling them
// Note: Assuming motor enable pins are active-low (LOW to enable, HIGH to disable)
void StopAllMotors() {
    digitalWrite(DOOR_ENABLE_PIN, HIGH);   // Disable door motor
    digitalWrite(PLATE_ENABLE_PIN, HIGH);  // Disable plate motor
}

// Function to start closing the door
// Note: Direction pin HIGH sets motor to close direction
//       Enable pin LOW activates the motor
void CloseDoor() {
    digitalWrite(DOOR_DIRECTION_PIN, HIGH);  // Set direction to close
    digitalWrite(DOOR_ENABLE_PIN, LOW);      // Enable motor
}

// Function to start opening the door
// Note: Direction pin LOW sets motor to open direction
//       Enable pin LOW activates the motor
void OpenDoor() {
    digitalWrite(DOOR_DIRECTION_PIN, LOW);   // Set direction to open
    digitalWrite(DOOR_ENABLE_PIN, LOW);      // Enable motor
}

// Function to start retracting the landing plate (move in)
// Note: Direction pin HIGH sets motor to retract direction
//       Enable pin LOW activates the motor
void RetractPlate() {
    digitalWrite(PLATE_DIRECTION_PIN, HIGH); // Set direction to retract (in)
    digitalWrite(PLATE_ENABLE_PIN, LOW);     // Enable motor
}

// Function to start extending the landing plate (move out)
// Note: Direction pin LOW sets motor to extend direction
//       Enable pin LOW activates the motor
void ExtendPlate() {
    digitalWrite(PLATE_DIRECTION_PIN, LOW);  // Set direction to extend (out)
    digitalWrite(PLATE_ENABLE_PIN, LOW);     // Enable motor
}