/**
 * XFactor SoundBox — DFPlayer Mini Controller
 *
 * Target:  Seeed Studio XIAO ESP32-S3
 * Library: DFRobotDFPlayerMini (declared in platformio.ini lib_deps)
 *
 * Wiring:
 *   XIAO GPIO 43 (D6 / TX) ──── 1 kΩ ──── DFPlayer RX
 *   XIAO GPIO 44 (D7 / RX) ────────────── DFPlayer TX
 *   XIAO GND               ────────────── DFPlayer GND
 *   DFPlayer VCC            ────────────── 3.3 V or 5 V (module accepts both)
 *
 * SD card layout expected on DFPlayer:
 *   /mp3/0001.mp3   ← played on boot
 *        0002.mp3
 *        ...
 *
 * Note: Use myDFPlayer.playMp3Folder(n) to play /mp3/000n.mp3.
 *       playFolder(15, n) looks for a folder literally named "15" — it does
 *       NOT address the special /mp3/ folder.
 */

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// ── Pin & Serial Configuration ───────────────────────────────────────────────
// Named from the XIAO's perspective (matching the silk-screen labels):
//   XIAO TX = GPIO43 / D6  → transmits to DFPlayer RX (via 1 kΩ resistor)
//   XIAO RX = GPIO44 / D7  ← receives from DFPlayer TX

static constexpr int  XIAO_UART_TX = 43;   // GPIO43 | D6 | TX  →  DFPlayer RX
static constexpr int  XIAO_UART_RX = 44;   // GPIO44 | D7 | RX  ←  DFPlayer TX
static constexpr long DEBUG_BAUD      = 115200;
static constexpr long DFPLAYER_BAUD   = 9600;

// ── Audio Configuration ───────────────────────────────────────────────────────

static constexpr uint8_t  VOLUME_LEVEL       = 20;      // 0 – 30
static constexpr uint8_t  START_TRACK        = 1;
static constexpr uint32_t TRACK_INTERVAL_MS  = 8000UL;  // advance to next track every 8 s

// ── Objects ───────────────────────────────────────────────────────────────────

// UART1 keeps USB Serial (UART0) free for the debug monitor.
HardwareSerial      dfPlayerSerial(1);
DFRobotDFPlayerMini myDFPlayer;

// ── Module State ─────────────────────────────────────────────────────────────

static bool     dfPlayerReady  = false;
static uint8_t  totalTracks    = 2;
static uint8_t  currentTrack   = START_TRACK;
static uint32_t lastTrackMs    = 0;    // millis() timestamp of the last playFolder() call
static uint32_t ledOffMs       = 0;    // millis() timestamp when LED should turn off

// ─────────────────────────────────────────────────────────────────────────────
// printDFPlayerDetail()
//   Decodes and logs the asynchronous status messages sent by the DFPlayer.
//   Call this whenever myDFPlayer.available() returns true.
// ─────────────────────────────────────────────────────────────────────────────
static void printDFPlayerDetail(uint8_t type, int value) {
    switch (type) {
        case TimeOut:
            Serial.println(F("[DFPlayer] Timeout waiting for response."));
            break;
        case WrongStack:
            Serial.println(F("[DFPlayer] Stack error — check wiring/baud rate."));
            break;
        case DFPlayerCardInserted:
            Serial.println(F("[DFPlayer] SD card inserted."));
            break;
        case DFPlayerCardRemoved:
            Serial.println(F("[DFPlayer] SD card removed."));
            break;
        case DFPlayerCardOnline:
            Serial.println(F("[DFPlayer] SD card online."));
            break;
        case DFPlayerUSBInserted:
            Serial.println(F("[DFPlayer] USB storage inserted."));
            break;
        case DFPlayerUSBRemoved:
            Serial.println(F("[DFPlayer] USB storage removed."));
            break;
        case DFPlayerPlayFinished:
            Serial.print(F("[DFPlayer] Playback finished — Folder: "));
            Serial.print(value >> 8);
            Serial.print(F(", Track: "));
            Serial.println(value & 0xFF);
            break;
        case DFPlayerError:
            Serial.print(F("[DFPlayer] Runtime error: "));
            switch (value) {
                case Busy:             Serial.println(F("Busy / no SD card."));           break;
                case Sleeping:         Serial.println(F("Module is sleeping."));           break;
                case SerialWrongStack: Serial.println(F("Serial framing error."));         break;
                case CheckSumNotMatch: Serial.println(F("Checksum mismatch."));            break;
                case FileIndexOut:     Serial.println(F("File index out of bounds."));     break;
                case FileMismatch:     Serial.println(F("File not found on SD card."));    break;
                case Advertise:        Serial.println(F("Advertise mode active."));        break;
                default:               Serial.println(F("Unknown error code."));           break;
            }
            break;
        default:
            break;
    }
}

static void triggerLedBlink() {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOffMs = millis() + 200;
}

