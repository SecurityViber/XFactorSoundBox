# XFactor Soundbox — Schematic Design Plan

**Title:** XFactor Soundbox  
**Revision:** 0.1  
**Author:** Chris  
**Date:** 2026-05-15  

---

## 1. Datasheet Summary

### 1.1 BS412 — Digital Smart Pyroelectric Detector (Nanyang Senba)

**Source:** `data_sheets/Datasheet.pdf` (9 pages)

#### Pinout (4-pin radial, bottom view)
| Pin | Name   | Direction | Function |
|-----|--------|-----------|----------|
| 1   | VSS    | Power     | Ground |
| 2   | ONTIME | Input     | On-time programming (analog or digital R/C) |
| 3   | VDD    | Power     | Supply voltage |
| 4   | REL    | Output    | Motion output (Schmitt trigger, HIGH on motion) |

#### Electrical Specs (T=25°C, VDD=3V nominal)
| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| Supply voltage (VDD) | 2.0 | 3.0 | 3.3 | V |
| Absolute max VDD | — | — | 3.6 | V |
| Working current (IDD) | 9 | 9.5 | 11 | µA |
| REL output LOW current (IOL) | — | — | 10 | mA (VOL < 1 V) |
| REL output HIGH current (IOH) | — | — | -10 | mA (VOH > VDD−1 V) |
| Lock time (TOL) | — | 2.3 | — | s |
| On-time (TOH) | 2 | — | 3600 | s (configurable) |

#### Logic Levels
- REL output HIGH ≥ VDD−1 V (≥ 2.3 V at VDD=3.3 V) → comfortably drives 3.3 V GPIO.
- REL output LOW ≤ 1 V.
- Output is a **Schmitt REL** (Schmitt trigger relay-style push-pull output).

#### On-time Configuration (ONTIME pin)
The ONTIME pin has an **internal 1 MΩ pull-up to VDD**. An external pull-down resistor to GND selects the on-time. Connecting ONTIME directly to GND (0 Ω) gives the minimum 2-second on-time.

| External pull-down | On-time |
|--------------------|---------|
| 0 Ω (short to GND) | 2 s |
| 51 kΩ | 5 s |
| 82 kΩ | 10 s |
| 124 kΩ | 15 s |

**Design choice:** Wire ONTIME → GND for 2 s (minimum), since firmware controls playback duration. Requires only a wire — no external resistor or capacitor.

#### Quirks
- The BS412 is a complete single-chip solution: pyroelectric element, ADC, BPF, DSP, comparator, and output driver are all internal.
- The "ELEMENT" label visible in the cross-section refers to the internal pyro element; no external connections needed.
- VDD maximum is **3.6 V** — directly compatible with the XIAO's 3V3 pin (3.3 V ± 5% = 3.465 V max, safely within spec).

---

### 1.2 DFPlayer Mini — Serial MP3 Module (DFRobot)

**Source:** `data_sheets/DFPlayer Mini Manual.pdf` (12 pages)

#### Pinout (16-pin module, 8 per side)
| Pin | Name    | Direction | Function |
|-----|---------|-----------|----------|
| 1   | VCC     | Power     | Supply voltage (3.2–5.0 V, typ 4.2 V) |
| 2   | RX      | Input     | UART serial input from MCU |
| 3   | TX      | Output    | UART serial output to MCU |
| 4   | DAC_R   | Output    | Audio out right channel (headphone/amp) |
| 5   | DAC_L   | Output    | Audio out left channel (headphone/amp) |
| 6   | SPK2    | Output    | Speaker− (H-bridge, ≤ 3 W) |
| 7   | GND     | Power     | Ground |
| 8   | SPK1    | Output    | Speaker+ (H-bridge, ≤ 3 W) |
| 9   | IO1     | Input     | Trigger port 1 (short = prev track; long = vol−) |
| 10  | GND     | Power     | Ground |
| 11  | IO2     | Input     | Trigger port 2 (short = next track; long = vol+) |
| 12  | ADKEY1  | Input     | AD key port 1 (segment 1) |
| 13  | ADKEY2  | Input     | AD key port 2 (segment 5) |
| 14  | USB+    | USB       | USB D+ |
| 15  | USB−    | USB       | USB D− |
| 16  | BUSY    | Output    | Playing status: **LOW = playing, HIGH = idle/stopped** |

