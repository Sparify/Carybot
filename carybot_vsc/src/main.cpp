#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "ArduinoJson.h"

int leftfrontwheel_pwm = 32;
int leftfrontwheel_brake = 33;
int leftfrontwheel = 25;

/*------------------WIP*/
int leftrearwheel_pwm = 19;
int leftrearwheel_brake = 18;
int leftrearwheel = 5;

int rightfrontwheel = 15;
int rightfrontwheel_brake = 2;
int rightfrontwheel_pwm = 4;

/*-----------------WIP*/
int rightrearwheel = 12;
int rightrearwheel_brake = 14;
int rightrearwheel_pwm = 27;

// Deine WLAN-Zugangsdaten
const char *ssid = "Carybot";
const char *password = "123456789";

//Spannung-Messung für Akkustand
#define PIN_TEST 34               // Analoger Eingangspin
#define REF_VOLTAGE 3.3           // Referenzspannung des ESP32
#define PIN_STEPS 4095.0          // ADC-Auflösung des ESP32 (12-bit)
const float R1 = 120000.0;        // Widerstand R1 (120 kOhm)
const float R2 = 10800.0;         // Widerstand R2 (11 kOhm)
float vout = 0.0;                 // Gemessene Ausgangsspannung
float vin = 0.0;                  // Berechnete Eingangsspannung
int rawValue = 0;                 // Rohwert vom ADC
//--------------------------------

// Webserver läuft auf Port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

enum Direction
{
  HALT,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  CAM_RIGHT,
  CAM_LEFT
};

String speed = "0";
Direction dir = HALT;

Direction stringToDirection(const String &str)
{
  if (str == "up")
  {
    return UP;
  }
  else if (str == "down")
  {
    return DOWN;
  }
  else if (str == "left")
  {
    return LEFT;
  }
  else if (str == "right")
  {
    return RIGHT;
  }
  else if (str == "cam_left")
  {
    return CAM_LEFT;
  }
  else if (str == "cam_right")
  {
    return CAM_RIGHT;
  }
  else
  {
    return HALT;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    String message = String((char *)data).substring(0, len);
    // Serial.println("Nachricht empfangen: " + message + "\n");

    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, message);

    if (!error)
    {
      if (jsonDoc.containsKey("robot_direction"))
      {
        const char *robot_direction = jsonDoc["robot_direction"];
        if (robot_direction)
        {
          dir = stringToDirection(robot_direction);
        }
        speed = jsonDoc["speed"].as<String>();
        // Serial.println("Direction :" + String(dir) + "\nSpeed: " + speed + "\n\n");
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

// Funktion zum Laden der HTML-Datei aus SPIFFS
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

void setup()
{
  // Serielle Kommunikation starten
  Serial.begin(115200);

  // WLAN-Verbindung herstellen
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.println("Hotspot started");

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // SPIFFS initialisieren
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    while (true)
    {
      delay(1); // Programm stoppen
    }
  }
  Serial.println("SPIFFS mounted successfully");

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String dpad = readFile(SPIFFS, "/dpad.html");
    request->send(200, "text/html", dpad); });

  server.on("/menu-icon.svg", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String icon = readFile(SPIFFS, "/menu-icon.svg");
    request->send(200, "image/svg+xml", icon); });

  server.on("/mystyles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String css = readFile(SPIFFS, "/mystyles.css");
    request->send(200, "text/css", css); });

  server.on("/carybot.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String js = readFile(SPIFFS, "/carybot.js");
    request->send(200, "application/javascript", js); });

  // Webserver starten
  server.begin();
  Serial.println("HTTP server started");

  pinMode(leftfrontwheel, OUTPUT);
  pinMode(leftfrontwheel_pwm, OUTPUT);
  pinMode(leftfrontwheel_brake, OUTPUT);
  digitalWrite(leftfrontwheel_brake, HIGH);

  pinMode(rightfrontwheel, OUTPUT);
  pinMode(rightfrontwheel_pwm, OUTPUT);
  pinMode(rightfrontwheel_brake, OUTPUT);
  digitalWrite(rightfrontwheel_brake, HIGH);

  pinMode(leftrearwheel, OUTPUT);
  pinMode(leftrearwheel_pwm, OUTPUT);
  pinMode(leftrearwheel_brake, OUTPUT);
  digitalWrite(leftrearwheel_brake, HIGH);

  pinMode(rightrearwheel, OUTPUT);
  pinMode(rightrearwheel_pwm, OUTPUT);
  pinMode(rightrearwheel_brake, OUTPUT);
  digitalWrite(rightrearwheel_brake, HIGH);

  pinMode(PIN_TEST, INPUT);
}

