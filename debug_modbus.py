#!/usr/bin/env python3
"""Debug Modbus TCP communication with Solakon ONE."""
import socket
import struct

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

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)
sock.connect(('192.168.178.121', 502))

print("=== Register Scan (39000-39099) ===")
req = build_req(1, 39000, 100)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            ascii_s = ''
            c1 = (v >> 8) & 0xFF
            c2 = v & 0xFF
            if 32 <= c1 <= 126: ascii_s += chr(c1)
            if 32 <= c2 <= 126: ascii_s += chr(c2)
            print(f'  39{39000+i:04d}: {v:5d} (0x{v:04X}) "{ascii_s}"')

print("\n=== Energy/Power Registers (39600-39631) ===")
req = build_req(2, 39600, 32)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  39{39600+i:04d}: {v:5d} (0x{v:04X})')

print("\n=== Meter1 Registers (38800-38863) ===")
req = build_req(3, 38800, 64)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  38{38800+i:04d}: {v:5d} (0x{v:04X})')

print("\n=== Meter2 Registers (38900-38963) ===")
req = build_req(4, 38900, 64)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  38{38900+i:04d}: {v:5d} (0x{v:04X})')

print("\n=== BMS Registers (37000-37063) ===")
req = build_req(5, 37000, 64)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            ascii_s = ''
            c1 = (v >> 8) & 0xFF
            c2 = v & 0xFF
            if 32 <= c1 <= 126: ascii_s += chr(c1)
            if 32 <= c2 <= 126: ascii_s += chr(c2)
            print(f'  37{37000+i:04d}: {v:5d} (0x{v:04X}) "{ascii_s}"')

print("\n=== What the code reads (I32 interpretation) ===")
# Simulate what the code reads
code_reads = [
    ('RATED_POWER (I32 @ 39053)', 39053, 2, 1000.0),
    ('MAX_ACTIVE (I32 @ 39055)', 39055, 2, 1000.0),
    ('MAX_APPARENT (I32 @ 39057)', 39057, 2, 1000.0),
    ('MAX_REACTIVE_NEG (I32 @ 39059)', 39059, 2, 1000.0),
    ('PV_TOTAL_POWER (U32 @ 39600)', 39600, 2, 1.0),
    ('TOTAL_CHARGE_CAP (U32 @ 39603)', 39603, 2, 10.0),
    ('METER1_CONN (U16 @ 38801)', 38801, 1, 1.0),
    ('METER1_R_VOLTAGE (I32 @ 38802)', 38802, 2, 10.0),
    ('METER1_COMBINED_POWER (I32 @ 38838)', 38838, 2, 10.0),
    ('METER1_FREQ (I32 @ 38844)', 38844, 2, 100.0),
    ('BMS1 (U16 @ 37002)', 37002, 1, 1.0),
    ('BMS1_VOLTAGE (U16 @ 37609)', 37609, 1, 10.0),
    ('BMS1_CURRENT (I16 @ 37610)', 37610, 1, 10.0),
    ('BMS1_SOC (U16 @ 37612)', 37612, 1, 1.0),
]

for name, addr, count, divisor in code_reads:
    req = build_req(1, addr, count)
    sock.sendall(req)
    resp = sock.recv(1024)
    fc, vals = parse_resp(resp)
    if isinstance(fc, str):
        print(f'{name}: {fc}')
        continue
    if count == 1:
        val = vals[0]
        display = val / divisor if divisor != 1 else val
        print(f'{name}: raw={vals[0]} -> {display:.4f}')
    else:
        i32 = (vals[0] << 16) | vals[1]
        if i32 & (1 << 31):
            i32 -= (1 << 32)  # sign extend
        display = i32 / divisor
        print(f'{name}: raw=0x{vals[0]:04X}0x{vals[1]:04X} -> {i32} -> {display:.4f}')

sock.close()
