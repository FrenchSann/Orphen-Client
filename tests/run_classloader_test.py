"""Run the native loader test with an isolated, generated Java class.

python tests/run_classloader_test.py build/Release/classloader_test.exe path/to/jvm.dll
"""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def utf8(text):
    data = text.encode('utf-8')
    return b'\x01' + struct.pack('>H', len(data)) + data


with tempfile.TemporaryDirectory(prefix='orphen-loader-') as directory:
    root = Path(directory)
    target = root / 'net/minecraft/client/Minecraft.class'
    target.parent.mkdir(parents=True)
    # Java 8 public empty class, with Object as superclass and no members.
    pool = (utf8('net/minecraft/client/Minecraft') + b'\x07\x00\x01'
            + utf8('java/lang/Object') + b'\x07\x00\x03')
    target.write_bytes(struct.pack('>IHHH', 0xCAFEBABE, 0, 52, 5) + pool
                       + struct.pack('>7H', 0x0021, 2, 4, 0, 0, 0, 0))
    result = subprocess.run([str(Path(sys.argv[1]).resolve()), str(Path(sys.argv[2]).resolve()), root.as_uri() + '/'])
    sys.exit(result.returncode)