void moveForward()
{
  digitalWrite(leftfrontwheel, HIGH);
  digitalWrite(rightfrontwheel, LOW);
  digitalWrite(leftrearwheel, HIGH);
  digitalWrite(rightrearwheel, LOW);
  Serial.println("Vorwärts");
}

void moveBackward()
{
  digitalWrite(leftfrontwheel, LOW);
  digitalWrite(rightfrontwheel, HIGH);
  digitalWrite(leftrearwheel, LOW);
  digitalWrite(rightrearwheel, HIGH);
  Serial.println("Rückwärts");
}

void turnLeft()
{
  digitalWrite(leftfrontwheel, LOW);
  digitalWrite(rightfrontwheel, LOW);
  digitalWrite(leftrearwheel, LOW);
  digitalWrite(rightrearwheel, LOW);
  Serial.println("Links");
}

void turnRight()
{
  digitalWrite(leftfrontwheel, HIGH);
  digitalWrite(rightfrontwheel, HIGH);
  digitalWrite(leftrearwheel, HIGH);
  digitalWrite(rightrearwheel, HIGH);
  Serial.println("Rechts");
}

void stop()
{
  digitalWrite(leftfrontwheel_brake, HIGH);
  digitalWrite(rightfrontwheel_brake, HIGH);
  digitalWrite(leftrearwheel_brake, HIGH);
  digitalWrite(rightrearwheel_brake, HIGH);
  Serial.println("Stop");
}

void cam_turnRight()
{
  // #TODO
  Serial.println("Cam_Rechts");
  digitalWrite(leftfrontwheel_brake, HIGH);
  digitalWrite(rightfrontwheel_brake, HIGH);
  digitalWrite(leftrearwheel_brake, HIGH);
  digitalWrite(rightrearwheel_brake, HIGH);
}

void cam_turnLeft()
{
  // #TODO
  Serial.println("Cam_Links");
  digitalWrite(leftfrontwheel_brake, HIGH);
  digitalWrite(rightfrontwheel_brake, HIGH);
  digitalWrite(leftrearwheel_brake, HIGH);
  digitalWrite(rightrearwheel_brake, HIGH);
}

int speed_cb = 0;

void navigate()
{
  static Direction lastDir = HALT;
  static int lastSpeed = -1;

  if (dir != lastDir || speed_cb != lastSpeed)
  {
    speed_cb = speed.toInt() * 2.55;
    lastSpeed = speed_cb;

    // Motorbremsen deaktivieren
    digitalWrite(leftfrontwheel_brake, LOW);
    digitalWrite(rightfrontwheel_brake, LOW);
    digitalWrite(leftrearwheel_brake, LOW);
    digitalWrite(rightrearwheel_brake, LOW);

    switch (dir)
    {
    case UP:
      moveForward();
      break;

    case DOWN:
      moveBackward();
      break;

    case LEFT:
      turnLeft();
      break;

    case RIGHT:
      turnRight();
      break;

    case HALT:
      stop();
      break;

    case CAM_LEFT:
      cam_turnLeft();
      break;

    case CAM_RIGHT:
      cam_turnRight();
      break;

    default:
      stop();
      break;
    }

    static int lastPWM = -1;
    if (lastPWM != speed_cb)
    {
      analogWrite(leftfrontwheel_pwm, speed_cb);
      analogWrite(rightfrontwheel_pwm, speed_cb);
      analogWrite(leftrearwheel_pwm, speed_cb);
      analogWrite(rightrearwheel_pwm, speed_cb);
      lastPWM = speed_cb;
    }

    lastDir = dir;
  }
}

unsigned int lastCleanup = 0;
const unsigned long cleanupInterval = 100;

void loop()
{
  unsigned long now = millis();
  if (now - lastCleanup > cleanupInterval)
  {
    ws.cleanupClients();
    lastCleanup = now;
  }
  navigate();
  //Spannungsmessung
  rawValue = analogRead(PIN_TEST);
  vout = (rawValue * REF_VOLTAGE) / PIN_STEPS;
  vin = vout / (R2 / (R1 + R2));
  if (vin < 0.09) {
      vin = 0.0;
  }
  Serial.println("U = " + String(vin+1, 1) + " V"); // Spannung mit 2 Dezimalstellen
  float batteryPercentage = ((vin -9) /4) *100; // Vin in mV gemessen
  float akku_round = round(batteryPercentage /10 ) *10; 
  Serial.println("Akkustand: " + String(akku_round,0) + "%");
  //------------------

}