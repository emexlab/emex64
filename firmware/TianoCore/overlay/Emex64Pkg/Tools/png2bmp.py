#!/usr/bin/env python3

import struct
import sys
import zlib

def chunks(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit("png2bmp: not a PNG")
    pos = 8
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        yield kind, data[pos + 8:pos + 8 + length]
        pos += 12 + length

def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c

def decode(path):
    """width, height, rows of (r, g, b, a)"""
    data = open(path, "rb").read()
    idat = b""
    palette = []
    trns = b""
    for kind, body in chunks(data):
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = [tuple(body[i:i + 3]) for i in range(0, len(body), 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat += body
    if interlace:
        sys.exit("png2bmp: interlaced PNGs aren't read; save it without interlacing")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    if depth == 16:
        sys.exit("png2bmp: 16 bits per channel aren't read; save it with 8")
    if depth < 8 and ctype not in (0, 3):
        sys.exit("png2bmp: unexpected bit depth")
    bits = channels * depth
    stride = (width * bits + 7) // 8
    bpp = max(1, bits // 8)
    raw = zlib.decompress(idat)

    rows = []
    prev = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if kind == 1:
                line[i] = (line[i] + a) & 255
            elif kind == 2:
                line[i] = (line[i] + b) & 255
            elif kind == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif kind == 4:
                line[i] = (line[i] + paeth(a, b, c)) & 255
        prev = line

        pixels = []
        for x in range(width):
            if depth < 8:
                per = 8 // depth
                v = (line[x // per] >> (8 - depth * (x % per + 1))) & ((1 << depth) - 1)
            else:
                v = None
            if ctype == 3:
                idx = v if v is not None else line[x]
                r, g, b = palette[idx]
                alpha = trns[idx] if idx < len(trns) else 255
            elif ctype == 0:
                gray = v * 255 // ((1 << depth) - 1) if v is not None else line[x]
                r = g = b = gray
                alpha = 255
            elif ctype == 4:
                r = g = b = line[2 * x]
                alpha = line[2 * x + 1]
            elif ctype == 2:
                r, g, b = line[3 * x:3 * x + 3]
                alpha = 255
            else:
                r, g, b, alpha = line[4 * x:4 * x + 4]
            pixels.append((r, g, b, alpha))
        rows.append(pixels)
    return width, height, rows


def main():
    if len(sys.argv) not in (3, 5):
        sys.exit("usage: png2bmp.py IN.png OUT.bmp [MAX_WIDTH MAX_HEIGHT]")
    max_w, max_h = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) == 5 else (192, 128)
    width, height, rows = decode(sys.argv[1])

    rows = [[(r * a // 255, g * a // 255, b * a // 255) for r, g, b, a in row] for row in rows]

    factor = 1
    while width // factor > max_w or height // factor > max_h:
        factor += 1
    out_w, out_h = width // factor, height // factor
    area = factor * factor
    small = []
    for oy in range(out_h):
        line = []
        for ox in range(out_w):
            r = g = b = 0
            for y in range(oy * factor, oy * factor + factor):
                for pr, pg, pb in rows[y][ox * factor:ox * factor + factor]:
                    r += pr
                    g += pg
                    b += pb
            line.append((r // area, g // area, b // area))
        small.append(line)

    # BMP: bottom row first, BGR, rows padded to 4 bytes
    pad = (4 - out_w * 3 % 4) % 4
    body = bytearray()
    for y in range(out_h - 1, -1, -1):
        for r, g, b in small[y]:
            body += bytes((b, g, r))
        body += b"\0" * pad
    header = struct.pack("<2sIHHI", b"BM", 54 + len(body), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, out_w, out_h, 1, 24, 0, len(body), 2835, 2835, 0, 0)
    with open(sys.argv[2], "wb") as f:
        f.write(header + info + body)
    print("png2bmp: %s %dx%d -> %dx%d" % (sys.argv[1], width, height, out_w, out_h))


main()
