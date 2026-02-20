#include <Arduino.h>
#include <U8g2lib.h>

// Exact pins from source 180 (EXP1 Header)
#define LCD_CLK    PE13   
#define LCD_DATA   PE15   
#define LCD_CS     PE14   
#define LCD_RESET  PE10   
#define REAL_BEEPER PE8   

// Using the ST7920 driver from the working project
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_CLK, LCD_DATA, LCD_CS, LCD_RESET);

void setup() {
  pinMode(REAL_BEEPER, OUTPUT);
  digitalWrite(REAL_BEEPER, LOW); // Silence chirping

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr); // Standard font from source 236
}

void loop() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "ST7920 - 10K TEST");
  u8g2.drawStr(0, 40, "CHECK EXP1 SLOT");
  u8g2.sendBuffer();
}