#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <FS.h>

#include "rgbled.h"
#include "rainbow.h"

#define NUM_LEDS 2

#define SRV_PORT 80
const char* ssid = "<your_ssid>";
const char* password = "<your_password>";

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

//================= HANDLERS =================
void handleRoot() {
    String msg = "Hello from MoodLamp!\n\n";
    msg += "Available commands:\n";
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
    msg += "/status: Show the current mode and color of LEDs.\n";
    msg += "/interactive: Interactive lamp control page.\n";

    msg += "\n";

    server.send(200, "text/plain", msg);
}

void handleStatus() {
    String status = "Mode: ";
    status += rainbow.isRunning() == true ? "Rainbow" : "Simple color";
    status += "\n";
    for (int i = 0; i < NUM_LEDS; i++) {
        RGB led = leds[i].getColor();
        status += "LED #";
        status += i;
        status += ": R = ";
        status += led.r;
        status += "; G = ";
        status += led.g;
        status += "; B = ";
        status += led.b;
        status += "\n";
    }
    status += "\n\n";

    server.send(200, "text/plain", status);
}

void handleNotFound() {
    server.send(404, "text/plain", "Error 404 - Not found\n\n");
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

void handleInteractive() {
    String path = "/index.html";
    if (SPIFFS.exists(path)) {
        File f = SPIFFS.open("/index.html", "r");
        String content_type = "text/html";
        server.streamFile(f, content_type);
        f.close();
    } else {
        handleNotFound();
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
    Serial.print("[SETUP] Connecting to ");
    Serial.print(ssid);

    WiFi.begin(ssid, password);

    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED)
            break;
        delay(500);
        Serial.print(" .");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[ERROR] Cannot connect to AP");
        setErrorIndicator();
        while(1) delay(500);
    }

    Serial.println("\n[SETUP] Connected");
}

void setErrorIndicator() {
    RGB error = {255, 0, 0};
    setAllLed(error);
}

void setReadyIndicator() {
    RGB ready = {0, 70, 0};
    setAllLed(ready);
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/color", HTTP_GET, handleColor);
    server.on("/rainbow", HTTP_GET, handleRainbow);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/restart", HTTP_GET, handleRestart);
    server.on("/interactive", HTTP_GET, handleInteractive);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.print("[SETUP] MoodLamp is ready to use at ");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(SRV_PORT);
    Serial.println("[SETUP] MoodLamp initialized");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nStart");
    analogWriteFreq(2000);
    analogWriteRange(255);
    connectToAP();
    SPIFFS.begin();
    setupWebServer();
}

void loop() {
    setReadyIndicator();

    while(1) {
        server.handleClient();
        delay(10);
    }
}
