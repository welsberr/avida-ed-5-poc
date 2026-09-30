#!/usr/bin/env python3
"""Validate the shared progress record and render its static GitHub Pages view."""
import argparse
from collections import Counter
from datetime import date, datetime
from html import escape
import json
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/roadmap/progress.json"
OUTPUT = ROOT / "docs/roadmap.html"
STATUSES = {"planned": "Planned", "in_progress": "In progress", "blocked": "Blocked",
            "review": "Awaiting review", "complete": "Complete", "deferred": "Deferred"}


def validate(data):
    assert data["schema_version"] == 1, "Unsupported schema"
    assert datetime.fromisoformat(data["updated_at"].replace("Z", "+00:00")).tzinfo, "Timestamp needs timezone"
    date.fromisoformat(data["demo_date"])
    milestones = data["milestones"]
    ids = [m["id"] for m in milestones]
    assert len(ids) == len(set(ids)), "Duplicate milestone IDs"
    assert all(i and all(c.islower() or c.isdigit() or c == "-" for c in i) for i in ids), "Invalid ID"
    by_id = {m["id"]: m for m in milestones}
    for m in milestones:
        assert m["status"] in STATUSES, "Unknown status"
        assert m["title"] and m["phase"] and m["owner"] and m["next_action"], "Missing milestone metadata"
        assert m["criteria"], "Missing acceptance criteria"
        assert all(c["text"] and type(c["met"]) is bool for c in m["criteria"]), "Invalid criterion"
        for field in ("start", "target"):
            if m[field]: date.fromisoformat(m[field])
        if m["start"] and m["target"]: assert m["start"] <= m["target"], "Invalid date window"
        assert all(d in by_id and d != m["id"] for d in m["dependencies"]), "Invalid dependency"
        for e in m["evidence"]:
            assert e["label"] and urlsplit(e["url"]).scheme == "https" and urlsplit(e["url"]).netloc, "Evidence must have a labeled HTTPS URL"
        if m["status"] == "blocked": assert m["blockers"], "Blocked milestone needs a reason"
        if m["status"] == "complete":
            assert all(c["met"] for c in m["criteria"]), "Completion needs all acceptance criteria"
            assert m["evidence"], "Completion needs evidence"
            assert all(by_id[d]["status"] == "complete" for d in m["dependencies"]), "Completion has unmet dependency"
    def visit(ident, path):
        assert ident not in path, "Dependency cycle"
        for dep in by_id[ident]["dependencies"]: visit(dep, path | {ident})
    for ident in ids: visit(ident, set())
    latest = {}
    last_date = ""
    for update in data["updates"]:
        date.fromisoformat(update["date"])
        assert update["date"] >= last_date, "Updates must be chronological"
        last_date = update["date"]
        assert update["milestone"] in by_id and update["status"] in STATUSES, "Invalid update"
        assert update["author"] and update["summary"], "Update needs attribution and summary"
        latest[update["milestone"]] = update["status"]
    assert all(latest.get(m["id"]) == m["status"] for m in milestones), "Status needs a matching latest update"


def render(data):
    esc = escape
    counts = Counter(m["status"] for m in data["milestones"])
    next_milestone = next((m for m in data["milestones"] if m["status"] not in {"complete", "deferred"}), None)
    next_action = next_milestone["next_action"] if next_milestone else "Review the completed scope and record the next release decision."
    cards = []
    for m in data["milestones"]:
        dependencies = ", ".join(f'<a href="#{esc(d)}">{esc(d)}</a>' for d in m["dependencies"]) or "None"
        criteria = "".join(f'<li><strong>{"Met" if c["met"] else "Pending"}:</strong> {esc(c["text"])}</li>' for c in m["criteria"])
        evidence = "".join(f'<li><a href="{esc(e["url"], quote=True)}">{esc(e["label"])}</a></li>' for e in m["evidence"])
        blockers = "; ".join(m["blockers"]) or "None recorded"
        window = f'{m["start"]} to {m["target"]}' if m["target"] else "Not yet scheduled; acceptance-gated"
        cards.append(f'''<section id="{esc(m['id'])}" class="milestone">
<p class="small">{esc(m['phase'])} · {esc(m['id'])}</p>
<h3>{esc(m['title'])}</h3><p><span class="badge {m['status']}">{STATUSES[m['status']]}</span></p>
<dl><dt>Working window</dt><dd>{esc(window)}</dd><dt>Owner</dt><dd>{esc(m['owner'])}</dd>
<dt>Depends on</dt><dd>{dependencies}</dd><dt>Blockers</dt><dd>{esc(blockers)}</dd></dl>
<h4>Acceptance criteria</h4><ul>{criteria}</ul>
<p><strong>Next action:</strong> {esc(m['next_action'])}</p>
<h4>Evidence</h4>{'<ul>'+evidence+'</ul>' if evidence else '<p>No acceptance evidence recorded yet.</p>'}</section>''')
    updates = "".join(f'''<li><time datetime="{u['date']}">{u['date']}</time> · <a href="#{u['milestone']}">{esc(u['milestone'])}</a> · {STATUSES[u['status']]}<br>{esc(u['summary'])} <span class="small">Recorded by {esc(u['author'])}.</span></li>''' for u in reversed(data["updates"]))
    toc = "".join(f'<li><a href="#{m["id"]}">{esc(m["title"])}</a> — {STATUSES[m["status"]]}</li>' for m in data["milestones"])
    return f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="description" content="Avida-ED 5 milestones, acceptance criteria, evidence, and dated project progress toward the October 23 demonstration and a functional teaching release.">
