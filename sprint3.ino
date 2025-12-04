#include <WiFi.h>
#include <WebServer.h>

// Pins im using are D32 & 34 & 26 & 27
const int moisture_pin = 34;
const int relay_pin    = 32;
const int button_pin   = 26;
const int led_pin      = 27;

// New thresholds:
int low_moisture_threshold  = 100;
int high_moisture_threshold = 400;

// WiFi credentials (your hotspot / WiFi)
const char* ssid     = "T-Mobile Hotspot_5026_2.4GHz";
const char* password = "55225026";

WebServer server(80);

// Shared 10-second watering duration (button + app)
const unsigned long WATER_DURATION_MS = 10UL * 1000UL;

bool buttonWatering = false;
bool appWatering    = false;
unsigned long waterEndTime = 0;

// GET /status moisture = 0, isWatering = 1}
void handleStatus() {
  int current_moisture = analogRead(moisture_pin);
  int relayState       = digitalRead(relay_pin); // LOW = ON, HIGH = OFF
  bool wateringNow     = (relayState == LOW);

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
  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println("Booting...");

  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);

  pinMode(button_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW); 

  pinMode(moisture_pin, INPUT);

  // My WiFi setup 
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

  server.on("/status", HTTP_GET, handleStatus);
  server.on("/water/start", HTTP_POST, handleWaterStart);

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
      digitalWrite(relay_pin, LOW); // ON
      digitalWrite(led_pin, HIGH);
    } else {
      // Stop watering
      buttonWatering = false;
      appWatering    = false;
      digitalWrite(relay_pin, HIGH);
      digitalWrite(led_pin, LOW);

      int new_moisture = analogRead(moisture_pin);
      Serial.print("Timed watering DONE. Moisture: ");
      Serial.println(new_moisture);
    }

    delay(100);
    return; // skip auto logice
  }

  // Automatic mode with new thresholds
  if (current_moisture != 0 && current_moisture < low_moisture_threshold) {
    digitalWrite(relay_pin, LOW);  // ON
    digitalWrite(led_pin, HIGH);
  } else if (current_moisture > high_moisture_threshold) {
    digitalWrite(relay_pin, HIGH); // OFF
    digitalWrite(led_pin, LOW);
  }

  Serial.print("Current moisture is ");
  Serial.println(current_moisture);

  delay(1000);
}
