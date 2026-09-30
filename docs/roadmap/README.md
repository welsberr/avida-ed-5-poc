# Maintaining the roadmap and progress page

The [published tracker](https://welsberr.github.io/avida-ed-5-poc/roadmap.html)
summarizes the [detailed plan](../porting-learning-roadmap.md).
`progress.json` is the authoritative progress record; `../roadmap.html` is generated.
No server, account database, or browser-local storage is involved.

## Record an update

1. Edit the relevant milestone in `progress.json`. Preserve its stable ID. Update
   its status, owner, acceptance criteria, next action, blockers, and evidence as
   appropriate. Never mark acceptance criteria met just because code exists.
2. Append an entry to `updates`, in chronological order, with the date,
   milestone ID, resulting status, recorder's name, and a concise summary of
   what changed and why. Add an entry for new evidence even if status stays the
   same. Do not overwrite earlier entries to conceal a regression.
3. Update `updated_at` with the actual UTC recording time. Link evidence to
   public, preferably immutable commits, reports, workflow runs, or review
   records. Name the reviewer and the build/configuration in the evidence when
   acceptance depends on them. Do not copy private GroundRecall material into
   this public record.
4. Run from the repository root:

   ```sh
   python3 scripts/render_roadmap.py
   python3 scripts/render_roadmap.py --check
   ```

5. Inspect the rendered page, commit both data and generated HTML, and push or
   open a pull request. The Pages workflow validates and regenerates the page
   before deployment. A data-only edit through GitHub also updates the live
   view; regenerate the checked-in HTML on the next local update.

Example log entry (illustrative, not a completed project action):

```json
{
  "date": "2026-10-02",
  "milestone": "demo-contracts",
  "status": "in_progress",
  "author": "Recorder name",
  "summary": "Linked the first tutorial segment; metric review remains pending."
}
```

## Status and acceptance rules

- `planned`: work defined; execution not recorded.
- `in_progress`: execution underway, with an explanatory log entry.
- `review`: implementation is ready for the required acceptance review.
- `blocked`: a concrete blocker is listed, with a next action to resolve it.
- `complete`: every acceptance criterion is met, evidence is linked, and all
  prerequisite milestones are complete.
- `deferred`: a dated decision explains why it is outside current work.

The validator checks unique IDs, dependency references and cycles, dates,
recognized statuses, acceptance/evidence requirements, and consistency between
each current status and its latest log entry. It cannot verify the truth of
linked evidence or substitute for scientific, instructional, or language review.

For a regression, reopen the milestone, mark affected criteria pending, and add
a dated explanation. Reconsider completed dependent milestones; stale evidence
must not remain an implicit certification of new code. Preserve earlier evidence
as historical evidence and explain its scope in the link label or update.

Dates are targets, not automatic completion or overdue flags. Counts are not
effort-weighted and must not be advertised as percentage of the application
ported. Extend or split milestones only with a logged scope decision. The initial
record marks the published planning deliverable complete and all implementation
milestones planned; it does not erase the existing prototype's working features.
