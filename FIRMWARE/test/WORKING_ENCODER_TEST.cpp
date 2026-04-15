#include <Arduino.h>
#include <SPI.h>

#define PIN_CS   PA15
#define PIN_MISO PB4
#define PIN_MOSI PB5
#define PIN_SCLK PB3

// 500kHz is very stable for these encoders
SPISettings settingsA(500000, MSBFIRST, SPI_MODE0);

void setup() {
Serial.begin(115200);
while (!Serial); 

pinMode(PIN_CS, OUTPUT);
digitalWrite(PIN_CS, HIGH);

SPI.setMISO(PIN_MISO);
SPI.setMOSI(PIN_MOSI);
SPI.setSCLK(PIN_SCLK);
SPI.begin();

Serial.println("AMT252B Robust Test Initializing...");
delay(500);
}

bool verifyParity(uint16_t message) {
bool p1 = !!(message & 0x8000); 
bool p0 = !!(message & 0x4000); 

bool oddXOR = !!(message & 0x0002) ^ !!(message & 0x0008) ^ !!(message & 0x0020) ^ 
                !!(message & 0x0080) ^ !!(message & 0x0200) ^ !!(message & 0x0800) ^ !!(message & 0x2000);

bool evenXOR = !!(message & 0x0001) ^ !!(message & 0x0004) ^ !!(message & 0x0010) ^ 
                !!(message & 0x0040) ^ !!(message & 0x0100) ^ !!(message & 0x0400) ^ !!(message & 0x1000);

return (p1 != oddXOR) && (p0 != evenXOR);
}

void loop() {
uint8_t msb, lsb;

SPI.beginTransaction(settingsA);
digitalWrite(PIN_CS, LOW);
delayMicroseconds(10); // Lead time

// SPI is full-duplex: response comes back DURING the same transfers.
// Send 0x00, 0x00 — receive HIGH byte then LOW byte simultaneously.
msb = SPI.transfer(0x00);   // byte 1: K1, K0, D13..D8 come back here
delayMicroseconds(3);       // TB: >= 2.5us between bytes
lsb = SPI.transfer(0x00);  // byte 2: D7..D0 come back here

digitalWrite(PIN_CS, HIGH);
SPI.endTransaction();

uint16_t fullWord = ((uint16_t)msb << 8) | lsb;

Serial.print("MSB: 0x"); Serial.print(msb, HEX);
Serial.print(" LSB: 0x"); Serial.print(lsb, HEX);

if (verifyParity(fullWord)) {
    // The AMT252B (12-bit) sends data in bits 13 through 2.
    // Bits 15-14 are parity. Bits 1-0 are always 0.
    // To get a 0-4095 value, we mask parity and shift right by 2.
    uint16_t position12bit = (fullWord & 0x3FFF) >> 2;
    float degrees = (position12bit * 360.0) / 4096.0;

    Serial.print(" | Valid! Angle: ");
    Serial.print(degrees, 2);
    Serial.println("°");
} else {
    Serial.println(" | [PARITY ERROR]");
}

delay(200);
}