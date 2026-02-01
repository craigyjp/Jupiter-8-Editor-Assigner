import re
import csv
import sys
from pathlib import Path

DEFAULT_PATCH_NAME = None

if len(sys.argv) > 1:
    DEFAULT_PATCH_NAME = sys.argv[1]

# ---------------- CONFIG ----------------

OUTPUT_ORDER = [
    "PATCH_NAME",
    "BEND RANGE",
    "BLANK",
    "BLANK",
    "BLANK",
    "PORTA TIME",
    "BALANCE",
    "VOLUME",
    "BLANK",
    "LFO RATE",
    "LFO DELAY TIME",
    "LFO WAVE",
    "OSC LFO MOD",
    "OSC ENV MOD",
    "PWM",
    "OSC1 CROSS MOD",
    "OSC1 RANGE",
    "OSC1 WAVE",
    "OSC2 RANGE",
    "OSC2 TUNE",
    "OSC2 WAVE",
    "MIX BALANCE",
    "HPF",
    "CUTOFF",
    "RESONANCE",
    "FLT ENV MOD",
    "FLT LFO MOD",
    "FLT KEY FOLLOW",
    "AMP LEVEL",
    "ENV1 ATTACK",
    "ENV1 DECAY",
    "ENV1 SUSTAIN",
    "ENV1 RELEASE",
    "ENV2 ATTACK",
    "ENV2 DECAY",
    "ENV2 SUSTAIN",
    "ENV2 RELEASE",
    "DELAY LEVEL",
    "DELAY TIME",
    "DELAY FEEDBACK",
    "BLANK",
    "BLANK",
    "PORTA SW",
    "BLANK",
    "OSC FREQ MOD DST",
    "PWM SOURCE",
    "OSC2 SYNC",
    "BLANK",
    "FLT LPF SLOPE",
    "FLT ENV MOD SRC",
    "AMP LFO MOD",
    "ENV1 POLARITY",
    "ENV2 KEY FOLLOW",
    "ASSIGN MODE",
    "BLANK",
    "BLANK",
    "BLANK",
    "BLANK",
    "BLANK",
    "BLANK",
    "BLANK",
]

SCALE_255_TO_127 = {
}

FIXED_OUTPUT_VALUES = {
    "VOLUME": "160",
    "BALANCE": "127",
}

INPUT_DIR = Path("input_prm")
OUTPUT_DIR = Path("output_out")  # files named 11..88, no extension

# If True, trims PATCH_NAME whitespace at ends (recommended)
TRIM_STRINGS = True

# ---------------- END CONFIG ----------------


OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

LINE_RE = re.compile(r"^\s*(.*?)\s*\(\s*(.*?)\s*\)\s*;?\s*$")
PATCHNUM_RE = re.compile(r"JP08_PATCH(\d+)\.PRM$", re.IGNORECASE)


def scale_255_to_127(v: int) -> int:
    return int(round(v * 127.0 / 255.0))


def patch_index_to_roland_11_88(n: int) -> str:
    if n < 1 or n > 64:
        raise ValueError(f"Patch index out of range (1..64): {n}")
    row = ((n - 1) // 8) + 1
    col = ((n - 1) % 8) + 1
    return f"{row}{col}"


def normalize_param_name(name: str) -> str:
    if name.replace(" ", "") == "PATCH_NAME":
        return "PATCH_NAME"
    return name.strip()


def parse_prm_file(path: Path) -> dict:
    params = {}
    for raw_line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line:
            continue

        m = LINE_RE.match(line)
        if not m:
            continue

        name = normalize_param_name(m.group(1))
        raw_val = m.group(2)

        v = raw_val.strip()
        # numeric or string?
        try:
            params[name] = int(v)
        except ValueError:
            params[name] = v

    return params


def to_out_string(v) -> str:
    if isinstance(v, str):
        return v.strip() if TRIM_STRINGS else v
    return str(v)


def render_output_row(params: dict) -> list[str]:
    row = []

    for name in OUTPUT_ORDER:

        # Fixed synthetic parameters
        if name in FIXED_OUTPUT_VALUES:
            row.append(FIXED_OUTPUT_VALUES[name])
            continue

        # Explicit BLANK slot
        if name == "BLANK":
            row.append("0")
            continue

        # PATCH_NAME override from command line (always wins)
        if name == "PATCH_NAME" and DEFAULT_PATCH_NAME is not None:
            row.append(DEFAULT_PATCH_NAME)
            continue

        # Missing parameter => "0"
        if name not in params:
            row.append("0")
            continue

        v = params[name]

        # Optional scaling
        if name in SCALE_255_TO_127:
            if isinstance(v, int):
                v = scale_255_to_127(v)
            else:
                v = 0

        row.append(to_out_string(v))

    return row


def process_all():
    prm_files = sorted(INPUT_DIR.glob("JP08_PATCH*.PRM"))
    if not prm_files:
        raise FileNotFoundError(f"No PRM files found in {INPUT_DIR.resolve()}")

    for prm_path in prm_files:
        m = PATCHNUM_RE.search(prm_path.name)
        if not m:
            continue

        patch_n = int(m.group(1))          # 1..64
        out_name = patch_index_to_roland_11_88(patch_n)  # "11".."88"

        params = parse_prm_file(prm_path)
        out_row = render_output_row(params)

        out_path = OUTPUT_DIR / out_name   # no extension
        with out_path.open("w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(out_row)

        print(f"{prm_path.name} -> {out_path.name}")


if __name__ == "__main__":
    process_all()
