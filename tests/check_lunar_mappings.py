"""Validate every SDK lookup against the installed Lunar 1.8.9 mapping archive.

Usage: python tests/check_lunar_mappings.py [path/to/lunar-platform-mappings-v1_8.jar]
No game binaries or mappings are redistributed by this test.
"""
from pathlib import Path
import re
import struct
import sys
from zipfile import ZipFile


def read_mappings(path):
    with ZipFile(path) as archive:
        data = archive.read('lunar/lunar_named_b5_1.8.9.kin')
    if data[:9] != bytes.fromhex('05e8f0460100000000'):
        raise ValueError('Unsupported Lunar mapping format')
    offset = 9
    classes = {}

    def number():
        nonlocal offset
        value = struct.unpack_from('>I', data, offset)[0]
        offset += 4
        return value

    def string():
        nonlocal offset
        size = struct.unpack_from('>H', data, offset)[0]
        offset += 2
        value = data[offset:offset + size].decode('utf-8')
        offset += size
        return value

    def read_class(source='', target=''):
        source += string()
        target += string()
        for _ in range(number()):
            read_class(source + '$', target + '$')
        fields = [(string(), string(), string()) for _ in range(number())]
        methods = [(string(), string(), string()) for _ in range(number())]
        classes[source] = (target, fields, methods)

    for _ in range(number()):
        read_class()
    if offset != len(data):
        raise ValueError('Unconsumed mapping data')
    return classes


def remap_descriptor(desc, mappings):
    return re.sub(r'L([^;]+);', lambda m: 'L' + mappings.get(m[1], (m[1],))[0] + ';', desc)


def check(path):
    mappings = read_mappings(path)
    named = {value[0]: value for value in mappings.values()}
    source = (Path(__file__).resolve().parents[1] / 'src/sdk/sdk.cpp').read_text(encoding='utf-8')
    classes = dict(re.findall(r'C\.(\w+)\s*= cls\(env, "([^"]+)"\)', source))
    errors = []
    count = 0
    for kind, owner, name, desc in re.findall(r'(fld|mth)\(env, C\.(\w+),\s*"([^"]+)",\s*"([^"]+)"', source):
        class_name = classes[owner]
        if class_name.startswith('java/'):
            continue
        count += 1
        members = named.get(class_name, ('', [], []))[1 if kind == 'fld' else 2]
        if not any(name == target and desc == remap_descriptor(sig, mappings) for _, sig, target in members):
            errors.append(f'{class_name}.{name} {desc}')
    if errors:
        raise AssertionError('Lookups absent from Lunar mappings:\n' + '\n'.join(errors))
    print(f'{count} Lunar member lookups validated against {path.name}')


if __name__ == '__main__':
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / '.lunarclient/offline/multiver/lunar-platform-mappings-v1_8.jar'
    check(path)
