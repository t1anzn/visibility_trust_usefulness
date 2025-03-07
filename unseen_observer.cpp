/*
 * The Ultrasonic LED Alert System
 * 
 * This programme uses an ultrasonic sensor to detect objects and shows proximity
 * warnings with LEDs:
 * - Green LED blinks when objects are at medium distance
 * - Red LED blinks when objects are too close
 * - LEDs are off when no objects are detected
 * 
 * Hardware:
 * - Arduino Uno
 * - HC-SR04 Ultrasonic Sensor
 * - 2 LEDs (Red, Green)
 * - 2 x 220 Ohm resistors
 */

// Pin definitions
const int trigPin = 2;      // Trigger pin of the ultrasonic sensor
const int echoPin = 3;      // Echo pin of the ultrasonic sensor
const int redLedPin = 4;    // Red LED pin
const int greenLedPin = 5;  // Green LED pin

// Detection parameters
const int maxDistance = 250;  // Maximum distance to detect (in cm)
const int minDistance = 50;   // Distance for red alert zone (in cm)

// Blinking parameters
const int blinkRate = 500;     // Milliseconds between blinks (slowed down)
unsigned long lastBlinkTime = 0;
bool blinkState = false;       // Current blink state

// Timing parameters for slower detection
const int detectionInterval = 1000;  // Only check distance every 1 second
unsigned long lastDetectionTime = 0;

// Averaging parameters for smoother readings
const int numReadings = 5;     // Number of readings to average
int readings[numReadings];     // Array to store readings
int readIndex = 0;             // Current position in the array
int totalReadings = 0;         // Running total
int averageDistance = 0;       // Average distance

// Variables
long duration;    // Duration of the echo pulse
int distance;     // Current distance reading
bool inRedZone = false;    // Object in red alert zone
bool inGreenZone = false;  // Object in normal detection zone

// Timing variables for outputs
unsigned long lastOutputTime = 0;
const int outputInterval = 500;  // Output serial info every 500ms

// Timing for concept reminder
const unsigned long conceptInterval = 30000;  // Print concept every minute
unsigned long lastConceptTime = 0;

void setup() {
  // Initialise Serial Monitor
  Serial.begin(9600);
  
  // Configure pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  
  // Initialise all LEDs to OFF
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  
  // Initialise readings array
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  
  // Test LEDs
  Serial.println("Testing LEDs...");
  
  digitalWrite(redLedPin, HIGH);
  delay(500);
  digitalWrite(redLedPin, LOW);
  
  digitalWrite(greenLedPin, HIGH);
  delay(500);
  digitalWrite(greenLedPin, LOW);
  
  Serial.println("LED test complete");
  
  // Output initial concept and parameter info
  printConceptInfo();
  
  delay(1000);  // Pause to allow time to read settings
}

void loop() {
  unsigned long currentTime = millis();
  
  // Only update distance readings at the detection interval
  if (currentTime - lastDetectionTime >= detectionInterval) {
    updateDistanceReading();
    lastDetectionTime = currentTime;
    
    // Determine which zone the object is in based on the averaged distance
    inRedZone = (averageDistance < minDistance && averageDistance > 0);
    inGreenZone = (averageDistance >= minDistance && averageDistance < maxDistance);
  }
  
  // Handle LED blinking
  if (currentTime - lastBlinkTime >= blinkRate) {
    blinkState = !blinkState;  // Toggle blink state
    lastBlinkTime = currentTime;
  }
  
  // Set LED states based on zone
  if (inRedZone) {
    // Blink red LED if in red zone (too close)
    digitalWrite(redLedPin, blinkState);
    digitalWrite(greenLedPin, LOW);
  } 
  else if (inGreenZone) {
    // Blink green LED if in green zone (medium distance)
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, blinkState);
  } 
  else {
    // No object in range - turn off LEDs
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, HIGH);
  }
  
  // Send status to serial monitor less frequently
  if (currentTime - lastOutputTime >= outputInterval) {
    sendDebugOutput();
    lastOutputTime = currentTime;
  }
  
  // Print concept info periodically
  if (currentTime - lastConceptTime >= conceptInterval) {
    printConceptInfo();
    lastConceptTime = currentTime;
  }
  
  // Small delay for stability
  delay(10);
}

/**
 * Prints the concept information and current parameters
 */
void printConceptInfo() {
  Serial.println("\n------------------------------------------------");
  Serial.println("THE UNSEEN OBSERVER - Security Visibility Exercise");
  Serial.println("------------------------------------------------");
  Serial.println("Concept: Security systems often collect data without");
  Serial.println("making it obvious to users. This project plays with");
  Serial.println("how much visibility users have.");
  Serial.println("------------------------------------------------");
  Serial.print("Red Alert Distance: < ");
  Serial.print(minDistance);
  Serial.println(" cm");
  Serial.print("Green Alert Distance: ");
  Serial.print(minDistance);
  Serial.print(" to ");
  Serial.print(maxDistance);
  Serial.println(" cm");
  Serial.print("Detection update interval: ");
  Serial.print(detectionInterval / 1000.0);
  Serial.println(" seconds");
  Serial.println("------------------------------------------------");
}

/**
 * Updates the distance reading by taking a new measurement
 * and updating the rolling average
 */
void updateDistanceReading() {
  // Take a new distance measurement
  distance = measureDistance();
  
  // Make sure distance is in a reasonable range
  if (distance <= 0 || distance > 400) {
    distance = 400;  // Consider it out of range
  }
  
  // Update running average
  totalReadings = totalReadings - readings[readIndex];
  readings[readIndex] = distance;
  totalReadings = totalReadings + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  
  // Calculate the average
  averageDistance = totalReadings / numReadings;
}

/**
 * Measures the distance using the HC-SR04 ultrasonic sensor
 * 
 * @return Distance in centimetres
 */
int measureDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send a 10µs pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echo pin, which returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH, 30000);  // Timeout after 30ms
  
  // If we timed out, return maximum distance
  if (duration == 0) {
    return 400;  // Out of range
  }
  
  // Calculate the distance
  // Speed of sound is 343 m/s = 0.0343 cm/µs
  // Distance = (time × speed) / 2 (divided by 2 because sound travels to object and back)
  return duration * 0.0343 / 2;
}

/**
 * Sends formatted debug output to the serial port
 */
void sendDebugOutput() {
  Serial.print("Avg Distance: ");
  Serial.print(averageDistance);
  Serial.print(" cm | Zone: ");
  
  if (inRedZone) {
    Serial.print("RED");
  } else if (inGreenZone) {
    Serial.print("GREEN");
  } else {
    Serial.print("NONE");
  }
  
  Serial.print(" | Red LED: ");
  Serial.print(digitalRead(redLedPin) ? "ON" : "OFF");
  Serial.print(" | Green LED: ");
  Serial.println(digitalRead(greenLedPin) ? "ON" : "OFF");
}