> **SPK1/SPK2 label discrepancy:** Figure 2.1 (module photo) labels pins in the reverse order from Table 2.2. Table 2.2 is taken as authoritative: pin 6 = SPK2 (−), pin 8 = SPK1 (+). This does not affect function as the amplifier is an H-bridge.

#### Electrical Specs
| Parameter | Value |
|-----------|-------|
| Working voltage (VCC) | DC 3.2–5.0 V (typ 4.2 V) |
| Standby current | 20 mA |
| UART baud rate | 9600 bps (default, adjustable) |
| I/O logic level | 3.3 V TTL (per Note 6, module spec) |
| I/O input VIH min | 0.7 × VDD |
| I/O input VIL max | 0.3 × VDD |
| I/O output VOH min | 2.7 V (at VDD = 3.3 V test condition) |
| I/O output VOL max | 0.33 V |
| Max speaker output | 3 W |

#### UART Protocol
- 9600 8N1, no flow control.
- Frame: `7E FF 06 CMD FB DH DL CKH CKL EF` (10 bytes).
- Play track N: `7E FF 06 03 00 00 NN CK CK EF`.
- Module initialises in 1.5–3 s after power-on before accepting commands.

#### Quirks
- **BUSY polarity conflict in datasheet:** Table 2.2 states "Low means playing, High means no [playing]." Section 3.3.2 text says the opposite ("Output high level at playback status"). Table 2.2 is the authoritative reference and matches common hardware behaviour. BUSY = LOW during playback, HIGH when idle.
- **Power-on glitch:** After inserting microSD, module auto-plays first track. Firmware should send Pause (`7E FF 06 0E 00 00 00 FF EE EF`) after the ~3 s init delay.
- **Current spikes during playback:** The 3 W internal amplifier draws significant current pulses into the speaker. A 100 µF electrolytic bulk capacitor on the 5 V rail is required to prevent brownouts and audio glitches.
- **UART TX voltage at 5 V supply:** The datasheet states "3.3 V TTL level" I/O, but the I/O spec table's test condition is VDD = 3.3 V. If powered at 5 V and the I/O tracks VDD, TX could swing to ~5 V. See Open Questions §6.1.

---

### 1.3 XIAO ESP32-S3 — Seeed Studio Dev Module

**Source:** `data_sheets/esp32-s3_datasheet.pdf` (Espressif, v1.6, 75 pages) + pinout diagrams in `data_sheets/schematic/`.

#### Module Header Pinout (from front-indication.jpg)
Left column, top to bottom:

| Module Label | GPIO | Alt Functions | XIAO Silk |
|-------------|------|---------------|-----------|
| D0 | GPIO1 | TOUCH1, ADC1_CH0 | A0 |
| D1 | GPIO2 | TOUCH2, ADC1_CH1 | A1 |
| D2 | GPIO3 | TOUCH3, ADC1_CH2, **JTAG strapping** | A2 |
| D3 | GPIO4 | TOUCH4, ADC1_CH3 | A3 |
| D4 | GPIO5 | TOUCH5, ADC1_CH4, SDA | A4 |
| D5 | GPIO6 | TOUCH6, ADC1_CH5, SCL | A5 |
| D6 | GPIO43 | **UART0 TX** (USB-CDC) | TX |

Right column, top to bottom:

| Module Label | GPIO | Alt Functions | XIAO Silk |
|-------------|------|---------------|-----------|
| 5V | — | USB VBUS pass-through | 5V |
| GND | — | Ground | GND |
| 3V3 | — | Onboard LDO output | 3V3 |
| D10 | GPIO9 | TOUCH9, ADC1_CH8, MOSI | A10 |
| D9 | GPIO8 | TOUCH8, ADC1_CH7, MISO | A9 |
| D8 | GPIO7 | TOUCH7, ADC1_CH6, SCK | A8 |
| D7 | GPIO44 | **UART0 RX** (USB-CDC) | RX |

Camera expansion (bottom pads, Sense version only):

