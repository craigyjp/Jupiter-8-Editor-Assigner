from pathlib import Path

# --------- CONFIG ---------
IN_DIR  = Path("output_out")   # your extensionless "11".."88" CSV-line files
OUT_DIR = Path("syx_out")      # output folder

NAME_LEN = 13
EXPECTED_PARAM_COUNT = None    # set to an int to enforce fixed length (recommended)

MFR = [0x00, 0x7D, 0x01]       # custom manufacturer id
DEVICE_ID = 0x01
CMD_PATCH_DUMP = 0x01
VERSION = 0x01

OUT_EXT = ".syx"               # set "" if you want extensionless sysex files too
BULK_FILENAME = "bulk_dump.syx"
# --------------------------


def nibblize_byte(b: int) -> list[int]:
    return [(b >> 4) & 0x0F, b & 0x0F]

def nibblize_bytes(data: bytes) -> list[int]:
    out = []
    for b in data:
        out.extend(nibblize_byte(b))
    return out

def checksum7(payload: list[int]) -> int:
    # 7-bit checksum: makes (sum(payload)+checksum) == 0x7F (mod 0x80)
    return (0x7F - (sum(payload) & 0x7F)) & 0x7F

def pad_name_13(name: str) -> bytes:
    s = name[:NAME_LEN].ljust(NAME_LEN, " ")
    return s.encode("ascii", errors="replace")

def parse_patch_csv_line(line: str) -> tuple[str, list[int]]:
    parts = [p.strip() for p in line.strip().split(",")]
    if len(parts) < 2:
        raise ValueError("Line must contain name + at least 1 param")

    name = parts[0]
    params = []
    for i, s in enumerate(parts[1:], start=1):
        if s == "":
            params.append(0)
            continue
        v = int(s)
        if not (0 <= v <= 255):
            raise ValueError(f"Param {i} out of range 0..255: {v}")
        params.append(v)

    if EXPECTED_PARAM_COUNT is not None and len(params) != EXPECTED_PARAM_COUNT:
        raise ValueError(f"Expected {EXPECTED_PARAM_COUNT} params, got {len(params)}")

    return name, params

def roland_11_88_to_slot_index(fn: str) -> int:
    # "11".."88" -> 0..63
    if len(fn) != 2 or not fn.isdigit():
        raise ValueError(fn)
    r = int(fn[0]); c = int(fn[1])
    if not (1 <= r <= 8 and 1 <= c <= 8):
        raise ValueError(fn)
    return (r - 1) * 8 + (c - 1)

def build_patch_sysex_fixed(slot_idx: int, name: str, params: list[int]) -> bytes:
    name_nibbles = nibblize_bytes(pad_name_13(name))

    param_nibbles = []
    for p in params:
        param_nibbles.extend(nibblize_byte(p))

    # payload: device..data (fixed layout)
    payload = [
        DEVICE_ID,
        CMD_PATCH_DUMP,
        VERSION,
        slot_idx,
        *name_nibbles,
        *param_nibbles
    ]

    cc = checksum7(payload)
    return bytes([0xF0, *MFR, *payload, cc, 0xF7])

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    bulk_path = OUT_DIR / BULK_FILENAME
    bulk = bytearray()

    slot_files = []
    for p in IN_DIR.iterdir():
        if p.is_file() and len(p.name) == 2 and p.name.isdigit():
            slot_files.append(p)
    slot_files.sort(key=lambda x: x.name)  # 11..88 string-sorts correctly here

    for path in slot_files:
        slot_name = path.name
        slot_idx = roland_11_88_to_slot_index(slot_name)

        line = path.read_text(encoding="utf-8", errors="ignore").strip()
        if not line:
            print(f"Skipping empty: {slot_name}")
            continue

        name, params = parse_patch_csv_line(line)
        syx = build_patch_sysex_fixed(slot_idx, name, params)

        # write individual file
        out_path = OUT_DIR / f"{slot_name}{OUT_EXT}"
        out_path.write_bytes(syx)

        # append to bulk dump
        bulk.extend(syx)

        print(f"{slot_name} -> {out_path.name}  (name='{name}', params={len(params)})")

    bulk_path.write_bytes(bulk)
    print(f"Bulk dump written: {bulk_path.name} ({len(bulk)} bytes)")

if __name__ == "__main__":
    main()
