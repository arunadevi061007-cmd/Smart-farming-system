#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// ================= PINS =================

#define MQ_PIN      A0
#define SOIL_PIN    A1
#define RAIN_PIN    A2
#define LDR_PIN     A3

#define DHT_PIN     4
#define IR_PIN      5

#define TRIG_PIN    6
#define ECHO_PIN    7

#define SERVO_PIN   8
#define RELAY_PIN   9

#define BT_RX       10
#define BT_TX       11

#define BUZZER_PIN  13

#define DHTTYPE DHT11

// ================= OBJECTS =================

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHTTYPE);
Servo servoMotor;

SoftwareSerial BT(BT_RX, BT_TX);

// ================= SETTINGS =================

#define SOIL_DRY 600
#define RAIN_LIMIT 500
#define GAS_LIMIT 600
#define LDR_DAY 400

// Most IR modules:
// LOW = detected
// HIGH = not detected
#define IR_DETECTED LOW

// ================= VARIABLES =================

int soilValue = 0;
int rainValue = 0;
int gasValue = 0;
int ldrValue = 0;
int irValue = HIGH;

long distanceCM = -1;

float temperature = 0;
float humidity = 0;

bool autoMode = true;
bool pumpON = false;
bool dayTime = false;

// ================= TIMERS =================

unsigned long sensorTime = 0;
unsigned long lcdTime = 0;
unsigned long mobileTime = 0;

#define SENSOR_INTERVAL 500
#define LCD_INTERVAL 2000
#define MOBILE_INTERVAL 10000

// ================= BLUETOOTH BUFFER =================

char command[16];
byte commandPos = 0;

// ================= SETUP =================

void setup() {

  Serial.begin(9600);

  // HC-05
  BT.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();

  // DHT
  dht.begin();

  // Servo
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(0);

  // IR
  pinMode(IR_PIN, INPUT);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // Relay
  pinMode(RELAY_PIN, OUTPUT);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Pump OFF
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  // ================= LCD START =================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(F("SMART FARMING"));

  lcd.setCursor(0, 1);
  lcd.print(F("BT STARTING..."));

  delay(1000);

  // ================= BLUETOOTH START =================

  BT.println();
  BT.println(F("SMART FARMING"));
  BT.println(F("BLUETOOTH CONNECTED"));
  BT.println(F("SYSTEM READY"));
  BT.println(F("----------------"));
  BT.println(F("AUTO/MANUAL"));
  BT.println(F("ON/OFF"));
  BT.println(F("STATUS"));
  BT.println();

  // First sensor reading

  readSensors();

  controlPump();

  // Send immediately

  sendMobile();

  mobileTime = millis();
}

// ================= MAIN LOOP =================

void loop() {

  // -------- Sensors --------

  if (millis() - sensorTime >= SENSOR_INTERVAL) {

    sensorTime = millis();

    readSensors();

    controlPump();
  }

  // -------- LCD --------

  if (millis() - lcdTime >= LCD_INTERVAL) {

    lcdTime = millis();

    showLCD();
  }

  // -------- Bluetooth commands --------

  readBluetooth();

  // -------- Mobile dashboard --------

  if (millis() - mobileTime >= MOBILE_INTERVAL) {

    mobileTime = millis();

    sendMobile();
  }
}

// ================= READ SENSORS =================

void readSensors() {

  soilValue = analogRead(SOIL_PIN);

  rainValue = analogRead(RAIN_PIN);

  gasValue = analogRead(MQ_PIN);

  ldrValue = analogRead(LDR_PIN);

  irValue = digitalRead(IR_PIN);

  distanceCM = ultrasonicDistance();

  float t = dht.readTemperature();

  float h = dht.readHumidity();

  if (!isnan(t)) {
    temperature = t;
  }

  if (!isnan(h)) {
    humidity = h;
  }

  if (ldrValue >= LDR_DAY) {
    dayTime = true;
  } else {
    dayTime = false;
  }
}

// ================= ULTRASONIC =================

long ultrasonicDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  unsigned long time =
    pulseIn(ECHO_PIN, HIGH, 30000);

  if (time == 0) {
    return -1;
  }

  long cm = time / 58;

  if (cm >= 2 && cm <= 400) {
    return cm;
  }

  return -1;
}

// ================= PUMP CONTROL =================

