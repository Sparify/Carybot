#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "ArduinoJson.h"

int leftwheel_pwm = 32;
int leftwheel_brake = 33;
int leftwheel = 25;

int rightwheel = 19;
int rightwheel_brake = 18;
int rightwheel_pwm = 5;

// Deine WLAN-Zugangsdaten
const char *ssid = "Carybot";
const char *password = "123456789";

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
    //Serial.println("Nachricht empfangen: " + message + "\n");

    const size_t CAPACITY = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<CAPACITY> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, message);

    if (!error)
    {
      if (jsonDoc.containsKey("robot_direction"))
      {
        dir = stringToDirection(jsonDoc["robot_direction"].as<String>());
        speed = jsonDoc["speed"].as<String>();
        //Serial.println("Direction :" + String(dir) + "\nSpeed: " + speed + "\n\n");
      }
      else if (jsonDoc.containsKey("cam_direction"))
      {
        // #ToDo: KameraSteuerung
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

  pinMode(leftwheel, OUTPUT);
  pinMode(leftwheel_pwm, OUTPUT);
  pinMode(leftwheel_brake, OUTPUT);
  digitalWrite(leftwheel_brake, HIGH);

  pinMode(rightwheel, OUTPUT);
  pinMode(rightwheel_pwm, OUTPUT);
  pinMode(rightwheel_brake, OUTPUT);
  digitalWrite(rightwheel_brake, HIGH);
}

void moveForward()
{
  digitalWrite(leftwheel, HIGH);
  digitalWrite(rightwheel, LOW);
  Serial.println("Vorwärts");
}

void moveBackward()
{
  digitalWrite(leftwheel, LOW);
  digitalWrite(rightwheel, HIGH);
  Serial.println("Rückwärts");
}

void turnLeft()
{
  digitalWrite(leftwheel, LOW);
  digitalWrite(rightwheel, LOW);
  Serial.println("Links");
}

void turnRight()
{
  digitalWrite(leftwheel, HIGH);
  digitalWrite(rightwheel, HIGH);
  Serial.println("Rechts");
}

void stop()
{
  digitalWrite(leftwheel_brake, HIGH);
  digitalWrite(rightwheel_brake, HIGH);
  Serial.println("Stop");
}

void cam_turnRight(){
  // #TODO
  Serial.println("Cam_Rechts");
  
}

void cam_turnLeft(){
  // #TODO
  Serial.println("Cam_Links");
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
    lastDir = dir;

    // Motorbremsen deaktivieren
    digitalWrite(leftwheel_brake, LOW);
    digitalWrite(rightwheel_brake, LOW);

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
      analogWrite(leftwheel_pwm, speed_cb);
      analogWrite(rightwheel_pwm, speed_cb);
      lastPWM = speed_cb;
    }
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
}
