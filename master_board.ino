//Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

//WiFi credentials
char ssid[] = "s22plus";
char pass[] = "test123!";   

//Wifi client for wifi connection
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

//Broker
const char broker[] = "broker.hivemq.com";
int port = 1883;

// MQTT topic from bed/desk board
const char topicBedLight[] = "home/bedroom/light";
const char topicBedMotion[] = "home/bedroom/motion";
const char topicBedStatus[] = "home/bedroom/status";

const char topicDeskLight[] = "desk/light";
const char topicDeskDistance[] = "desk/distance";
const char topicDeskStatus[] = "desk/status";

// stores last message from bed node
String bedMessage = "";
bool bedMessageAvailable = false;

//  Sensor and IO pin setup

#define DHTPIN   3          // DHT signal pin
#define DHTTYPE  DHT11      // using DHT11 sensor

const int SOUND_PIN  = 2;   // digital output from sound sensor
const int BUZZER_PIN = 5;   // buzzer output
const int BUTTON_PIN = 6;   // button to silence alarms

// normal temp and humidity limits
float NORMAL_TEMP = 25.0;
float NORMAL_HUM  = 50.0;

// LCD and DHT objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// alarm state variables
bool alarmActive = false;         // true while buzzer is sounding
bool alarmSystemEnabled = true;   // false when user disables alarms

unsigned long alarmStartTime  = 0;  // when the alarm started
unsigned long alarmResumeTime = 0;  // when alarms turn back on

// scrolling text settings
String alertMessage = "Alert! ";
unsigned long lastScrollTime = 0;
int scrollPosition = 0;

int scrollIndexBed = 0;
unsigned long lastScrollBed = 0;
unsigned long scrollIntervalBed = 250;

// DHT read timer
unsigned long lastDHTUpdate = 0;

// -------------------- WiFi and MQTT helper functions --------------------

// connect to WiFi network
void connectWiFi() {
  // if already connected do nothing
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("CONNECTING TO WIFI");
  delay(400);
  Serial.println("Connecting to WiFi");
//   WiFi.begin(ssid, pass);

int status = WiFi.begin(ssid, pass);
  while (status!= WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
    Serial.print(".");
  delay(1000);

  status = WiFi.status();

  if (status == WL_CONNECT_FAILED) {
    Serial.println("Wrong password!");
  }

  if (status == WL_NO_SSID_AVAIL) {
    Serial.println("SSID NOT FOUND!");
  }
  }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WIFI CONNECTED");
    delay(400);
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// connect to MQTT broker and subscribe to topic
void connectMqtt() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CONNECTING TO MQTT BROKER");
    delay(400);
  Serial.print("Connecting to MQTT broker ");

  while (!mqttClient.connect(broker, port)) {
    Serial.print(".");
    Serial.print(" error = ");
    Serial.println(mqttClient.connectError());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR CONNECTING TO MQTT BROKER");
    delay(400);
    delay(2000);

  }

  Serial.println();
  Serial.println("MQTT connected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT CONNECTED");
    delay(400);

    //Subscribe to Bed Node
  mqttClient.subscribe(topicBedLight);
  mqttClient.subscribe(topicBedMotion);
  mqttClient.subscribe(topicBedStatus);

//Subscribed to Desk Node
  mqttClient.subscribe(topicDeskLight);
  mqttClient.subscribe(topicDeskDistance);
  mqttClient.subscribe(topicDeskStatus);
  Serial.print("Subscribed to topic: ");
  Serial.println(topicBedLight);
  Serial.println(topicBedMotion);
  Serial.println(topicBedStatus);

  Serial.println(topicDeskLight);
  Serial.println(topicDeskDistance);
  Serial.println(topicDeskStatus);
}

// handle incoming MQTT messages
void handleMqttMessages() {
  // keep MQTT client alive
  mqttClient.poll();

  // check if a message is available
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    String topic = mqttClient.messageTopic();
    Serial.print("Incoming message on topic: ");
    Serial.println(topic);

    String payload = "";
    while (mqttClient.available()) {
      char c = (char)mqttClient.read();
      payload += c;
    }
    payload.trim();

    Serial.print("Payload: ");
    Serial.println(payload);

    // if message is from bed topic, store and show it
    if (topic == topicBedLight || topic == topicBedStatus || topic == topicBedMotion ||
    topic == topicDeskLight || topic == topicDeskStatus || topic == topicDeskDistance) {

    bedMessage = payload;
    bedMessageAvailable = true;

    // only scroll on LCD when no alarm is active
    if (!alarmActive && alarmSystemEnabled) {

        // ensure message is long enough for scrolling
        String scrollText = "Bed/Desk: " + bedMessage + "   ";  // extra spaces for smooth scroll

        unsigned long now = millis();
        if (now - lastScrollBed >= scrollIntervalBed) {
            lastScrollBed = now;

            // wrap-around scrolling
            scrollIndexBed++;
            if (scrollIndexBed > scrollText.length() - 16) {
                scrollIndexBed = 0;
            }

            // extract 16-character window
            String window = scrollText.substring(scrollIndexBed, scrollIndexBed + 16);

            lcd.setCursor(0, 0);
            lcd.print(window);
        }
    }
}
    // if (topic == topicBedLight || topic == topicBedStatus || topic == topicBedMotion || topic == topicDeskLight || topic == topicDeskStatus || topic == topicDeskDistance) {
    //   bedMessage = payload;
    //   bedMessageAvailable = true;

    //   // only update LCD top row if no alarm is active
    //   if (!alarmActive && alarmSystemEnabled) {
    //     lcd.setCursor(0, 0);
    //     lcd.print("Bed/Desk: ");

    //     // show up to remaining characters on line
    //     String show = bedMessage.substring(0, 16 - 5);
    //     lcd.print(show);

    //     // pad with spaces to clear previous text
    //     int pad = 16 - 5 - show.length();
    //     for (int i = 0; i < pad; i++) {
    //       lcd.print(" ");
    //     }
    //   }
    // }
  }
}

