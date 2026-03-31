#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>

// === RDM6300 (125 кГц) - используем аппаратный Serial1 ===
#define RDM6300 Serial1

// === RC522 (13.56 МГц) ===
#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Светодиоды
const int redLed = 8;
const int yellowLed = 7;
const int blueLed = 4;
const int builtinLed = LED_BUILTIN;
const int buzzer = 6;

String lastTagId = "";
unsigned long lastReadTime = 0;
const unsigned long debounceTime = 2000;

// Функция преобразования HEX строки в DEC число
unsigned long hexToDec(String hexString) {
  unsigned long decimalValue = 0;
  for (int i = 0; i < hexString.length(); i++) {
    char c = hexString.charAt(i);
    decimalValue = decimalValue * 16;
    if (c >= '0' && c <= '9') {
      decimalValue += (c - '0');
    } else if (c >= 'A' && c <= 'F') {
      decimalValue += (c - 'A' + 10);
    } else if (c >= 'a' && c <= 'f') {
      decimalValue += (c - 'a' + 10);
    }
  }
  return decimalValue;
}

// Функция отправки символа
void sendChar(char c) {
  if (c >= 'A' && c <= 'Z') {
    Keyboard.press(KEY_LEFT_SHIFT);
    Keyboard.write(c);
    Keyboard.release(KEY_LEFT_SHIFT);
  } 
  else if (c >= '0' && c <= '9') {
    Keyboard.write(c);
  }
  delay(5);
}

void sendString(String str) {
  for (int i = 0; i < str.length(); i++) {
    sendChar(str[i]);
  }
}

// Функция обработки RDM6300 (125 кГц)
bool processRDM6300() {
  if (RDM6300.available() > 0) {
    if (RDM6300.peek() == 2) {
      delay(20);
      
      if (RDM6300.available() >= 14) {
        RDM6300.read();
        
        String hexId = "";
        for (int i = 0; i < 10; i++) {
          if (RDM6300.available()) {
            char c = RDM6300.read();
            if (c >= 'a' && c <= 'f') {
              c = c - 32;
            }
            hexId += c;
          }
        }
        
        RDM6300.read();
        RDM6300.read();
        byte stopByte = RDM6300.read();
        
        if (stopByte == 3 && hexId.length() == 10) {
          if (hexId != lastTagId || (millis() - lastReadTime) > debounceTime) {
            
            // Преобразуем HEX в DEC
            unsigned long decId = hexToDec(hexId);
            
            // Сигнал для 125 кГц
            digitalWrite(redLed, LOW);
            digitalWrite(yellowLed, HIGH);
            
            for (int i = 0; i < 2; i++) {
              digitalWrite(builtinLed, HIGH);
              tone(buzzer, 2000, 100);
              delay(120);
              digitalWrite(builtinLed, LOW);
              delay(120);
            }
            
            // Отправляем десятичный ID
            sendString(String(decId));
            Keyboard.write(KEY_RETURN);
            
            delay(800);
            
            digitalWrite(yellowLed, LOW);
            digitalWrite(redLed, HIGH);
            
            lastTagId = hexId;
            lastReadTime = millis();
            
            return true;
          }
        } else {
          while (RDM6300.available() > 0) {
            RDM6300.read();
          }
        }
      }
    } else {
      RDM6300.read();
    }
  }
  return false;
}

// Функция обработки RC522 (13.56 МГц)
bool processRC522() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      
      String hexId = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
          hexId += "0";
        }
        hexId += String(mfrc522.uid.uidByte[i], HEX);
      }
      hexId.toUpperCase();
      
      if (hexId != lastTagId || (millis() - lastReadTime) > debounceTime) {
        
        // Преобразуем HEX в DEC
        unsigned long decId = hexToDec(hexId);
        
        digitalWrite(redLed, LOW);
        digitalWrite(blueLed, HIGH);
        digitalWrite(builtinLed, HIGH);
        tone(buzzer, 2000, 200);
        delay(200);
        digitalWrite(builtinLed, LOW);
        delay(400);
        
        // Отправляем десятичный ID
        sendString(String(decId));
        Keyboard.write(KEY_RETURN);
        
        delay(800);
        
        digitalWrite(blueLed, LOW);
        digitalWrite(redLed, HIGH);
        
        lastTagId = hexId;
        lastReadTime = millis();
        
        return true;
      }
      
      mfrc522.PICC_HaltA();
    }
  }
  return false;
}

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(builtinLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  // Тест светодиодов
  digitalWrite(redLed, HIGH);
  digitalWrite(yellowLed, HIGH);
  digitalWrite(blueLed, HIGH);
  digitalWrite(builtinLed, HIGH);
  tone(buzzer, 1000, 300);
  delay(300);
  
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(blueLed, LOW);
  digitalWrite(builtinLed, LOW);
  delay(300);
  
  // Инициализация модулей
  RDM6300.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Keyboard.begin();
  
  // Обратный отсчет
  for (int i = 0; i < 5; i++) {
    digitalWrite(redLed, HIGH);
    delay(250);
    digitalWrite(redLed, LOW);
    delay(250);
  }
  
  digitalWrite(redLed, HIGH);
  tone(buzzer, 1500, 300);
}

void loop() {
  bool cardRead = processRDM6300();
  
  if (!cardRead) {
    processRC522();
  }
}