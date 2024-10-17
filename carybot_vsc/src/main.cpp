#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>

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

// Handler für die Root-Seite ("/")
void handleRoot()
{
  String html = readFile(SPIFFS, "/webserver.html"); // HTML-Datei von SPIFFS laden
  server.send(200, "text/html", html);               // HTML an den Client senden
}

void handleCoords()
{
  String speed = server.arg("speed");
  String dir = server.arg("direction");

  Serial.print("Speed: ");
  Serial.println(speed);
  Serial.print("Direction: ");
  Serial.println(dir);

  server.send(200, "text/plain", "Coords received");
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

  // Webserver-Root definieren
  server.on("/", handleRoot);
  server.on("/coords", HTTP_GET, handleCoords);

  // Webserver starten
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  // Handle Client-Anfragen
  server.handleClient();
}
