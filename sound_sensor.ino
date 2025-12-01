int soundPin = 2;  
int ledPin = 5;

unsigned long lastTrigger = 0;  // store time of last detection
int cooldown = 300;             // ms, ignore sound for this long

void setup() {
  Serial.begin(115200);
  pinMode(soundPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  int state = digitalRead(soundPin);

  // Only react if sensor goes LOW (sound detected)
  if (state == LOW && (millis() - lastTrigger > cooldown)) {
    Serial.println("Sound Detected!");
    digitalWrite(ledPin, HIGH);

    delay(100);  // LED on briefly
    digitalWrite(ledPin, LOW);

    lastTrigger = millis();  // remember last event
  }
}
