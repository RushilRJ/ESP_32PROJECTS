#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// WiFi credentials (replace privately!)
const char* ssid = "SAU-Conference";
const char* password = "sau@123456";

// RSS feed
const char* url = "https://aicf.in/category/arbiters-news/feed/";

// ←←← YOUR BASELINE (change only if a newer post appears before your result)
const char* baselineLink = "https://aicf.in/fide-arbiter-seminar-2026fide-arbiter-seminar-in-pune-28-feb-02-march-2026-thoothukudi-07-09-march-2026/";

#define LED_PIN 48
#define LED_COUNT 1
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const unsigned long refreshInterval = 60000;
const unsigned long blinkSlow = 500;
const unsigned long blinkFast = 200;

enum State { ERROR_STATE, REFRESHING, NO_CHANGE, HAS_CHANGE };
State currentState = ERROR_STATE;

unsigned long lastRefresh = 0;
unsigned long lastBlink = 0;
bool ledOn = false;

Preferences prefs;
String lastETag = "";
String lastModified = "";
bool alertSet = false;

void setup() {
  Serial.begin(115200);
  pixel.begin();
  pixel.setBrightness(50);
  setLED(0, 0, 0);

  prefs.begin("website_monitor", false);
  lastETag = prefs.getString("etag", "");
  lastModified = prefs.getString("last_mod", "");
  alertSet = prefs.getBool("alert", false);

  connectToWiFi();
  checkWebsite();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    currentState = ERROR_STATE;
    connectToWiFi();
  }
  if (millis() - lastRefresh > refreshInterval) {
    checkWebsite();
  }
  handleLED();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

void checkWebsite() {
  if (WiFi.status() != WL_CONNECTED) return;

  currentState = REFRESHING;
  lastRefresh = millis();
  Serial.println("Checking RSS feed...");

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (lastETag != "") http.addHeader("If-None-Match", lastETag);
  if (lastModified != "") http.addHeader("If-Modified-Since", lastModified);

  int httpCode = http.GET();
  Serial.printf("HTTP Code: %d\n", httpCode);

  String currentLatestLink = "";

  if (httpCode == 200) {
    String payload = http.getString();

    int itemPos = payload.indexOf("<item>");
    if (itemPos != -1) {
      int linkPos = payload.indexOf("<link>", itemPos);
      if (linkPos != -1) {
        int start = linkPos + 6;
        int end = payload.indexOf("</link>", start);
        if (end != -1) {
          currentLatestLink = payload.substring(start, end);
          currentLatestLink.trim();
        }
      }
    }

    Serial.println("Latest article link: " + (currentLatestLink == "" ? "None" : currentLatestLink));

    if (currentLatestLink == baselineLink) {
      Serial.println("Matches baseline - no new update. Clearing any old alert.");
      alertSet = false;
      prefs.putBool("alert", false);          // ← this is the magic line
      currentState = NO_CHANGE;
    }
    else if (currentLatestLink != "") {
      Serial.println("NEW UPDATE DETECTED!");
      alertSet = true;
      prefs.putBool("alert", true);
      currentState = HAS_CHANGE;
    }
    else {
      currentState = NO_CHANGE;
    }
  }
  else if (httpCode == 304) {
    Serial.println("No change (304).");
    currentState = alertSet ? HAS_CHANGE : NO_CHANGE;
  }
  else {
    Serial.println("HTTP error: " + String(httpCode));
    currentState = ERROR_STATE;
  }

  http.end();

  if (alertSet) currentState = HAS_CHANGE;   // still keep persistent red once triggered
}

void handleLED() {
  unsigned long now = millis();
  unsigned long interval = 0;
  int r = 0, g = 0, b = 0;

  switch (currentState) {
    case NO_CHANGE:   r = 0; g = 80; b = 0; setLED(r, g, b); break;
    case HAS_CHANGE:  r = 80; g = 0; b = 0; setLED(r, g, b); break;
    case REFRESHING:  interval = blinkSlow; r = 0; g = 80; b = 0; break;
    case ERROR_STATE: interval = blinkFast; r = 80; g = 0; b = 0; break;
  }

  if (interval > 0 && now - lastBlink > interval) {
    ledOn = !ledOn;
    lastBlink = now;
    if (ledOn) setLED(r, g, b);
    else setLED(0, 0, 0);
  }
}

void setLED(int r, int g, int b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}