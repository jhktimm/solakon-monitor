# solakonOne

C++-Bibliothek und CLI-Tool zum Auslesen eines **Solakon ONE** Hybrid-Wechselrichters über **Modbus TCP**.

## Funktionen

- **Modbus-TCP-Client** — Low-Level-Zugriff auf Holding/Discrete Registers (CRC-Prüfung, Big-Endian)
- **Geräte-Abstraktion** — Liest Inverter-Info, Zähler-Daten (Meter1/2), BMS-Daten und Energiebilanzen
- **CLI-Tool** — Ausgabe als CSV, JSON oder JSONL (Zeilenweise)
- **Test-Suite** — Catch2-Tests für CRC, Datenstrukturen, Formatierung

## Build

```bash
meson setup build
ninja -C build
```

### Tests

```bash
ninja -C build test
# oder: ./build/solakonOne_test
```

### Coverage

```bash
ninja -C build coverage-c
open build/coverage-report/index.html
```

## Installieren

```bash
sudo dpkg -i solakonone_1.0.0_amd64.deb
```

## Verwendung

```bash
# CSV-Ausgabe (Standard)
solakonOne 192.168.178.121

# JSON-Ausgabe
solakonOne 192.168.178.121 --format json

# JSONL (jede Messung eine Zeile)
solakonOne 192.168.178.121 --format jsonl --interval 5

# Hilfe
solakonOne --help
```

## Register-Adressen

Alle Adressen und Datentypen basieren auf *Solakon ONE Modbus Protocol v02/26*.
Korrekte Adressen und Skalierungsfaktoren sind in `include/solakon_device.h` definiert.

## Projektstruktur

```
meson.build           # Build-Konfiguration
include/
  modbus_client.h     # Modbus-TCP-Client (Header)
  solakon_device.h    # Geräte-Abstraktion (Header)
  ui.h                # CLI-Ausgabe (Header)
src/
  modbus_client.cpp   # Modbus-TCP-Client (Implementierung)
  solakon_device.cpp  # Geräte-Abstraktion (Implementierung)
  main.cpp            # CLI-Hauptprogramm
  ui.cpp              # Ausgabe-Formatierung
tests/
  test_main.cpp       # Catch2-Test-Hauptprogramm
  test_modbus_crc.cpp # CRC-Tests
  test_data_structures.cpp # Struktur-Tests
  test_ui_formatting.cpp # Formatierungs-Tests
Makefile              # Convenience-TARGETS (build, test, clean, debian, etc.)
```

## Lizenz

Private Nutzung.