| Module Label | GPIO | XIAO Silk |
|-------------|------|-----------|
| D11 | GPIO42 | A11 |
| D12 | GPIO41 | A12 |

#### Power
| Rail | Voltage | Max current | Notes |
|------|---------|-------------|-------|
| 5V pin | USB VBUS (~5 V) | 500 mA (USB 2.0 host) | Direct pass-through — no regulation |
| 3V3 pin | 3.3 V | ~700 mA | Onboard LDO |
| GND | 0 V | — | |

#### ESP32-S3 GPIO Electrical Characteristics (3.3 V, 25°C)
| Symbol | Parameter | Min | Max | Unit |
|--------|-----------|-----|-----|------|
| VIH | High-level input voltage | 0.75 × 3.3 = **2.475 V** | 3.6 V | V |
| VIL | Low-level input voltage | −0.3 V | 0.25 × 3.3 = 0.825 V | V |
| VOH | High-level output voltage | 0.8 × 3.3 = 2.64 V | — | V |
| VOL | Low-level output voltage | — | 0.1 × 3.3 = 0.33 V | V |
| Abs. max input | Maximum GPIO input voltage | — | **3.6 V** | V |
| RPU | Internal weak pull-up | — | 45 | kΩ |
| RPD | Internal weak pull-down | — | 45 | kΩ |

#### Strapping Pins (sampled at chip reset)
| Pin | XIAO Header | Default | Controls |
|-----|-------------|---------|----------|
| GPIO0 | Not exposed | Pull-up (→ SPI Boot) | Chip boot mode |
| GPIO3 | **D2** | Floating | JTAG signal source — **must not float** |
| GPIO45 | Not exposed | Pull-down | VDD_SPI voltage |
| GPIO46 | Not exposed | Pull-down | Boot mode + ROM print |

GPIO3 (D2) is accessible on the XIAO header. The Seeed module handles it internally for the standard firmware workflow, but this pin must carry a no-connect (X) flag in the schematic and must not be driven by external logic during power-up.

#### UART Peripherals
The ESP32-S3 has **3 UART controllers** (UART0, UART1, UART2). All can be routed to any GPIO via the GPIO matrix.

- **UART0:** Fixed to GPIO43 (TX) / GPIO44 (RX) on the XIAO module — shared with USB-CDC. **Must not be used for DFPlayer.**
- **UART1:** Available on any GPIO (routed via GPIO matrix in firmware). Assigned to GPIO5 (TX → DFPlayer RX) and GPIO4 (RX ← DFPlayer TX) in this design.

---

## 2. Block Diagram

```
USB-C (5 V, 500 mA)
        │
        ▼
┌───────────────────────────┐
│   XIAO ESP32-S3 module    │
│                           │
│  USB-C ──► 5V pin ──────────────────────────────────────────┐
│            │                                                │
│           LDO                                               │
│            │                                                │
│          3V3 pin ──────────────────────────┐                │
│            │                               │                │
│           GND ─────────────────────────────┼──────────┐     │
│                                            │          │     │
│  UART1 TX ── GPIO5 (D4) ── MP3_TX ─────────┼──►[RX] ──┼─────┼─► DFPlayer Mini
│  UART1 RX ◄─ GPIO4 (D3) ── MP3_RX ─[R1/R2]─┼──[TX] ──┼─────┘     │
│                                            │          │            │
│  GPIO2 (D1) ◄─ MP3_BUSY ───────────────────┼──[BUSY]──┘            │
│                                            │                       │
│  GPIO1 (D0) ◄─ PIR_OUT                    │    [SPK1]─────────────►┤
│                │                           │    [SPK2]─────────────►┤ JST-PH 2.0
│              BS412 PIR                     │                       │
│              VDD◄──────────────────────────┘               [FIT0502 Speaker]
│              VSS◄──────────────────────────────────────────GND
│                                            │
└───────────────────────────────────────────┘

Power flow:
  USB 5V → XIAO 5V pin → DFPlayer VCC (with 100nF + 100µF bulk decoupling)
  USB 5V → XIAO LDO → XIAO 3V3 pin → BS412 VDD (with 100nF decoupling)
  USB 5V → XIAO internal → ESP32-S3 core (3.3V internal regulation)

Signal flow:
  BS412 REL → PIR_OUT → GPIO1 (D0): digital input, interrupt on rising edge
  GPIO5 (D4) → MP3_TX → DFPlayer RX: UART1 TX, 3.3V direct (no level shift needed)
  DFPlayer TX → R1 (1kΩ) → node → R2 (2kΩ) → GND; node → MP3_RX → GPIO4 (D3): UART1 RX
  DFPlayer BUSY → MP3_BUSY → GPIO2 (D1): digital input, LOW=playing
  DFPlayer SPK1 → SPK_P → JST pin 1 → FIT0502 (+)
  DFPlayer SPK2 → SPK_N → JST pin 2 → FIT0502 (−)

Level shifting (RX path only):
  DFPlayer TX at up to 5V → 1kΩ/2kΩ resistor divider → ~3.33V at XIAO GPIO4
  (XIAO GPIO max input: 3.6V; VIH min: 2.475V — this is within spec for 5V swing)
```

