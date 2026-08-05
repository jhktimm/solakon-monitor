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
