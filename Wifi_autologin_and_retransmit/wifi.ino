#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// === CONFIGURATION ===
const char* sta_ssid     = "SAU-Net";
const char* sta_password = "sau12345";  // "" if open/no WPA password

const char* ap_ssid      = "MyRoom-Alexa";
const char* ap_password  = "supersecret123";  // 8+ characters

// *** REPLACE PRIVATELY WITH YOUR REAL CREDENTIALS (NEVER SHARE/PUBLIC!) ***
const char* portal_username = "2013064";  // your roll no/email
const char* portal_password = "123456";            // your password

IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_gateway(192, 168, 4, 1);
IPAddress ap_mask(255, 255, 255, 0);
IPAddress ap_lease_start(192, 168, 4, 2);
IPAddress ap_dns(8, 8, 8, 8);

volatile bool loginNeeded = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-S3 WiFi Bridge + Exact SAU ISE Auto-Login Starting ===");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  Network.onEvent(onNetworkEvent);

  Serial.print("Connecting to STA first: ");
  Serial.println(sta_ssid);
  WiFi.begin(sta_ssid, sta_password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nSTA Connected! IP: " + WiFi.localIP().toString());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("\nSTA failed – reposition board");
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.AP.begin();
  WiFi.AP.config(ap_ip, ap_gateway, ap_mask, ap_lease_start, ap_dns);
  WiFi.AP.create(ap_ssid, ap_password);

  if (WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 3000)) {
    Serial.print("AP Started: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    WiFi.AP.enableNAPT(true);
    Serial.println("NAT Enabled – bridge ready!");
  }
}

void loop() {
  if (loginNeeded) {
    loginNeeded = false;
    detectAndLoginToPortal();
  }
  delay(1000);
}

void onNetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("STA Got IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
      loginNeeded = true;
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.print("STA Disconnected – Reason: ");
      Serial.println(info.wifi_sta_disconnected.reason);
      Serial.println("Auto-reconnect on...");
      WiFi.AP.enableNAPT(false);
      break;

    default:
      break;
  }
}

void detectAndLoginToPortal() {
  Serial.println("\n=== Detecting portal ===");
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  String detectUrl = "http://captive.apple.com/hotspot-detect.html";
  http.begin(detectUrl);
  int code = http.GET();

  String payload = http.getString();
  http.end();

  if (code == HTTP_CODE_OK && payload.indexOf("Success") != -1) {
    Serial.println("No portal – full internet!");
    return;
  }

  int urlStart = payload.indexOf("URL=");
  if (urlStart == -1) {
    Serial.println("No gateway URL found");
    return;
  }
  urlStart += 4;
  int urlEnd = payload.indexOf("\"", urlStart);
  String gatewayUrl = payload.substring(urlStart, urlEnd > 0 ? urlEnd : payload.length());
  Serial.print("Gateway URL: ");
  Serial.println(gatewayUrl);

  // Extract base host (up to /portal/)
  String baseHost = gatewayUrl.substring(0, gatewayUrl.indexOf("/portal/") + 8);
  Serial.print("Base host: ");
  Serial.println(baseHost);

  String token = getUrlParam(gatewayUrl, "token");
  String portalId = getUrlParam(gatewayUrl, "portal");

  // Step 1: Credentials
  String credentialsUrl = baseHost + "LoginSubmit.action?from=LOGIN";
  performPost(credentialsUrl, token, portalId, "credentials");

  // Step 2: Accept policy
  String aupUrl = baseHost + "AupSubmit.action?from=AUP";
  performPost(aupUrl, token, portalId, "accept");

  // Step 3: Continue
  String continueUrl = baseHost + "Continue.action?from=POST_ACCESS_BANNER";
  performPost(continueUrl, token, portalId, "continue");
}

String getUrlParam(String url, String param) {
  int start = url.indexOf(param + "=");
  if (start == -1) return "";
  start += param.length() + 1;
  int end = url.indexOf("&", start);
  return (end == -1) ? url.substring(start) : url.substring(start, end);
}

void performPost(String url, String token, String portalId, String step) {
  Serial.print("Step ");
  Serial.print(step);
  Serial.println(": POSTing...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  https.setTimeout(20000);
  https.begin(client, url);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "";
  if (step == "credentials") {
    postData = "user.username=" + String(portal_username) +
               "&user.password=" + String(portal_password) +
               "&submit=Sign On";
  } else if (step == "accept") {
    postData = "aupAccepted=true&submit=Accept";
  } else if (step == "continue") {
    postData = "submit=Continue";
  }

  if (token.length() > 0) postData += "&token=" + token;
  if (portalId.length() > 0) postData += "&portal=" + portalId;

  int code = https.POST(postData);
  Serial.print("POST code: ");
  Serial.println(code);

  String loc = https.header("Location");
  if (loc.length() > 0) {
    Serial.print("Redirect: ");
    Serial.println(loc);
  }

  if (step == "continue" && loc.length() == 0) {
    Serial.println("*** AUTO-LOGIN SUCCESSFUL! Full internet for Alexa/IoT ***");
  }

  https.end();
}