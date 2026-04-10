# Project Master Reference Document
## Consolidated from: Board Pinout, Board Manual, TMC2209 Pinout, TMC2209 Manual, Motor/Encoder Datasheet

---

# 1. BTT OCTOPUS V1.0 — MOTOR DRIVER PIN MAPPING

| Driver   | EN   | STEP | DIR  | CS   |
|----------|------|------|------|------|
| DRIVER0  | PF14 | PF13 | PF12 | PC4  |
| DRIVER1  | PF15 | PG0  | PG1  | PD11 |
| DRIVER2  | PG5  | PF11 | PG3  | PC6  |
| DRIVER3  | PA0  | PG4  | PC1  | PC7  |
| DRIVER4  | PG2  | PF9  | PF10 | PF2  |
| DRIVER5  | PF1  | PC13 | PF0  | PE4  |
| DRIVER6  | PD4  | PE2  | PE3  | PE1  |
| DRIVER7  | PE0  | PE6  | PA14 | PD3  |

Motor-SPI Bus:
- MOSI: PA7
- MISO: PA6
- SCK: PA5

---

# 2. BTT OCTOPUS V1.0 — FULL BOARD PINOUT

## Heaters
| Heater | Voltage | Signal Pin |
|--------|---------|------------|
| HE0    | 24V     | PA2        |
| HE1    | 24V     | PA3        |
| HE2    | 24V     | PB10       |
| HE3    | 24V     | PB11       |

## Bed Heater
- BED Power: VBI / GND
- BED-OUT: VBO / PA1

## Temperature Sensors
| Sensor | GND | Signal |
|--------|-----|--------|
| TB     | GND | PF3    |
| T0     | GND | PF4    |
| T1     | GND | PF5    |
| T2     | GND | PF6    |
| T3     | GND | PF7    |

## Fans (PWM)
| Fan   | Voltage Pin | Signal Pin |
|-------|-------------|------------|
| FAN0  | VFAN0       | PA8        |
| FAN1  | VFAN1       | PE5        |
| FAN2  | VFAN2       | PD12       |
| FAN3  | VFAN3       | PD13       |
| FAN4  | VFAN4       | PD14       |
| FAN5  | VFAN5       | PD15       |
| FAN6  | VFAN6       | GND (always on) |
| FAN7  | VFAN7       | GND (always on) |

Fan voltage selectable per fan: Vin (24V), 12V, or 5V via jumpers.

## DIAG Pins (StallGuard)
| DIAG  | 5V  | GND | Signal |
|-------|-----|-----|--------|
| DIAG0 | 5V  | GND | PG6    |
| DIAG1 | 5V  | GND | PG9    |
| DIAG2 | 5V  | GND | PG10   |
| DIAG3 | 5V  | GND | PG11   |
| DIAG4 | 5V  | GND | PG12   |
| DIAG5 | 5V  | GND | PG13   |
| DIAG6 | 5V  | GND | PG14   |
| DIAG7 | 5V  | GND | PG15   |

## I2C & EEPROM
- SCL: PB8
- SDA: PB9
- Onboard 32K EEPROM (AT24C32)

## SPI Expansion Header
- 3.3V, PB5 (MOSI), PA15 (CS), PB4 (MISO), PB3 (SCK), GND

## USB-A Port
- GND, PB15, PB14, 5V

## Raspberry Pi Header
- 5V, GND (multiple), PD6(RX), PD5(TX), 5V

## TFT Header
- RST, PA10(RX), PA9(TX), GND, 5V

## BL Touch Header
- GND, 5V, PB6, PB7, GND

## EXP1 (Display)
- PE8, PE9, PE12, PE14, GND
- PE7, PE10, PE13, PE15, 5V

## EXP2 (Display)
- PA6, PB1, PB2, PC15, RST
- PA5, PA4, PA7, GND, PC5

## PS ON (Power Supply Control)
- PE11, GND

## SENSOR (Probe)
- VS, GND, PB7

## CAN Bus (RJ11)
- NC, CAN_L, CAN_H, NC
- RX: PD0, TX: PD1

## SDIO-MicroSD
- D0: PC8, D1: PC9, D2: PC10, D3: PC11, CLK: PC12, DET: PC14, CMD: PD2

## SPI3 Expansion Header
Row 1: 3.3V, PC3, PC2, PB13, NC, PG8, NC, PG7 (MOSI, MISO, SCK)
Row 2: GND, PB12, NC, PD7, PD10, NC, PD8, PD9 (CS, TX, RX)

## RGB LED
- GND, PB0, 5V

## PT100 Interface
- PF8, PT100_P, PT100_N

## Power Detection
- PWR-DET: 3.3V, GND, PC0

---

# 3. BTT OCTOPUS V1.0 — BOARD MANUAL SUMMARY

