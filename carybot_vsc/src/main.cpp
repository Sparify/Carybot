#include <WiFi.h>
#include "ArduinoJson.h"
#include "HCSR04.h"
#include "WebSocketsServer.h"
#include <HX711_ADC.h>

#include <ESP32Servo.h>

#include <Adafruit_MCP23X17.h>

const int HX711_dout = 33;
const int HX711_sck = 14;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

Adafruit_MCP23X17 mcp;
const int light_pin = 0;
int light_st = 0;

UltraSonicDistanceSensor distanceSensor_rear(23, 19);
UltraSonicDistanceSensor distanceSensor_front(13,12);
float distance_front = 0.0;
float distance_rear = 0.0;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000;

int leftfrontwheel_pwm = 32;
const int leftfrontwheel_brake = 1;
const int leftfrontwheel = 2;

int leftrearwheel_pwm = 18;
const int leftrearwheel_brake = 3;
const int leftrearwheel = 4;

int rightfrontwheel_pwm = 27;
const int rightfrontwheel_brake = 5;
int rightfrontwheel = 6;

int rightrearwheel_pwm = 25;
const int rightrearwheel_brake = 7;
const int rightrearwheel = 8;


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
int camera_pos = 90;
Servo myservo;
static const int servoPin = 26;
//-----------------

int speed_cb = 0;

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
void lights_on();
void lights_off();

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
      bool light_status = jsonDoc["light_status"]; 
      light_st = light_status ? 1 : 0;             

      if (light_st == 1)
      {
        lights_on();
      }
      else
      {
        lights_off();
      }
    }
  }
}

void stop()
{
    mcp.digitalWrite(leftfrontwheel_brake, HIGH);
    mcp.digitalWrite(rightfrontwheel_brake, HIGH);
    mcp.digitalWrite(leftrearwheel_brake, HIGH);
    mcp.digitalWrite(rightrearwheel_brake, HIGH);
    Serial.println("Stop");
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
    dir = HALT;
    stop();
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

  if (!mcp.begin_I2C())
  {
    Serial.println("MCP Error.");
    while (1);
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

  mcp.pinMode(leftfrontwheel, OUTPUT);
  pinMode(leftfrontwheel_pwm, OUTPUT);
  mcp.pinMode(leftfrontwheel_brake, OUTPUT);
  mcp.digitalWrite(leftfrontwheel_brake, HIGH);

  mcp.pinMode(rightfrontwheel, OUTPUT);
  pinMode(rightfrontwheel_pwm, OUTPUT);
  mcp.pinMode(rightfrontwheel_brake, OUTPUT);
  mcp.digitalWrite(rightfrontwheel_brake, HIGH);

  mcp.pinMode(leftrearwheel, OUTPUT);
  pinMode(leftrearwheel_pwm, OUTPUT);
  mcp.pinMode(leftrearwheel_brake, OUTPUT);
  mcp.digitalWrite(leftrearwheel_brake, HIGH);

  mcp.pinMode(rightrearwheel, OUTPUT);
  pinMode(rightrearwheel_pwm, OUTPUT);
  mcp.pinMode(rightrearwheel_brake, OUTPUT);
  mcp.digitalWrite(rightrearwheel_brake, HIGH);

  pinMode(PIN_TEST, INPUT);

  startMillis = millis();

  LoadCell.begin();
  float calibrationValue;               // calibration value (see example file "Calibration.ino")
  calibrationValue = 21.82;             // uncomment this if you want to set the calibration value in the sketch
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

  mcp.pinMode(light_pin, OUTPUT);
}

void moveForward()
{
    mcp.digitalWrite(leftfrontwheel, HIGH);
    mcp.digitalWrite(rightfrontwheel, LOW);
    mcp.digitalWrite(leftrearwheel, HIGH);
    mcp.digitalWrite(rightrearwheel, LOW);
    Serial.println("Vorwärts");
}

void moveBackward()
{
    mcp.digitalWrite(leftfrontwheel, LOW);
    mcp.digitalWrite(rightfrontwheel, HIGH);
    mcp.digitalWrite(leftrearwheel, LOW);
    mcp.digitalWrite(rightrearwheel, HIGH);
    Serial.println("Rückwärts");
}

void turnLeft()
{
  mcp.digitalWrite(leftfrontwheel, LOW);
  mcp.digitalWrite(rightfrontwheel, LOW);
  mcp.digitalWrite(leftrearwheel, LOW);
  mcp.digitalWrite(rightrearwheel, LOW);
  Serial.println("Links");
}

void turnRight()
{
  mcp.digitalWrite(leftfrontwheel, HIGH);
  mcp.digitalWrite(rightfrontwheel, HIGH);
  mcp.digitalWrite(leftrearwheel, HIGH);
  mcp.digitalWrite(rightrearwheel, HIGH);
  Serial.println("Rechts");
}

void cam_turn()
{
  Serial.println("Cam_Turn");
  myservo.write(camera_pos);
  Serial.println(String(camera_pos));
}

// Licht einschalten und auschalten
void lights_on()
{
  Serial.println("Licht an");
  mcp.digitalWrite(light_pin, HIGH);
}

void lights_off()
{
  Serial.println("Licht aus");
  mcp.digitalWrite(light_pin, LOW);
}
//-------------------------------------------


void navigate()
{
  static Direction lastDir = HALT;
  static int lastSpeed = -1;

  if (dir != lastDir || speed_cb != lastSpeed)
  {
    speed_cb = speed.toInt() * 2.55;
    lastSpeed = speed_cb;

    // Motorbremsen deaktivieren
    mcp.digitalWrite(leftfrontwheel_brake, LOW);
    mcp.digitalWrite(rightfrontwheel_brake, LOW);
    mcp.digitalWrite(leftrearwheel_brake, LOW);
    mcp.digitalWrite(rightrearwheel_brake, LOW);

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

  if (websocket.connectedClients() == 0)
  {
    Serial.println("fahr in arsch");
    dir = HALT;
   stop();
  }
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
    Serial.println("U = " + String(vin + 2, 1) + " V"); // Spannung mit 2 Dezimalstellen
    float batteryPercentage = ((vin - 9) / 4) * 100;    // Vin in mV gemessen
    float akku_round = round(batteryPercentage / 10) * 10;
    Serial.println("Akkustand: " + String(akku_round, 0) + "%");
    distance_front = distanceSensor_front.measureDistanceCm();
    distance_rear = distanceSensor_rear.measureDistanceCm();
    Serial.println(distance_front);
    static boolean newDataReady = 0;
    if (LoadCell.update()) newDataReady = true;
    if (newDataReady) {
      weight = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(weight);
      newDataReady=0;
    }

    if(((distance_front >= 5 && distance_front <= 20) && speed_cb >= 50) || ((distance_rear >= 5 && distance_rear <= 20) && speed_cb >= 50)){
      stop();
    }

    String batterystatus = String(akku_round, 0);
    String weightstatus = String(weight, 0);
    String message = "{\"battery\": \"" + batterystatus + "\", \"weight\": \"" + weightstatus + "\"}";
    websocket.broadcastTXT(message.c_str());
  }
}
