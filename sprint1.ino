// Pins im using are D32 & 34 & 26 & 27
const int moisture_pin = 34;
const int relay_pin = 32;
const int button_pin = 26;
const int led_pin = 27;

// I want a low threshold to know when the soil needs watering.
int low_moisture_threshold = 1500;  
// I want a high threshold to know when it likely rained and to skip watering.
int high_moisture_threshold = 3000;

// Keeping a log of the manual watering time.

void setup() {
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);
  Serial.begin(9600);
  pinMode(button_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW); 
}

void loop() {
  // current moisture or button declaration
  int current_moisture = analogRead(moisture_pin);
  int current_button = digitalRead(button_pin);

  // if botton is pushed 
  if (current_button == LOW) { 
    // Turn on the relay to water
    digitalWrite(relay_pin, LOW);
    digitalWrite(led_pin, HIGH); 
    // keep checking if the button is being pressed
    while (digitalRead(button_pin) == LOW) {
      delay(10);
    }
    // If not, turn it off
    digitalWrite(relay_pin, HIGH);
    digitalWrite(led_pin, LOW); 
    // Display 
    Serial.print("Manual watering done. Current moisture is ");
    Serial.println(current_moisture);
    return;
  }

  else {
    // If moisture is low turn relay on (LOW)
    if (current_moisture < low_moisture_threshold) {
      digitalWrite(relay_pin, LOW);
      digitalWrite(led_pin, HIGH);   
    } 
      // Once the soil is wet enough, turn relay off (HIGH)
    else if (current_moisture > high_moisture_threshold) {
      digitalWrite(relay_pin, HIGH);
      digitalWrite(led_pin, LOW);
    }
    // Display
    Serial.print("Current moisture is ");
    Serial.println(current_moisture);
    // delay 10 minutes
    delay(1000);
  }
}
