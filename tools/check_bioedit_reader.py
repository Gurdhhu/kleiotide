#!/usr/bin/env python3
"""Exercise BioEdit 7.7.1's ABIF reader in an x86 emulator (not its Windows GUI).
Usage: python check_bioedit_reader.py PATH/TO/BioEdit.exe EXPORT_FOLDER
Requires: pip install unicorn pefile. The official executable is not distributed.
Runtime stubs provide files, allocation and AnsiStrings; the parser instructions
run unmodified. Check their binary fingerprint before using fixed entry points.
"""
import csv
import hashlib
import pathlib
import struct
import sys
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI


def read(executable, path):
    b = pathlib.Path(path).read_bytes()
    pe = pefile.PE(str(executable))
    # Identify the complete parser and byte-swap routines, not the PE timestamp.
    code = pe.get_data(0x2f95d4, 0x1c00)
    if hashlib.sha256(code).hexdigest() != PARSER_HASH:
        raise ValueError('Unsupported BioEdit binary; fixed entry points require the official 7.7.1 executable.')
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x400000, 0x660000)
    u.mem_write(0x400000, pe.get_memory_mapped_image())
    u.mem_map(0x1000000, 0x200000)
    u.mem_map(0x2000000, 0x8000000)
    heap, bp, obj, pos = 0x2000000, 0x10ff000, 0x1100000, 0

    def r(a):
        return struct.unpack('<I', u.mem_read(a, 4))[0]

    def w(a, v):
        u.mem_write(a, struct.pack('<I', v & 0xffffffff))

    def alloc(n):
        nonlocal heap
        if n > 64 * 1024 * 1024:
            raise ValueError('Invalid allocation ' + str(n))
        a = heap
        heap += (n + 31) & ~15
        if heap > 0x9fff000:
            raise ValueError('Emulated heap exhausted')
        return a

    def cstr(p):
        if not p:
            return b''
        out = bytearray()
        while True:
            c = bytes(u.mem_read(p + len(out), 1))
            if c == b'\0':
                return bytes(out)
            out += c
            if len(out) > 1000000:
                raise ValueError('Unterminated string')

    def ansi(s):
        a = alloc(len(s) + 12)
        w(a, 0xffffffff)
        w(a + 4, len(s))
        u.mem_write(a + 8, s + b'\0')
        return a + 8

    def ret(value=None):
        sp = u.reg_read(UC_X86_REG_ESP)
        ip = r(sp)
        u.reg_write(UC_X86_REG_ESP, sp + 4)
        u.reg_write(UC_X86_REG_EIP, ip)
        if value is not None:
            u.reg_write(UC_X86_REG_EAX, value & 0xffffffff)

    def hook(uc, address, size, data):
        nonlocal pos
        sp = u.reg_read(UC_X86_REG_ESP)
        eax, edx = u.reg_read(UC_X86_REG_EAX), u.reg_read(UC_X86_REG_EDX)
        if address == 0x81ea18:
            ret(alloc(r(sp + 4)))
        elif address == 0x81e850:
            ret()
        elif address == 0x822a78:
            offset, whence = r(sp + 8), r(sp + 12)
            offset = offset if offset < 2**31 else offset - 2**32
            pos = offset + (pos if whence == 1 else len(b) if whence == 2 else 0)
            if pos < 0 or pos > len(b):
                raise ValueError(f'Invalid file seek {pos} from reader {r(sp):x}')
            ret(0)
        elif address == 0x8229d8:
            dest, n, count = r(sp + 4), r(sp + 8), r(sp + 12)
            length = n * count
            if pos < 0 or pos + length > len(b):
                raise ValueError(f'Invalid read at {pos}, length {length}, reader {r(sp):x}')
            if length:
                u.mem_write(dest, b[pos:pos + length])
            pos += length
            ret(count)
        elif address == 0x855270:
            w(eax, ansi(cstr(edx)))
            ret(eax)
        elif address == 0x85557c:
            w(eax, r(edx))
            ret(eax)
        elif address == 0x85554c:
            ret()
        elif address == 0x85537c:
            w(eax, ansi(bytes([edx & 255])))
            ret(eax)
        elif address == 0x855590:
            w(eax, ansi(cstr(r(eax)) + cstr(r(edx))))
            ret(eax)
        elif address in (0x8552a8, 0x820964):
            ret()

    def invalid(uc, access, address, size, value, data):
        raise ValueError(f'Invalid memory {address:x}, reader {u.reg_read(UC_X86_REG_EIP):x}')

    u.hook_add(UC_HOOK_CODE, hook, begin=0x810000, end=0x856000)
    u.hook_add(UC_HOOK_MEM_INVALID, invalid)
    f = alloc(len(b))
    u.mem_write(f, b)
    for reg, value in ((UC_X86_REG_EDI, f), (UC_X86_REG_ESI, obj), (UC_X86_REG_EBP, bp), (UC_X86_REG_ESP, bp - 0x300)):
        u.reg_write(reg, value)
    w(bp - 0xcc, 0xffffffff)  # Optional MTXF absent, initialized by parser prologue.
    w(obj + 0xa10, len(b))
    w(bp - 0x94, 1)
    w(bp - 0x98, int.from_bytes(b[26:30], 'big'))
    # Start after fopen/initial form housekeeping. Stop before UI/path formatting.
    u.emu_start(0x6f982a, 0x6fb1c5, count=80000000)
    if u.reg_read(UC_X86_REG_EIP) != 0x6fb1c5:
        raise ValueError('Reader did not finish')
    calls = cstr(r(obj + 0x4c8))
    n = r(obj + 0x530)
    peaks = [r(r(obj + 0x4dc) + i * 4) for i in range(n)]
    channels = []
    for i in range(4):
        ns = r(obj + 0x4a4 + i * 4)
        p = r(r(obj + 0x494) + i * 4)
        channels.append(list(struct.unpack('<' + 'i' * ns, u.mem_read(p, ns * 4))))
    return calls, peaks, channels


PARSER_HASH = '37b081397b027d92a9fef8acd2867e2ada5a9205137bafb75b317124b8c78429'


def main():
    executable, folder = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
    count = skipped = 0
    # Expected calls and positions come from the application's independently
    # generated manifest; signal expectations come from the standard ABIF parser.
    from Bio import SeqIO
    with (folder / 'manifest.tsv').open(encoding='utf8') as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            if len(row['calls']) <= 4:
                skipped += 1
                continue  # BioEdit assumes PBAS is external, even if <=4 bytes.
            path = folder / row['file']
            calls, peaks, channels = read(executable, path)
            assert calls.decode() == row['calls'], path
            assert peaks == [int(v) for v in row['peaks'].split(',') if v], path
            tags = SeqIO.read(path, 'abi').annotations['abif_raw']
            for i in range(4):
                assert channels[i] == list(tags['DATA' + str(9 + i)]), (path, i)
            count += 1
    print(f'PASS BioEdit 7.7.1 reader emulation: {count} exports, exact edited calls, positions and all analyzed signal samples.')
    print(f'{skipped} short-call fixtures excluded: BioEdit does not support standard inline PBAS. Windows GUI not executed.')


if __name__ == '__main__':
    main()
