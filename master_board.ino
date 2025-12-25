// Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

// WiFi credentials
char ssid[] = "s22plus";
char pass[] = "test123!";

// WiFi and MQTT clients
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// MQTT broker details
const char broker[] = "broker.hivemq.com";
int port = 1883;

// MQTT topics from Bed node
const char topicBedLight[]  = "home/bedroom/light";
const char topicBedMotion[] = "home/bedroom/motion";
const char topicBedStatus[] = "home/bedroom/status";

// MQTT topics from Desk node
const char topicDeskLight[]    = "desk/light";
const char topicDeskDistance[] = "desk/distance";
const char topicDeskStatus[]   = "desk/status";

// Stores last received MQTT message
String bedMessage = "";
bool bedMessageAvailable = false;

// Sensor pin definitions
#define DHTPIN   3
#define DHTTYPE  DHT11

const int SOUND_PIN  = 2;
const int BUZZER_PIN = 5;
const int BUTTON_PIN = 6;

// Normal threshold values
float NORMAL_TEMP = 25.0;
float NORMAL_HUM  = 50.0;

// LCD and DHT objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// Alarm state flags
bool alarmActive = false;
bool alarmSystemEnabled = true;

// Used to control alarm timing
unsigned long alarmStartTime  = 0;
unsigned long alarmResumeTime = 0;

// Variables for scrolling text
String alertMessage = "Alert! ";
unsigned long lastScrollTime = 0;
int scrollPosition = 0;

int scrollIndexBed = 0;
unsigned long lastScrollBed = 0;

// Timer to avoid reading DHT too often
unsigned long lastDHTUpdate = 0;


// Connect to WiFi network
void connectWiFi() {

  // Do nothing if already connected
  if (WiFi.status() == WL_CONNECTED) return;

  // LCD Message
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  Serial.println("Connecting to WiFi");

  //Wifi begin
  WiFi.begin(ssid, pass);

  // Keep trying until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }

  // LCD Clear and show Message
  lcd.clear();
  lcd.print("WiFi Connected");
  delay(500);
}


// Connect to MQTT broker and subscribe to topics
void connectMqtt() {

// LCD Clear and show Message
  lcd.clear();
  lcd.print("Connecting MQTT");
  delay(400);

  // Retry until MQTT connection is successful
  while (!mqttClient.connect(broker, port)) {
    delay(2000);
  }

  // LCD Clear and show Message
  lcd.clear();
  lcd.print("MQTT Connected");
  delay(500);

  // Subscribe to Bed node topics
  mqttClient.subscribe(topicBedLight);
  mqttClient.subscribe(topicBedMotion);
  mqttClient.subscribe(topicBedStatus);

  // Subscribe to Desk node topics
  mqttClient.subscribe(topicDeskLight);
  mqttClient.subscribe(topicDeskDistance);
  mqttClient.subscribe(topicDeskStatus);
}


// Handle incoming MQTT messages
void handleMqttMessages() {

  // Keep MQTT connection alive
  mqttClient.poll();

  // Exit if no new message
  if (!mqttClient.parseMessage()) return;

  String payload = "";

  // Read full message
  while (mqttClient.available()) {
    payload += (char)mqttClient.read();
  }

  payload.trim();
  bedMessage = payload;
  bedMessageAvailable = true;

  // Show MQTT messages only if alarm is not active
  // Messages from other boards
  if (!alarmActive && alarmSystemEnabled) {

    String scrollText = "Bed/Desk: " + bedMessage + "   ";
    unsigned long now = millis();

    if (now - lastScrollBed > 250) {
      lastScrollBed = now;

      scrollIndexBed++;
      if (scrollIndexBed > scrollText.length() - 16) {
        scrollIndexBed = 0;
      }

      lcd.setCursor(0, 0);
      lcd.print(scrollText.substring(scrollIndexBed, scrollIndexBed + 16));
    }
  }
}


