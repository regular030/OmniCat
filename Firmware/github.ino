#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credentials
const char* ssid = "AkhilsIphone";
const char* password = "Akhil123";

// GitHub details
const char* githubToken = ""; //optional
const char* owner = "R-driste";
String repo = "DataSciForestFires";

// OLED screen setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// GitHub data
int openPRCount = 0;
int watchersCount = 0;
int starsCount = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Connecting to WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display.setTextSize(2); 
  Serial.println("\nWiFi connected!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi Connected");
  display.display();
  delay(1000);

  fetchAndDisplayGitHubDataMode4();
}

// Mode 1: Fetch open PR and issue counts
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
    if (linkHeader.length() > 0) {
      int pos = linkHeader.indexOf("page=");
      if (pos >= 0) {
        int start = pos + 5;
        int end = linkHeader.indexOf(">", start);
        String pageNumStr = linkHeader.substring(start, end);
        openPRCount = pageNumStr.toInt();
      }
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
    if (linkHeader.length() > 0) {
      int pos = linkHeader.indexOf("page=");
      if (pos >= 0) {
        int start = pos + 5;
        int end = linkHeader.indexOf(">", start);
        String pageNumStr = linkHeader.substring(start, end);
        openIssCount = pageNumStr.toInt();
      }
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
  display.printf("Open Issues: %d\n", openIssCount);
  Serial.printf("Open PRs: %d\n", openPRCount);
  Serial.printf("Open Issues: %d\n", openIssCount);
  display.display();
}

// Mode 2: Fetch stars and watchers
void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String repoUrl = "https://api.github.com/repos/" + String(owner) + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) http.addHeader("Authorization", "token " + String(githubToken));
  int httpCode = http.GET();

  starsCount = 0;
  watchersCount = 0;

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
  display.printf("Repo: %s\n", repo.c_str());
  display.printf("Stars:    %d\n", starsCount);
  display.printf("Watchers: %d\n", watchersCount);
  Serial.printf("Stars:    %d\n", starsCount);
  Serial.printf("Watchers: %d\n", watchersCount);
  display.display();
}

// Mode 4: Fetch last push and update time
void fetchAndDisplayGitHubDataMode4() {
  HTTPClient http;
  String repoUrl = "https://api.github.com/repos/" + String(owner) + "/" + repo;
  http.begin(repoUrl);
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
    } else {
      Serial.println("JSON parsing failed for mode 4");
    }
  } else {
    Serial.printf("GitHub API error (mode 4): %d\n", httpCode);
  }
  http.end();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Last Push/Update");
  display.println("----------------");
  display.printf("Pushed: %s\n", pushedAt.substring(0, 10).c_str());
  display.printf("Updated: %s\n", pulledAt.substring(0, 10).c_str());
  Serial.printf("Pushed: %s\n", pushedAt.substring(0, 10).c_str());
  Serial.printf("Updated: %s\n", pulledAt.substring(0, 10).c_str());
  display.display();
}

void loop() {
  fetchAndDisplayGitHubDataMode1();
  delay(5000);

  fetchAndDisplayGitHubDataMode2();
  delay(5000);

  fetchAndDisplayGitHubDataMode4();
  delay(5000);
}
