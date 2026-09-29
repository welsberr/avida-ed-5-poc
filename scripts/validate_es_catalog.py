#!/usr/bin/env python3
"""Validate the Spanish runtime candidate and its source-contract fingerprints."""
import hashlib
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / "docs/locales/es/draft-catalog.json").read_text(encoding="utf-8"))
messages = catalog["messages"]
token_re = re.compile(r"\{[A-Za-z_][A-Za-z0-9_]*\}")
ids = [item["id"] for item in messages]
assert len(ids) == len(set(ids)), "duplicate message IDs"
for item in messages:
    assert item["source"].strip(), f"empty source: {item['id']}"
    assert item["value"].strip(), f"empty Spanish value: {item['id']}"
    source_tokens = sorted(token_re.findall(item["source"]))
    value_tokens = sorted(token_re.findall(item["value"]))
    assert source_tokens == value_tokens, f"placeholder mismatch: {item['id']}"
    placeholder_names = sorted(token[1:-1] for token in source_tokens)
    assert sorted(item.get("placeholders", [])) == placeholder_names, f"placeholder metadata mismatch: {item['id']}"
    contract = json.dumps(
        [item["id"], item["source"], placeholder_names],
        ensure_ascii=False,
        separators=(",", ":"),
    )
    fingerprint = hashlib.sha256(contract.encode("utf-8")).hexdigest()
    assert item.get("source_contract_sha256") == fingerprint, f"stale source fingerprint: {item['id']}"

metadata = catalog["metadata"]
assert metadata["runtime_enabled"] is True
assert metadata["runtime_eligible"] is False
assert metadata["release_blocked_pending_review"] is True
assert metadata["human_reviewed_entry_count"] == 0
assert metadata["catalog_entry_count"] == len(messages)
assert (
    metadata["model_translated_entry_count"]
    + metadata["glossary_preserved_entry_count"]
    + metadata["assistant_edit_suggestion_count"]
) == len(messages), "candidate provenance counts do not partition the catalog"
for source in metadata["source_files"]:
    relative = source["path"]
    path = root / relative
    if "sha256_current" in source:
        assert hashlib.sha256(path.read_bytes()).hexdigest() == source["sha256_current"], (
            f"source hash is stale: {relative}"
        )

print(
    f"validated {len(messages)} Spanish candidate entries; "
    f"{metadata['model_translated_entry_count']} local model translations; "
    f"{metadata['glossary_preserved_entry_count']} glossary entries; "
    f"{metadata['assistant_edit_suggestion_count']} assistant edits pending review"
)