// Check sound, temperature and humidity conditions
void checkAlarmConditions() {

  int soundDetected = digitalRead(SOUND_PIN);

  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  // Validate sensor readings
  bool tempHigh = !isnan(temperature) && temperature > NORMAL_TEMP;
  bool humHigh  = !isnan(humidity) && humidity > NORMAL_HUM;
  bool noise    = (soundDetected == LOW);

  // Trigger alarm if any condition is abnormal
  if (!alarmActive && (noise || tempHigh || humHigh)) {

    //Alarm Trigger
    tone(BUZZER_PIN, 1000);
    alarmActive = true;
    alarmStartTime = millis();
    scrollPosition = 0;

    // Build alert message based on cause
    alertMessage = "Alert! ";
    if (noise)    alertMessage += "Noise ";
    if (tempHigh) alertMessage += "Temp ";
    if (humHigh)  alertMessage += "Humidity ";
    alertMessage += "   ";
  }

  // Scroll alert text while alarm is active
  if (alarmActive) {
    scrollAlertText();
  }

  // Stop alarm automatically after 1 second
  if (alarmActive && millis() - alarmStartTime > 1000) {
    stopAlarm();
  }
}


// Stop buzzer and reset alarm state
void stopAlarm() {

  // Alarm
  noTone(BUZZER_PIN);
  alarmActive = false;

  //LCD Message
  lcd.setCursor(0, 0);
  lcd.print("READY           ");
}


// Disable alarm temporarily using button
void disableAlarmForFiveMinutes() {

  // Alarm
  alarmSystemEnabled = false;
  alarmActive = false;
  noTone(BUZZER_PIN);

  // 1 minute used for demonstration
  alarmResumeTime = millis() + 60000UL;

  // LCD Message
  lcd.setCursor(0, 0);
  lcd.print("ALARM OFF 5min ");
}

// Scroll alert message on LCD
void scrollAlertText() {

  if (millis() - lastScrollTime > 180) {
    lastScrollTime = millis();

    lcd.setCursor(0, 0);
    lcd.print(alertMessage.substring(scrollPosition, scrollPosition + 16));

    scrollPosition++;
    if (scrollPosition > alertMessage.length() - 16) {
      scrollPosition = 0;
    }
  }
}

// Update temperature and humidity display
void updateTemperatureDisplay() {

  // Read DHT sensor every 1.5 seconds
  if (millis() - lastDHTUpdate < 1500) return;
  lastDHTUpdate = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  lcd.setCursor(0, 1);

  // Handle sensor read error
  if (isnan(t) || isnan(h)) {
    lcd.print("Sensor Error   ");
    return;
  }
  // Continuous Temperature and Humidity measure in LCD
  lcd.print("T:");
  lcd.print(t, 1);
  lcd.print((char)223);
  lcd.print("C H:");
  lcd.print(h, 0);
  lcd.print("% ");
}


// Setup runs once at startup
void setup() {

  Serial.begin(9600);

  // LCD Message
  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

  // Pin Mode setup
  pinMode(SOUND_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //DHT sensor begin
  dht.begin();

  // WIFI Connection function call
  connectWiFi();
  connectMqtt();
}


// Main loop runs continuously
void loop() {

  // Maintain WiFi and MQTT connections
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMqtt();

  // Read incoming MQTT messages
  handleMqttMessages();

  // Re-enable alarm after timeout
  if (!alarmSystemEnabled && millis() >= alarmResumeTime) {
    alarmSystemEnabled = true;
    lcd.setCursor(0, 0);
    lcd.print("ALARM ACTIVE   ");
  }

  // Check if user pressed button
  if (digitalRead(BUTTON_PIN) == LOW) {
    disableAlarmForFiveMinutes();
  }

  // Always update temperature and humidity
  updateTemperatureDisplay();

  // Skip alarm checks if disabled
  if (!alarmSystemEnabled) return;

  // Normal monitoring mode
  checkAlarmConditions();
}
