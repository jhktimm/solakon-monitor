#!/usr/bin/env python3
"""Read and display Solakon ONE data from the actual registers."""
import socket, struct, sys

def build_req(trans_id, start, count, unit=1):
    return struct.pack('>HHHBBHH', trans_id, 0, 6, unit, 3, start, count)

def parse_resp(data):
    fc = data[7]
    if fc == 0x83:
        return 'EXCEPTION ' + str(data[9]), None
    bc = data[8]
    vals = []
    for i in range(bc // 2):
        vals.append(struct.unpack('>H', data[9+i*2:9+i*2+2])[0])
    return fc, vals

def read_u32(data, offset):
    return (data[offset] << 8 | data[offset+1]) << 16 | (data[offset+2] << 8 | data[offset+3])

def read_i32(data, offset):
    v = read_u32(data, offset)
    if v & (1 << 31):
        v -= (1 << 32)
    return v

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(5)
sock.connect(('192.168.178.121', 502))

def read_regs(start, count):
    req = build_req(1, start, count)
    sock.sendall(req)
    resp = sock.recv(1024)
    fc, vals = parse_resp(resp)
    if isinstance(fc, str):
        return None
    return vals

print("=" * 70)
print("SOLAKON ONE - Modbus Register Readout")
print("=" * 70)

# Inverter Info
print("\n--- Inverter Info (Table 2-1/2-2) ---")
info = read_regs(39000, 100)
if info:
    print(f"  Protocol Version (39000): {read_u32(info, 0)}")
    model = ''.join(chr(info[i*2] >> 8) + chr(info[i*2] & 0xFF) for i in range(16)).rstrip('\x00 ').strip()
    print(f"  Model Name (39002): '{model}'")
    pn = ''.join(chr(info[i*2+18] >> 8) + chr(info[i*2+18] & 0xFF) for i in range(16)).rstrip('\x00 ').strip()
    print(f"  PN (39018): '{pn}'")
    print(f"  Model ID (39050): {info[50]}")
    print(f"  Num Strings (39051): {info[51]}")
    print(f"  Num MPPT (39052): {info[52]}")
    rated = read_i32(info, 53)
    print(f"  Rated Power (39053): {rated} (kW={rated/1000:.3f})")
    max_act = read_i32(info, 55)
    print(f"  Max Active (39055): {max_act} (kW={max_act/1000:.3f})")
    print(f"  Status1 (39063): {info[63]}")
    print(f"  Status3 (39065): {read_u32(info, 65)}")
    print(f"  Alarm1 (39066): {info[66]}")
    print(f"  Alarm2 (39067): {info[67]}")

# Meter1
print("\n--- Meter1/CT1 (Table 2-4) ---")
m1 = read_regs(38800, 64)
if m1:
    print(f"  Conn (38801): {m1[1]}")
    r_v = read_i32(m1, 2)
    print(f"  R-Voltage (38802): {r_v} ({r_v/10:.1f} V)")
    s_v = read_i32(m1, 4)
    print(f"  S-Voltage (38804): {s_v} ({s_v/10:.1f} V)")
    t_v = read_i32(m1, 6)
    print(f"  T-Voltage (38806): {t_v} ({t_v/10:.1f} V)")
    r_c = read_i32(m1, 8)
    print(f"  R-Current (38808): {r_c} ({r_c/1000:.3f} A)")
    s_c = read_i32(m1, 10)
    print(f"  S-Current (38810): {s_c} ({s_c/1000:.3f} A)")
    t_c = read_i32(m1, 12)
    print(f"  T-Current (38812): {t_c} ({t_c/1000:.3f} A)")
    r_p = read_i32(m1, 14)
    print(f"  R-Power (38814): {r_p} ({r_p/10:.1f} W)")
    s_p = read_i32(m1, 16)
    print(f"  S-Power (38816): {s_p} ({s_p/10:.1f} W)")
    t_p = read_i32(m1, 18)
    print(f"  T-Power (38818): {t_p} ({t_p/10:.1f} W)")
    comb_p = read_i32(m1, 38)
    print(f"  Combined Power (38838): {comb_p} ({comb_p/10:.1f} W)")
    freq = read_i32(m1, 44)
    print(f"  Frequency (38844): {freq} ({freq/100:.2f} Hz)")

# Meter2
print("\n--- Meter2/CT2 ---")
m2 = read_regs(38900, 64)
if m2:
    print(f"  Conn (38901): {m2[1]}")
    r_v = read_i32(m2, 2)
    print(f"  R-Voltage (38902): {r_v} ({r_v/10:.1f} V)")
    comb_p = read_i32(m2, 38)
    print(f"  Combined Power (38938): {comb_p} ({comb_p/10:.1f} W)")
    freq = read_i32(m2, 46)
    print(f"  Frequency (38946): {freq} ({freq/100:.2f} Hz)")

# BMS
print("\n--- BMS (Table 2-3) ---")
bms = read_regs(37000, 64)
if bms:
    print(f"  BMS1 (37002): {bms[2]}")
    print(f"  Main Ctrl (37003): {bms[3]}")
    sn = ''.join(chr(bms[i*2+5] >> 8) + chr(bms[i*2+5] & 0xFF) for i in range(16)).rstrip('\x00 ').strip()
    print(f"  SN (37005): '{sn}'")
    print(f"  Slave Count (37032): {bms[32]}")
    print(f"  Voltage (37609): {bms[609]} ({bms[609]/10:.1f} V)")
    cur = bms[610]
    if cur & 0x8000:
        cur = cur - 0x10000
    print(f"  Current (37610): {cur} ({cur/10:.1f} A)")
    temp = bms[611]
    if temp & 0x8000:
        temp = temp - 0x10000
    print(f"  Temp (37611): {temp} ({temp/10:.1f} °C)")
    print(f"  SoC (37612): {bms[612]}%")
    print(f"  SoH (37624): {bms[624]}%")

# Energy
print("\n--- Energy Totals (Table 2-6) ---")
en = read_regs(39600, 32)
if en:
    pv = read_u32(en, 0)
    print(f"  PV Total Power (39600): {pv} W")
    chg_cap = read_u32(en, 3)
    print(f"  Total Charge Cap (39603): {chg_cap} ({chg_cap/10:.1f} kWh)")
    chg_today = read_u32(en, 5)
    print(f"  Total Charge Today (39605): {chg_today} ({chg_today/10:.1f} kWh)")
    dischg = read_u32(en, 7)
    print(f"  Total Discharge (39607): {dischg} ({dischg/10:.1f} kWh)")
    dischg_today = read_u32(en, 9)
    print(f"  Total Discharge Today (39609): {dischg_today} ({dischg_today/10:.1f} kWh)")
    feeder = read_u32(en, 11)
    print(f"  Total Feeder (39611): {feeder} ({feeder/10:.1f} kWh)")
    feeder_today = read_u32(en, 13)
    print(f"  Total Feeder Today (39613): {feeder_today} ({feeder_today/10:.1f} kWh)")
    consumption = read_u32(en, 15)
    print(f"  Total Consumption (39615): {consumption} ({consumption/10:.1f} kWh)")
    consumption_today = read_u32(en, 17)
    print(f"  Total Consumption Today (39617): {consumption_today} ({consumption_today/10:.1f} kWh)")
    output = read_u32(en, 19)
    print(f"  Total Output (39619): {output} ({output/10:.1f} kWh)")
    output_today = read_u32(en, 21)
    print(f"  Total Output Today (39621): {output_today} ({output_today/10:.1f} kWh)")
    load = read_u32(en, 23)
    print(f"  Total Load (39623): {load} ({load/10:.1f} kWh)")
    load_today = read_u32(en, 25)
    print(f"  Total Load Today (39625): {load_today} ({load_today/10:.1f} kWh)")

sock.close()
print("\n" + "=" * 70)
print("Done.")
