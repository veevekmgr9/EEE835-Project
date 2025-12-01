#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ----- LCD CONFIG -----
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Try 0x27 first, if not working use 0x3F

// ----- DHT CONFIG -----
#define DHTPIN 2            // DHT Data connected to pin D2 on MKR1010
#define DHTTYPE DHT11       // Change to DHT22 if you are using DHT22: DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Serial (optional for debugging)
  Serial.begin(9600);
  while (!Serial);  // Required for MKR boards

  // Initialize LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // Display starting message
  lcd.setCursor(0, 0);
  lcd.print("DHT + LCD Test");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  // Start DHT sensor
  dht.begin();
}

void loop() {
  // Read humidity & temperature
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Celsius

  // If sensor fails
  if (isnan(h) || isnan(t)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");
    delay(2000);
    return;
  }

  // Print to Serial (debug)
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print(" C  Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  // Print Temperature
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(t, 1);
  lcd.print((char)223);  // Degree symbol
  lcd.print("C");

  // Print Humidity
  lcd.setCursor(0, 1);
  lcd.print("H: ");
  lcd.print(h, 1);
  lcd.print("%");

  delay(2000);   // Update every 2 seconds
}
