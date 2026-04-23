# DATPO — Dual Airfoil Test Platform Octopus (Group 8, Yang)

Capstone project. A motorized dual-airfoil rig for wind tunnel testing, controlled over USB or Bluetooth serial with a browser-based GUI. Controls stagger (X), gap (Y), and angle of attack (ZA bot, ZB top) for both airfoils independently.

---

## Hardware

**Controller:** BTT Octopus V1.0 (STM32F446, 180MHz)

**Motors:** 8× NEMA 17 steppers (17HS19-2004D-E1K, 2A, 1.8°/step)
**Drivers:** TMC2209 in step/dir mode, 8 microsteps

| Axis        | Motors   | Pitch / Ratio         | Limit     |
|-------------|----------|-----------------------|-----------|
| X (stagger) | M0, M1   | 5mm/rev               | ±415mm    |
| Y (gap)     | M2–M5    | 2mm/rev               | ±225mm    |
| ZA (AoA bot)| M6       | 5.1975× gear ratio    | ±20°      |
| ZB (AoA top)| M7       | 5.1975× gear ratio    | ±20°      |

**Encoders:** 4× quadrature encoders (1000 PPR / 4000 CPR) on E0–E3, covering X left/right and Y left/right. Used for sync checking only — not closed-loop. E6/E7 slots in the GUI mirror ZA/ZB (see Known Issues).

**Fan:** PA8, on by default, toggle with `FAN` command.

---

## Firmware

Built with PlatformIO (Arduino framework). Source is `FIRMWARE/src/WORKING_MAIN.cpp`.

**You'll need:**
- VS Code + PlatformIO extension
- Serial Monitor extension (or use the GUI)
- Target board: `btt_octopus_f446`

**Key behaviors:**
- Accepts commands over USB (`Serial`) or Bluetooth (`Serial1`, 115200 baud both)
- Broadcasts `POS:` telemetry lines after every move and on `POSITIONS` — this is what the GUI parses
- X and Y encoder pairs are compared each loop cycle; if divergence exceeds 5mm, all motors stop and it auto-enters jog mode for realignment
- Motion is open-loop for ZA/ZB (no encoder feedback on AoA axes)

---

## GUI

`GUI/ENCODER_GUI.html` — open in Chrome, uses Web Serial API. No install needed.

`GUI/WORKING_GUI.html` is the older version, kept for reference.

---

## Serial Command Reference

| Command         | Description                                      |
|-----------------|--------------------------------------------------|
| `X[mm]`         | Move X axis to position (e.g. `X50`)            |
| `Y[mm]`         | Move Y axis to position                          |
| `ZA[deg]`       | Move AoA bot to angle (e.g. `ZA-5`)             |
| `ZB[deg]`       | Move AoA top to angle                            |
| `HOME`          | Move all axes to zero                            |
| `POSITIONS`     | Print current positions + POS: line              |
| `ZERO`          | Enter jog & zero mode                            |
| `ESTOP`         | Immediate stop, clears alarm                     |
| `RESUME`        | Clear sync alarm                                 |
| `FAN`           | Toggle fan                                       |
| `COMMANDS`      | Print command list                               |

**In jog/zero mode:** `X+` `X-` `Y+` `Y-` `YL+` `YL-` `YR+` `YR-` `ZA+` `ZA-` `ZB+` `ZB-`, individual motors `M0+` through `M7+/-`, `STEP[size]` (0.1/0.25/0.5/1.0/2.0/3.0), `SET` to zero, `EXIT` to leave.

---

## Known Issues & Lessons Learned

**AMT25 absolute encoders on ZA/ZB were attempted and abandoned.** The hardware and firmware implementation were correct, but we got unbeatable EMI from the motor cabling corrupting the encoder signal. The DIAG pins on the Octopus (used as encoder inputs) are physically adjacent to motor driver lines — not great for signal integrity.

**Encoder sync alarm is functional but conservative.** 5mm threshold works well in practice. If you're getting nuisance trips, check that encoder cables are routed away from motor wiring.

**ZA/ZB home angles are not set.** `ZA_HOME_ANGLE` and `ZB_HOME_ANGLE` in firmware are both 0.0 — the rig assumes the airfoils start at 0° on power-up. If you add hard stops, measure and set these.

---

## TODO (Next Group)

- [ ] **Integrate AMT25 absolute encoders on ZA/ZB** — the big one. Hardware and firmware were working; the blocker was EMI. Suggested approach: use shielded cable for encoder lines, route far from motor cabling, or add an external encoder interface board to put distance between the noisy motor drivers and the encoder signal path. The AMT25 uses SSI/SPI — an isolator IC between the Octopus and encoders would likely fix it. This would give true closed-loop AoA control, which is the main thing missing.
- [ ] Measure and set physical hard-stop home angles for ZA/ZB in firmware, automating zeroing.
- [ ] Y axis has 4 motors (M2–M5) but only 2 encoders, there are two existing quadrature encoders left unplugged & ready to implemented.
---

## Repo Layout

```
DATPO-G8-YANG/
├── FIRMWARE/       main firmware (PlatformIO project)
├── GUI/            browser-based serial GUI (Chrome)
├── CAD/            airfoil and zero-tool 3D files
├── DOCS/           manuals, datasheets, master reference, capstone poster
├── PINOUTS/        board and driver pinout images
├── MEDIA/          demo videos
├── ARCHIVE/        old firmware builds
└── TODO.txt        running notes
```

Full consolidated hardware reference (board pinout, driver config, motor specs) is in `DOCS/PROJECT_MASTER_REFERENCE.md`.

Claude was used as a major assistant for writing software.
