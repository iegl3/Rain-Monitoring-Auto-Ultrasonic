#include <LiquidCrystal.h>
#include <Servo.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

int buttonPin = 32;
int rainPin = A0;
int greenLED = 8;
int redLED = 7;
int relayPin = 3;
int servoPin = 6;
int buzzerPin = 10;
int lm35Pin = A1; // Pin untuk sensor suhu LM35
int trigPin = 11; // Pin untuk sensor ultrasonik HC-SR04 (Trig)
int echoPin = 12; // Pin untuk sensor ultrasonik HC-SR04 (Echo)
int motorPin = 13; // Pin untuk motor DC

int thresholdValue = 500;
int waterThreshold = 10; // Jarak ketinggian air dalam cm

Servo myservo;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 52
#define DATA_PIN 51
#define CS_PIN 53

MD_Parola matrix = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Inisialisasi LCD (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(2, A2, A3, A4, A5, A6);

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(rainPin, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(relayPin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(motorPin, LOW);
  myservo.attach(servoPin);
  myservo.write(0);
  Serial.begin(9600);
  Serial1.begin(9600);

  // Inisialisasi LCD
  lcd.begin(16, 2);

  matrix.begin();
  matrix.setIntensity(1);
  matrix.displayClear();
}

void displayMessage(const char *message) {
  matrix.displayClear();
  matrix.print(message);
  matrix.displayReset();
}

long readUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.0344 / 2;
  return distance;
}

bool manualMode = false;

void loop() {
  // Mengecek apakah push button ditekan
  if (digitalRead(buttonPin) == LOW) {
    delay(200); // Debounce
    if (digitalRead(buttonPin) == LOW) {
      // Menggantikan mode manual
      manualMode = !manualMode;

      Serial.print("Mode manual: ");
      Serial.println(manualMode ? "ON" : "OFF");

      if (manualMode) {
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        digitalWrite(relayPin, HIGH);
        myservo.write(180);
      } else {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        digitalWrite(relayPin, LOW);
        myservo.write(0);
      }
    }
    // Menunggu tombol dilepas sebelum melanjutkan
    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }
  }

  // Tambahkan blok kode ini untuk kontrol Bluetooth
  if (Serial1.available()) {
    char command = Serial1.read();
    if (command == 'M' || command == 'm') {
      manualMode = !manualMode;

      if (manualMode) {
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        digitalWrite(relayPin, HIGH);
        myservo.write(180);
      } else {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        digitalWrite(relayPin, LOW);
        myservo.write(0);
      }
    }
  }

  if (!manualMode) {
    static bool wetState = false;
    int sensorValue = analogRead(rainPin);
    if (sensorValue < thresholdValue && !wetState) {
      wetState = true;
           digitalWrite(greenLED, LOW);
      digitalWrite(redLED, HIGH);
      digitalWrite(relayPin, HIGH);
      myservo.write(180);

      displayMessage("Hujan!");

      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
    } else if (sensorValue >= thresholdValue) {
      wetState = false;
      digitalWrite(greenLED, HIGH);
      digitalWrite(redLED, LOW);
      digitalWrite(relayPin, LOW);
      myservo.write(0);

      displayMessage("Cerah");
    }
  }

  // Baca suhu dari LM35
  int rawValue = analogRead(lm35Pin);
  float voltage = (rawValue / 1024.0) * 5.0;
  float temperature = voltage * 100;

  // Baca jarak dari sensor ultrasonik HC-SR04
  long distance = readUltrasonicDistance(trigPin, echoPin);

  // Aktifkan atau nonaktifkan motor DC berdasarkan jarak ketinggian air
  if (distance <= waterThreshold) {
    digitalWrite(motorPin, HIGH);
  } else {
    digitalWrite(motorPin, LOW);
  }

  // Tampilkan suhu dan jarak pada LCD
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.print(temperature);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Jarak: ");
  lcd.print(distance);
  lcd.print(" cm");

  delay(1000);
}