## Key Specs
- MCU: STM32F446ZET6 (ARM Cortex-M4, 180MHz)
- Board size: 160mm x 100mm (mounting holes: 150mm x 90mm)
- Supports Klipper and Marlin firmware
- Recommended input: DC 24V (12V supported on motor and bed rails only; logic must be >14.1V)
- Built-in regulators: 12V (4A), 5V (8A), 3.3V (1A)
- Up to 8 stepper drivers, 9 stepper outputs (MOTOR2 has dual output for parallel Z)
- 4 hotend heaters, 1 heated bed (up to 300W, use external MOSFET for higher)
- 6 PWM fans + 2 always-on fans, individually voltage-selectable (Vin/12V/5V)
- Flyback protection on all PWM fan ports
- 6 endstop inputs, 2 filament runout inputs
- USB-C (virtual serial), UART, SPI connections to Raspberry Pi
- Onboard 32K EEPROM (AT24C32)
- BL Touch support, dedicated PROBE port (optocoupler-protected, no BAT85 needed)
- CAN Bus via RJ11 (6P6C)
- StallGuard support for sensorless homing (TMC drivers)
- SPI expansion port (for MAX31865, etc.)
- DIY PT100 interface (requires user-soldered INA826AIDR amplifier, SOP-8)
- Thermistor inputs protected up to Vin; unused inputs usable as GPIO
- WiFi interface for ESP8266 modules
- RGB LED interface
- SD card firmware update (firmware.bin → FIRMWARE.CUR)
- Supports DFU mode firmware update (overwrites bootloader — not recommended for novices)

## Power Wiring
Three separate power inputs (common ground):
1. Motherboard power (must be >14.1V for logic regulators)
2. Motor power (12V or 24V)
3. Bed heater power (12V or 24V)

## Stepper Driver Modes

### Step/Dir Mode
Jumper layout beneath each driver (simplified):
```
Row 1 (top):    0V     0V     0V     0V
Row 2 (middle): RST    MS3/2  MS2/1  MS1/0
Row 3 (bottom): SLP    3.3V   3.3V   3.3V
```
- Jumper top-to-middle: sets MS pin to 0V (LOW)
- Jumper middle-to-bottom: sets MS pin to 3.3V (HIGH)
- First column: connects SLP to RST (required for A4988/DRV8825)
- Note: Using non-16 microstepping in step/dir mode prevents SPI mode on other drivers (shared pins)

### UART Mode
Single jumper on the designated UART position beneath the driver.

### SPI Mode
Specific jumper configuration beneath the driver for SPI communication.

## StallGuard / Sensorless Homing
DIAG jumpers connect driver diagnostic output to endstop inputs. Located in a dedicated jumper block on the board.

## Probe Port
- Internally protected via optocoupler (no BAT85 diode needed)
- NPN probes: add external 4K7 pull-up between probe V+ and signal
- PNP probes: connect directly
- Must enable internal pull-down resistor on probe input pin in firmware

## Klipper Firmware Settings
When compiling Klipper, use settings to preserve factory bootloader:
- Micro-controller: STM32
- Processor: STM32F446
- Bootloader offset: 32KiB
- Communication: USB on PA11/PA12

## Precautions
1. Never work on board with power applied
2. Double-check all jumpers/wiring before powering on
3. Heated bed max 300W (use external MOSFET for more)
4. Always reference PINS.pdf for connections
5. SD card firmware update recommended over DFU
6. Early production silkscreen errors on: Fan polarity, SPI3 header, Raspberry Pi UART header

---

# 4. BIGTREETECH TMC2209 V1.2 — SCHEMATIC/PINOUT

## IC Pin Connections (TMC2209 chip — U1)
| Pin # | Name      | Function                        |
|-------|-----------|----------------------------------|
| 2     | ENN       | Enable (active low)              |
| 16    | STEP      | Step pulse input                 |
| 19    | DIR       | Direction input                  |
| 11    | DIAG      | Diagnostic/StallGuard output     |
| 12    | INDEX     | Configurable index output        |
| 9     | MS1       | Microstep config / UART addr     |
| 10    | MS2       | Microstep config / UART addr     |
| 9     | MS1_AD0   | Address bit 0 (UART mode)        |
| 10    | MS2_AD1   | Address bit 1 (UART mode)        |
| 13    | CLK       | Clock input                      |
| 14    | PDN_UART  | UART interface                   |
| 17    | VREF      | Analog reference for current     |
| 15    | VCC_IO    | Logic supply voltage             |
| 8     | 5VOUT     | Internal 5V regulator output     |
| 22    | VS        | Motor supply voltage             |
| 28    | VS        | Motor supply voltage             |
| 6     | VCP       | Charge pump capacitor            |
| 5     | CPI       | Charge pump input                |
| 4     | CPO       | Charge pump output               |
| 1     | OB2       | Output bridge B2                 |
| 26    | OB1       | Output bridge B1                 |
| 24    | OA2       | Output bridge A2                 |
| 21    | OA1       | Output bridge A1                 |
| 23    | BRA       | Bridge sense resistor A          |
| 27    | BRB       | Bridge sense resistor B          |
| 7     | SPREAD    | SpreadCycle/StealthChop select   |
| 20    | STDBY     | Standby                          |

