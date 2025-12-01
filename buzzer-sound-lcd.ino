#include <LiquidCrystal_I2C.h>

int soundPin  = 2;     // sound sensor OUT
int buzzerPin = 5;     // buzzer pin

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool buzzing = false;
unsigned long buzzStart = 0;

String scrollMsg = "Too loud noise detected!";
int scrollIndex = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  pinMode(soundPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
}

void scrollText() {
  // Display 16 characters starting at scrollIndex
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(scrollMsg.substring(scrollIndex, scrollIndex + 16));

  scrollIndex++;
  if (scrollIndex > scrollMsg.length() - 16) {
    scrollIndex = 0;  // Loop back to start
  }
}

void loop() {
  int state = digitalRead(soundPin);

  // Noise detected
  if (!buzzing && state == LOW) {
    Serial.println("LOUD SOUND DETECTED!");
    tone(buzzerPin, 2500);

    scrollIndex = 0;   // restart scroll from beginning
    buzzStart = millis();
    buzzing = true;
  }

  // During buzzing → show scrolling text
  if (buzzing) {
    scrollText();
    delay(200);       // adjust speed (lower = faster)
  }

  // Stop after 5 seconds
  if (buzzing && millis() - buzzStart >= 5000) {
    noTone(buzzerPin);
    buzzing = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("READY...");
    delay(300);
  }
}
