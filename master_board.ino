#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//  Pin setup 
#define DHTPIN   3          // DHT signal pin
#define DHTTYPE  DHT11      // using DHT11 sensor

const int SOUND_PIN  = 2;   // digital output from sound sensor
const int BUZZER_PIN = 5;   // buzzer output
const int BUTTON_PIN = 6;   // button to silence alarms

// Normal temp and humidity
float NORMAL_TEMP = 28.0;
float NORMAL_HUM  = 50.0;

//  LCD and DHT 
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

//  Alarm state variables 
bool alarmActive = false;          // true while buzzer is sounding
bool alarmSystemEnabled = true;    // false when user disables alarms

unsigned long alarmStartTime = 0;     // when the alarm started
unsigned long alarmResumeTime = 0;    // when alarms turn back on

//  Scrolling text settings 
String alertMessage = "Alert! ";
unsigned long lastScrollTime = 0;
int scrollPosition = 0;

//  DHT read timer 
unsigned long lastDHTUpdate = 0;

// Setup
void setup()
{
    Serial.begin(9600);
    while (!Serial);      // wait for Serial to connect

    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("Starting...");
    delay(1200);
    lcd.clear();

    pinMode(SOUND_PIN,  INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // button not pressed = HIGH

    dht.begin();     // start DHT sensor
}

// Main Loop
void loop()
{
    // If alarms were disabled earlier, re-enable them when time runs out.
    if (!alarmSystemEnabled && millis() >= alarmResumeTime)
    {
        alarmSystemEnabled = true;

        lcd.setCursor(0, 0);
        lcd.print("ALARM ACTIVE");
        delay(800);

        lcd.setCursor(0, 0);
        lcd.print("            ");
    }

    // Check if the cancel button is pressed (LOW = pressed)
    if (digitalRead(BUTTON_PIN) == LOW)
    {
        disableAlarmForFiveMinutes();
    }

    // Temperature & humidity should always update, even when alarms are off
    updateTemperatureDisplay();

    // If the user disabled alarms, skip checking noise/temp/humidity
    if (!alarmSystemEnabled)
    {
        noTone(BUZZER_PIN);   // make sure buzzer is off
        alarmActive = false;
        return;
    }

    // Normal alarm checking happens only if system is enabled
    checkAlarmConditions();
}

// Check Noise / Temp / Humidity and Trigger Alarm
void checkAlarmConditions()
{
    int sound = digitalRead(SOUND_PIN);   // LOW = noise detected

    bool tempHigh = false;
    bool humHigh  = false;

    // Read updated temperature and humidity
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Validate sensor values (DHT sometimes returns NaN)
    if (!isnan(h) && !isnan(t))
    {
        if (t > NORMAL_TEMP) tempHigh = true;
        if (h > NORMAL_HUM)  humHigh  = true;
    }

    bool noiseDetected = (sound == LOW);
    bool somethingIsWrong = noiseDetected || tempHigh || humHigh;

    // If nothing was happening before and now an alarm is needed
    if (!alarmActive && somethingIsWrong)
    {
        tone(BUZZER_PIN, 1000);       // simple alarm tone
        alarmStartTime = millis();
        alarmActive = true;
        scrollPosition = 0;

        // scrolling text depending on what caused the alarm
        alertMessage = "Alert! ";
        if (noiseDetected) alertMessage += "Noise! ";
        if (tempHigh)      alertMessage += "Temp! ";
        if (humHigh)       alertMessage += "Humidity! ";
        alertMessage += "   ";        // spacing buffer
    }

    // If alarm is active, keep scrolling the alert text
    if (alarmActive)
    {
        scrollAlertText();
    }

    // Stop alarm after 1 second (short beep)
    if (alarmActive && millis() - alarmStartTime >= 3000)
    {
        stopAlarm();
    }
}

// Stop the alarm sound and update LCD
void stopAlarm()
{
    noTone(BUZZER_PIN);        // stop buzzer
    alarmActive = false;

    lcd.setCursor(0, 0);
    lcd.print("READY         ");
    delay(400);

    lcd.setCursor(0, 0);
    lcd.print("               ");
}

// Disable all alarms for 5 minutes
void disableAlarmForFiveMinutes()
{
    alarmSystemEnabled = false;   // block new alarms
    alarmActive = false;
    noTone(BUZZER_PIN);

    // Set a timer to re-enable alarms later
    alarmResumeTime = millis() + 60000UL;

    lcd.setCursor(0, 0);
    lcd.print("ALARM OFF 5min");
    delay(400);
}

// Scroll the alert message on the top row
void scrollAlertText()
{
    // Only update text every ~0.18s to make it readable
    if (millis() - lastScrollTime >= 180)
    {
        lastScrollTime = millis();

        lcd.setCursor(0, 0);
        lcd.print(alertMessage.substring(scrollPosition, scrollPosition + 16));

        scrollPosition++;
        if (scrollPosition > alertMessage.length() - 16)
            scrollPosition = 0;   // loop text
    }
}

// Update temperature & humidity display
void updateTemperatureDisplay()
{
    // Keep sensor reads spaced out to avoid slowdowns
    if (millis() - lastDHTUpdate >= 1500)
    {
        lastDHTUpdate = millis();

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (isnan(h) || isnan(t))
        {
            // DHT sensors sometimes fail a reading; show a friendly message
            lcd.setCursor(0, 1);
            lcd.print("Sensor Error   ");
            return;
        }

        // Show temp + humidity on bottom row
        lcd.setCursor(0, 1);
        lcd.print("T:");
        lcd.print(t, 1);
        lcd.print((char)223);  // degree symbol
        lcd.print("C ");

        lcd.print("H:");
        lcd.print(h, 0);
        lcd.print("%  ");
    }
}
