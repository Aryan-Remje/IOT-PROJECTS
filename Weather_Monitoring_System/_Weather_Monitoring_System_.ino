#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

// WiFi credentials
const char* ssid = "Aryan";
const char* password = "Aryan@3501";

// ThingSpeak details
const char* server = "api.thingspeak.com";
String apiKey = "4PVLL6FRBMCBE2IB";

// DHT11 setup
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// BMP280 setup
Adafruit_BMP280 bmp;

WiFiClient client;

unsigned long lastUpload = 0;
const unsigned long uploadInterval = 16000; // ThingSpeak safe interval
const unsigned long localInterval = 4000;   // local serial refresh
unsigned long lastLocalPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found, check wiring!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
  Serial.println("========================================");
}

void loop() {
  unsigned long currentMillis = millis();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(1013.25);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("⚠ Failed to read from DHT sensor!");
    return;
  }

  // Local fast print every 4 sec
  if (currentMillis - lastLocalPrint >= localInterval) {
    lastLocalPrint = currentMillis;
    Serial.println("┌──────────────────────────────┐");
    Serial.println("│   WEATHER STATION - LIVE DATA │");
    Serial.println("├──────────────────────────────┤");
    Serial.print("│ Temperature : "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("│ Humidity    : "); Serial.print(humidity); Serial.println(" %");
    Serial.print("│ Pressure    : "); Serial.print(pressure); Serial.println(" hPa");
    Serial.print("│ Altitude    : "); Serial.print(altitude); Serial.println(" m");
    Serial.println("└──────────────────────────────┘");
  }

  // ThingSpeak upload every 16 sec
  if (currentMillis - lastUpload >= uploadInterval) {
    lastUpload = currentMillis;

    if (client.connect(server, 80)) {
      String postStr = apiKey;
      postStr += "&field1=" + String(temperature);
      postStr += "&field2=" + String(humidity);
      postStr += "&field3=" + String(pressure);
      postStr += "&field4=" + String(altitude);

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      Serial.println("✅ Data uploaded to ThingSpeak");
    } else {
      Serial.println("❌ Connection to ThingSpeak failed");
    }
    client.stop();
  }
}