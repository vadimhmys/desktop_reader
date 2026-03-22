#include <SoftwareSerial.h>
#include <Keyboard.h>

SoftwareSerial rfidLF(10, 9); // RX на пине 10

// Светодиоды
const int greenLed = 8;   // Зеленый - готовность
const int yellowLed = 7;  // Желтый - 125 кГц карта
const int builtinLed = LED_BUILTIN;
const int buzzer = 6;

String lastTagId = "";
unsigned long lastReadTime = 0;
const unsigned long debounceTime = 2000;

void setup() {
  pinMode(greenLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(builtinLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  // Тест светодиодов
  digitalWrite(greenLed, HIGH);
  digitalWrite(yellowLed, HIGH);
  digitalWrite(builtinLed, HIGH);
  tone(buzzer, 1000, 300);
  delay(300);
  
  digitalWrite(greenLed, LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(builtinLed, LOW);
  delay(300);
  
  rfidLF.begin(9600);
  Keyboard.begin();
  
  // Обратный отсчет
  for (int i = 0; i < 5; i++) {
    digitalWrite(greenLed, HIGH);
    delay(250);
    digitalWrite(greenLed, LOW);
    delay(250);
  }
  
  digitalWrite(greenLed, HIGH); // Готовность
  tone(buzzer, 1500, 300);
}

void loop() {
  if (rfidLF.available() >= 14) {
    if (rfidLF.read() == 2) {
      byte tagData[13];
      for (int i = 0; i < 13; i++) {
        tagData[i] = rfidLF.read();
      }
      if (tagData[12] == 3) {
        String tagId = "";
        for (int i = 0; i < 10; i++) {
          tagId += (char)tagData[i];
        }
        
        if (tagId != lastTagId || (millis() - lastReadTime) > debounceTime) {
          
          // Сигнал
          digitalWrite(greenLed, LOW);
          digitalWrite(yellowLed, HIGH); // Зажигаем желтый - старая карта
          
          for (int i = 0; i < 2; i++) {
            digitalWrite(builtinLed, HIGH);
            tone(buzzer, 2000, 100);
            delay(120);
            digitalWrite(builtinLed, LOW);
            delay(120);
          }
          
          Keyboard.print("[125kHz] ");
          Keyboard.print(tagId);
          Keyboard.write(KEY_RETURN);
          
          delay(800);
          
          digitalWrite(yellowLed, LOW);
          digitalWrite(greenLed, HIGH);
          
          lastTagId = tagId;
          lastReadTime = millis();
        }
      }
    }
  }
}