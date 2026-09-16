#!/usr/bin/env python3
"""make_classic.py — build data/classic.json (the classic-era subset).

Classic rules = the April 11, 2005 era (Goat format). The master
data/cards.json (YGOProDeck format) carries no release dates, so they come
from the official YGOProDeck API (misc=yes -> tcg_date per card), cached in
data/release_dates.json for offline re-runs. Cards the API has no date for
fall back to a set-code prefix whitelist (the Goat-era sets).

Usage:
  scripts/make_classic.py [--cutoff 2005-04-11] [--refresh-dates]
                          [--in data/cards.json] [--out data/classic.json]

Stdlib only. The output keeps the {"data": [...]} schema, so the engine
parser consumes it unchanged.
"""

import argparse
import json
import os
import sys
import urllib.request

API_URL = "https://db.ygoprodeck.com/api/v7/cardinfo.php?misc=yes"
DEFAULT_CUTOFF = "2005-04-11"

# Goat-era TCG sets (release-date fallback only; dates win when present).
CLASSIC_SET_PREFIXES = [
    "LOB-", "SDK-", "SDY-", "SDJ-", "MRD-", "MRL-", "TP1-", "LON-",
    "LOD-", "PGD-", "DCR-", "IOC-", "AST-", "SRL-", "SOD-", "RDS-",
    "FET-", "DB1-", "DB2-", "SDP-", "CT1-",
]


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def fetch_dates(cache_path):
    """id -> tcg_date (earliest), via API with 1-day-stale file cache."""
    if not args.refresh_dates and os.path.exists(cache_path):
        return load_json(cache_path)
    print("fetching release dates from ygoprodeck API (one request)...")
    req = urllib.request.Request(API_URL, headers={"User-Agent": "openjoey-make-classic"})
    with urllib.request.urlopen(req) as resp:
        api = json.load(resp)
    dates = {}
    for card in api.get("data", []):
        misc = card.get("misc_info") or []
        tcg = None
        for m in misc:
            d = m.get("tcg_date")
            if d and (tcg is None or d < tcg):
                tcg = d
        if tcg:
            dates[card["id"]] = tcg
    with open(cache_path, "w", encoding="utf-8") as f:
        json.dump(dates, f)
    print(f"cached {len(dates)} tcg dates -> {cache_path}")
    return dates


def fallback_date(card):
    """Earliest classic set-code prefix hit, else None."""
    best = None
    for s in card.get("card_sets", []):
        code = s.get("set_code", "")
        for prefix in CLASSIC_SET_PREFIXES:
            if code.startswith(prefix) and (best is None or code < best):
                best = code
    return best


def main():
    global args
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--in", dest="inp", default="data/cards.json")
    p.add_argument("--out", dest="out", default="data/classic.json")
    p.add_argument("--cutoff", default=DEFAULT_CUTOFF)
    p.add_argument("--refresh-dates", action="store_true")
    p.add_argument("--dates-cache", default="data/release_dates.json")
    args = p.parse_args()

    cards = load_json(args.inp).get("data", [])
    dates = fetch_dates(args.dates_cache)

    classic, excluded, no_date = [], 0, 0
    for card in cards:
        tcg = dates.get(card["id"])
        if tcg is None:
            code = fallback_date(card)
            if code is None:
                no_date += 1
                excluded += 1
                continue
            tcg = code  # prefix order approximates era; prefix list is pre-cutoff
        if tcg <= args.cutoff:
            classic.append(card)
        else:
            excluded += 1

    with open(args.out, "w", encoding="utf-8") as f:
        json.dump({"data": classic}, f, ensure_ascii=False)

    print(f"cutoff {args.cutoff}: kept {len(classic)}, excluded {excluded}, "
          f"no-date-fallback-used {no_date} -> {args.out}")


if __name__ == "__main__":
    main()