// ─────────────────────────────────────────────────────────────────────────────
// initDFPlayer()
//   Blocks until the DFPlayer responds or halts with diagnostic output.
// ─────────────────────────────────────────────────────────────────────────────
static bool initDFPlayer() {
    Serial.println(F("[Init] Starting DFPlayer Mini..."));

    // isACK=false → skip ACK handshake; required for most clone modules
    // doReset=false → don't reset; avoids ~1-2 s delay and clone incompatibilities
    if (!myDFPlayer.begin(dfPlayerSerial, /*isACK=*/false, /*doReset=*/false)) {
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
    // USB debug serial — wait up to 3 s for a host to attach.
    Serial.begin(DEBUG_BAUD);
    const uint32_t bootWaitMs = millis() + 3000UL;
    while (!Serial && millis() < bootWaitMs) { /* spin */ }

    Serial.println(F(""));
    Serial.println(F("╔══════════════════════════════════╗"));
    Serial.println(F("║   XFactor SoundBox — Booting…   ║"));
    Serial.println(F("╚══════════════════════════════════╝"));

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // UART1: begin(baud, config, rxPin, txPin)
    dfPlayerSerial.begin(DFPLAYER_BAUD, SERIAL_8N1, XIAO_UART_RX, XIAO_UART_TX);

    // DFPlayer needs ~1 s after power-on to enumerate the SD card.
    delay(1000);

    if (!initDFPlayer()) {
        Serial.println(F("[Init] Continuing anyway — module may still respond."));
    }

    // ── Audio settings ───────────────────────────────────────────────────────
    myDFPlayer.setTimeOut(500);                    // ms before a command times out
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);   // source: SD card
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
    myDFPlayer.volume(VOLUME_LEVEL);               // 0–30

    Serial.print(F("[Audio] Volume: "));
    Serial.print(VOLUME_LEVEL);
    Serial.println(F(" / 30"));
    Serial.println(F("[Audio] EQ: Normal"));

    // ── Initial playback — fires immediately on boot ─────────────────────────
    // playMp3Folder(n) plays /mp3/000n.mp3 — the correct command for the
    // special /mp3/ folder. playFolder(15, n) looks for a folder named "15"
    // and does NOT address /mp3/.
    currentTrack = START_TRACK;
    Serial.print(F("[Audio] Playing /mp3/000"));
    Serial.print(currentTrack);
    Serial.println(F(".mp3"));

    myDFPlayer.playMp3Folder(currentTrack);
    triggerLedBlink();
    lastTrackMs = millis();   // start the 3 s countdown from now

    dfPlayerReady = true;
    Serial.println(F("[Init] Setup complete.\n"));
}

// ─────────────────────────────────────────────────────────────────────────────
// loop() — non-blocking
//   Structure is intentionally flat so touch/button handlers can be added
//   without restructuring around a blocking delay.
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    // ── 1. Poll DFPlayer for async status / error messages ───────────────────
    if (dfPlayerReady && myDFPlayer.available()) {
        printDFPlayerDetail(myDFPlayer.readType(), myDFPlayer.read());
    }

    // ── 2. Auto-advance: play next track every TRACK_INTERVAL_MS, wrap to 1 ──
    if (dfPlayerReady) {
        uint32_t now = millis();
        if (now - lastTrackMs >= TRACK_INTERVAL_MS) {
            lastTrackMs = now;
            currentTrack = (currentTrack % totalTracks) + 1;  // 1→2→…→N→1

            Serial.print(F("[Audio] Auto-advance → track "));
            Serial.print(currentTrack);
            Serial.print(F(" / "));
            Serial.println(totalTracks);

            myDFPlayer.playMp3Folder(currentTrack);
            triggerLedBlink();
        }
    }

    // ── LED blink off ────────────────────────────────────────────────────────
    if (ledOffMs > 0 && millis() >= ledOffMs) {
        digitalWrite(LED_BUILTIN, LOW);
        ledOffMs = 0;
    }

    // ── 3. Touch input — expand here when hardware is connected ──────────────
    //
    // Example (XIAO ESP32-S3 touch-capable pins: GPIO 1–5, 7–9, etc.):
    //
    // static uint32_t lastTouchMs = 0;
    // static constexpr uint32_t DEBOUNCE_MS     = 80;
    // static constexpr int      TOUCH_THRESHOLD = 40000; // tune per pad
    //
    // uint32_t now = millis();
    // if ((now - lastTouchMs) > DEBOUNCE_MS) {
    //     if (touchRead(T1) < TOUCH_THRESHOLD) {  // T1 = GPIO 1
    //         lastTouchMs = now;
    //         myDFPlayer.next();
    //         Serial.println(F("[Input] Touch T1 — next track."));
    //     }
    //     if (touchRead(T2) < TOUCH_THRESHOLD) {  // T2 = GPIO 2
    //         lastTouchMs = now;
    //         myDFPlayer.previous();
    //         Serial.println(F("[Input] Touch T2 — previous track."));
    //     }
    // }

    // ── 4. Other non-blocking tasks go here ──────────────────────────────────
}
