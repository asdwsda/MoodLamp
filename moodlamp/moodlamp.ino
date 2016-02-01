#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <FS.h>

#include "rgbled.h"
#include "rainbow.h"

#define STA_TRY_COUNT 3
#define AP_FALLBACK true
#define NUM_LEDS 2
#define SRV_PORT 80
#define OTA_PORT 8266

const char* ssid = "<your_ssid>";
const char* password = "<your_password>";
const char* mdns_hostname = "moodlamp";
const char* ota_password = "<OTA_password>";

bool ap_mode = false;
const char* ap_ssid = "MoodLamp";
const char* ap_password = "<ap_password>";

ESP8266WebServer server(SRV_PORT);

RGBLed leds[] = {{13, 12, 14}, {5, 4, 15}};

Rainbow rainbow(leds, NUM_LEDS);


void setAllLed(RGB rgb) {
    for (int i; i < NUM_LEDS; i++) {
        leds[i].setColor(rgb);
    }
}

//================= responses =================
void responseOk() {
    server.send(200, "text/plain", "ok\n\n");
}

void responseError400(String msg) {
    server.send(400, "text/plain", msg + "\n\n");
}

void responseError404() {
    server.send(404, "text/plain", "404 - Not Found\n\n");
}

//================= HANDLERS =================
void handleRoot() {
    serveFile("/index.html");
}

void handleHelp() {
    serveFile("/help.html");
}

void handleStatus() {
    String status = "{\"mode\": ";
    status += rainbow.isRunning() == true ? "\"rainbow\"" : "\"color\"";
    status += ", \"leds\": [";
    for (int i = 0; i < NUM_LEDS; i++) {
        RGB led = leds[i].getColor();
        status += "{\"r\": ";
        status += led.r;
        status += ", \"g\": ";
        status += led.g;
        status += ", \"b\": ";
        status += led.b;
        status += (i == NUM_LEDS - 1) ? "}" : "},";
    }
    status += "]}\n\n";

    server.send(200, "text/json", status);
}

void handleNotFound() {
    serveFile(server.uri());
}

void handleRestart() {
    responseOk();
    ESP.restart();
}

void handleColor() {
    if (rainbow.isRunning()) {
        responseError400("Cannot set color in Rainbow mode.");
        return;
    }
    RGB rgb = getRGBFromArguments();

    int id = getIdFromArguments();
    if (id > NUM_LEDS) {
        responseError400("Invalid id");
        return;
    }

    if (id == -1) {
        setAllLed(rgb);
    } else {
        leds[id].setColor(rgb);
    }

    responseOk();
}

void handleRainbow() {
    if (server.hasArg("action")){
        String action = server.arg("action");
        if (action == "start") {
            rainbow.start();
        } else if (action == "stop") {
            rainbow.stop();
        } else if (action == "pause") {
            rainbow.pause();
        } else if (action == "reset") {
            rainbow.reset();
        } else {
            responseError400("Invalid action.");
            return;
        }
        responseOk();
    } else {
        responseError400("Missing argument: 'action'");
    }
}

String getContentType(String filename){
  if (filename.endsWith(".html"))
      return "text/html";
  else if (filename.endsWith(".css"))
      return "text/css";
  else if (filename.endsWith(".js"))
      return "application/javascript";
  return "text/plain";
}

void serveFile(String path) {
    path = "/www" + path;
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        String content_type = getContentType(path);
        server.streamFile(file, content_type);
        file.close();
    } else {
        responseError404();
    }
}

RGB getRGBFromArguments() {
    RGB rgb = {};
    if(server.hasArg("r")) {
        rgb.r = server.arg("r").toInt();
    }
    if(server.hasArg("g")) {
        rgb.g = server.arg("g").toInt();
    }
    if(server.hasArg("b")) {
        rgb.b = server.arg("b").toInt();
    }
    return rgb;
}

int getIdFromArguments() {
    if (server.hasArg("id"))
        return server.arg("id").toInt();
    return -1;
}

bool getRainbowActionFromArguments() {
    if (server.hasArg("action"))
        return server.arg("action") == "start";
    return false;
}

//================= INIT =================

bool connectAsStation() {
    Serial.print("[WIFI-STA] Connecting to '");
    Serial.print(ssid);
    Serial.println("'");

    WiFi.mode(WIFI_STA);
    for (uint8_t i = 1; i <= STA_TRY_COUNT; i++) {
        WiFi.begin(ssid, password);
        Serial.print("[WIFI-STA] Try #");
        Serial.println(i);
        if (WiFi.waitForConnectResult() == WL_CONNECTED) {
            Serial.println("[WIFI-STA] Connected");
            Serial.print("[WIFI-STA] MoodLamp is available at ");
            Serial.print(WiFi.localIP());
            Serial.println("");
            ap_mode = false;
            return true;
        } else {
            Serial.println("[WIFI-STA][ERROR] Failed to connect, retrying");
            delay(1000);
        }
    }
    return false;
}

void setupAPMode() {
    Serial.println("[WIFI-AP] Configuring access point '");
    Serial.print(ap_ssid);
    Serial.println("'");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    ap_mode = true;
    IPAddress ip = WiFi.softAPIP();
    Serial.print("[WIFI-AP] Moodlamp is available at ");
    Serial.println(ip);
    Serial.println("[WIFI-AP] Access point configured");
}

void errorLoop() {
    setErrorIndicator();
    while(1) delay(500);
}

void setErrorIndicator() {
    RGB error = {100, 0, 0};
    setAllLed(error);
}

void setInitIndicator() {
    RGB init = {120, 30, 0};
    setAllLed(init);
}

void setStaIndicator() {
    RGB sta = {0, 30, 0};
    setAllLed(sta);
}

void setAPIndicator() {
    RGB ap = {0, 40, 70};
    setAllLed(ap);
}

void mountFS() {
    Serial.println("[SPIFFS] Mounting FS...");
    if(!SPIFFS.begin()) {
        Serial.println("[SPIFFS] Failed to mount file system");
        errorLoop();
    }
    Serial.println("[SPIFFS] Mounted");
}

void setupWiFi() {
    if (!connectAsStation()) {
        if (AP_FALLBACK) {
            Serial.println("[ERROR] Falling back to AP mode");
            setupAPMode();
        } else {
            Serial.println("[ERROR] Failed to setup WiFi");
            errorLoop();
        }
    }
}

void setupOTA() {
    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(mdns_hostname);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.begin();
    Serial.println("[OTA] Initialized");
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/color", HTTP_GET, handleColor);
    server.on("/rainbow", HTTP_GET, handleRainbow);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/help", HTTP_GET, handleHelp);
    server.on("/restart", HTTP_GET, handleRestart);
    server.onNotFound(handleNotFound);

    Serial.print("[WEB] Server started on port ");
    Serial.println(SRV_PORT);
    server.begin();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nStart");
    analogWriteFreq(2000);
    analogWriteRange(255);
    setInitIndicator();
    mountFS();
    setupWiFi();
    setupOTA();
    setupWebServer();
    Serial.println("[INFO] MoodLamp initialized");
}

void loop() {
    if (ap_mode) {
        setAPIndicator();
    } else {
        setStaIndicator();
    }


    while(1) {
        server.handleClient();
        ArduinoOTA.handle();
        delay(10);
    }
}
