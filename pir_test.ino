int pirPin = 2;     // PIR OUT pin
int ledPin = 5;     // LED pin

void setup() {
  Serial.begin(115200);

  pinMode(pirPin, INPUT_PULLUP); // PIR output
  pinMode(ledPin, OUTPUT);       // LED output

  digitalWrite(ledPin, LOW);     // LED OFF initially

  Serial.println("PIR warming up... wait 30 seconds");
}

void loop() {
  int motion = digitalRead(pirPin);

  if (motion == LOW) {     // LOW = motion detected (because using INPUT_PULLUP)
    Serial.println("Motion detected! Light ON");
    digitalWrite(ledPin, HIGH);  // turn light ON
  } else {
    Serial.println("No motion. Light OFF");
    digitalWrite(ledPin, LOW);   // turn light OFF
  }

  delay(200); // debounce
}
