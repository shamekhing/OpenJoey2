#!/usr/bin/env python3
"""Derive data/classic_cards.json from data/cards.json.

The classic pool is the card list of the classic PC trilogy (the three
early-2000s PC releases), taken from the community wiki's "Complete card
list" (see scripts/classic_card_names.json for the bundled snapshot and its
source URLs).

Every entry of classic_cards.json is an untouched entry copied out of
cards.json (same id / name / desc / frameType / atk / def / level / rank), so
the derived file is exactly as authoritative as the full database it comes
from; only the selection differs.

Name matching tolerates the current TCG wording of classics that were
renamed since 2004 (e.g. "Red-Eyes B. Dragon" is now "Red-Eyes Black Dragon",
"Harpie's Brother" is now "Sky Scout"). For renamed cards the bundled
``password_aliases`` table maps the classic-era release name to its official
password, which equals the database ``id``.

Usage:
    python3 scripts/make_classic_cards.py

The full data/cards.json is never modified.
"""
import json
import pathlib
import re
import unicodedata

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
NAMES = pathlib.Path(__file__).resolve().parent / "classic_card_names.json"

# Same parser contract as fetch_cards.py (see openjoey-cards/docs/API.md).
KEEP = ("id", "name", "desc", "frameType", "atk", "def", "level", "rank")


def norm(name):
    """Loose case/punctuation-insensitive form for name matching."""
    s = unicodedata.normalize("NFKC", name).lower()
    s = s.replace("#", " ").replace("-", " ").replace("&", "and")
    s = re.sub(r"[.:'\u2019\u2018\u201c\u201d\"!?,()\u00b7]", " ", s)
    return re.sub(r"\s+", " ", s).strip()


def main():
    bundle = json.loads(NAMES.read_text())
    wiki_names = set()
    for key in ("yugi_the_destiny", "kaiba_the_revenge",
                "joey_the_passion", "all_cards_list"):
        wiki_names.update(bundle[key])

    db = json.loads((DATA / "cards.json").read_text())["data"]

    by_norm = {}
    by_id = {str(c["id"]): c for c in db}
    for c in db:
        by_norm.setdefault(norm(c["name"]), c)

    # wiki name -> matched db entry (never a dict collision: verified unique)
    matched = {}
    unmatched = []
    for w in wiki_names:
        c = by_norm.get(norm(w))
        if c is None:
            pw = bundle["password_aliases"].get(w)
            c = by_id.get(str(pw)) if pw else None
        if c is None:
            unmatched.append(w)
        else:
            matched[w] = c

    selected = {id(c) for c in matched.values()}
    out = [c for c in db if id(c) in selected]

    target = DATA / "classic_cards.json"
    tmp = target.with_suffix(".json.tmp")
    tmp.write_text(json.dumps({"data": out}, ensure_ascii=False, indent=2) + "\n")
    tmp.replace(target)

    print("wiki names: %d" % len(wiki_names))
    print("included:   %d  ->  %s" % (len(out), target))
    print("unmatched:  %s" % unmatched)
    for w, why in bundle["known_missing"].items():
        print("  known missing: %s (%s)" % (w, why))


if __name__ == "__main__":
    main()
