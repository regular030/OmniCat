#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//creds
const char* ssid = "Github Guest";
const char* password = "octocat11";

//repo login
const char* githubToken = "#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//creds
const char* ssid = "Github Guest";
const char* password = "octocat11";

//repo login
const char* githubToken = "YOUR_PERSONAL_ACCESS_TOKEN";
const char* owner = "R-driste";
char* repo = "REPO";

//screen details
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String topRepo = "";
int topStars = 0;
int openPRCount = 0;
int watchersCount = 0;
int starsCount = 0;

//starting display
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

  //use wifi details to access
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi Connected");
  display.display();

  delay(1000); //wait a second
  fetchAndDisplayGitHubData();
}

//MODE DEFAULT (1): get your highest value repository to extract stats from
bool getTopRepo() {
  HTTPClient http;
  String reposUrl = String("https://api.github.com/users/") + owner + "/repos?per_page=100";
  http.begin(reposUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Failed to get repos: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("JSON parsing failed");
    return false;
  }

  topStars = -1;
  topRepo = "";

  for (JsonObject repo : doc.as<JsonArray>()) {
    const char* repoName = repo["name"];
    int stars = repo["stargazers_count"];
    Serial.printf("Repo: %s Stars: %d\n", repoName, stars);
    if (stars > topStars) {
      topStars = stars;
      topRepo = String(repoName);
    }
  }
  Serial.printf("Top repo name:", topRepo.c_str());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Top repo name:", topRepo.c_str());
  display.display();
  return topRepo.length() > 0;
}

repo = getTopRepo();

//MODE 2: get open pr count from top repo
void fetchAndDisplayGitHubDataMode1() {
  HTTPClient http;
  String prUrl = String("https://api.github.com/repos/") + owner + "/" + repo + "/pulls?state=open&per_page=1";
  http.begin(prUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  int httpCode = http.GET();

  int openPRCount = 0;
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

  String issuesUrl = String("https://api.github.com/repos/") + owner + "/" + repo + "/issues?state=open&per_page=1";
  http.begin(issuesUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  httpCode = http.GET();
  
  int openIssCount = 0;
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
  Serial.println("Open PRs: %d\n", openPRCount);
  Serial.println("Open Issues: %d\n", openIssCount);
  display.display();
}

//MODE 3: get total stars and watchers
void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
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
  Serial.println("Stars:    %d\n", starsCount);
  Serial.println("Watchers: %d\n", watchersCount);
  display.display();
}


void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  httpCode = http.GET();

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
  display.printf("Top Repo Stars: %d\n", starsCount);
  display.printf("Top Repo Watch: %d\n", watchersCount);
  Serial.println("Top Repo Stars: %d\n", starsCount);
  Serial.println("Top Repo Watch: %d\n", watchersCount);
  display.display();
}

//Mode 4: Last user activity
void fetchAndDisplayGitHubDataMode4() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
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
  display.printf("Pushed: %s\n", pushedAt.substring(0, 10).c_str());  // trim ISO format
  display.printf("Updated: %s\n", pulledAt.substring(0, 10).c_str());
  Serial.println("Last Pushed: %s\n", pushedAt.substring(0, 10).c_str());  // trim ISO format
  Serial.println("Updated: %s\n", pulledAt.substring(0, 10).c_str());
  display.display();
}

void loop() {
  fetchAndDisplayGitHubDataMode1();
  delay(5000);

  fetchAndDisplayGitHubDataMode2();
  delay(5000);

  fetchAndDisplayGitHubDataMode4();
  delay(5000);
}";

const char* owner = "OWNER";
char* repo = "REPO";

//screen details
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String topRepo = "";
int topStars = 0;
int openPRCount = 0;
int watchersCount = 0;
int starsCount = 0;

//starting display
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

  //use wifi details to access
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi Connected");
  display.display();

  delay(1000); //wait a second
  fetchAndDisplayGitHubData();
}

//MODE DEFAULT (1): get your highest value repository to extract stats from
bool getTopRepo() {
  HTTPClient http;
  String reposUrl = String("https://api.github.com/users/") + owner + "/repos?per_page=100";
  http.begin(reposUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Failed to get repos: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("JSON parsing failed");
    return false;
  }

  topStars = -1;
  topRepo = "";

  for (JsonObject repo : doc.as<JsonArray>()) {
    const char* repoName = repo["name"];
    int stars = repo["stargazers_count"];
    Serial.printf("Repo: %s Stars: %d\n", repoName, stars);
    if (stars > topStars) {
      topStars = stars;
      topRepo = String(repoName);
    }
  }
  Serial.printf("Top repo name:", topRepo.c_str());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Top repo name:", topRepo.c_str());
  display.display();
  return topRepo.length() > 0;
}

repo = getTopRepo();

//MODE 2: get open pr count from top repo
void fetchAndDisplayGitHubDataMode1() {
  HTTPClient http;
  String prUrl = String("https://api.github.com/repos/") + owner + "/" + repo + "/pulls?state=open&per_page=1";
  http.begin(prUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  int httpCode = http.GET();

  int openPRCount = 0;
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

  String issuesUrl = String("https://api.github.com/repos/") + owner + "/" + repo + "/issues?state=open&per_page=1";
  http.begin(issuesUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  httpCode = http.GET();
  
  int openIssCount = 0;
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
  Serial.println("Open PRs: %d\n", openPRCount);
  Serial.println("Open Issues: %d\n", openIssCount);
  display.display();
}

//MODE 3: get total stars and watchers
void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
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
  Serial.println("Stars:    %d\n", starsCount);
  Serial.println("Watchers: %d\n", watchersCount);
  display.display();
}


void fetchAndDisplayGitHubDataMode2() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
  httpCode = http.GET();

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
  display.printf("Top Repo Stars: %d\n", starsCount);
  display.printf("Top Repo Watch: %d\n", watchersCount);
  Serial.println("Top Repo Stars: %d\n", starsCount);
  Serial.println("Top Repo Watch: %d\n", watchersCount);
  display.display();
}

//Mode 4: Last user activity
void fetchAndDisplayGitHubDataMode4() {
  HTTPClient http;
  String repoUrl = String("https://api.github.com/repos/") + owner + "/" + repo;
  http.begin(repoUrl);
  http.addHeader("User-Agent", "Arduino-GitHub-OLED");
  if (githubToken[0] != 0) {
    http.addHeader("Authorization", String("token ") + githubToken);
  }
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
  display.printf("Pushed: %s\n", pushedAt.substring(0, 10).c_str());  // trim ISO format
  display.printf("Updated: %s\n", pulledAt.substring(0, 10).c_str());
  Serial.println("Last Pushed: %s\n", pushedAt.substring(0, 10).c_str());  // trim ISO format
  Serial.println("Updated: %s\n", pulledAt.substring(0, 10).c_str());
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