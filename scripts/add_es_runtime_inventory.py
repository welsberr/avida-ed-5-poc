#!/usr/bin/env python3
"""Add current Organism-workspace controls missing from the Spanish catalog."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
path = root / "docs/locales/es/draft-catalog.json"
catalog = json.loads(path.read_text(encoding="utf-8"))
items = [
    ("organism.toolbar.rewind.aria", "Rewind organism execution"),
    ("organism.toolbar.step.aria", "Execute one organism instruction"),
    ("organism.toolbar.play.aria", "Play at two instructions per second"),
    ("organism.toolbar.pause.aria", "Pause organism execution"),
    ("organism.toolbar.fast_forward.aria", "Fast-forward at twenty instructions per second"),
    ("organism.toolbar.rewind.title", "Rewind"),
    ("organism.toolbar.step.title", "Step (Space)"),
    ("organism.toolbar.play.title", "Play (2 instructions per second)"),
    ("organism.toolbar.fast_forward.title", "Fast-forward (20 instructions per second)"),
    ("organism.toolbar.view_offspring", "View offspring →"),
    ("organism.freezer.label", "Organism"),
    ("organism.freezer.placeholder", "Choose from freezer…"),
    ("organism.freezer.accessible", "Choose an organism from the freezer"),
    ("organism.timeline.label", "Execution position"),
    ("organism.timeline.accessible", "Organism execution position"),
    ("population.color_mode.accessible", "Population color mode"),
    ("population.color_mode.title", "Color organisms by"),
    ("organism.empty.heading", "Select an organism first"),
    ("organism.empty.help", "Select an organism from the freezer (above) or return to Population mode and select an occupied cell."),
    ("organism.cycle.heading", "Single life cycle"),
    ("organism.cycle.instructions_executed", "instructions executed"),
    ("organism.genome.eyebrow", "Execution map"),
    ("organism.genome.heading", "Genome"),
    ("organism.genome.instruction_count", "instructions"),
    ("organism.genome.end", "end of genome"),
    ("organism.status.complete", "Life cycle complete"),
    ("organism.status.offspring", "Offspring produced · {count} instructions. Use “View offspring” above to start it with fresh hardware."),
    ("organism.status.preview_limit", "Preview limit reached"),
    ("organism.status.continues", "Execution continues"),
    ("organism.status.no_offspring", "No offspring was produced in the first {count} instructions."),
    ("organism.instruction.just_executed_at", "Just executed at {update}"),
    ("organism.instruction.just_executed", "Just executed"),
    ("organism.instruction.nothing_yet", "Nothing yet"),
    ("organism.instruction.start_help", "Use Step, Play, or Fast-forward to begin this life cycle."),
    ("organism.instruction.about_to_execute", "About to execute at {update}"),
    ("organism.registers.heading", "Registers"),
    ("organism.stacks.heading", "Stacks"),
    ("organism.stack.label", "Stack"),
    ("organism.memory.eyebrow", "Working state"),
    ("organism.memory.heading", "Memory"),
    ("organism.memory.copied", "Copied"),
    ("organism.memory.errors", "Errors"),
    ("organism.tasks.eyebrow", "Life-cycle progress"),
    ("organism.tasks.heading", "Tasks"),
    ("organism.tasks.empty", "No configured reaction tasks are performed in this life cycle."),
    ("organism.traits.eyebrow", "Starting phenotype"),
    ("organism.traits.heading", "Organism traits"),
    ("organism.traits.empty", "No printable traits."),
    ("organism.stats.empty", "Select an occupied population cell to inspect its organism."),
    ("organism.stats.active", "Active organism"),
    ("organism.stats.phenotype", "Phenotype"),
    ("organism.stats.hardware", "Hardware"),
    ("organism.stats.executed", "Executed"),
    ("organism.stats.heads", "Heads"),
    ("organism.stats.memory", "Nonzero memory"),
    ("organism.stats.memory.empty", "All locations are zero."),
    ("organism.genome.next", "NEXT"),
    ("organism.head.follow", "Follow"),
    ("organism.head.stop_following", "Stop following"),
]
existing = {message["id"] for message in catalog["messages"]}
for message_id, source in items:
    if message_id in existing:
        continue
    catalog["messages"].append({
        "id": message_id,
        "source": source,
        "value": "",
        "source_state": "present",
        "status": "awaiting_local_translation",
        "placeholders": [],
        "translation_method": "pending local Qwen batch",
    })
path.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"inventory now contains {len(catalog['messages'])} entries")