void controlPump() {

  bool rain = rainValue < RAIN_LIMIT;

  // Rain always OFF

  if (rain) {

    pumpON = false;

    digitalWrite(RELAY_PIN, HIGH);

    return;
  }

  // AUTO

  if (autoMode) {

    if (soilValue > SOIL_DRY) {

      pumpON = true;

      digitalWrite(RELAY_PIN, LOW);

    } else {

      pumpON = false;

      digitalWrite(RELAY_PIN, HIGH);
    }

    return;
  }

  // MANUAL

  if (pumpON) {

    digitalWrite(RELAY_PIN, LOW);

  } else {

    digitalWrite(RELAY_PIN, HIGH);
  }
}

// ================= BLUETOOTH INPUT =================

void readBluetooth() {

  while (BT.available()) {

    char c = BT.read();

    if (c == '\n' || c == '\r') {

      if (commandPos > 0) {

        command[commandPos] = '\0';

        processCommand();

        commandPos = 0;
      }

    } else {

      if (commandPos < 15) {

        command[commandPos] = c;

        commandPos++;
      }
    }
  }
}

// ================= PROCESS COMMAND =================

void processCommand() {

  // lowercase → uppercase

  for (byte i = 0; command[i] != '\0'; i++) {

    if (command[i] >= 'a' &&
        command[i] <= 'z') {

      command[i] -= 32;
    }
  }

  // AUTO

  if (strcmp(command, "AUTO") == 0) {

    autoMode = true;

    controlPump();

    BT.println(F("MODE=AUTO"));
    sendStatus();
  }

  // MANUAL

  else if (strcmp(command, "MANUAL") == 0) {

    autoMode = false;

    pumpON = false;

    digitalWrite(RELAY_PIN, HIGH);

    BT.println(F("MODE=MANUAL"));
    sendStatus();
  }

  // ON

  else if (strcmp(command, "ON") == 0) {

    if (autoMode) {

      BT.println(F("USE MANUAL FIRST"));

    } else {

      if (rainValue < RAIN_LIMIT) {

        pumpON = false;

        digitalWrite(RELAY_PIN, HIGH);

        BT.println(F("RAIN=PUMP OFF"));

      } else {

        pumpON = true;

        digitalWrite(RELAY_PIN, LOW);

        BT.println(F("PUMP=ON"));
      }
    }
  }

  // OFF

  else if (strcmp(command, "OFF") == 0) {

    pumpON = false;

    digitalWrite(RELAY_PIN, HIGH);

    BT.println(F("PUMP=OFF"));
  }

  // STATUS

  else if (strcmp(command, "STATUS") == 0) {

    sendMobile();
  }
}

// ================= STATUS =================

void sendStatus() {

  BT.print(F("MODE="));

  if (autoMode)
    BT.println(F("AUTO"));
  else
    BT.println(F("MANUAL"));

  BT.print(F("PUMP="));

  if (pumpON)
    BT.println(F("ON"));
  else
    BT.println(F("OFF"));

  BT.print(F("SOIL="));
  BT.println(soilValue);

  BT.print(F("RAIN="));
  BT.println(rainValue);

  BT.print(F("IR="));
  BT.println(irValue);

  BT.print(F("DIST="));

  if (distanceCM > 0) {
    BT.print(distanceCM);
    BT.println(F("cm"));
  } else {
    BT.println(F("NONE"));
  }
}

// ================= MOBILE DASHBOARD =================

void sendMobile() {

  BT.println();
  BT.println(F("===================="));
  BT.println(F(" SMART FARMING"));
  BT.println(F(" LIVE DASHBOARD"));
  BT.println(F("===================="));

  // PUMP

  BT.print(F("MODE : "));

  if (autoMode)
    BT.println(F("AUTO"));
  else
    BT.println(F("MANUAL"));

  BT.print(F("PUMP : "));

  if (pumpON)
    BT.println(F("ON"));
  else
    BT.println(F("OFF"));

  // SOIL

  BT.print(F("SOIL : "));
  BT.println(soilValue);

  BT.print(F("SOIL STATUS : "));

  if (soilValue > SOIL_DRY)
    BT.println(F("DRY"));
  else
    BT.println(F("WET"));

  // RAIN

  BT.print(F("RAIN : "));
  BT.println(rainValue);

  BT.print(F("RAIN STATUS : "));

  if (rainValue < RAIN_LIMIT)
    BT.println(F("RAIN"));
  else
    BT.println(F("NO RAIN"));

  // IR

  BT.print(F("IR RAW : "));
  BT.println(irValue);

  BT.print(F("HUMAN : "));

  if (irValue == IR_DETECTED)
    BT.println(F("DETECTED"));
  else
    BT.println(F("NOT DETECTED"));

  // ULTRASONIC

  BT.print(F("DISTANCE : "));

  if (distanceCM > 0) {

    BT.print(distanceCM);
    BT.println(F(" CM"));

  } else {

    BT.println(F("NO OBJECT"));
  }

  // GAS

  BT.print(F("GAS : "));
  BT.println(gasValue);

  BT.print(F("GAS STATUS : "));

  if (gasValue > GAS_LIMIT)
    BT.println(F("ALERT"));
  else
    BT.println(F("NORMAL"));

  // DHT

  BT.print(F("TEMP : "));
  BT.print(temperature, 1);
  BT.println(F(" C"));

  BT.print(F("HUMIDITY : "));
  BT.print(humidity, 1);
  BT.println(F(" %"));

  // LDR

  BT.print(F("LDR : "));
  BT.println(ldrValue);

  BT.print(F("LIGHT : "));

  if (dayTime)
    BT.println(F("DAY"));
  else
    BT.println(F("NIGHT"));

  // SYSTEM

  BT.println(F("LCD : ACTIVE"));
  BT.println(F("ULTRASONIC : ACTIVE"));
  BT.println(F("IR : ACTIVE"));
  BT.println(F("BLUETOOTH : OK"));

  BT.println(F("--------------------"));
  BT.println(F("UPDATE : 10 SEC"));
  BT.println(F("===================="));
  BT.println();
}

