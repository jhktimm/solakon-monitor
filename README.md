# solakon-monitor

Real-time terminal monitoring for **Solakon ONE** hybrid solar inverters via **Modbus TCP**.

## Quick Start

### Find the Solakon ONE IP

```bash
# Scan your network for the Solakon ONE (port 502 = Modbus)
nmap -p 502 192.168.178.0/24

# Or use your router's DHCP client list
# Look for devices named "Solakon" or vendor "Solakon"
```

### Copy-Paste Setup & Run

```bash
# 1. Clone
git clone https://github.com/YOURUSER/solakon-monitor.git
cd solakon-monitor

# 2. Build
meson setup build && ninja -C build

# 3. Run (replace IP with your Solakon ONE)
./build/solakon-monitor 192.168.178.121
```

## Features

- **Modbus-TCP-Client** — Low-level access to Holding/Input Registers (CRC check, Big-Endian)
- **Device abstraction** — Reads inverter info, meter data (Meter1/2), BMS data, energy statistics
- **Terminal monitor** — btop-like real-time view in your terminal
- **Test suite** — Catch2 tests for CRC, data structures, formatting

## Build

```bash
meson setup build
ninja -C build
```

### With Make

```bash
make build    # Build
make test     # Build + tests
make clean    # Clean
make run      # Start monitor
make run-once # Single snapshot
```

### Tests

```bash
ninja -C build test
# or: ./build/solakon-monitor_test
```

## Usage

```bash
# Start monitor (default IP: 192.168.178.121)
./build/solakon-monitor

# With custom IP
./build/solakon-monitor 192.168.178.121

# With interval (Hz)
./build/solakon-monitor 192.168.178.121 --interval 2

# Single snapshot
./build/solakon-monitor --once

# JSON output (for Chrome plugins, dashboards, APIs)
./build/solakon-monitor --once --json

# Help
./build/solakon-monitor --help
```

### JSON Output

```bash
./build/solakon-monitor --once --json
```

Returns a JSON object with all inverter data:
```json
{
  "timestamp": 1722883200,
  "valid": true,
  "power": {
    "smart_meter_w": 1234.5,
    "ac_power_w": 2345.6,
    "pv_power_w": 3456.7,
    "battery_power_w": -500.0
  },
  "energy": {
    "charge_total_kwh": 1234.5,
    "charge_today_kwh": 12.3,
    "discharge_total_kwh": 1100.2,
    "discharge_today_kwh": 10.1,
    "feeder_total_kwh": 2000.0,
    "feeder_today_kwh": 15.5,
    "consumption_total_kwh": 1800.0,
    "consumption_today_kwh": 8.2,
    "output_total_kwh": 1500.0,
    "output_today_kwh": 5.0,
    "load_total_kwh": 1300.0,
    "load_today_kwh": 4.5
  },
  "battery": {
    "soc_pct": 75,
    "voltage_v": 48.5,
    "current_a": -10.2,
    "temperature_c": 25.3
  },
  "meter1": {
    "connected": true,
    "voltage_v": 230.5,
    "current_a": 5.43,
    "power_w": 1234.5
  },
  "meter2": {
    "connected": true,
    "voltage_v": 230.5,
    "current_a": 5.43,
    "power_w": 1234.5
  }
}
```

Useful for Chrome plugins, Grafana dashboards, or any HTTP-based monitoring tool.

### Monitor Keys

| Key | Action |
|-----|--------|
| `q` | Quit |

## Register Addresses

All addresses and data types are based on *Solakon ONE Modbus Protocol v02/26*.
Correct addresses and scaling factors are defined in `include/solakon_device.h`.

## Project Structure

```
meson.build           # Build configuration
Makefile              # Convenience targets (build, test, clean, run)
include/
  modbus_client.h     # Modbus-TCP client (header)
  solakon_device.h    # Device abstraction (header)
  ui.h                # Terminal UI (header)
src/
  modbus_client.cpp   # Modbus-TCP client (implementation)
  solakon_device.cpp  # Device abstraction (implementation)
  main.cpp            # Main program
  ui.cpp              # Terminal UI
tests/
  test_main.cpp       # Catch2 test main
  test_modbus_crc.cpp # CRC tests
  test_data_structures.cpp # Structure tests
  test_ui_formatting.cpp # Formatting tests
  test_helpers.hpp    # Test helpers
LICENSE               # GPL-3.0
```

