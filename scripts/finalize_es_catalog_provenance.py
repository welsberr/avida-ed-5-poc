#!/usr/bin/env python3
"""Refresh Spanish candidate fingerprints and provenance after runtime integration."""
from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "docs/locales/es/draft-catalog.json"
TALKORIGINS_COMPENDIUM = Path(os.environ.get(
    "TALKORIGINS_TRANSLATION_COMPENDIUM",
    ROOT / "docs/talkorigins-translation-compendium.json",
))
SOURCE_FILES = [
    "source/web/Localization.hpp",
    "source/web/SpanishCatalog.hpp",
    "source/web/Layout.hpp",
    "source/web/FreezerPanel.hpp",
    "source/web/ConfigurationPanels.hpp",
    "source/web/OrganismLayout.hpp",
    "source/web/EducationRecording.hpp",
    "source/web/EducationArchive.hpp",
    "source/web/SequenceComparison.hpp",
    "source/web/RunControl.hpp",
    "source/web/PopulationView.hpp",
    "source/web/Ed4ColorPalettes.hpp",
    "source/web/OrganismAnalysis.hpp",
    "web/Avida.css",
    "web/Avida.html",
    "web/education-plots.js",
    "web/vendor/plotly-1.53.0.min.js",
    "web/vendor/plotly-locale-es-1.53.0.js",
    "source/web/LocalizationTests.cpp",
    "scripts/localize_es_catalog.py",
    "scripts/generate_es_runtime_catalog.py",
    "scripts/add_es_runtime_inventory.py",
    "scripts/import_ed4_color_palettes.py",
    "scripts/finalize_es_catalog_provenance.py",
    "scripts/validate_es_catalog.py",
]
PLACEHOLDER_RE = re.compile(r"\{([A-Za-z_][A-Za-z0-9_]*)\}")

catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
messages = catalog["messages"]
for entry in messages:
    entry["placeholders"] = sorted(PLACEHOLDER_RE.findall(entry["source"]))
    contract = json.dumps(
        [entry["id"], entry["source"], entry["placeholders"]],
        ensure_ascii=False,
        separators=(",", ":"),
    )
    entry["source_contract_sha256"] = hashlib.sha256(contract.encode("utf-8")).hexdigest()

metadata = catalog["metadata"]
model_count = sum(
    entry.get("translation_method", "").startswith("local Qwen")
    and not entry["id"].startswith("glossary.") for entry in messages
)
glossary_count = sum(entry["id"].startswith("glossary.") for entry in messages)
assistant_edit_count = sum(
    entry.get("translation_method", "").startswith("Codex-assisted") for entry in messages
)
metadata.update({
    "status": "machine_translated_pending_human_review",
    "reviewed": False,
    "reviewer": None,
    "reviewed_utc": None,
    "review_confidence": None,
    "runtime_enabled": True,
    "runtime_enablement_scope": "Development preview only; not release-eligible until human review.",
    "runtime_eligible": False,
    "release_blocked_pending_review": True,
    "ui_validation_status": "pending_browser_visual_layout_checks",
    "source_revision": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
    ).strip(),
    "source_worktree_was_dirty": bool(subprocess.check_output(
        ["git", "status", "--porcelain", "--ignore-submodules=all"], cwd=ROOT, text=True
    ).strip()),
    "updated_utc": dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
    "catalog_entry_count": len(messages),
    "model_translated_entry_count": model_count,
    "glossary_preserved_entry_count": glossary_count,
    "assistant_edit_suggestion_count": assistant_edit_count,
    "human_reviewed_entry_count": 0,
    "glossary_version": "avida-ed-es-glossary-draft-2",
    "terminology_lineage": (
        "Relevant general terms follow the TalkOrigins Spanish translation compendium; "
        "Avida-specific distinctions are explicit overrides."
    ),
    "coverage_note": (
        f"{model_count} entries were translated locally; {glossary_count} glossary entries were "
        f"preserved, and the {len(messages)}-entry candidate covers current stable messages and "
        "inventoried Avida-ED controls. Codex-assisted wording suggestions remain pending competent "
        "review. A full visible-string inventory and qualified Spanish scientific-language review "
        "remain open."
    ),
})

if TALKORIGINS_COMPENDIUM.exists():
    metadata["terminology_source"] = "locally supplied TalkOrigins Spanish translation compendium"
    metadata["terminology_source_sha256"] = hashlib.sha256(
        TALKORIGINS_COMPENDIUM.read_bytes()
    ).hexdigest()
else:
    metadata.pop("terminology_source", None)
    metadata.pop("terminology_source_sha256", None)
for relative in SOURCE_FILES:
    path = ROOT / relative
    record = next((item for item in metadata["source_files"] if item.get("path") == relative), None)
    if record is None:
        record = {"path": relative}
        metadata["source_files"].append(record)
    record["sha256_current"] = hashlib.sha256(path.read_bytes()).hexdigest()

temp = CATALOG.with_suffix(".json.tmp")
temp.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
json.loads(temp.read_text(encoding="utf-8"))
temp.replace(CATALOG)
print(f"recorded provenance for {len(messages)} Spanish candidate entries")
