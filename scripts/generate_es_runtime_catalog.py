#!/usr/bin/env python3
"""Generate the compiled Spanish candidate table from the stewarded JSON catalog."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / "docs/locales/es/draft-catalog.json").read_text(encoding="utf-8"))
messages = catalog["messages"]
escape = lambda value: json.dumps(value, ensure_ascii=False)
rows = "\n".join(
    f"  {{{escape(item['id'])}, {escape(item['source'])}, {escape(item['value'])}}},"
    for item in messages
)
header = f'''#pragma once

// Generated from docs/locales/es/draft-catalog.json.
// Machine-assisted candidate only; pending competent Spanish-language review.
#include <array>
#include <string_view>

namespace avida::web::localization {{
struct SpanishEntry {{ std::string_view id; std::string_view source; std::string_view value; }};
inline constexpr std::array<SpanishEntry, {len(messages)}> SPANISH_CANDIDATE{{{{
{rows}
}}}};

[[nodiscard]] constexpr const SpanishEntry * FindSpanish(std::string_view id) noexcept {{
  for (const auto & entry : SPANISH_CANDIDATE) if (entry.id == id) return &entry;
  return nullptr;
}}

[[nodiscard]] constexpr std::string_view SpanishText(std::string_view source) noexcept {{
  const SpanishEntry * match = nullptr;
  for (const auto & entry : SPANISH_CANDIDATE) {{
    if (entry.source != source || entry.id.starts_with("glossary.")) continue;
    if (!match) match = &entry;
    else if (match->value != entry.value) return source; // Require an ID for ambiguous text.
  }}
  return match ? match->value : source;
}}
}} // namespace avida::web::localization
'''
(root / "source/web/SpanishCatalog.hpp").write_text(header, encoding="utf-8")
print(f"generated Spanish candidate header with {len(messages)} entries")
