#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// ===== Wi-Fi =====
const char* ssid = "AkhilsIphone";
const char* password = "Akhil123";

// ===== GitHub OLED Stats =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* githubToken = "";
const char* owner = "R-driste";
String repo = "DataSciForestFires";

int openPRCount = 0;
int watchersCount = 0;
int starsCount = 0;

// ===== UDP + Motors + Servo =====
WiFiUDP udp;
const unsigned int localUdpPort = 4210;
char incomingPacket[255];
String serialInput = "";

#define RIGHT_IN1 1
#define RIGHT_IN2 2
#define LEFT_IN1  3
#define LEFT_IN2  4
#define SERVO_PIN 21
Servo nodServo;

// ===== NeoPixel =====
#define NEOPIXEL_PIN 22
#define NUM_PIXELS 2
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint16_t rainbowOffset = 0;

void setup() {
  Serial.begin(115200);

  // OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Connecting...");
  display.display();

  // Motors
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  stopMotors();

  // Servo
  nodServo.attach(SERVO_PIN);
  nodServo.write(90);

  // NeoPixel
  pixels.begin();
  pixels.setBrightness(50);

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi Connected");
  display.display();
  delay(1000);

  udp.begin(localUdpPort);
  Serial.printf("Listening on UDP port %d\n", localUdpPort);

  fetchAndDisplayGitHubDataMode4();
}

void loop() {
  // UDP
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

  // Serial
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

  rainbowCycle();
}

void fetchAndDisplayGitHubDataMode1() {
  HTTPClient http;
  String prUrl = "https://api.github.com/repos/" + String(owner) + "/" + repo + "/pulls?state=open&per_page=1";
  http.begin(prUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) http.addHeader("Authorization", "token " + String(githubToken));
  int httpCode = http.GET();

  openPRCount = 0;
  if (httpCode == HTTP_CODE_OK) {
    String linkHeader = http.header("Link");
    if (linkHeader.indexOf("page=") >= 0) {
      int start = linkHeader.indexOf("page=") + 5;
      int end = linkHeader.indexOf(">", start);
      openPRCount = linkHeader.substring(start, end).toInt();
    } else {
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      openPRCount = doc.size();
    }
  }
  http.end();

  int openIssCount = 0;
  String issuesUrl = "https://api.github.com/repos/" + String(owner) + "/" + repo + "/issues?state=open&per_page=1";
  http.begin(issuesUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) http.addHeader("Authorization", "token " + String(githubToken));
  httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String linkHeader = http.header("Link");
    if (linkHeader.indexOf("page=") >= 0) {
      int start = linkHeader.indexOf("page=") + 5;
      int end = linkHeader.indexOf(">", start);
      openIssCount = linkHeader.substring(start, end).toInt();
    } else {
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      openIssCount = doc.size();
    }
  }
  http.end();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Open PRs: %d\n", openPRCount);
  display.printf("Issues:   %d\n", openIssCount);
  display.display();
  delay(2000);
}

void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String url = "https://api.github.com/repos/" + String(owner) + "/" + repo;
  http.begin(url);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) http.addHeader("Authorization", "token " + String(githubToken));
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    if (!deserializeJson(doc, payload)) {
      starsCount = doc["stargazers_count"] | 0;
      watchersCount = doc["subscribers_count"] | 0;
    }
  }
  http.end();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Stars: %d\n", starsCount);
  display.printf("Watch: %d\n", watchersCount);
  display.display();
  delay(2000);
}

void fetchAndDisplayGitHubDataMode4() {
  HTTPClient http;
  String url = "https://api.github.com/repos/" + String(owner) + "/" + repo;
  http.begin(url);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) http.addHeader("Authorization", "token " + String(githubToken));
  int httpCode = http.GET();

  String pushedAt = "N/A";
  String pulledAt = "N/A";

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      pushedAt = doc["pushed_at"] | "N/A";
      pulledAt = doc["updated_at"] | "N/A";
    }
  }
  http.end();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Last Push:");
  display.println(pushedAt.substring(0, 10));
  display.println("Updated:");
  display.println(pulledAt.substring(0, 10));
  display.display();
  delay(2000);
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
  else if (cmd == "mode1") fetchAndDisplayGitHubDataMode1();
  else if (cmd == "mode2") fetchAndDisplayGitHubDataMode2();
  else if (cmd == "mode4") fetchAndDisplayGitHubDataMode4();
  else {
    Serial.println("Unknown command: " + cmd);
    stopMotors();
  }
}

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

void nod() {
  Serial.println("Nodding...");
  nodServo.write(30);
  delay(300);
  nodServo.write(90);
  delay(300);
}

void rainbowCycle() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint16_t hue = (rainbowOffset + (i * 65536L / NUM_PIXELS)) % 65536;
    pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue)));
  }
  pixels.show();
  rainbowOffset += 256;
  delay(10);
}
