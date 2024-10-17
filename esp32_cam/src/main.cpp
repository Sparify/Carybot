#include "esp_camera.h"
#include <WiFi.h>
#include "WebServer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Kamera-Modell, das du verwendest (meist AI Thinker)
#define CAMERA_MODEL_AI_THINKER

// Kamera-Pins für AI Thinker
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

// WLAN-Zugangsdaten
const char *ssid = "Carybot";
const char *password = "123456789";

WebServer server(80);  // HTTP Webserver auf Port 80

void startCameraServer();

void setup() {
  Serial.begin(115200);

  // Brownout-Detektor deaktivieren, um Resets zu vermeiden
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  // WLAN-Verbindung herstellen
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Kamera konfigurieren
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Reduzierte Bildgröße für schnellere Übertragung
  config.frame_size = FRAMESIZE_VGA;  // 640x480
  config.jpeg_quality = 12;           // 0-63 (niedrigere Zahl = höhere Qualität)
  config.fb_count = 1;                // Nur 1 Framebuffer für bessere Performance

  // Kamera initialisieren
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera-Init fehlgeschlagen mit Fehler 0x%x", err);
    return;
  }

  startCameraServer();  // Kamera-Webserver starten
}

void loop() {
  server.handleClient();  // Eingehende Anfragen verarbeiten
}

// Funktion, um die Kamera-Website zu starten
void startCameraServer() {
  // HTML-Seite mit JavaScript für die automatische Aktualisierung
  server.on("/", HTTP_GET, []() {
    String html = "<html><head><title>ESP32-CAM Live-Stream</title>";
    html += "<script type='text/javascript'>";
    html += "function reloadImage() {";
    html += "  var img = document.getElementById('camImage');";
    html += "  img.src = '/capture?_t=' + new Date().getTime();";  // Cache-Busting durch Zeitstempel
    html += "}";
    html += "setInterval(reloadImage, 500);";  // Aktualisiert alle 500ms
    html += "</script></head><body>";
    html += "<h1>ESP32-CAM Live-Stream</h1>";
    html += "<img id='camImage' src='/capture' width='100%'>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  });

  // Route für die Kamerabilder
  server.on("/capture", HTTP_GET, []() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      server.send(503, "text/plain", "Kamerafehler");
      return;
    }

    // Add CORS header here
    server.sendHeader("Access-Control-Allow-Origin", "*");  // Allow all origins

    // Kein manuelles Setzen des Content-Length-Headers!
    server.setContentLength(fb->len);    // Stelle die Content-Length fest
    server.send(200, "image/jpeg", "");  // Leere Daten senden, aber Header korrekt setzen

    // Verwende den Client zum Senden des Bildes
    WiFiClient client = server.client();
    client.write(fb->buf, fb->len);  // Sende den Bildbuffer

    esp_camera_fb_return(fb);  // Rückgabe des Bildspeichers
  });

  server.begin();
  Serial.println("Kamera-Server gestartet.");
}
