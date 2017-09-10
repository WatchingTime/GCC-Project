#!/usr/bin/env python3
from pathlib import Path
import argparse
import hashlib
import platform
import subprocess
import sys

is_windows = platform.system() == 'Windows'

parser = argparse.ArgumentParser()
parser.add_argument('-a', '--arduino-path', type=Path, required=not is_windows, help='Arduino IDE path')
parser.add_argument('-c', '--check', action='store_true', help='read contents of flash memory and hash')
parser.add_argument('-i', '--isp', action='store_true', help='use usbtiny programmer')
parser.add_argument('-p', '--port', help='specify Arduino serial port')
parser.add_argument('-v', '--verbose', action='store_true')
args = parser.parse_args()

if args.arduino_path is None:
    assert is_windows
    args.arduino_path = Path(r'C:\Program Files (x86)\Arduino')
if not args.arduino_path.exists():
    sys.exit('Arduino IDE not found at "%s"; specify using --arduino-path' % args.arduino_path)

gcc_path = Path(__file__).resolve().parent

if not args.check:
    Path.mkdir(gcc_path / '_out', exist_ok=True)
    build_cmd = [
        str(args.arduino_path / 'arduino-builder'),
        '-hardware', str(args.arduino_path / 'hardware'),
        '-tools', str(args.arduino_path / 'hardware/tools'),
        '-tools', str(args.arduino_path / 'tools-builder'),
        '-fqbn', 'arduino:avr:nano:cpu=atmega328',
        '-libraries', str(gcc_path / 'libraries'),
        '-build-path', str(gcc_path / '_out'),
        '-warnings', 'all',
        '-prefs=build.extra_flags=-Werror',
        'src/gcc.ino',
    ]
    if args.verbose:
        build_cmd.insert(-1, '-verbose')
        print(' '.join(build_cmd))
    if subprocess.run(build_cmd).returncode:
        sys.exit()

if args.port is None and not args.check:
    sys.exit()

avrdude_cmd = [
    str(args.arduino_path / 'hardware/tools/avr/bin/avrdude'),
    '-C', str(args.arduino_path / 'hardware/tools/avr/etc/avrdude.conf'),
    '-p' 'atmega328p',
    '-D',
]
if args.isp:
    avrdude_cmd += ['-c', 'usbtiny']
else:
    if args.port is None:
        sys.exit('--port option is needed')
    avrdude_cmd += ['-c', 'arduino', '-b', '57600', '-P', args.port]
if args.check:
    bin_path = gcc_path / '_out/flash.bin'
    avrdude_cmd += ['-U', 'flash:r:%s:r' % bin_path]
else:
    avrdude_cmd += ['-U', 'flash:w:%s:i' % (gcc_path / '_out/gcc.ino.hex')]
if args.verbose:
    avrdude_cmd.append('-v')
    print(' '.join(avrdude_cmd))
subprocess.run(avrdude_cmd)

if args.check:
    sha1 = hashlib.sha1()
    with open(bin_path, 'rb') as f:
        while True:
            data = f.read(4096)
            if not data:
                break
            sha1.update(data)
    print('SHA1 hash:', sha1.hexdigest())
