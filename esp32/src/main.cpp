/**
 * XFactor SoundBox — DFPlayer Mini + PIR Controller
 *
 * Target:  Seeed Studio XIAO ESP32-S3
 * Library: DFRobotDFPlayerMini (declared in platformio.ini lib_deps)
 *
 * DFPlayer wiring:
 *   XIAO GPIO 43 (D6 / TX) ──── 1 kΩ ──── DFPlayer RX
 *   XIAO GPIO 44 (D7 / RX) ────────────── DFPlayer TX
 *   XIAO GND               ────────────── DFPlayer GND
 *   DFPlayer VCC            ────────────── 3.3 V or 5 V (module accepts both)
 *
 * PIR sensor BS412 (4-pin radial, bottom view):
 *   Pin 1 (VSS)    ──── XIAO GND
 *   Pin 2 (ONTIME) ──── GND via 0 Ω  (on-time = 2 s min, datasheet §2 row 0)
 *   Pin 3 (VDD)    ──── XIAO 3.3 V
 *   Pin 4 (REL)    ──── XIAO GPIO 6 (D6)   ← HIGH when motion detected
 *   No pull-up needed — REL is a push-pull Schmitt trigger output.
 *
 * SD card layout expected on DFPlayer:
 *   /mp3/0001.mp3
 *        0002.mp3
 *        0003.mp3
 *
 * PIR detection note:
 *   BS412 output is polled every loop iteration (~1–3 ms).
 *   Rising-edge detection fires once per event.
 *   With ONTIME tied to GND, the output stays HIGH for ≥2 s,
 *   so the minimum re-trigger interval is ~2 s.
 *
 * Behaviour:
 *   Audio plays ONLY when the PIR detects motion. Each trigger
 *   advances to the next track (1 → 2 → 3 → 1 → …). If a track
 *   is already playing when motion is detected it restarts from
 *   the next track immediately.
 */

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// ── Pin Configuration ─────────────────────────────────────────────────────────

static constexpr int XIAO_UART_TX = 43;  // GPIO43 | TX  →  DFPlayer RX
static constexpr int XIAO_UART_RX = 44;  // GPIO44 | RX  ←  DFPlayer TX
static constexpr int PIR_PIN      = 6;   // GPIO6         ←  BS412 REL (HIGH = motion)

// ── Serial Configuration ──────────────────────────────────────────────────────

static constexpr long DEBUG_BAUD    = 115200;
static constexpr long DFPLAYER_BAUD = 9600;

// ── Audio Configuration ───────────────────────────────────────────────────────

static constexpr uint8_t  VOLUME_LEVEL    = 15;      // 0 – 30
static constexpr uint8_t  TOTAL_TRACKS   = 3;       // fallback if SD query fails
static constexpr uint32_t PLAY_DURATION_MS = 3000UL; // stop playback after this many ms

// ── Objects ───────────────────────────────────────────────────────────────────

HardwareSerial      dfPlayerSerial(1);  // UART1 — leaves USB Serial free for debug
DFRobotDFPlayerMini myDFPlayer;

// ── Module State ──────────────────────────────────────────────────────────────

static bool     dfPlayerReady = false;
static uint8_t  totalTracks   = TOTAL_TRACKS;  // overwritten in setup() from SD card

static bool     pirLastLevel  = false;  // previous REL level for rising-edge detection
static bool     isPlaying     = false;
static uint32_t playStartMs   = 0;