## Module Connector Pinout (J1 and J2 headers on module)
### J1 (Left side, active pins):
| Pin | Name     | Function            |
|-----|----------|---------------------|
| 1   | EN       | Enable              |
| 2   | MS1      | Microstep setting   |
| 3   | MS2      | Microstep setting   |
| 4   | PDN_UART | UART communication  |
| 5   | PDN_UART | UART communication  |
| 6   | CLK      | Clock               |
| 7   | STEP     | Step pulse input    |
| 8   | DIR      | Direction input     |

### J2 (Right side, motor/power):
| Pin | Name   | Function         |
|-----|--------|------------------|
| 1   | VM     | Motor voltage    |
| 2   | GND    | Ground           |
| 3   | A2     | Motor Phase A    |
| 4   | A1     | Motor Phase A    |
| 5   | B1     | Motor Phase B    |
| 6   | B2     | Motor Phase B    |
| 7   | VCC_IO | Logic voltage    |
| 8   | GND    | Ground           |

### Additional Module Pins:
- DIAG: Diagnostic/StallGuard output (directly accessible via header)
- VREF: Analog reference (accessible via potentiometer)
- INDEX: Configurable index pulse output

## SPREAD Pin Configuration
| Mode          | SPREAD |
|---------------|--------|
| StealthChop   | 0 (LOW) |
| SpreadCycle   | 1 (HIGH) |

## Microstep Configuration
| MS1 | MS2 | Microstep |
|-----|-----|-----------|
| 0   | 0   | 1/8       |
| 1   | 0   | 1/2       |
| 0   | 1   | 1/4       |
| 1   | 1   | 1/16      |

Note: MicroPlyer interpolation provides up to 256 microsteps regardless of MS pin setting.

## Vref / Current Setting
- Vref range: 0.2V — 2.2V
- Factory default Vref: 1.2V ±0.1V
- Factory default current: 0.9A
- Clockwise potentiometer rotation: decreases Vref → decreases current
- Counter-clockwise rotation: increases Vref → increases current

## Key External Components (from schematic)
- C2: 100nF/50V (VDD decoupling)
- C5: 4.7nF (GND)
- C1: 1nF/50V, C3/C4: 100nF/50V (VS decoupling)
- C6: 100nF, C7: 22nF/50V (charge pump)
- R1: 20K (EN pull-up)
- R3: 100K (CLK), R4: 100K (PDN_UART)
- R5: (VREF setting)
- R6: 20K, R7: 20K (address resistors)
- 0R11 sense resistors on BRA/BRB

---

# 5. BIGTREETECH TMC2209 V1.2 — DRIVER MODULE MANUAL

## Product Specs
- Continuous drive current: 2A RMS
- Peak current: 2.8A
- Voltage range: 4.75V — 28V DC
- Module size: 15.24mm x 20.32mm
- Operating modes: STEP/DIR or UART
- Microstep settings: 2, 4, 8, 16 (interpolated up to 256)

## Technologies
- SpreadCycle: high-dynamic motor control chopper
- StealthChop2: ultra-quiet operation
- MicroPlyer: microstep interpolation to 256
- StallGuard4: stall/locked-motor detection
- CoolStep: dynamic current control (up to 75% energy savings)

## UART Mode Setup
- Factory default: UART connected to pin 4 (PDN_UART on left header)
- To use pin 5 as UART: remove resistor and re-solder to alternate pads

## StealthChop vs SpreadCycle
- Factory default: StealthChop (mute/quiet mode)
- To switch to SpreadCycle (anti-shake mode): remove resistor and re-solder to alternate pads

## Safety Precautions
1. Disconnect power before installing/removing driver
2. Confirm driver orientation before insertion (prevent reverse insertion)
3. Verify wiring sequence and I/O connections
4. Do not hot-swap driver modules
5. Ensure heatsink does not short any pins
6. Handle with ESD precautions
7. Always provide heat dissipation (heatsink + fan recommended)

---

# 6. STEPPER MOTOR — 17HS19-2004D-E1K (StepperOnline)

