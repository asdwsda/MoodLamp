#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <FS.h>

#include "rgbled.h"
#include "rainbow.h"

#define NUM_LEDS 2

#define SRV_PORT 80
#define OTA_PORT 8266

const char* ssid = "<your_ssid>";
const char* password = "<your_password>";
const char* mdns_hostname = "moodlamp";
const char* ota_password = "<OTA_password>";

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
    String msg = "Available commands:\n\n";
    msg += "/color: Set color of given led.\n";
    msg += "\tArguments:\n";
    msg += "\t\tid:\t0..255\tId of the LED to set. If not set, all leds will be set.\n";
    msg += "\t\tr:\t0..255\tRed color component. Will be 0 if not set.\n";
    msg += "\t\tg:\t0..255\tGreen color component. Will be 0 if not set.\n";
    msg += "\t\tb:\t0..255\tBlue color component. Will be 0 if not set.\n";
    msg += "/rainbow: Control rainbow animation.\n";
    msg += "\tArguments:\n";
    msg += "\t\taction:\tstart|stop|pause|reset\tStart, stop, pause or reset animation.\n";
    msg += "/restart: Restart the mood lamp.\n";
    msg += "/status: Return the current mode and color of LEDs in JSON format.\n";
    msg += "/help: Show this page.\n";

    msg += "\n";

    server.send(200, "text/plain", msg);
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

void connectToAP() {
    Serial.print("[WIFI-STA] Connecting to ");
    Serial.print(ssid);

    WiFi.begin(ssid, password);

    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED)
            break;
        delay(500);
        Serial.print(" .");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[WIFI-STA][ERROR] Cannot connect to AP");
        errorLoop();
    }

    Serial.println("\n[WIFI-STA] Connected");
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

void setAPIndicator() {
    RGB ap = {0, 30, 0};
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

    server.begin();
    Serial.print("[WIFI-STA] MoodLamp is ready to use at ");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(SRV_PORT);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nStart");
    analogWriteFreq(2000);
    analogWriteRange(255);
    setInitIndicator();
    mountFS();
    connectToAP();
    setupOTA();
    setupWebServer();
    Serial.println("[INFO] MoodLamp initialized");
}

void loop() {
    setAPIndicator();

    while(1) {
        server.handleClient();
        ArduinoOTA.handle();
        delay(10);
    }
}
