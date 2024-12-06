#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>

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
WebServer server(80);

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

void setCommonHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot()
{
  setCommonHeaders();
  String dpad = readFile(SPIFFS, "/dpad.html");
  server.send(200, "text/html", dpad);
}

void handleControl()
{
  setCommonHeaders();
  String dpad = readFile(SPIFFS, "/dpad.html");
  server.send(200, "text/html", dpad);
}

void handleIcon()
{
  setCommonHeaders();
  String icon = readFile(SPIFFS, "/menu-icon.svg");
  server.send(200, "image/svg+xml", icon);
}

void handleStyles()
{
  setCommonHeaders();
  String css = readFile(SPIFFS, "/mystyles.css");
  server.send(200, "text/css", css);
}

void handleScript()
{
  setCommonHeaders();
  String js = readFile(SPIFFS, "/carybot.js");
  server.send(200, "application/javascript", js);
}

void handleFavicon()
{
  setCommonHeaders();
  server.send(204);
}

String speed = "0";
String dir = "halt";

void handleRobot()
{
  setCommonHeaders();

  speed = server.arg("speed");
  dir = server.arg("direction");

  Serial.print("Speed: ");
  Serial.println(speed);
  Serial.print("Direction: ");
  Serial.println(dir);

  server.send(200, "text/plain", "Coords received");
}

void handleCam()
{
  // setCommonHeaders();

  // ToDo: Kamera-Servo Steuerung
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

  server.on("/move_robot", HTTP_GET, handleRobot);
  server.on("/move_cam", HTTP_GET, handleCam);

  server.onNotFound([]()
                    { 
                    setCommonHeaders();
                    server.send(404, "text/plain", "Not Found"); });

  server.on("/", HTTP_GET, []()
            { handleRoot(); });

  server.on("/dpad.html", handleControl);
  server.on("/menu-icon.svg", handleIcon);
  server.on("/mystyles.css", handleStyles);
  server.on("/carybot.js", handleScript);
  server.on("/favicon.ico", handleFavicon);

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
}

void moveBackward()
{
  digitalWrite(leftwheel, LOW);
  digitalWrite(rightwheel, HIGH);
}

void turnLeft()
{
  digitalWrite(leftwheel, LOW);
  digitalWrite(rightwheel, LOW);
}

void turnRight()
{
  digitalWrite(leftwheel, HIGH);
  digitalWrite(rightwheel, HIGH);
}

void stop()
{
  digitalWrite(leftwheel_brake, HIGH);
  digitalWrite(rightwheel_brake, HIGH);
}

int speed_cb = 0;

void navigate()
{
  static String lastDir = "";
  static int lastSpeed = -1;

  if (dir != lastDir || speed_cb != lastSpeed)
  {
    speed_cb = speed.toInt() * 2.55;
    lastSpeed = speed_cb;
    lastDir = dir;

    // Motorbremsen deaktivieren
    digitalWrite(leftwheel_brake, LOW);
    digitalWrite(rightwheel_brake, LOW);

    if (dir == "up") // Vorwärts
    {
      moveForward();
    }
    else if (dir == "down") // Rückwärts
    {
      moveBackward();
    }
    else if (dir == "left") // Links
    {
      turnLeft();
    }
    else if (dir == "right") // Rechts
    {
      turnRight();
    }
    else if (dir == "halt")
    {
      stop();
    }
    else
    {
      stop();
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

void loop()
{
  server.handleClient();
  navigate();
}
