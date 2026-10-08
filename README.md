# Solar Battery Monitoring and Protection System

A PIC16F877A-based embedded system for monitoring battery voltage, load current, and solar charging current, with automatic load protection and charging control.

## Project Overview

This project implements a simulated solar battery monitoring and protection system using a PIC16F877A microcontroller.

The system monitors:

- Battery voltage
- Load current
- Solar charging current
- Charging status

The controller provides:

- Low-battery load protection
- High-battery voltage indication
- Automatic charging cutoff
- Automatic charging resume
- Load overcurrent protection
- Automatic retry after overcurrent
- Fault-lock after repeated failures
- Manual fault reset
- LCD monitoring
- LED and buzzer status indication

The system was designed and tested in Proteus using embedded C, MPLAB X IDE, and the XC8 compiler.

---

## System Architecture

```text
                 SOLAR SOURCE
                      |
                      v
               Charging Relay
                      |
                   ACS712
                      |
                      v
                   BATTERY
                      |
              +-------+-------+
              |               |
              v               v
         Voltage ADC      Load Relay
              |               |
              |            ACS712
              |               |
              |               v
              |              LOAD
              |            (Motor)
              |
              v
          PIC16F877A
              |
      +-------+--------+
      |       |        |
     LCD     LEDs    Buzzer
```

---

## Main Components

| Component | Purpose |
|---|---|
| PIC16F877A | Main microcontroller |
| 16x2 LCD | Measurements and system status |
| ACS712 | Load and solar current sensing |
| Resistor divider | Battery voltage sensing |
| BC547 | Relay and buzzer drivers |
| 12 V relay | Load and charging control |
| 1N4007 | Relay flyback protection |
| LEDs | Battery status indication |
| Buzzer | Protection indication |
| DC motor | Simulated load |
| 18 V DC source | Simplified solar source |

---

## Pin Configuration

| PIC Pin | Function |
|---|---|
| RA0 / AN0 | Battery voltage ADC |
| RA1 / AN1 | Load current ADC |
| RA3 / AN3 | Solar charging current ADC |
| RB0 | Low-battery LED |
| RB1 | LCD RS |
| RB2 | LCD Enable |
| RB3 | Normal-status LED |
| RB4-RB7 | LCD D4-D7 |
| RC0 | Load relay control |
| RC1 | High-battery LED |
| RC2 | Buzzer control |
| RC3 | Solar charging relay control |
| RD0 | Manual fault-reset button |

---

## Battery Voltage Measurement

A resistor divider reduces the battery voltage before it reaches the PIC ADC.

```text
Battery + ---- 30 kΩ ----+---- 10 kΩ ---- GND
                         |
                        AN0
```
The divider ratio is:

```text
V_ADC = V_BAT × 10 / (30 + 10)
      = 0.25 × V_BAT
```
Therefore, a 12 V battery produces approximately 3 V at the ADC input.
The PIC16F877A uses a 10-bit ADC with a 5 V reference:

```text
V_ADC = ADC_value × 5000 / 1023 mV
```
Therefore:

```text
V_BAT = ADC_value × 20000 / 1023 mV
```
---

## ACS712 Current Measurement

Two ACS712 current sensors are used:

- One for load current
- One for solar charging current

The sensor model uses approximately:

- Zero-current output: 2.5 V
- Sensitivity: 185 mV/A

Sensor voltage:

```text
V_sensor = ADC_value × 5000 / 1023 mV
```
Current:
```text
I = (V_sensor − 2500) / 185 A
```
A small zero-current deadband is used to remove minor sensor offsets.

---

## Protection Parameters

| Parameter | Value |
|---|---:|
| Low-battery threshold | 10.5 V |
| High-battery threshold | 14.4 V |
| Charging stop voltage | 14.4 V |
| Charging resume voltage | 13.8 V |
| Overcurrent threshold | 1.5 A |
| Cooldown period | 3 s |
| Maximum retries | 3 |

These values are design/simulation values and would need to be adjusted according to the selected battery chemistry in a physical implementation.

---

## Charging Control

Charging uses voltage hysteresis.

Charging is stopped when:

```text
Battery voltage ≥ 14.4 V
```
Charging resumes when:
```text
Battery voltage ≤ 13.8 V
```
The hysteresis helps prevent rapid relay switching near the charging limit.
The simplified simulation uses an 18 V DC source and a 10 Ω series resistor.
Expected charging current at a 12 V battery voltage:
```text
I = (18 − 0.7 − 12) / 10
  ≈ 0.53 A
```
The simulated charging current was approximately 0.51-0.54 A.

---

## Load Overcurrent Protection

The load current is continuously monitored using an ACS712 sensor.

When:

```text
Load current ≥ 1.5 A
```
the controller:
1. Disconnects the load.
2. Enters a 3-second cooldown period.
3. Attempts automatic reconnection.
4. Checks the current again.
5. Repeats the process if necessary.
6. Enters fault-lock after three failed retries.
The fault can then be cleared using the manual reset button connected to RD0.

---

## Battery Protection

### Low Battery

When:

```text
Battery voltage < 10.5 V
```
the load relay is disconnected and the low-battery LED is activated.
### Normal Battery
Within the normal voltage range, the normal-status LED is activated.
### High Battery
When:
```text
Battery voltage > 14.4 V
```
the high-battery LED is activated and charging is stopped.

---

## LCD Display

During normal operation, the LCD alternates between:

### Battery / Load

- Battery voltage
- Load current

### Solar / Charging

- Solar charging current
- Charging status

Protection messages override the normal monitoring display when necessary.

---

## Software

### Development Environment

- MPLAB X IDE v6.35
- XC8 Compiler v4.00
- PIC16F877A
- 4 MHz crystal oscillator
- Embedded C

### Main Software Functions

The firmware includes:

- LCD initialization and communication
- ADC initialization and reading
- Battery voltage conversion
- ACS712 current conversion
- Battery status control
- Solar charging control
- Load overcurrent protection
- Automatic retry logic
- Fault-lock and manual reset
- Buzzer control
- LCD monitoring display

---

## Testing and Results

| Test | Result |
|---|---|
| 12 V battery | Normal operation |
| Load motor ON | Approximately 1.06 A |
| Solar charging | Approximately 0.54 A |
| 10 V battery | Load disconnected |
| 14.5 V battery | Charging disconnected |
| Overcurrent ≥ 1.5 A | Load relay disconnected |
| Repeated overcurrent | Automatic retry sequence |
| Three failed retries | Fault-lock |
| Manual reset | Fault cleared successfully |
| Charging cutoff | Verified |
| Charging resume | Verified |

---

## Limitations

- The project was tested in Proteus simulation rather than on physical hardware.
- An 18 V DC source represents the solar source in the simulation.
- No MPPT algorithm is implemented.
- A series resistor is used to limit the simulated charging current.
- ACS712 simulation behavior may differ from physical hardware.
- Battery thresholds must be adjusted according to battery chemistry.
- Some control operations use blocking delays.

A physical implementation would require appropriate battery-management, protection, PCB, component-rating, thermal, and electrical-safety considerations.

---

## Project Status

**Completed and simulated successfully.**

The repository contains the MPLAB X source project, Proteus simulation files, and project documentation.

---

## Repository Structure

```text
solar-battery-monitoring-protection/
│
├── source-code/
│   └── SolarBatteryMonitor.X/
│
├── proteus/
│   └── SolarBatteryMonitor/
│
├── documentation/
│   └── Solar_Battery_Monitoring_and_Protection.docx
│
├── .gitignore
└── README.md