## License

GPL-3.0

This project was developed with assistance from AI tools (Qwen 3.6).
The author retains all rights and licenses this work under GPL-3.0.

---

# solakon-monitor

C++-Tool zum Echtzeit-Monitoring eines **Solakon ONE** Hybrid-Wechselrichters über **Modbus TCP**.

## Funktionen

- **Modbus-TCP-Client** — Low-Level-Zugriff auf Holding/Input Registers (CRC-Prüfung, Big-Endian)
- **Geräte-Abstraktion** — Liest Inverter-Info, Zähler-Daten (Meter1/2), BMS-Daten und Energiebilanzen
- **Terminal-Monitor** — btop-ähnliche Echtzeit-Ansicht im Terminal
- **Test-Suite** — Catch2-Tests für CRC, Datenstrukturen, Formatierung

## Build

```bash
meson setup build
ninja -C build
```

### Mit Make

```bash
make build    # Build
make test     # Build + Tests
make clean    # Clean
make run      # Monitor starten
make run-once # Ein Snapshot
```

### Tests

```bash
ninja -C build test
# oder: ./build/solakon-monitor_test
```

## Verwendung

```bash
# Monitor starten (default IP: 192.168.178.121)
solakon-monitor

# Mit eigener IP
solakon-monitor 192.168.178.121

# Mit Intervall (Hz)
solakon-monitor 192.168.178.121 --interval 2

# Ein Snapshot
solakon-monitor --once

# Hilfe
solakon-monitor --help
```

### Tasten im Monitor

| Taste | Funktion |
|-------|----------|
| `q` | Beenden |

## Register-Adressen

Alle Adressen und Datentypen basieren auf *Solakon ONE Modbus Protocol v02/26*.
Korrekte Adressen und Skalierungsfaktoren sind in `include/solakon_device.h` definiert.

## Projektstruktur

```
meson.build           # Build-Konfiguration
Makefile              # Convenience-TARGETS (build, test, clean, run)
Makefile.nice         # Globale Makefile-Hilfsziele
include/
  modbus_client.h     # Modbus-TCP-Client (Header)
  solakon_device.h    # Geräte-Abstraktion (Header)
  ui.h                # Terminal-UI (Header)
src/
  modbus_client.cpp   # Modbus-TCP-Client (Implementierung)
  solakon_device.cpp  # Geräte-Abstraktion (Implementierung)
  main.cpp            # Hauptprogramm
  ui.cpp              # Terminal-UI
tests/
  test_main.cpp       # Catch2-Test-Hauptprogramm
  test_modbus_crc.cpp # CRC-Tests
  test_data_structures.cpp # Struktur-Tests
  test_ui_formatting.cpp # Formatierungs-Tests
  test_helpers.hpp    # Test-Hilfsfunktionen
LICENSE               # GPL-3.0
```

## Lizenz

GPL-3.0

Dieses Projekt wurde mit Unterstützung von KI-Tools (Qwen 3.6) entwickelt.
Der Autor behält alle Rechte und lizenziert dieses Werk unter GPL-3.0.

---

## Disclaimer

This software is provided "as is" without warranty of any kind.
The author assumes no liability for damages arising from the use or
inability to use this software. Use at your own risk.

Data obtained via Modbus TCP is for informational purposes only and
must not be used for safety-critical decisions. No guarantee is given
for the correctness, completeness, or timeliness of the data.

## Haftungsausschluss

Dieses Tool wird ohne jegliche Garantie oder Gewährleistung bereitgestellt.
Der Autor übernimmt keine Haftung für Schäden, die durch die Nutzung oder
Nichtnutzung dieser Software entstehen. Die Nutzung erfolgt auf eigene Gefahr.

Die über Modbus TCP ermittelten Daten dienen nur zur Information und dürfen
nicht für sicherheitsrelevante Entscheidungen verwendet werden. Für die
Korrektheit, Vollständigkeit und Aktualität der Daten wird keine Gewähr
übernommen.