// ─────────────────────────────────────────────────────────────────────────────
// printDFPlayerDetail()
//   Decodes and logs the asynchronous status messages sent by the DFPlayer.
// ─────────────────────────────────────────────────────────────────────────────
static void printDFPlayerDetail(uint8_t type, int value) {
    switch (type) {
        case TimeOut:
            Serial.println(F("[DFPlayer] Timeout waiting for response."));
            break;
        case WrongStack:
            Serial.println(F("[DFPlayer] Stack error — check wiring/baud rate."));
            break;
        case DFPlayerCardInserted:   Serial.println(F("[DFPlayer] SD card inserted."));    break;
        case DFPlayerCardRemoved:    Serial.println(F("[DFPlayer] SD card removed."));     break;
        case DFPlayerCardOnline:     Serial.println(F("[DFPlayer] SD card online."));      break;
        case DFPlayerUSBInserted:    Serial.println(F("[DFPlayer] USB inserted."));        break;
        case DFPlayerUSBRemoved:     Serial.println(F("[DFPlayer] USB removed."));         break;
        case DFPlayerPlayFinished:
            Serial.print(F("[DFPlayer] Finished — Folder: "));
            Serial.print(value >> 8);
            Serial.print(F(", Track: "));
            Serial.println(value & 0xFF);
            break;
        case DFPlayerError:
            Serial.print(F("[DFPlayer] Runtime error: "));
            switch (value) {
                case Busy:             Serial.println(F("Busy / no SD card."));          break;
                case Sleeping:         Serial.println(F("Module sleeping."));             break;
                case SerialWrongStack: Serial.println(F("Serial framing error."));        break;
                case CheckSumNotMatch: Serial.println(F("Checksum mismatch."));           break;
                case FileIndexOut:     Serial.println(F("File index out of bounds."));    break;
                case FileMismatch:     Serial.println(F("File not found on SD card."));   break;
                case Advertise:        Serial.println(F("Advertise mode active."));       break;
                default:               Serial.println(F("Unknown error code."));          break;
            }
            break;
        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// initDFPlayer()
// ─────────────────────────────────────────────────────────────────────────────
static bool initDFPlayer() {
    Serial.println(F("[Init] Starting DFPlayer Mini..."));

    if (!myDFPlayer.begin(dfPlayerSerial, /*isACK=*/true, /*doReset=*/true)) {
        Serial.println(F(""));
        Serial.println(F("══════════════════════════════════════════════════"));
        Serial.println(F("  [FATAL] DFPlayer Mini did not respond!"));
        Serial.println(F("══════════════════════════════════════════════════"));
        Serial.println(F("  Troubleshooting checklist:"));
        Serial.println(F("  1. Wiring: XIAO GPIO43 ──1kΩ──> DFPlayer RX"));
        Serial.println(F("             XIAO GPIO44 <──────── DFPlayer TX"));
        Serial.println(F("  2. Power:  DFPlayer VCC connected to 3.3 V/5 V."));
        Serial.println(F("  3. Ground: shared GND between XIAO and DFPlayer."));
        Serial.println(F("  4. SD card: FAT32 formatted, properly seated."));
        Serial.println(F("  5. SD layout: /mp3/0001.mp3 must exist."));
        Serial.println(F("  6. Resistor: 1 kΩ on DFPlayer RX line (not TX)."));
        Serial.println(F("  7. Swap TX/RX if unsure — a common first mistake."));
        Serial.println(F("══════════════════════════════════════════════════"));
        return false;
    }

    Serial.println(F("[Init] DFPlayer Mini responded OK."));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(DEBUG_BAUD);
    const uint32_t bootWaitMs = millis() + 3000UL;
    while (!Serial && millis() < bootWaitMs) { /* spin */ }

    Serial.println(F(""));
    Serial.println(F("╔══════════════════════════════════╗"));
    Serial.println(F("║   XFactor SoundBox — Booting…   ║"));
    Serial.println(F("╚══════════════════════════════════╝"));

    // PIR input — push-pull output, no internal pull-up needed
    pinMode(PIR_PIN, INPUT);
    pirLastLevel = digitalRead(PIR_PIN);

    // UART1: begin(baud, config, rxPin, txPin)
    dfPlayerSerial.begin(DFPLAYER_BAUD, SERIAL_8N1, XIAO_UART_RX, XIAO_UART_TX);

    // DFPlayer needs ~1 s after power-on to enumerate the SD card.
    delay(1000);

    if (!initDFPlayer()) {
        while (true) { delay(1000); }  // halt — no point continuing without audio
    }

    myDFPlayer.setTimeOut(500);
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
    myDFPlayer.volume(VOLUME_LEVEL);

    Serial.print(F("[Audio] Volume: "));
    Serial.print(VOLUME_LEVEL);
    Serial.println(F(" / 30  |  EQ: Normal"));

    int count = myDFPlayer.readFileCountsInFolder(15);  // /mp3/ folder = ID 15
    if (count > 0 && count <= 255) {
        totalTracks = static_cast<uint8_t>(count);
    } else {
        Serial.print(F("[Audio] Warning: could not read track count — defaulting to "));
        Serial.print(TOTAL_TRACKS);
        Serial.println(F("."));
        totalTracks = TOTAL_TRACKS;
    }
    Serial.print(F("[Audio] Tracks in /mp3/: "));
    Serial.println(totalTracks);

    // Seed with ESP32 hardware RNG for true randomness on every boot
    randomSeed(esp_random());

    Serial.println(F("[Init] Waiting for motion…\n"));
    dfPlayerReady = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// loop() — fully non-blocking
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    // ── 1. Poll DFPlayer for async status / error messages ───────────────────
    if (dfPlayerReady && myDFPlayer.available()) {
        printDFPlayerDetail(myDFPlayer.readType(), myDFPlayer.read());
    }

    // ── 2. Auto-stop after PLAY_DURATION_MS ──────────────────────────────────
    if (isPlaying && (millis() - playStartMs >= PLAY_DURATION_MS)) {
        myDFPlayer.stop();
        isPlaying = false;
        Serial.println(F("[Audio] Stopped after 3 s."));
    }

    // ── 3. PIR — play a random track on rising edge (LOW → HIGH) ─────────────
    bool pirLevel = digitalRead(PIR_PIN);
    if (pirLevel && !pirLastLevel && dfPlayerReady) {
        uint8_t track = random(1, totalTracks + 1);  // inclusive range [1, totalTracks]

        Serial.print(F("[PIR] Motion detected — playing random track "));
        Serial.print(track);
        Serial.print(F(" / "));
        Serial.println(totalTracks);

        myDFPlayer.playMp3Folder(track);
        playStartMs = millis();
        isPlaying   = true;
    }
    pirLastLevel = pirLevel;
}
