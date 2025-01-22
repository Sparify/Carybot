#include "esp_camera.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_task_wdt.h"
#include "SPIFFS.h"


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

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void startCameraServer();

String readFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  return fileContent;
}

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,   // 10 Sekunden Timeout
    .idle_core_mask = 0,   // Standardkonfiguration
    .trigger_panic = true  // System neu starten bei Auslösung
  };

  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  // WLAN-Verbindung herstellen
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.println("Hotspot started");

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    while (true) {
      delay(1);  // Programm stoppen
    }
  }
  Serial.println("SPIFFS mounted successfully");

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

  // Erhöhte XCLK-Frequenz
  config.xclk_freq_hz = 24000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Kleinere Bildgröße und höhere Qualität für schnellere Übertragung
  config.frame_size = FRAMESIZE_QVGA;  // 320x240 Pixel für mehr FPS
  config.jpeg_quality = 12;            // Höhere Kompression, aber noch akzeptable Qualität
  config.fb_count = 3;                 // Mehr Frame-Buffer für schnellere Verarbeitung


  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera-Init fehlgeschlagen mit Fehler 0x%x", err);
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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>ESP32-CAM Live Stream</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 0; text-align: center; }
                img { width: 100%; max-width: 640px; }
            </style>
            <script>
                function reloadImage() {
                    const img = document.getElementById('stream');
                    img.src = '/capture?_t=' + new Date().getTime(); // Cache-Busting
                }
                setInterval(reloadImage, 100); // 10 Bilder pro Sekunde
            </script>
        </head>
        <body>
            <h1>ESP32-CAM Live Stream</h1>
            <img id="stream" src="/capture" alt="Live Stream">
        </body>
        </html>
    )rawliteral";
    request->send(200, "text/html", html);
});

server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        request->send(503, "text/plain", "Kamerafehler");
        return;
    }

    AsyncWebServerResponse *response = request->beginResponse("image/jpeg", fb->len,
        [fb](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            size_t len = fb->len - index;
            if (len > maxLen) len = maxLen;
            memcpy(buffer, fb->buf + index, len);
            if (index + len == fb->len) esp_camera_fb_return(fb);
            return len;
        });
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
});




  // Webserver starten
  server.begin();
  Serial.println("HTTP server started");

  //startCameraServer();
}

void loop() {
  //server.handleClient();

  esp_task_wdt_reset();
}

/*void startCameraServer() {
  server.on("/", HTTP_GET, []() {
    String html = "<html><head><title>ESP32-CAM Live-Stream</title>";
    html += "<script type='text/javascript'>";
    html += "function reloadImage() {";
    html += "  var img = document.getElementById('camImage');";
    html += "  img.src = '/capture?_t=' + new Date().getTime();";
    html += "}";
    // Schnellere Bildaktualisierung
    html += "setInterval(reloadImage, 100);";
    html += "</script></head><body>";
    html += "<h1>ESP32-CAM Live-Stream</h1>";
    html += "<img id='camImage' src='/capture' width='100%'>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/capture", HTTP_GET, []() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      server.send(503, "text/plain", "Kamerafehler");
      return;
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.setContentLength(fb->len);
    server.send(200, "image/jpeg", "");

    WiFiClient client = server.client();
    client.write(fb->buf, fb->len);

    esp_camera_fb_return(fb);
    Serial.printf("Freier Speicher: %d bytes\n", esp_get_free_heap_size());
    Serial.printf("WiFi RSSI: %d\n", WiFi.RSSI());
  });

  server.begin();
  Serial.println("Kamera-Server gestartet.");
}*/
