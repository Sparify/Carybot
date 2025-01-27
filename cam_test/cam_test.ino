#include "esp_camera.h"
#include <WiFi.h>
#include "WebServer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_task_wdt.h"
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include "ArduinoJson.h"

#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#endif

const char *ssid = "Carybot";
const char *password = "123456789";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
WebSocketsServer websocket(81);  // WebSocket-Server auf Port 81

void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {
  String message = String((char *)payload).substring(0, length);
  Serial.println("WebSocket-Nachricht empfangen: " + message);

  StaticJsonDocument<200> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, message);

  if (!error) {
    if (jsonDoc.containsKey("robot_direction")) {
      const char *robot_direction = jsonDoc["robot_direction"];
      Serial.println("Robot direction: " + String(robot_direction));
    }
  }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      handleWebSocketMessage(num, payload, length);
      break;

    case WStype_BIN:
      Serial.println("Binärdaten empfangen (nicht unterstützt)");
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected.\n", num);
      break;

    case WStype_CONNECTED:
      IPAddress ip = websocket.remoteIP(num);
      Serial.printf("[WS] Client %u connected from %s.\n", num, ip.toString().c_str());
      break;
  }
}

void sendCameraFrame() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Kamerafehler: Kein Frame erhalten");
    return;
  }

  websocket.broadcastBIN(fb->buf, fb->len);  // Senden der Binärdaten an alle verbundenen Clients
  esp_camera_fb_return(fb);
}

String readFile(fs::FS &fs, const char *path) {
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  return fileContent;
}


void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("AP-Konfiguration fehlgeschlagen.");
    return;
  }

  if (!WiFi.softAP(ssid, password)) {
    Serial.println("AP konnte nicht gestartet werden.");
    return;
  }

  Serial.println("Access Point gestartet!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("Fehler beim Mounten von SPIFFS");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String dpad = readFile(SPIFFS, "/dpad.html");
    request->send(200, "text/html", dpad);
  });

  server.on("/menu-icon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
    String icon = readFile(SPIFFS, "/menu-icon.svg");
    request->send(200, "image/svg+xml", icon);
  });

  server.on("/mystyles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    String css = readFile(SPIFFS, "/mystyles.css");
    request->send(200, "text/css", css);
  });

  server.on("/carybot.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    String js = readFile(SPIFFS, "/carybot.js");
    request->send(200, "application/javascript", js);
  });

  Serial.println("SPIFFS-Dateien erfolgreich geladen");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>ESP32-CAM WebSocket Stream</title>
          <script>
              let websocket;
              function connectWebSocket() {
                  websocket = new WebSocket('ws://' + window.location.hostname + ':81');

                  websocket.onmessage = function(event) {
                      const blob = new Blob([event.data], { type: 'image/jpeg' });
                      const url = URL.createObjectURL(blob);
                      const img = document.getElementById('stream');
                      img.src = url;
                  };

                  websocket.onopen = function() {
                      console.log('WebSocket verbunden');
                  };

                  websocket.onclose = function() {
                      console.log('WebSocket getrennt. Erneuter Versuch in 5 Sekunden...');
                      setTimeout(connectWebSocket, 5000);
                  };
              }
              connectWebSocket();
          </script>
      </head>
      <body>
          <h1>ESP32-CAM WebSocket Stream</h1>
          <img id="stream" alt="Live Stream" style="width: 100%; max-width: 640px;" />
      </body>
      </html>
    )rawliteral";
    request->send(100, "text/html", html);
  });

  server.begin();
  websocket.begin();
  websocket.onEvent(onWebSocketEvent);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 24000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 3;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera-Init fehlgeschlagen mit Fehler 0x%x", err);
    return;
  }
}

unsigned int lastCleanup = 0;
const unsigned long cleanupInterval = 50; // ~20 FPS

void loop() {
  unsigned long now = millis();

  if(now - lastCleanup > cleanupInterval){
  websocket.loop();
  sendCameraFrame();
  lastCleanup = now;
  }
;

}
