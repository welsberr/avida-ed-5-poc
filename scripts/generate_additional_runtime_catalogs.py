#!/usr/bin/env python3
"""Validate and compile the French, German and Portuguese draft catalogs."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = json.loads((root / 'docs/locales/es/draft-catalog.json').read_text(encoding='utf-8'))['messages']
tokens = re.compile(r'\{[A-Za-z_][A-Za-z0-9_]*\}')
catalogs = {}
for code in ('fr', 'de', 'pt'):
    entries = json.loads((root / f'docs/locales/{code}/draft-catalog.json').read_text(encoding='utf-8'))['messages']
    if len(entries) != len(source) or len({item['id'] for item in entries}) != len(entries):
        raise ValueError(f'{code}: incomplete or duplicate catalog')
    catalogs[code] = {item['id']: item for item in entries}
    for item in source:
        candidate = catalogs[code][item['id']]
        if candidate['source'] != item['source'] or not candidate['value'].strip():
            raise ValueError(f'{code}: invalid source or empty value for {item["id"]}')
        if any(character in candidate['value'] and character not in candidate['source'] for character in '<>'):
            raise ValueError(f'{code}: unexpected markup character for {item["id"]}')
        if sorted(tokens.findall(candidate['source'])) != sorted(tokens.findall(candidate['value'])):
            raise ValueError(f'{code}: placeholder mismatch for {item["id"]}')

quote = lambda value: json.dumps(value, ensure_ascii=False)
rows = '\n'.join('  {' + ', '.join(quote(value) for value in (
    item['id'], item['source'], *(catalogs[code][item['id']]['value'] for code in ('fr', 'de', 'pt'))
)) + '},' for item in source)
header = f'''#pragma once
// Generated from docs/locales/{{fr,de,pt}}/draft-catalog.json.
// Machine-generated candidates; language and scientific review are pending.
#include <array>
#include <string_view>
namespace avida::web::localization {{
struct AdditionalEntry {{ std::string_view id, source, fr, de, pt; }};
inline constexpr std::array<AdditionalEntry, {len(source)}> ADDITIONAL_CANDIDATES{{{{
{rows}
}}}};
[[nodiscard]] constexpr const AdditionalEntry * FindAdditional(std::string_view id) noexcept {{
  for (const auto & entry : ADDITIONAL_CANDIDATES) if (entry.id == id) return &entry;
  return nullptr;
}}
}} // namespace avida::web::localization
'''
(root / 'source/web/AdditionalCatalogs.hpp').write_text(header, encoding='utf-8')
print(f'validated and generated {len(source)} entries for each additional locale')
