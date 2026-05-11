// DFPlayer Mini test for XIAO ESP32-S3
//
// Wiring:
//   XIAO 3.3V  ──────────────── DFPlayer VCC
//   XIAO GND   ──────────────── DFPlayer GND
//   XIAO D6 (GPIO43, TX) ─[1kΩ]─ DFPlayer RX
//   XIAO D7 (GPIO44, RX) ──────── DFPlayer TX
//   DFPlayer SPK_1 ──────────── Speaker red  (+)
//   DFPlayer SPK_2 ──────────── Speaker black (-)
//
// SD card: FAT32, files named 0001.mp3, 0002.mp3, ... in root folder

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

#define DF_RX_PIN 44  // D7 — receives data from DFPlayer TX
#define DF_TX_PIN 43  // D6 — sends data to DFPlayer RX (via 1kΩ resistor)

HardwareSerial dfSerial(1);   // Use ESP32 UART1
DFRobotDFPlayerMini dfPlayer;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Initializing DFPlayer...");

  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);

  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer not found! Check:");
    Serial.println("  - Wiring (1kΩ resistor on RX line?)");
    Serial.println("  - SD card inserted and FAT32 formatted?");
    Serial.println("  - MP3 files named 0001.mp3 in root?");
    while (true) delay(100);
  }

  Serial.println("DFPlayer ready!");
  dfPlayer.volume(20);  // 0 (mute) to 30 (max)
  dfPlayer.play(1);     // Play 0001.mp3
  Serial.println("Playing track 1...");
}

void loop() {
  if (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    if (type == DFPlayerPlayFinished) {
      Serial.println("Track finished, replaying...");
      dfPlayer.play(1);
    } else if (type == DFPlayerError) {
      Serial.print("DFPlayer error: ");
      Serial.println(dfPlayer.read());
    }
  }
}