<title>Avida-ED 5 roadmap and progress</title><link rel="stylesheet" href="site-report.css">
<style>
.skip-link {{ position:absolute; left:1rem; top:-5rem; background:white; padding:.5rem; z-index:1; }} .skip-link:focus {{ top:1rem; }}
.summary-counts {{ display:flex; flex-wrap:wrap; gap:.75rem; }}
.summary-counts span {{ padding:.6rem .9rem; background:#edf3f6; border-radius:.4rem; }}
.badge {{ display:inline-block; padding:.15rem .65rem; border-radius:.35rem; background:#e8edf1; font-weight:700; }}
.complete {{ background:#dcf3e4; color:#185132; }} .blocked {{ background:#fbe4e4; color:#802727; }}
.review, .in_progress {{ background:#fff0ce; color:#6e4800; }}
.milestone h3 {{ font-size:1.35rem; margin-top:.3rem; }} h4 {{ margin:1rem 0 .3rem; }}
dl {{ display:grid; grid-template-columns:9rem minmax(0,1fr); gap:.3rem .8rem; }} dt {{ font-weight:650; }} dd {{ margin:0; }}
main a, dd {{ overflow-wrap:anywhere; }} .history li {{ margin-bottom:1rem; }}
@media(max-width:550px) {{ dl {{ grid-template-columns:1fr; }} dd {{ margin-bottom:.5rem; }} }}
</style></head><body>
<a href="#main" class="skip-link">Skip to roadmap</a>
<header class="masthead"><div class="wrap"><div class="eyebrow">Avida-ED 5 proof of concept</div>
<h1>Roadmap and progress</h1><p>A complete demonstration for October 23, 2026, followed by a functional educational release.</p></div></header>
<main id="main" class="wrap"><nav aria-label="Project navigation"><a href="index.html">Project home</a> · <a href="porting-learning-roadmap.html">Detailed plan and rationale</a> · <a href="port-status.html">Port assessment</a> · <a href="codex-collaboration.html">Infrastructure assessment</a></nav>
<section class="callout"><h2>Current recorded status</h2>
<p>Each milestone records its own implementation and review acceptance. Existing prototype functionality is described in the <a href="port-status.html">port assessment</a>; pending milestones do not mean that the prototype has no working features.</p>
<div class="summary-counts"><span><strong>{counts['complete']}</strong> complete</span><span><strong>{counts['in_progress']}</strong> in progress</span><span><strong>{counts['review']}</strong> awaiting review</span><span><strong>{counts['blocked']}</strong> blocked</span><span><strong>{counts['planned']}</strong> planned</span><span><strong>{counts['deferred']}</strong> deferred</span></div>
<p class="small">Counts describe milestones, not percentage of software completed; milestones differ in scope. Dates are planning targets, not automatic status changes.</p>
<p><strong>Next:</strong> {esc(next_action)} Feature cutoff: October 15. Candidate freeze: October 20.</p>
<p class="small">Last record update: <time datetime="{esc(data['updated_at'])}">{esc(data['updated_at'])}</time>. Project steward: {esc(data['steward'])}.</p></section>
<section><h2>Milestones</h2><ol>{toc}</ol><p>Planned → In progress → Awaiting review → Complete. Blocked requires a recorded reason; deferred work requires a dated decision. Completion records acceptance, not merely the presence of code.</p></section>
{''.join(cards)}
<section id="progress-log"><h2>Progress log</h2><p>Dated records are retained when a status changes or new evidence is added. Reopening a milestone should explain what invalidated its earlier acceptance.</p><ol class="history">{updates}</ol></section>
<section id="record-progress"><h2>Record project progress</h2>
<p>This is a shared, version-controlled record. Update the milestone in <a href="https://github.com/welsberr/avida-ed-5-poc/edit/main/docs/roadmap/progress.json">progress.json on GitHub</a> or in a local checkout, append a dated progress entry, and include public evidence such as a commit, test report, or review. Repository write access or a pull request is required.</p>
<p>Run <code>python3 scripts/render_roadmap.py</code>, commit the data and generated page, and push. GitHub Pages rebuilds this view from the record. Validation rejects inconsistent status history and completion without accepted criteria and evidence. Read the <a href="https://github.com/welsberr/avida-ed-5-poc/blob/main/docs/roadmap/README.md">update instructions</a>.</p>
<p>No browser-local checkboxes or visitor submissions change the shared project record. The source history preserves who changed it and why.</p></section>
</main><footer><div class="wrap">Independent proof of concept. Initial record prepared with OpenAI Codex assistance; scientific, instructional, and release acceptance remain with the responsible reviewers.</div></footer></body></html>
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Fail if generated HTML is stale")
    args = parser.parse_args()
    data = json.loads(SOURCE.read_text())
    validate(data)
    html = render(data)
    if args.check:
        assert OUTPUT.read_text() == html, "Run python3 scripts/render_roadmap.py"
        print("Roadmap record valid; generated page current.")
    else:
        OUTPUT.write_text(html)
        print("Rendered docs/roadmap.html")


if __name__ == "__main__":
    main()
