#!/usr/bin/env python3
"""Refresh the Spanish Avida-ED message draft with a local TalkOrigins-style workflow.

Uses a local OpenAI-compatible llama.cpp endpoint only. It does not enable or
publish the resulting locale. Existing glossary and catalog contracts are
preserved, and the complete result is validated before the draft is replaced.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "docs/locales/es/draft-catalog.json"
SOURCES = [
    "source/web/Localization.hpp",
    "source/web/Layout.hpp",
    "source/web/OrganismLayout.hpp",
    "source/web/EducationRecording.hpp",
    "source/web/EducationArchive.hpp",
    "source/web/SequenceComparison.hpp",
    "source/web/RunControl.hpp",
    "source/web/PopulationView.hpp",
    "source/web/OrganismAnalysis.hpp",
    "source/web/SpanishCatalog.hpp",
    "source/web/LocalizationTests.cpp",
    "scripts/localize_es_catalog.py",
    "scripts/generate_es_runtime_catalog.py",
    "scripts/add_es_runtime_inventory.py",
    "scripts/finalize_es_catalog_provenance.py",
]
TALKORIGINS_COMPENDIUM = Path(os.environ.get(
    "TALKORIGINS_TRANSLATION_COMPENDIUM",
    ROOT / "docs/talkorigins-translation-compendium.json",
))
TERMS = {
    "Avida-ED": "keep exact",
    "Avida 5": "keep exact",
    "update": "actualización; keep distinct from generation",
    "generation": "generación; distinct from update",
    "sequence richness": "riqueza de secuencias",
    "ancestor sequence": "secuencia del ancestro; not ancestry",
    "ancestry": "ascendencia",
    "per-site mutation setting": "probabilidad de mutación por sitio",
    "fitness": "aptitud",
    "freezer": "almacén de organismos; use almacén in Avida-ED UI",
}
TALKORIGINS_TERMS = ("evolution", "mutation", "natural selection", "adaptation")
MANUAL_OVERRIDES = {
    "shell.sidepanel.run",
    "shell.sidepanel.saved_organisms",
    "shell.sidepanel.freezer.accessible",
    "shell.grid.placement_help",
    "status.update",
    "results.sample_context",
    "results.rows.one",
    "results.rows.other",
    "organism.freezer.placeholder",
    "organism.freezer.accessible",
    "population.color_mode.title",
    "organism.genome.heading",
    "organism.genome.instruction_count",
    "organism.genome.end",
    "organism.registers.heading",
    "organism.stacks.heading",
    "organism.stack.label",
    "organism.memory.heading",
    "organism.memory.copied",
    "organism.memory.errors",
    "organism.tasks.heading",
    "organism.stats.phenotype",
    "organism.stats.hardware",
    "organism.stats.executed",
    "organism.stats.heads",
    "organism.stats.memory",
    "organism.genome.eyebrow",
    "organism.status.offspring",
    "organism.genome.next",
    "organism.head.follow",
    "organism.head.stop_following",
    "sequence.ancestor",
}
TOKEN_RE = re.compile(r"\{[A-Za-z_][A-Za-z0-9_]*\}")


def call_model(endpoint: str, messages: list[dict], retries: int = 3) -> str:
    body = json.dumps({
        "model": "Qwen3.5-9B-Q5_K_M.gguf",
        "messages": messages,
        "temperature": 0.1,
        "top_p": 0.8,
        "max_tokens": 7000,
        "stream": False,
    }).encode()
    request = urllib.request.Request(
        endpoint.rstrip("/") + "/v1/chat/completions", body,
        {"Content-Type": "application/json"}, method="POST",
    )
    last_error = None
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(request, timeout=240) as response:
                payload = json.load(response)
            return payload["choices"][0]["message"]["content"]
        except (urllib.error.URLError, TimeoutError, KeyError, IndexError, json.JSONDecodeError) as err:
            last_error = err
            time.sleep(2 + attempt * 2)
    raise RuntimeError(f"local model request failed after {retries} attempts: {last_error}")


def parse_reply(reply: str) -> list[dict]:
    text = reply.strip()
    if text.startswith("```"):
        text = re.sub(r"^```(?:json)?\s*|\s*```$", "", text, flags=re.I)
    start, end = text.find("["), text.rfind("]")
    if start < 0 or end <= start:
        raise ValueError("model response did not contain a JSON array")
    result = json.loads(text[start:end + 1])
    if not isinstance(result, list):
        raise ValueError("model response JSON is not an array")
    return result


def placeholders(text: str) -> list[str]:
    return sorted(TOKEN_RE.findall(text))


def load_glossary() -> dict[str, str]:
    glossary = dict(TERMS)
    if TALKORIGINS_COMPENDIUM.exists():
        compendium = json.loads(TALKORIGINS_COMPENDIUM.read_text(encoding="utf-8"))
        for term in TALKORIGINS_TERMS:
            value = compendium.get("terms", {}).get(term, {}).get("es")
            if value:
                glossary[term] = value
    return glossary


def mask_placeholders(text: str) -> tuple[str, dict[str, str]]:
    replacements: dict[str, str] = {}

    def replace(match: re.Match[str]) -> str:
        token = f"ZXQPH{len(replacements)}QXZ"
        replacements[token] = match.group(0)
        return token

    return TOKEN_RE.sub(replace, text), replacements


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--endpoint", default="http://127.0.0.1:19191")
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--pause", type=float, default=0.5)
    args = parser.parse_args()
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    entries = catalog["messages"]
    originals = {entry["id"]: entry for entry in entries}
    # Glossary rows and explicit terminology/UI corrections are stewarded data.
    preserve = lambda item: (
        item["id"].startswith("glossary.")
        or item["id"] in MANUAL_OVERRIDES
        or item.get("translation_method", "").startswith("Codex-assisted")
        or (
            item.get("translation_method", "").startswith("local Qwen")
            and bool(item.get("value"))
        )
    )
    translation_entries = [item for item in entries if not preserve(item)]
    translations: dict[str, str] = {
        item["id"]: item["value"] for item in entries if preserve(item)
    }
    system = (
        "You are a careful Spanish-language translator for an educational digital-evolution app. "
        "Translate visible interface text into clear, region-neutral Spanish. Preserve meaning and "
        "scientific qualifications exactly. Return only a JSON array of objects with exactly the "
        "input ids and translated values. Do not translate product names, IDs, code, schema keys, "
        "or placeholders in braces. Keep Avida-ED and Avida 5 unchanged. Distinguish update from "
        "generation, sequence richness from mutation count, and ancestor sequence from ancestry. "
        "Do not add or omit claims."
    )
    glossary = "\n".join(f"- {source}: {target}" for source, target in load_glossary().items())
    for offset in range(0, len(translation_entries), args.batch_size):
        batch = translation_entries[offset:offset + args.batch_size]
        masks: dict[str, dict[str, str]] = {}
        payload = []
        for item in batch:
            masked, replacements = mask_placeholders(item["source"])
            masks[item["id"]] = replacements
            payload.append({"id": item["id"], "source": masked})
        user = (
            "Translate these catalog strings. Preserve every ZXQPH...QXZ token exactly; "
            "these tokens stand for runtime placeholders and will be restored after translation.\n"
            f"Glossary:\n{glossary}\nCatalog JSON:\n{json.dumps(payload, ensure_ascii=False)}"
        )
        error = None
        for attempt in range(3):
            try:
                parsed = parse_reply(call_model(args.endpoint, [
                    {"role": "system", "content": system},
                    {"role": "user", "content": user},
                ]))
                received = {}
                for row in parsed:
                    if not isinstance(row, dict) or "id" not in row:
                        raise ValueError("translation row must include an id")
                    value = row.get("value", row.get(
                        "translation", row.get("translated", row.get("target", row.get("es")))
                    ))
                    if value is None:
                        raise ValueError(
                            f"translation row lacks a value for {row['id']} (keys={sorted(row)})"
                        )
                    received[row["id"]] = value
                expected = {item["id"] for item in batch}
                if set(received) != expected:
                    raise ValueError(f"IDs mismatch: expected {expected}, got {set(received)}")
                for item in batch:
                    value = received[item["id"]]
                    if not isinstance(value, str) or not value.strip():
                        raise ValueError(f"empty translation for {item['id']}")
                    for token, original in masks[item["id"]].items():
                        if value.count(token) != 1:
                            raise ValueError(f"placeholder token {token} missing or duplicated for {item['id']}")
                        value = value.replace(token, original)
                    if placeholders(value) != placeholders(item["source"]):
                        raise ValueError(f"placeholder mismatch for {item['id']}")
                    translations[item["id"]] = value.strip()
                error = None
                break
            except (RuntimeError, ValueError, KeyError, TypeError, json.JSONDecodeError) as err:
                error = err
                time.sleep(2 + attempt * 2)
        if error:
            raise RuntimeError(f"batch starting at {offset} failed: {error}")
        print(f"translated {min(offset + len(batch), len(translation_entries))}/{len(translation_entries)} prose entries", flush=True)
        if args.pause:
            time.sleep(args.pause)

    if set(translations) != set(originals):
        raise RuntimeError("translation inventory does not match catalog IDs")
    for item in entries:
        item["value"] = translations[item["id"]]
        if not preserve(item):
            item["translation_method"] = "local Qwen3.5-9B; TalkOrigins glossary/protection workflow"

    metadata = catalog["metadata"]
    metadata.update({
        "status": "machine_translated_pending_human_review",
        "reviewed": False,
        "runtime_eligible": False,
        "runtime_enabled": True,
        "runtime_enablement_scope": "Development preview only; not release-eligible until human review.",
        "release_blocked_pending_review": True,
        "ui_validation_status": "pending_runtime_and_browser_validation",
        "source_revision": __import__("subprocess").check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip(),
        "source_worktree_was_dirty": bool(__import__("subprocess").check_output(
            ["git", "status", "--porcelain"], cwd=ROOT, text=True
        ).strip()),
        "updated_utc": dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        "translation_model": "Qwen3.5-9B-Q5_K_M.gguf via local llama.cpp endpoint",
        "translation_method": "Bounded batch translation with TalkOrigins glossary, exact-ID preservation, and placeholder checks.",
        "translation_confidence": "machine-generated candidate; not a human language or scientific review",
        "ui_validation_status": "pending_runtime_and_browser_validation",
        "catalog_entry_count": len(entries),
        "model_translated_entry_count": sum(
            item.get("translation_method", "").startswith("local Qwen")
            and not item["id"].startswith("glossary.") for item in entries
        ),
        "glossary_preserved_entry_count": sum(item["id"].startswith("glossary.") for item in entries),
        "human_reviewed_entry_count": 0,
        "glossary_version": "avida-ed-es-glossary-draft-2",
        "terminology_lineage": (
            "Relevant general terms follow the TalkOrigins Spanish translation compendium; "
            "Avida-specific distinctions are stewarded as explicit overrides."
        ),
    })
    for source in SOURCES:
        path = ROOT / source
        if path.exists():
            record = next((x for x in metadata["source_files"] if x.get("path") == source), None)
            if record is None:
                record = {"path": source}
                metadata["source_files"].append(record)
            record["sha256_current"] = hashlib.sha256(path.read_bytes()).hexdigest()
    if TALKORIGINS_COMPENDIUM.exists():
        metadata["terminology_source"] = "locally supplied TalkOrigins Spanish translation compendium"
        metadata["terminology_source_sha256"] = hashlib.sha256(
            TALKORIGINS_COMPENDIUM.read_bytes()
        ).hexdigest()
    else:
        metadata.pop("terminology_source", None)
        metadata.pop("terminology_source_sha256", None)
    for item in entries:
        contract = json.dumps(
            [item["id"], item["source"], placeholders(item["source"])],
            ensure_ascii=False, separators=(",", ":")
        )
        item["source_contract_sha256"] = hashlib.sha256(contract.encode("utf-8")).hexdigest()
    metadata["coverage_note"] = (
        f"{metadata['model_translated_entry_count']} entries were translated locally; "
        f"{metadata['glossary_preserved_entry_count']} glossary entries were preserved, and "
        f"{metadata['catalog_entry_count']} candidate entries now cover the current stable messages "
        "and inventoried Avida-ED controls. Codex-assisted edits remain pending competent review. "
        "A wider visible-string audit and qualified Spanish scientific-language review remain open."
    )
    # Validate full placeholder parity and invariant values before replacing the draft.
    protected = metadata.get("protected_values", [])
    for item in entries:
        if placeholders(item["source"]) != placeholders(item["value"]):
            raise RuntimeError(f"placeholder parity failed at {item['id']}")
        for token in TOKEN_RE.findall(item["source"]):
            if token not in item["value"]:
                raise RuntimeError(f"protected placeholder {token} missing at {item['id']}")
    # Schema identifiers in exported data stay protected by the catalog contract.
    if not all(key in json.dumps(catalog, ensure_ascii=False) for key in protected):
        raise RuntimeError("one or more catalog protected values are absent")
    temp = CATALOG.with_suffix(".json.tmp")
    temp.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    json.loads(temp.read_text(encoding="utf-8"))
    temp.replace(CATALOG)
    print(
        f"updated {CATALOG}: {metadata['model_translated_entry_count']} local model translations; "
        f"{len(entries)} total validated candidate entries"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"localization failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
