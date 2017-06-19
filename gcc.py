#!/usr/bin/env python3
from pathlib import Path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('-v', '--verbose', action='store_true')
parser.add_argument('arduino_path', type=Path)
args = parser.parse_args()

gcc_path = Path(__file__).resolve().parent
Path.mkdir(gcc_path / '_out', exist_ok=True)
build_cmd = [
    str(args.arduino_path / 'arduino-builder'),
    '-hardware', str(args.arduino_path / 'hardware'),
    '-tools', str(args.arduino_path / 'hardware' / 'tools'),
    '-tools', str(args.arduino_path / 'tools-builder'),
    '-fqbn', 'arduino:avr:nano:cpu=atmega328',
    '-libraries', 'libraries',
    '-build-path', str(gcc_path / '_out'),
    'gcc.ino',
]
if args.verbose:
    build_cmd.insert(-1, '-verbose')
    print(' '.join(build_cmd))
subprocess.run(build_cmd)