---

## 3. XIAO ESP32-S3 Pin Assignment Table

| XIAO Pin | GPIO | Net Name | Direction | Rationale |
|----------|------|----------|-----------|-----------|
| 5V | — | +5V | Power out | USB VBUS; feeds DFPlayer VCC |
| GND | — | GND | Power | Common ground |
| 3V3 | — | +3V3 | Power out | Onboard LDO; feeds BS412 VDD |
| D0 | GPIO1 | PIR_OUT | Input | BS412 REL output; no strapping conflict; supports interrupt |
| D1 | GPIO2 | MP3_BUSY | Input | DFPlayer BUSY status; active-LOW during playback |
| D2 | GPIO3 | — | **NC** | Strapping pin (JTAG source); must not be driven — add no-connect flag |
| D3 | GPIO4 | MP3_RX | Input | UART1 RX; receives from DFPlayer TX via 1kΩ/2kΩ divider |
| D4 | GPIO5 | MP3_TX | Output | UART1 TX; drives DFPlayer RX directly (3.3V → 5V MCU input, no shift needed) |
| D5 | GPIO6 | — | NC | Unused; no-connect |
| D6 | GPIO43 | — | NC | UART0 TX / USB-CDC — must not be used for DFPlayer |
| D7 | GPIO44 | — | NC | UART0 RX / USB-CDC — must not be used for DFPlayer |
| D8 | GPIO7 | — | NC | Unused; no-connect |
| D9 | GPIO8 | — | NC | Unused; no-connect |
| D10 | GPIO9 | — | NC | Unused; no-connect |

**GPIO4 / GPIO5 selected for UART1** because:
- They are adjacent (D3/D4), making schematic routing clean.
- Neither is a strapping pin.
- Both are in the RTC/analog domain (GPIO1–GPIO14), which means they can wake from deep sleep — useful if the firmware ever adds sleep-on-idle.
- UART1 TX/RX can be assigned to any GPIO through the ESP32-S3 GPIO matrix; firmware configures `UART_NUM_1` to these pins.

---

## 4. Component List and KiCad Symbol Plan

### 4.1 Standard KiCad 9 Library Parts

These exist in the standard KiCad 9 libraries and do **not** require custom symbols:

| Reference | Value | KiCad Library : Symbol | Notes |
|-----------|-------|------------------------|-------|
| R1 | 1 kΩ | Device:R | DFPlayer TX series resistor (divider) |
| R2 | 2 kΩ | Device:R | Divider pull-down to GND |
| C1 | 100 nF | Device:C | DFPlayer VCC decoupling (ceramic X7R) |
| C2 | 100 µF / 10 V | Device:C_Polarized | DFPlayer 5V bulk cap (electrolytic) |
| C3 | 100 nF | Device:C | BS412 VDD decoupling (ceramic X7R) |
| C4 | 100 nF | Device:C | XIAO 3V3 local decoupling (ceramic X7R) |
| J1 | JST-PH 2.0 2P | Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal | Speaker connector |
| PWR1–3 | +5V, +3V3, GND | power:+5V, power:+3V3, power:GND | Power symbols |
| PWR_FLAG × 2 | — | power:PWR_FLAG | Required by ERC for power nets |