// ================= LCD =================

void showLCD() {

  byte page = (millis() / 2000) % 8;

  lcd.clear();

  // PAGE 0

  if (page == 0) {

    lcd.setCursor(0, 0);

    if (autoMode)
      lcd.print(F("MODE:AUTO"));
    else
      lcd.print(F("MODE:MANUAL"));

    lcd.setCursor(0, 1);

    lcd.print(F("PUMP:"));

    if (pumpON)
      lcd.print(F("ON"));
    else
      lcd.print(F("OFF"));
  }

  // PAGE 1

  else if (page == 1) {

    lcd.setCursor(0, 0);
    lcd.print(F("SOIL:"));
    lcd.print(soilValue);

    lcd.setCursor(0, 1);

    if (soilValue > SOIL_DRY)
      lcd.print(F("DRY"));
    else
      lcd.print(F("WET"));
  }

  // PAGE 2

  else if (page == 2) {

    lcd.setCursor(0, 0);
    lcd.print(F("RAIN:"));

    if (rainValue < RAIN_LIMIT)
      lcd.print(F("YES"));
    else
      lcd.print(F("NO"));

    lcd.setCursor(0, 1);
    lcd.print(F("PUMP:"));

    if (pumpON)
      lcd.print(F("ON"));
    else
      lcd.print(F("OFF"));
  }

  // PAGE 3

  else if (page == 3) {

    lcd.setCursor(0, 0);

    if (irValue == IR_DETECTED)
      lcd.print(F("HUMAN:DETECT"));
    else
      lcd.print(F("HUMAN:SAFE"));

    lcd.setCursor(0, 1);

    lcd.print(F("DIST:"));

    if (distanceCM > 0) {

      lcd.print(distanceCM);
      lcd.print(F("CM"));

    } else {

      lcd.print(F("NONE"));
    }
  }

  // PAGE 4

  else if (page == 4) {

    lcd.setCursor(0, 0);

    lcd.print(F("TEMP:"));
    lcd.print(temperature, 1);
    lcd.print(F("C"));

    lcd.setCursor(0, 1);

    lcd.print(F("HUM:"));
    lcd.print(humidity, 1);
    lcd.print(F("%"));
  }

  // PAGE 5

  else if (page == 5) {

    lcd.setCursor(0, 0);

    lcd.print(F("LDR:"));
    lcd.print(ldrValue);

    lcd.setCursor(0, 1);

    if (dayTime)
      lcd.print(F("DAY"));
    else
      lcd.print(F("NIGHT"));
  }

  // PAGE 6

  else if (page == 6) {

    lcd.setCursor(0, 0);

    lcd.print(F("GAS:"));
    lcd.print(gasValue);

    lcd.setCursor(0, 1);

    if (gasValue > GAS_LIMIT)
      lcd.print(F("GAS ALERT"));
    else
      lcd.print(F("GAS NORMAL"));
  }

  // PAGE 7

  else {

    lcd.setCursor(0, 0);
    lcd.print(F("DISTANCE:"));

    if (distanceCM > 0)
      lcd.print(distanceCM);
    else
      lcd.print(F("NONE"));

    lcd.setCursor(0, 1);

    lcd.print(F("IR:"));
    lcd.print(irValue);
  }
}
