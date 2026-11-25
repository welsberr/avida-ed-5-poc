#!/usr/bin/env python3
"""Align the app's Freezer label across the static-page review drafts."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'docs/site-locales'
TERMS = {
    'es': (r'\b(?:Freezer|congelador)\b', 'Almacén', 'almacén'),
    'fr': (r'\b(?:Freezer|Freezeur)\b', 'Congélateur', 'congélateur'),
    'de': (r'\b(?:Freezer|Eisfach)\b', 'Gefrierfach', 'Gefrierfach'),
    'pt': (r'\bFreezer\b', 'Congelador', 'congelador'),
}

for code, (pattern, upper, lower) in TERMS.items():
    path = ROOT / code / 'catalog.json'
    catalog = json.loads(path.read_text(encoding='utf-8'))
    count = 0
    for entry in catalog['messages'].values():
        if 'freezer' not in entry['source'].lower():
            continue
        def replacement(match):
            return upper if match.group(0)[0].isupper() else lower
        value = re.sub(pattern, replacement, entry['value'], flags=re.I)
        if value != entry['value']:
            entry['value'] = value
            entry['method'] = 'assistant-normalized app terminology; pending human review'
            count += 1
    catalog['metadata']['terminology_note'] = (
        'TalkOrigins and Avida terms were provided to the model; the Freezer label was '
        'also normalized afterward to match the application. All translations remain drafts.'
    )
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'{code}: normalized {count} drafts')