// -------------------- Alarm and display functions --------------------

// check noise / temp / humidity and trigger alarm
void checkAlarmConditions() {
  int sound = digitalRead(SOUND_PIN);   // LOW = noise detected

  bool tempHigh = false;
  bool humHigh  = false;

  // read updated temperature and humidity
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // validate sensor values (DHT sometimes returns NaN)
  if (!isnan(h) && !isnan(t)) {
    if (t > NORMAL_TEMP) tempHigh = true;
    if (h > NORMAL_HUM)  humHigh  = true;
  }

  bool noiseDetected    = (sound == LOW);
  bool somethingIsWrong = noiseDetected || tempHigh || humHigh;

  // if nothing was happening before and now an alarm is needed
  if (!alarmActive && somethingIsWrong) {
    tone(BUZZER_PIN, 1000);        // simple alarm tone
    alarmStartTime = millis();
    alarmActive    = true;
    scrollPosition = 0;

    // build scrolling text depending on what caused the alarm
    alertMessage = "Alert! ";
    if (noiseDetected) alertMessage += "Noise! ";
    if (tempHigh)      alertMessage += "Temp! ";
    if (humHigh)       alertMessage += "Humidity! ";
    alertMessage += "   ";         // spacing buffer
  }

  // if alarm is active, keep scrolling the alert text
  if (alarmActive) {
    // during alarm, top row is used by scrolling text, so we do not show bed message here
    scrollAlertText();
  }

  // stop alarm after 1 second (short beep)
  if (alarmActive && millis() - alarmStartTime >= 1000) {
    stopAlarm();
  }
}

// stop the alarm sound and update LCD
void stopAlarm() {
  noTone(BUZZER_PIN);        // stop buzzer
  alarmActive = false;

  lcd.setCursor(0, 0);
  lcd.print("READY          ");
  delay(400);

  // clear top row and, if bed message exists, show it again
  lcd.setCursor(0, 0);
  if (bedMessageAvailable) {
    lcd.print("Bed: ");
    String show = bedMessage.substring(0, 16 - 5);
    lcd.print(show);
    int pad = 16 - 5 - show.length();
    for (int i = 0; i < pad; i++) lcd.print(" ");
  } else {
    lcd.print("                ");
  }
}

// disable all alarms for 5 minutes (here set to 1 minute in code)
void disableAlarmForFiveMinutes() {
  alarmSystemEnabled = false;   // block new alarms
  alarmActive        = false;
  noTone(BUZZER_PIN);

  // set a timer to re-enable alarms later
  // 60000UL = 1 minute; change to 300000UL for real 5 minutes if needed
  alarmResumeTime = millis() + 60000UL;

  lcd.setCursor(0, 0);
  lcd.print("ALARM OFF 5min ");
  delay(400);
}

// scroll the alert message on the top row
void scrollAlertText() {
  // only update text every ~0.18s to make it readable
  if (millis() - lastScrollTime >= 180) {
    lastScrollTime = millis();

    lcd.setCursor(0, 0);
    lcd.print(alertMessage.substring(scrollPosition, scrollPosition + 16));

    scrollPosition++;
    if (scrollPosition > alertMessage.length() - 16) {
      scrollPosition = 0;   // loop text
    }
  }
}

// update temperature & humidity display (bottom row)
void updateTemperatureDisplay() {
  // keep sensor reads spaced out to avoid slowdowns
  if (millis() - lastDHTUpdate >= 1500) {
    lastDHTUpdate = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      // DHT sensors sometimes fail a reading; show a friendly message
      lcd.setCursor(0, 1);
      lcd.print("Sensor Error   ");
      return;
    }

    // show temp + humidity on bottom row
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

// -------------------- Setup and main loop --------------------

void setup() {
  // serial for debugging
  Serial.begin(9600);
  while (!Serial);

  // lcd setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(1200);
  lcd.clear();

  // pins setup
  pinMode(SOUND_PIN,  INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // button not pressed = HIGH

  // DHT init
  dht.begin();

  // connect WiFi and MQTT
  connectWiFi();
  connectMqtt();
}

void loop() {
  // keep WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // keep MQTT connection
  if (!mqttClient.connected()) {
    connectMqtt();
  }

  // handle mqtt incoming messages
  handleMqttMessages();

  // if alarms were disabled earlier, re-enable them when time runs out
  if (!alarmSystemEnabled && millis() >= alarmResumeTime) {
    alarmSystemEnabled = true;

    lcd.setCursor(0, 0);
    lcd.print("ALARM ACTIVE   ");
    delay(800);

    lcd.setCursor(0, 0);
    lcd.print("                ");
  }

  // check if the cancel button is pressed (LOW = pressed)
  if (digitalRead(BUTTON_PIN) == LOW) {
    disableAlarmForFiveMinutes();
  }

  // temperature & humidity should always update, even when alarms are off
  updateTemperatureDisplay();

  // if the user disabled alarms, skip checking noise/temp/humidity
  if (!alarmSystemEnabled) {
    noTone(BUZZER_PIN);   // make sure buzzer is off
    alarmActive = false;
    return;
  }

  // normal alarm checking happens only if system is enabled
  checkAlarmConditions();
}
