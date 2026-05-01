#!/usr/bin/env python3
"""Validate ESP32 Wardriver Pro WiGLE CSV exports before upload."""

import csv
import sys
from pathlib import Path

EXPECTED_HEADER = [
    "MAC",
    "SSID",
    "AuthMode",
    "FirstSeen",
    "Channel",
    "Frequency",
    "RSSI",
    "CurrentLatitude",
    "CurrentLongitude",
    "AltitudeMeters",
    "AccuracyMeters",
    "Type",
]


def fail(message: str) -> int:
    print(f"ERROR: {message}", file=sys.stderr)
    return 1


def validate(path: Path) -> int:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.reader(handle)
        rows = list(reader)

    if len(rows) < 3:
        return fail("file must contain WiGLE metadata, header, and at least one data row")
    if not rows[0] or not rows[0][0].startswith("WigleWifi-1.4"):
        return fail("first row must start with WigleWifi-1.4")
    if rows[1] != EXPECTED_HEADER:
        return fail(f"unexpected header: {rows[1]}")

    for line_number, row in enumerate(rows[2:], start=3):
        if len(row) != len(EXPECTED_HEADER):
            return fail(f"line {line_number} has {len(row)} columns")
        if not row[0]:
            return fail(f"line {line_number} has an empty MAC/BSSID")
        if row[11] != "WIFI":
            return fail(f"line {line_number} Type must be WIFI")
        try:
            int(row[4])
            int(row[5])
            int(row[6])
            float(row[7])
            float(row[8])
            float(row[9])
            float(row[10])
        except ValueError as exc:
            return fail(f"line {line_number} has invalid numeric data: {exc}")

    print(f"OK: {path} has {len(rows) - 2} WiFi rows")
    return 0


def main() -> int:
    if len(sys.argv) != 2:
        return fail("usage: tools/validate_wigle_csv.py /path/to/wigle_YYYYMMDD_HHMMSS.csv")
    return validate(Path(sys.argv[1]))


if __name__ == "__main__":
    raise SystemExit(main())
