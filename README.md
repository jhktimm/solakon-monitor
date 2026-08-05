# Solakon ONE Monitor

Echtzeit-Monitor für Solakon ONE Hybrid-Wechselrichter über Modbus-TCP.

## Funktionen

- Spannungen, Ströme, Leistungen (3-phasig)
- Batteriestatus (SoC, Spannung, Temperatur)
- Energiezähler (PV, Ladung, Entladung, Verbrauch)
- BMS-Daten (State of Charge, Spannung, Strom, Temperatur)

## Verwendung

```bash
# Einmaliges Auslesen
./solakonOne 192.168.178.121 --once

# Kontinuierlicher Modus (1 Hz)
./solakonOne 192.168.178.121

# Mit Interface-Name
./solakonOne 192.168.178.121 --interface eth0
```

## Bau

```bash
meson setup build
ninja -C build
```

## Debian-Paket

```bash
dpkg-buildpackage -us -uc -b
```

## Autor

Jan Timm <jhktimm@gmail.com>