### 4.2 Custom Project-Local Symbol Library

**File:** `kicad/XFactorBox/XFactorBox.kicad_sym`

Three parts require custom symbols because they do not exist in the standard KiCad 9 libraries:

---

#### 4.2.1 XIAO_ESP32S3

A module-level symbol representing the Seeed Studio XIAO ESP32-S3 dev board. Pin numbering follows physical header position (left column top→bottom = pins 1–7; right column top→bottom = pins 8–14).

| KiCad Pin # | Pin Name | Type | GPIO |
|-------------|----------|------|------|
| 1 | D0 | Bidirectional | GPIO1 |
| 2 | D1 | Bidirectional | GPIO2 |
| 3 | D2 | Bidirectional | GPIO3 |
| 4 | D3 | Bidirectional | GPIO4 |
| 5 | D4 | Bidirectional | GPIO5 |
| 6 | D5 | Bidirectional | GPIO6 |
| 7 | TX | Output | GPIO43 |
| 8 | 5V | Power input | — |
| 9 | GND | Power input | — |
| 10 | 3V3 | Power output | — |
| 11 | D10 | Bidirectional | GPIO9 |
| 12 | D9 | Bidirectional | GPIO8 |
| 13 | D8 | Bidirectional | GPIO7 |
| 14 | RX | Input | GPIO44 |

Body layout: rectangular, left column pins on the left, right column pins on the right, power pins (5V, GND, 3V3) grouped at top-right.

---

#### 4.2.2 DFPlayer_Mini

16-pin module. Left-side pins (1–8) on left body edge; right-side pins (9–16) on right body edge.

| KiCad Pin # | Pin Name | Type |
|-------------|----------|------|
| 1 | VCC | Power input |
| 2 | RX | Input |
| 3 | TX | Output |
| 4 | DAC_R | Output |
| 5 | DAC_L | Output |
| 6 | SPK2 | Output |
| 7 | GND | Power input |
| 8 | SPK1 | Output |
| 9 | IO1 | Input |
| 10 | GND | Power input |
| 11 | IO2 | Input |
| 12 | ADKEY1 | Input |
| 13 | ADKEY2 | Input |
| 14 | USB_P | Bidirectional |
| 15 | USB_N | Bidirectional |
| 16 | BUSY | Output |

---

#### 4.2.3 BS412

4-pin radial through-hole. Pins numbered per datasheet bottom view (pin 1 = VSS, counting clockwise).

| KiCad Pin # | Pin Name | Type |
|-------------|----------|------|
| 1 | VSS | Power input |
| 2 | ONTIME | Input |
| 3 | VDD | Power input |
| 4 | REL | Output |

---

## 5. Net Naming Convention

| Net Name | Type | Description |
|----------|------|-------------|
| `+5V` | Power | USB 5 V (from XIAO 5V pin to DFPlayer VCC) |
| `+3V3` | Power | 3.3 V (from XIAO 3V3 pin to BS412 VDD and XIAO decoupling) |
| `GND` | Power | Common ground |
| `PIR_OUT` | Signal | BS412 REL → XIAO GPIO1 (D0); HIGH on motion |
| `MP3_TX` | Signal | XIAO GPIO5 (D4) → DFPlayer RX; UART1 TX |
| `MP3_RX` | Signal | DFPlayer TX → resistor divider node → XIAO GPIO4 (D3); UART1 RX |
| `MP3_BUSY` | Signal | DFPlayer BUSY (pin 16) → XIAO GPIO2 (D1); LOW when playing |
| `SPK_P` | Signal | DFPlayer SPK1 (pin 8) → JST pin 1 → FIT0502 (+) |
| `SPK_N` | Signal | DFPlayer SPK2 (pin 6) → JST pin 2 → FIT0502 (−) |

The resistor divider mid-node between R1 and R2 carries the net label `MP3_RX` (it is the signal arriving at the XIAO RX pin after voltage scaling).

---

## 6. Open Questions

### 6.1 ⚠ DFPlayer TX voltage swing at 5 V supply (CRITICAL)

