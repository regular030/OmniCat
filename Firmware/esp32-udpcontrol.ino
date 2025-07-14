#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// Wi-Fi credentials
const char* ssid = "AkhilsIphone";         // <-- Replace
const char* password = "Akhil123"; // <-- Replace

// UDP settings
WiFiUDP udp;
const unsigned int localUdpPort = 4210;
char incomingPacket[255];

// Motor control pins
#define RIGHT_IN1 1
#define RIGHT_IN2 2
#define LEFT_IN1 3
#define LEFT_IN2 4

// Servo for nodding
#define SERVO_PIN 21
Servo nodServo;

String serialInput = "";

void setup() {
  Serial.begin(115200);

  // Motor pin setup
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  stopMotors();

  // Servo setup
  nodServo.attach(SERVO_PIN);
  nodServo.write(90); // Neutral

  // Wi-Fi setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // Start UDP
  udp.begin(localUdpPort);
  Serial.printf("Listening on UDP port %d\n", localUdpPort);

  Serial.println("Commands: forward | back | left | right | stop | nod");
}

void loop() {
  // === Handle UDP ===
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) {
      incomingPacket[len] = 0;
      String command = String(incomingPacket);
      Serial.println("[UDP] " + command);
      handleCommand(command);
    }
  }

  // === Handle Serial Input ===
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialInput.length() > 0) {
        Serial.println("[Serial] " + serialInput);
        handleCommand(serialInput);
        serialInput = "";
      }
    } else {
      serialInput += c;
    }
  }
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "forward") moveForward();
  else if (cmd == "back") moveBack();
  else if (cmd == "left") turnLeft();
  else if (cmd == "right") turnRight();
  else if (cmd == "stop") stopMotors();
  else if (cmd == "nod") nod();
  else {
    Serial.println("Unknown command: " + cmd);
    stopMotors();
  }
}

// === Movement ===

void moveForward() {
  digitalWrite(RIGHT_IN1, HIGH); digitalWrite(RIGHT_IN2, LOW);
  digitalWrite(LEFT_IN1, HIGH);  digitalWrite(LEFT_IN2, LOW);
}

void moveBack() {
  digitalWrite(RIGHT_IN1, LOW);  digitalWrite(RIGHT_IN2, HIGH);
  digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, HIGH);
}

void turnRight() {
  digitalWrite(RIGHT_IN1, HIGH); digitalWrite(RIGHT_IN2, LOW);
  digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, HIGH);
}

void turnLeft() {
  digitalWrite(RIGHT_IN1, LOW);  digitalWrite(RIGHT_IN2, HIGH);
  digitalWrite(LEFT_IN1, HIGH);  digitalWrite(LEFT_IN2, LOW);
}

void stopMotors() {
  digitalWrite(RIGHT_IN1, LOW); digitalWrite(RIGHT_IN2, LOW);
  digitalWrite(LEFT_IN1, LOW);  digitalWrite(LEFT_IN2, LOW);
}

// === Servo Nod ===

void nod() {
  Serial.println("Nodding...");
  nodServo.write(30);  // Down
  delay(300);
  nodServo.write(90);  // Back to neutral
  delay(300);
}
