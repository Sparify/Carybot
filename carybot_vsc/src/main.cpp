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

void handleRoot()
{
  String dpad = readFile(SPIFFS, "/dpad.html");
  server.send(200, "text/html", dpad);
}

void handleControl()
{
  String dpad = readFile(SPIFFS, "/dpad.html");
  server.send(200, "text/html", dpad);
}

void handleIcon()
{
  String icon = readFile(SPIFFS, "/menu-icon.svg");
  server.send(200, "image/svg+xml", icon);
}

void handleStyles()
{
  String css = readFile(SPIFFS, "/mystyles.css");
  server.send(200, "text/css", css);
}

void handleScript()
{
  String js = readFile(SPIFFS, "/carybot.js");
  server.send(200, "application/javascript", js);
}

void handleDaten()
{
  String daten = readFile(SPIFFS, "/daten.html");
  server.send(200, "text/html", daten);
}

void handleFavicon()
{
  server.send(204);
}

String speed;
String dir;

void handleRobot()
{
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
}

void setup()
{
  // Serielle Kommunikation starten
  Serial.begin(115200);

  // WLAN-Verbindung herstellen
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  Serial.println("Hotspot started");

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // SPIFFS initialisieren
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  server.on("/move_robot", HTTP_GET, handleRobot);
  server.on("/move_cam", HTTP_GET, handleCam);

  server.on("/", handleRoot);
  server.on("/dpad.html", handleControl);
  server.on("/menu-icon.svg", handleIcon);
  server.on("/mystyles.css", handleStyles);
  server.on("/carybot.js", handleScript);
  server.on("/daten.html", handleDaten);
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

int speed_cb = 0;

void navigate(String dir)
{
  speed_cb = speed.toInt() * 2.55;
  digitalWrite(leftwheel_brake, LOW);
  digitalWrite(rightwheel_brake, LOW);

  if (dir == "up")
  {
    digitalWrite(leftwheel, HIGH);
    digitalWrite(rightwheel, LOW);
  }
  else if (dir == "down")
  {
    digitalWrite(leftwheel, LOW);
    digitalWrite(rightwheel, HIGH);
  }
  else if (dir == "left")
  {
    digitalWrite(leftwheel, LOW);
    digitalWrite(rightwheel, LOW);
  }
  else if (dir == "right")
  {
    digitalWrite(leftwheel, HIGH);
    digitalWrite(rightwheel, HIGH);
  }
  else if (dir == "up_left")
  {
    digitalWrite(leftwheel_brake, HIGH);
    digitalWrite(rightwheel, LOW);
  }
  else if (dir == "up_right")
  {
    digitalWrite(leftwheel, HIGH);
    digitalWrite(rightwheel_brake, HIGH);
  }
  else if (dir == "down_left")
  {
    digitalWrite(leftwheel_brake, HIGH);
    digitalWrite(rightwheel, HIGH);
  }
  else if (dir == "down_right")
  {
    digitalWrite(leftwheel, LOW);
    digitalWrite(rightwheel_brake, HIGH);
  }
  else if (dir == "halt")
  {
    digitalWrite(leftwheel_brake, HIGH);
    digitalWrite(rightwheel_brake, HIGH);
  }
  else
  {
    digitalWrite(leftwheel_brake, HIGH);
    digitalWrite(rightwheel_brake, HIGH);
  }

  analogWrite(leftwheel_pwm, speed_cb);
  analogWrite(rightwheel_pwm, speed_cb);
}

void loop()
{
  server.handleClient();
  navigate(dir);
}