**Issue:** The DFPlayer datasheet Section 6 states "the module's external interfaces are 3.3 V TTL level." However, the I/O specs are given at VDD = 3.3 V test condition only. If the DFPlayer's I/O buffers track the 5 V supply rail (i.e., TX can swing to ~5 V), the designed 1 kΩ/2 kΩ divider correctly limits the XIAO RX input to ~3.33 V, which is within the XIAO's 3.6 V absolute maximum.

**Risk if TX only swings to 3.3 V:** The divider output would be only 2.2 V, which is **below** the XIAO's VIH minimum of 2.475 V. UART communication would be unreliable. In that case, the divider values should be changed (e.g., 0 Ω / ∞ — i.e., direct connection) since 3.3 V is within the XIAO's safe input range.

**Action needed:** Confirm DFPlayer TX HIGH level at 5 V VCC by probing with a meter or oscilloscope before PCB commit. If it measures ~5 V, keep the current divider. If it measures ~3.3 V, remove or bypass the divider.

### 6.2 ⚠ DFPlayer BUSY pin at 5 V supply (needs decision)

**Issue:** Same voltage ambiguity as §6.1 but for the BUSY output. If BUSY swings to 5 V when the DFPlayer is idle, and this is wired directly to XIAO GPIO2, it would exceed the 3.6 V absolute maximum and damage the ESP32-S3.

**Current schematic approach:** Direct wire from BUSY to GPIO2 (based on task spec "BUSY pin of DFPlayer → a XIAO GPIO, with pull-up if needed"). This is safe only if BUSY output HIGH ≤ 3.6 V.

**Mitigation options (if BUSY swings to 5 V):**
- Add a 1 kΩ/2 kΩ resistor divider identical to the RX path.
- Add a series 1 kΩ resistor plus enable the ESP32-S3 internal 45 kΩ pull-down — this creates a divider of 1 kΩ/(45 kΩ) giving ~4.9 V (still too high).
- Use a Schottky diode clamp to 3V3.

**Action needed:** Confirm BUSY HIGH level under the same bench measurement as §6.1. Schematic will show direct connection; a DNP (do-not-populate) footprint for a series resistor + shunt resistor can be added if needed.

### 6.3 BS412 ONTIME duration

**Issue:** Tying ONTIME to GND gives the minimum 2-second output pulse from the BS412. The firmware may retrigger the DFPlayer before this expires, or there may be a required lockout for user experience.

**Action needed:** Confirm 2 s is acceptable. If longer on-time is preferred, use a resistor (e.g., 124 kΩ for 15 s) from ONTIME to GND instead of a wire.

### 6.4 DFPlayer SPK1/SPK2 label discrepancy

**Issue:** Figure 2.1 in the DFPlayer Manual labels pin-position 6 as "SPK 1" and pin-position 8 as "SPK 2," but Table 2.2 says pin 6 = SPK2 (−) and pin 8 = SPK1 (+). The custom symbol and schematic follow **Table 2.2**.

**Impact:** The speaker may play with inverted phase if the module's physical pin 6 is actually SPK1. This has no audible consequence for a mono speaker — the sound level and quality are unaffected. If polarity matters, swap J1 pins 1 and 2 during bring-up.

### 6.5 DFPlayer BUSY polarity (documentation conflict)

**Issue:** Table 2.2 says "Low means playing, High means no [playing]." Section 3.3.2 text says "Output high level at playback status." These are contradictory. The pin table is assumed authoritative: **BUSY = LOW during playback, BUSY = HIGH when idle**. Firmware should be coded accordingly.

### 6.6 DFPlayer power-on auto-play

**Issue:** When a TF card is inserted, the DFPlayer automatically starts playing the first track ~1.5–3 s after power-up. Firmware must wait for the init completion message (`7E FF 06 3F ...`) and then send a Pause command before sending the first UART play command.

**No schematic change needed** — noted here for firmware author awareness.

### 6.7 JST-PH 2.0 connector polarity

**Action needed:** Confirm which pin of the FIT0502 JST-PH 2.0 plug is (+) and which is (−) so that SPK_P and SPK_N are wired to the correct JST positions. The FIT0502 datasheet should specify this; it was not included in the provided data sheets. The schematic will show generic pin 1/pin 2 — verify before soldering.
