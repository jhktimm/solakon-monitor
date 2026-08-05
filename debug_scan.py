#!/usr/bin/env python3
"""Full register scan for Solakon ONE."""
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

print("=== Full scan 39000-39100 (all non-zero) ===")
req = build_req(1, 39000, 101)
sock.sendall(req)
resp = sock.recv(1024)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  {39000+i:05d}: {v:5d} (0x{v:04X})')

print("\n=== Full scan 39400-39700 (all non-zero) ===")
req = build_req(2, 39400, 301)
sock.sendall(req)
resp = sock.recv(2048)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  {39400+i:05d}: {v:5d} (0x{v:04X})')

print("\n=== PV power candidates: scan 39050-39100 ===")
req = build_req(3, 39050, 51)
sock.sendall(req)
resp = sock.recv(256)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        print(f'  {39050+i:05d}: {v:5d} (0x{v:04X})')

print("\n=== Check 39600-39631 (ALL values) ===")
req = build_req(4, 39600, 32)
sock.sendall(req)
resp = sock.recv(256)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        print(f'  {39600+i:05d}: {v:5d} (0x{v:04X})')

print("\n=== Check 39632-39700 (non-zero) ===")
req = build_req(5, 39632, 69)
sock.sendall(req)
resp = sock.recv(512)
fc, vals = parse_resp(resp)
if vals:
    for i, v in enumerate(vals):
        if v != 0:
            print(f'  {39632+i:05d}: {v:5d} (0x{v:04X})')

sock.close()
