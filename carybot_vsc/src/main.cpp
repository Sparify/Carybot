#include <WiFi.h>
#include "ArduinoJson.h"
#include "HCSR04.h"
#include "WebSocketsServer.h"
#include <HX711_ADC.h>
#include <ESP32Servo.h>
#include <Adafruit_MCP23X17.h>

const int HX711_dout = 21;
const int HX711_sck = 22;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

Adafruit_MCP23X17 mcp;
const int light_pin = 0;

UltraSonicDistanceSensor distanceSensor(13, 12); // Initialize sensor that uses digital pins 13 and 12.

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000;

int leftfrontwheel_pwm = 32;
int leftfrontwheel_brake = 33;
int leftfrontwheel = 25;

int leftrearwheel_pwm = 19;
int leftrearwheel_brake = 18;
int leftrearwheel = 5;

int rightfrontwheel = 15;
int rightfrontwheel_brake = 2;
int rightfrontwheel_pwm = 4;

int rightrearwheel = 12;
int rightrearwheel_brake = 14;
int rightrearwheel_pwm = 27;

// Deine WLAN-Zugangsdaten
const char *ssid = "Carybot";
const char *password = "123456789";

WebSocketsServer websocket(8080);

IPAddress local_IP(192, 168, 4, 3);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Gewicht ueber HX711
float weight = 0.0;
//-----------------------

// Servo Motor
int camera_pos = 0;
Servo myservo;
static const int servoPin = 26; // Pin umändern auf PWM Pin
//-----------------

// Spannung-Messung für Akkustand
#define PIN_TEST 34        // Analoger Eingangspin
#define REF_VOLTAGE 3.3    // Referenzspannung des ESP32
#define PIN_STEPS 4095.0   // ADC-Auflösung des ESP32 (12-bit)
const float R1 = 120000.0; // Widerstand R1 (120 kOhm)
const float R2 = 10800.0;  // Widerstand R2 (11 kOhm)
float vout = 0.0;          // Gemessene Ausgangsspannung
float vin = 0.0;           // Berechnete Eingangsspannung
int rawValue = 0;          // Rohwert vom ADC
//--------------------------------

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

void cam_turn();

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
  else
  {
    return HALT;
  }
}

void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length)
{
  String message = String((char *)payload).substring(0, length);
  Serial.println("WebSocket-Nachricht empfangen: " + message);

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
    }
    else if (jsonDoc.containsKey("camera_position"))
    {
      const char *camera_position = jsonDoc["camera_position"];
      if (camera_position)
      {
        camera_pos = atoi(camera_position);
        cam_turn();
      }
    }
    else if (jsonDoc.containsKey("light_status"))
    {
      const char *light_status = jsonDoc["light_status"];
      if (light_status)
      {
        lights_on();
      }
      else if(light_status == 0){
        lights_off();
      }
    }
  }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
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

void setup()
{
  Serial.begin(115200);

  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("Fehler bei der IP-Konfiguration");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }
  Serial.println("WLAN verbunden");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  websocket.begin();
  websocket.onEvent(onWebSocketEvent);

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

  startMillis = millis();

  LoadCell.begin();
  float calibrationValue;               // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0;             // uncomment this if you want to set the calibration value in the sketch
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                 // set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }

  myservo.attach(servoPin);
  myservo.write(camera_pos);

  if (!mcp.begin_I2C()) {
    Serial.println("Error.");
    while (1);
  }

  mcp.pinMode(light_pin, OUTPUT);

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

void cam_turn()
{
  Serial.println("Cam_Turn");
  myservo.write(camera_pos);
  Serial.println(String(camera_pos));
}

//Licht einschalten und auschalten
void lights_on(){
  mcp.digitalWrite(light_pin, HIGH);
}

void lights_off(){
  mcp.digitalWrite(light_pin, LOW);
}
//-------------------------------------------

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

void loop()
{
  websocket.loop();
  navigate();

  // Spannungsmessung
  rawValue = analogRead(PIN_TEST);
  vout = (rawValue * REF_VOLTAGE) / PIN_STEPS;
  vin = vout / (R2 / (R1 + R2));
  if (vin < 0.09)
  {
    vin = 0.0;
  }

  currentMillis = millis();
  if (currentMillis - startMillis >= period)
  {
    startMillis = currentMillis;
    Serial.println("U = " + String(vin + 1, 1) + " V"); // Spannung mit 2 Dezimalstellen
    float batteryPercentage = ((vin - 9) / 4) * 100;    // Vin in mV gemessen
    float akku_round = round(batteryPercentage / 10) * 10;
    Serial.println("Akkustand: " + String(akku_round, 0) + "%");
    Serial.println(distanceSensor.measureDistanceCm());

    weight = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(weight);

    String batterystatus = String(akku_round, 0);
    String weightstatus = String(weight, 0);
    String message = "{\"battery\": \"" + batterystatus + "\", \"weight\": \"" + weightstatus + "\"}";
    websocket.broadcastTXT(message.c_str());
  }
}
