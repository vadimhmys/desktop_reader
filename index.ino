#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>

// === RDM6300 (125 кГц) - используем аппаратный Serial1 ===
// TX модуля подключен к пину 0 (RX1) на Pro Micro
#define RDM6300 Serial1

// === RC522 (13.56 МГц) ===
#define RST_PIN 9     // RST на пине 9
#define SS_PIN 10     // SDA (SS) на пине 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Светодиоды
const int redLed = 8;       // Красный - готовность
const int yellowLed = 7;    // Желтый - 125 кГц карта
const int blueLed = 4;      // Синий - 13.56 МГц карта
const int builtinLed = LED_BUILTIN;
const int buzzer = 6;

String lastTagId = "";
unsigned long lastReadTime = 0;
const unsigned long debounceTime = 2000;

// Функция отправки символа с гарантией английской буквы
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
    // Ищем стартовый байт (0x02 = 2)
    if (RDM6300.peek() == 2) {
      
      // Ждем накопления полного пакета (14 байт)
      delay(20);
      
      if (RDM6300.available() >= 14) {
        
        // Читаем стартовый байт (2)
        RDM6300.read();
        
        // Читаем 10 байт ID
        String tagId = "";
        for (int i = 0; i < 10; i++) {
          if (RDM6300.available()) {
            char c = RDM6300.read();
            if (c >= 'a' && c <= 'f') {
              c = c - 32;
            }
            tagId += c;
          }
        }
        
        // Пропускаем 2 байта контрольной суммы
        RDM6300.read();
        RDM6300.read();
        
        // Читаем стоп-байт (должен быть 3)
        byte stopByte = RDM6300.read();
        
        if (stopByte == 3 && tagId.length() == 10) {
          if (tagId != lastTagId || (millis() - lastReadTime) > debounceTime) {
            
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
            
            sendString(tagId);
            Keyboard.write(KEY_RETURN);
            
            delay(800);
            
            digitalWrite(yellowLed, LOW);
            digitalWrite(redLed, HIGH);
            
            lastTagId = tagId;
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
      
      String tagId = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
          tagId += "0";
        }
        tagId += String(mfrc522.uid.uidByte[i], HEX);
      }
      tagId.toUpperCase();
      
      if (tagId != lastTagId || (millis() - lastReadTime) > debounceTime) {
        
        digitalWrite(redLed, LOW);
        digitalWrite(blueLed, HIGH);
        digitalWrite(builtinLed, HIGH);
        tone(buzzer, 2000, 200);
        delay(200);
        digitalWrite(builtinLed, LOW);
        delay(400);
        
        sendString(tagId);
        Keyboard.write(KEY_RETURN);
        
        delay(800);
        
        digitalWrite(blueLed, LOW);
        digitalWrite(redLed, HIGH);
        
        lastTagId = tagId;
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
  RDM6300.begin(9600);     // Аппаратный Serial1 для RDM6300
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
  // Приоритет для RDM6300
  bool cardRead = processRDM6300();
  
  if (!cardRead) {
    processRC522();
  }
}