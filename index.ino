#include <SoftwareSerial.h>
#include <Keyboard.h>

SoftwareSerial rfidLF(10, 9); // RX на пине 10

// Светодиоды
const int redLed = 8;     // Красный - готовность
const int yellowLed = 7;  // Желтый - 125 кГц карта
const int builtinLed = LED_BUILTIN;
const int buzzer = 6;

String lastTagId = "";
unsigned long lastReadTime = 0;
const unsigned long debounceTime = 2000;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(builtinLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  // Тест светодиодов
  digitalWrite(redLed, HIGH);
  digitalWrite(yellowLed, HIGH);
  digitalWrite(builtinLed, HIGH);
  tone(buzzer, 1000, 300);
  delay(300);
  
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(builtinLed, LOW);
  delay(300);
  
  rfidLF.begin(9600);
  Keyboard.begin();
  
  // Обратный отсчет
  for (int i = 0; i < 5; i++) {
    digitalWrite(redLed, HIGH);
    delay(250);
    digitalWrite(redLed, LOW);
    delay(250);
  }
  
  digitalWrite(redLed, HIGH); // Готовность
  tone(buzzer, 1500, 300);
}

void loop() {
  // Проверяем наличие данных
  if (rfidLF.available() > 0) {
    
    // Ищем стартовый байт (0x02 = 2)
    if (rfidLF.peek() == 2) {
      
      // Ждем накопления полного пакета (14 байт)
      delay(20);
      
      if (rfidLF.available() >= 14) {
        
        // Читаем стартовый байт (2)
        rfidLF.read();
        
        // Читаем 10 байт ID
        String tagId = "";
        for (int i = 0; i < 10; i++) {
          if (rfidLF.available()) {
            char c = rfidLF.read();
            // Преобразуем в верхний регистр
            if (c >= 'a' && c <= 'f') {
              c = c - 32; // a->A, b->B и т.д.
            }
            tagId += c;
          }
        }
        
        // Пропускаем 2 байта контрольной суммы
        rfidLF.read();
        rfidLF.read();
        
        // Читаем стоп-байт (должен быть 3)
        byte stopByte = rfidLF.read();
        
        // Проверяем корректность пакета
        if (stopByte == 3 && tagId.length() == 10) {
          
          // Защита от повторного чтения
          if (tagId != lastTagId || (millis() - lastReadTime) > debounceTime) {
            
            // Сигнал
            digitalWrite(redLed, LOW);
            digitalWrite(yellowLed, HIGH);
            
            for (int i = 0; i < 2; i++) {
              digitalWrite(builtinLed, HIGH);
              tone(buzzer, 2000, 100);
              delay(120);
              digitalWrite(builtinLed, LOW);
              delay(120);
            }
            
            // Отправляем ТОЛЬКО ID (без префикса)
            Keyboard.print(tagId);
            Keyboard.write(KEY_RETURN);
            
            delay(800);
            
            digitalWrite(yellowLed, LOW);
            digitalWrite(redLed, HIGH);
            
            lastTagId = tagId;
            lastReadTime = millis();
          }
        } else {
          // Если пакет поврежден, очищаем буфер
          while (rfidLF.available() > 0) {
            rfidLF.read();
          }
        }
      }
    } else {
      // Если первый байт не стартовый - пропускаем мусор
      rfidLF.read();
    }
  }
}