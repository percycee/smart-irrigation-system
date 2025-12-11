#include <WiFi.h>
#include <WebServer.h>

// Pins im using are D32 & 34 & 26 & 27
const int moisture_pin = 34;
const int relay_pin    = 32;
const int button_pin   = 26;
const int led_pin      = 27;
const int RELAY_ON  = HIGH;  // your board: HIGH = relay energized
const int RELAY_OFF = LOW;


// New thresholds – tune as needed
int low_moisture_threshold  = 1300;  // below this = too dry -> turn ON
int high_moisture_threshold = 2600;  // above this = wet enough -> turn OFF

// WiFi credentials (your hotspot / WiFi)
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// IMPORTANT: only this line, no IPAddress variables
WebServer server(80);

// Shared 10-second watering duration (button + app)
const unsigned long WATER_DURATION_MS = 10UL * 1000UL;

bool buttonWatering = false;
bool appWatering    = false;
unsigned long waterEndTime = 0;

void handleStatus() {
  int current_moisture = analogRead(moisture_pin);
  int relayState   = digitalRead(relay_pin);
  bool wateringNow = (relayState == RELAY_ON);

  String json = "{";
  json += "\"moisture\":"   + String(current_moisture) + ",";
  json += "\"isWatering\":" + String(wateringNow ? 1 : 0);
  json += "}";

  server.send(200, "application/json", json);
}

void handleWaterStart() {
  appWatering    = true;
  buttonWatering = false;
  waterEndTime   = millis() + WATER_DURATION_MS;

  Serial.println("App-triggered watering STARTED (10 seconds).");
  server.send(200, "text/plain", "ok");
}


void setup() {
  Serial.begin(115200);   // <--- match Serial Monitor
  delay(500);
  Serial.println();
  Serial.println("Booting irrigation sketch...");

  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, RELAY_OFF);   // start OFF

  pinMode(button_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  pinMode(moisture_pin, INPUT);

  // WiFi setup
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  // REST endpoints
  server.on("/status",       HTTP_GET,  handleStatus);
  server.on("/water/start",  HTTP_POST, handleWaterStart);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();

  int current_moisture = analogRead(moisture_pin);
  int current_button   = digitalRead(button_pin);

  // Starts 10-second watering when pressed
  if (current_button == LOW && !buttonWatering && !appWatering) {
    delay(50);
    if (digitalRead(button_pin) == LOW) {
      buttonWatering = true;
      appWatering    = false;
      waterEndTime   = millis() + WATER_DURATION_MS;

      Serial.print("Button watering STARTED (10 seconds). Moisture: ");
      Serial.println(current_moisture);
    }
  }

  // Timed watering mode
  if (buttonWatering || appWatering) {
    if ((long)(millis() - waterEndTime) < 0) {
      // Keep water ON
      digitalWrite(relay_pin, RELAY_ON);
      digitalWrite(led_pin, HIGH);
    } else {
      // Stop watering
      buttonWatering = false;
      appWatering    = false;
      digitalWrite(relay_pin, RELAY_OFF);
      digitalWrite(led_pin, LOW);

      int new_moisture = analogRead(moisture_pin);
      Serial.print("Timed watering DONE. Moisture: ");
      Serial.println(new_moisture);
    }

    delay(100);
    return; // skip auto logic
  }

  // Automatic mode with new thresholds
  if (current_moisture != 0 && current_moisture < low_moisture_threshold) {
    digitalWrite(relay_pin, RELAY_ON);   // ON
    digitalWrite(led_pin, HIGH);
  } else if (current_moisture > high_moisture_threshold) {
    digitalWrite(relay_pin, RELAY_OFF);  // OFF
    digitalWrite(led_pin, LOW);
  }

  Serial.print("Current moisture is ");
  Serial.println(current_moisture);

  delay(1000);
}

