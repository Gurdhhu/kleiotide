#!/usr/bin/env python3
"""Validate probe exports using Biopython's independent ABI parser. Optional Staden JSON reader."""
import csv
import json
from pathlib import Path
import struct
import subprocess
import sys
from Bio import SeqIO
from Bio.SeqIO import AbiIO


def raw(path):
    with path.open('rb') as handle:
        assert handle.read(4) == b'ABIF'
        header = struct.unpack(AbiIO._HEADFMT, handle.read(struct.calcsize(AbiIO._HEADFMT)))
        return {name + str(number): value for name, number, value in AbiIO._abi_parse_header(header, handle)}


def numbers(text):
    return [int(n) for n in text.split(',') if n]


def main():
    folder = Path(sys.argv[1])
    staden = sys.argv[2] if len(sys.argv) > 2 else None
    count = staden_count = 0
    with (folder / 'manifest.tsv').open(encoding='utf8') as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            path = folder / row['file']
            record = SeqIO.read(path, 'abi')
            tags = record.annotations['abif_raw']
            calls, peaks, quality = row['calls'], numbers(row['peaks']), numbers(row['quality'])
            assert str(record.seq) == calls, path
            assert record.letter_annotations['phred_quality'] == quality, path
            for call_set in (1, 2):
                assert tags[f'PBAS{call_set}'].decode() == calls
                assert ([tags[f'PLOC{call_set}']] if isinstance(tags[f'PLOC{call_set}'], int) else list(tags[f'PLOC{call_set}'])) == peaks
                assert list(tags[f'PCON{call_set}']) == quality
            original = raw(folder / row['original'])
            original_order = original['FWO_1'].decode()
            source = {c: list(original[f'DATA{9+i}']) for i, c in enumerate(original_order)}
            expected = {}
            for c in 'ACGT':
                values = source[dict(zip('ACGT', 'TGCA'))[c] if row['reversed'] == '1' else c]
                expected[c] = values[::-1] if row['reversed'] == '1' else values
            for i, c in enumerate(tags['FWO_1'].decode()):
                assert list(tags[f'DATA{9+i}']) == expected[c], (path, c)
            if staden and len(calls) > 4:
                result = json.loads(subprocess.check_output([staden, str(path)], text=True))
                assert result['calls'] == calls.replace('N', '-'), path
                assert result['peaks'] == peaks, path
                for c in 'ACGT':
                    assert result[c] == [v & 65535 for v in expected[c]], (path, c)
                staden_count += 1
            count += 1
    print(f'PASS Biopython: {count} files; both call sets, edited sequence, peaks, Q and every signal sample')
    if staden:
        print(f'PASS Staden io_lib: {staden_count} files; calls, peaks and every channel sample (N represented internally as -)')
        print('Staden short-call inline-tag limitation: 1–4-call files checked with Biopython and Kleiotide.')


if __name__ == '__main__':
    main()
