#!/usr/bin/env python3
from pathlib import Path
import argparse
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--port')
parser.add_argument('-v', '--verbose', action='store_true')
parser.add_argument('arduino_path', type=Path)
args = parser.parse_args()

gcc_path = Path(__file__).resolve().parent
Path.mkdir(gcc_path / '_out', exist_ok=True)
build_cmd = [
    str(args.arduino_path / 'arduino-builder'),
    '-hardware', str(args.arduino_path / 'hardware'),
    '-tools', str(args.arduino_path / 'hardware/tools'),
    '-tools', str(args.arduino_path / 'tools-builder'),
    '-fqbn', 'arduino:avr:nano:cpu=atmega328',
    '-libraries', str(gcc_path / 'libraries'),
    '-build-path', str(gcc_path / '_out'),
    'gcc.ino',
]
if args.verbose:
    build_cmd.insert(-1, '-verbose')
    print(' '.join(build_cmd))
subprocess.run(build_cmd)

if args.port is None:
    sys.exit()
upload_cmd = [
    str(args.arduino_path / 'hardware/tools/avr/bin/avrdude'),
    '-C', str(args.arduino_path / 'hardware/tools/avr/etc/avrdude.conf'),
    '-p' 'atmega328p',
    '-c', 'arduino',
    '-P', args.port,
    '-b', '57600',
    '-D',
    '-U', 'flash:w:%s:i' % (gcc_path / '_out/gcc.ino.hex'),
]
if args.verbose:
    upload_cmd.append('-v')
    print(' '.join(upload_cmd))
subprocess.run(upload_cmd)