## Motor Specifications
| Parameter                        | Value                     |
|----------------------------------|---------------------------|
| Connection                       | Bipolar                   |
| Amps/Phase                       | 2.00A                     |
| Resistance/Phase @ 25°C         | 1.10Ω ±10%               |
| Inductance/Phase @ 1kHz         | 1.70mH ±20%              |
| Holding Torque                   | 0.52 Nm [4.60 lb-in]     |
| Step Angle                       | 1.8°                      |
| Step Accuracy (non-accumulative) | ±5.00%                    |
| Rotor Inertia                    | 82.00 g·cm²              |
| Weight                           | 0.50 kg [1.10 lb]        |
| Ambient Temperature              | -10°C to 50°C [14°F–122°F] |
| Insulation Class                 | B (130°C / 266°F)        |
| Temperature Rise                 | MAX 80°C                  |

## Motor Wiring (Bipolar)
| Pin | Bipolar | Lead Color | Winding |
|-----|---------|------------|---------|
| 1   | A+      | BLK (Black)| A+      |
| 2   | A-      | GRN (Green)| A-      |
| 3   | B+      | RED        | B+      |
| 4   | B-      | BLU (Blue) | B-      |

## Motor Cable
- TRVV 4×0.2² black servo towline cable
- Length: 500±10mm
- Heat shrink tube at motor junction

## Encoder Specifications
- Resolution: 1000 PPR (4000 CPR with quadrature)
- Cable: TRVVP 6×0.15² black shielded wire
- Cable length: 500±10mm

## Encoder Wiring
| Lead Color | Description |
|------------|-------------|
| RED        | VCC (power) |
| WHT (White)| GND         |
| BLK (Black)| EA+ (Encoder A+) |
| BLU (Blue) | EA- (Encoder A-) |
| YEL (Yellow)| EB+ (Encoder B+) |
| GRN (Green)| EB- (Encoder B-) |

## Mechanical Dimensions (mm)
- Body: 42.3 MAX × 42.3 MAX (NEMA 17)
- Flange: 31±0.1 × 31±0.1
- Mounting holes: 4× M3, depth 4.5mm
- Shaft diameter: Ø5 (0 / -0.012mm tolerance)
- Shaft length: 24±1mm (from flange)
- Total length with encoder: 68±1mm
- Shaft flat: 15±0.2mm from flange, width 2mm
- Rear shaft extension: 9.6mm, width 32mm

## Full Step Sequence (2-phase, facing mounting end)
| Step | A+ | B+ | A- | B- | Direction |
|------|----|----|----|----|-----------|
| 1    | +  | +  | -  | -  | CCW       |
| 2    | -  | +  | +  | -  | ↓         |
| 3    | -  | -  | +  | +  | ↓         |
| 4    | +  | -  | -  | +  | CW        |

## Operating Limits
- Dynamic axial load: 10N max
- Dynamic radial load @ shaft 20mm: 21N max
- TMBF: 6000h or more @ 24V/300RPM
- Insulation resistance: 100 MΩ (normal temp/humidity)
- Dielectric strength: 500VAC for 1 min (between coils and case)

---

# 7. APPENDIX — COMMON DRIVER MICROSTEPPING TABLES

## A4988 (max 16 microsteps, 35V 2A)
| MS1 | MS2 | MS3 | Microstep  |
|-----|-----|-----|------------|
| L   | L   | L   | Full Step  |
| H   | L   | L   | 1/2        |
| L   | H   | L   | 1/4        |
| H   | H   | L   | 1/8        |
| H   | H   | H   | 1/16       |

Current formula: Imax = Vref / (8 × Rs), Rs = 0.1Ω

## LV8729 (max 128 microsteps, 36V 1.8A)
| MD3 | MD2 | MD1 | Microstep  |
|-----|-----|-----|------------|
| L   | L   | L   | Full Step  |
| L   | L   | H   | 1/2        |
| L   | H   | L   | 1/4        |
| L   | H   | H   | 1/8        |
| H   | L   | L   | 1/16       |
| H   | L   | H   | 1/32       |
| H   | H   | L   | 1/64       |
| H   | H   | H   | 1/128      |

Current formula: Iout = (Vref / 5) / Rf1, Rs = 0.22Ω

## DRV8825 (max 32 microsteps, 8.2V–45V 2.5A @ 24V T=25°C)
| MODE2 | MODE1 | MODE0 | Microstep  |
|-------|-------|-------|------------|
| L     | L     | L     | Full Step  |
| L     | L     | H     | 1/2        |
| L     | H     | L     | 1/4        |
| L     | H     | H     | 1/8        |
| H     | L     | L     | 1/16       |
| H     | L     | H     | 1/32       |
| H     | H     | L     | 1/32       |
| H     | H     | H     | 1/32       |

Current formula: I_CHOP = V_REFX / (5 × R_ISENSE), Rs = 0.1Ω

## TMC2209 (on module, for reference)
| MS1 | MS2 | Microstep |
|-----|-----|-----------|
| 0   | 0   | 1/8       |
| 1   | 0   | 1/2       |
| 0   | 1   | 1/4       |
| 1   | 1   | 1/16      |

All modes interpolated to 256 microsteps via MicroPlyer.
