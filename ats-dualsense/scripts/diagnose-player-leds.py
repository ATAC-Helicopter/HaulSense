#!/usr/bin/python3
"""Explicit, reversible isolation test. Requires ATS parked/paused by the user."""
import argparse
import fcntl
import struct
import os
from pathlib import Path
import signal
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--steam-pid', type=int, required=True)
parser.add_argument('--device', default='/dev/hidraw4')
parser.add_argument('--game-paused', action='store_true', required=True)
args = parser.parse_args()
process = Path('/proc') / str(args.steam_pid)
if process.stat().st_uid != os.getuid() or process.joinpath('comm').read_text().strip() != 'steam':
    parser.error('PID must identify your running Steam client')
if Path(args.device).parent != Path('/dev') or not Path(args.device).name.startswith('hidraw'):
    parser.error('expected /dev/hidrawN')
# This diagnostic intentionally supports USB Sony controllers only.
with open(args.device, 'rb', buffering=0) as device:
    info = bytearray(8)
    fcntl.ioctl(device.fileno(), 0x80084803, info, True)  # HIDIOCGRAWINFO
    bus, vendor, product = struct.unpack('Ihh', info)
    if bus != 3 or vendor != 0x054c or product not in (0x0ce6, 0x0df2):
        parser.error('device must be a Sony DualSense/Edge over USB')
# Keep a separate systemd timer capable of resuming Steam even if this test dies.
unit = f'haulsense-steam-resume-{args.steam_pid}'
subprocess.run(['systemd-run', '--user', '--collect', '--unit', unit, '--on-active=25s',
                '--timer-property=AccuracySec=1s', '/bin/kill', '-CONT', str(args.steam_pid)], check=True)
fd = None
was_active = subprocess.run(['systemctl', '--user', 'is-active', '--quiet', 'haulsense.service']).returncode == 0

def interrupt(_signal, _frame):
    raise KeyboardInterrupt

for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
    signal.signal(sig, interrupt)

try:
    fd = os.open(args.device, os.O_RDWR | os.O_NONBLOCK)
    subprocess.run(['systemctl', '--user', 'stop', 'haulsense.service'], check=True)
    os.kill(args.steam_pid, signal.SIGSTOP)
    for mask, duration, label in [(0x18, 8, 'LEFT PAIR'), (0x03, 8, 'RIGHT PAIR')]:
        report = bytearray(63)
        report[0] = 0x02
        report[2] = 0x10
        report[44] = mask | 0x20
        print(f'{label}: player mask 0x{mask:02x}; one USB report', flush=True)
        if os.write(fd, report) != len(report):
            raise OSError('Incomplete HID output report')
        time.sleep(duration)
finally:
    # Resume input before attempting any secondary cleanup.
    try:
        os.kill(args.steam_pid, signal.SIGCONT)
    finally:
        try:
            if fd is not None:
                try:
                    report = bytearray(63)
                    report[0] = 0x02
                    report[2] = 0x10
                    report[44] = 0x20
                    os.write(fd, report)
                finally:
                    os.close(fd)
        finally:
            try:
                if was_active:
                    subprocess.run(['systemctl', '--user', 'start', 'haulsense.service'], check=True)
            finally:
                subprocess.run(['systemctl', '--user', 'stop', unit + '.timer'], check=False)
                print('Steam resumed; previous telemetry service state restored.', flush=True)
