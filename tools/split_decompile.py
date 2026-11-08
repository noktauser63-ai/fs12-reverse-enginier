#!/usr/bin/env python3
"""Ghidra .c ciktisini fonksiyonlara boler: sinif bazli dosyalar + index.tsv.
Kullanim: split_decompile.py <libapp.so.c> <cikti_dizini>"""
import os, re, sys, collections

src, out = sys.argv[1], sys.argv[2]
lines = open(src, encoding='utf-8', errors='replace').read().replace('\r\n', '\n').split('\n')
os.makedirs(out, exist_ok=True)

# Fonksiyon: '// Ad(imza)' yorum satiri (coklu satir olabilir), bos satir, imza, bos, '{' ... '}'
funcs = []  # (name, start, end)
i, n = 0, len(lines)
while i < n:
    if lines[i] == '{' and i >= 2:
        # imza satirlarina geri git
        j = i - 2 if lines[i - 1].strip() == '' else i - 1
        while j > 0 and lines[j].strip() != '':
            j -= 1
        sig_start = j + 1
        # onceki yorum blogu
        k = j - 1
        while k > 0 and lines[k].startswith('//'):
            k -= 1
        c_start = k + 1 if lines[k + 1:k + 2] and lines[k + 1].startswith('//') else sig_start
        # govdenin sonu: ilk '}' (sutun 0)
        e = i + 1
        while e < n and lines[e] != '}':
            e += 1
        comment = ' '.join(l[3:] for l in lines[c_start:sig_start - 1] if l.startswith('// '))
        sig = ' '.join(l.strip() for l in lines[sig_start:i] if l.strip())
        name = comment.strip() or sig
        funcs.append((name, c_start, e, sig))
        i = e + 1
    else:
        i += 1

def klass(name, sig):
    m = re.match(r'^(?:[A-Za-z_][\w<>,\s\*&]*\s)?([A-Za-z_][\w<>]*)::(~?[\w<>]+)\(', name)
    if m: return m.group(1)
    m = re.search(r'([A-Za-z_][\w<>]*)::(~?[\w<>]+)\(', sig)
    if m: return m.group(1)
    return '_global'

by = collections.defaultdict(list)
with open(os.path.join(out, 'index.tsv'), 'w') as ix:
    ix.write('class\tname\tstart\tend\tlines\n')
    for name, s, e, sig in funcs:
        k = klass(name, sig)
        by[k].append((name, s, e))
        ix.write(f'{k}\t{name}\t{s+1}\t{e+1}\t{e-s+1}\n')
for k, fl in by.items():
    with open(os.path.join(out, re.sub(r'[^\w]', '_', k) + '.c'), 'w') as f:
        for name, s, e in fl:
            f.write('\n'.join(lines[s:e + 1]) + '\n\n')
tot = collections.Counter({k: sum(e - s + 1 for _, s, e in v) for k, v in by.items()})
print(len(funcs), 'fonksiyon,', len(by), 'sinif')
for k, v in tot.most_common(60):
    print(f'{v:7d}  {len(by[k]):4d}  {k}')
