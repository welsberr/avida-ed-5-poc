# From a working demonstration to Avida-ED 5

**Reassessment and proposed roadmap · 30 September 2026 · Demonstration: 23 October 2026**

The strongest next step is to organize the port around **what learners must be able to do and explain**, with the existing interface map providing the control-level detail. Use a small, reviewed knowledge graph to connect those tasks to scientific definitions, tutorial instructions, implementation, and verification. Keep executable tests and instructor review as the evidence that the connections hold.

For October 23, finish and demonstrate one dependable mutation-and-variation activity. Show the infrastructure helping resolve an actual problem in that activity. Do not make completion of the entire legacy interface, or integration of every infrastructure repository, a condition of a successful demonstration.

This is a proposed delivery plan, not a report that the new graph, integrations, or acceptance tests have already been implemented. The baseline is prototype revision `3dd5f8c`; the earlier [port assessment](port-status.html) and [collaboration assessment](codex-collaboration.html) remain useful context. This plan extends the first experiment roadmap and narrows the broader upstream web design to educational priorities.

## What changes in the assessment

The conceptual map remains valuable: it captures labels, ranges, choices, and locations that matter to tutorial continuity. It cannot by itself establish behavioral equivalence, scientific equivalence, or successful learning. Maintain those as separate dimensions instead of compressing them into a single percentage of “ported” features.

| Dimension | Acceptance evidence |
|---|---|
| Task and interaction continuity | A learner can follow identified v4 tutorial steps, including transfers between views, with documented exceptions. |
| Scientific meaning | Metric definitions, mutation semantics, units, missing values, and experimental controls are reviewed against engine behavior. |
| Learning opportunity | The activity asks for a prediction, observations, and an explanation that can reveal specified misconceptions. |
| Learning effectiveness | Learner responses and an appropriate evaluation support a claim about learning; successful clicks alone do not. |
| Operational reliability | Repeatable browser workflows, saved-data round trips, error handling, and versioned evidence pass. |
| Presentation and access | Familiar desktop layout, usable small-screen adaptations, keyboard access, and reviewed localized controls survive visual checks. |

Model-based GUI testing offers a basis for interaction coverage; it does not supply the scientific or educational oracle automatically. Typed requirements relations can improve change-impact analysis, but mere graph reachability should identify **candidates for review**, not prove that a change broke them. These distinctions follow the scope of [Memon’s event-flow work](https://www.cs.umd.edu/~atif/pubs/MemonSTVR2007-abstract.html) and [Goknil and colleagues’ requirements analysis](https://doi.org/10.1016/j.infsof.2014.03.002). Applying them here is a design recommendation, not a tested result for Avida-ED.

Preserve the familiar Population, Organism, and Analysis workspaces, legacy graphics and palettes, and relative control placement. Explicitly distinguish a deliberate redesign from an unfinished port. Do not reproduce a legacy defect merely to match its behavior. Where engine differences prevent equivalence, preserve the learning purpose and explain the difference.

## Four connected models, with different rules

1. **Learning model:** concepts, proposed prerequisites, misconceptions, learning objectives, prompts, and assessment criteria. A strict prerequisite cycle warrants review; mutually reinforcing concepts may instead form a teaching unit. An edge remains an instructor-reviewable hypothesis.
2. **Interaction model:** states, actions, preconditions, and observable outcomes. Configure → run → pause → inspect → resume is a valid cycle. Include reset, cancellation, failed import, and stale asynchronous responses.
3. **Traceability model:** objective → task → requirement → implementation → test execution → evidence, plus tutorial and source references. A test definition is different from a passing execution against a named build.
4. **Software and data model:** engine capabilities, metric definitions, configuration, storage formats, and dependencies. Use this to identify compatibility and migration work without treating dependency counts as educational importance.

Keep stable IDs and typed relations in small versioned JSON or YAML files. Generate the conceptual map and readable status report from them after the schema stabilizes. No graph database or interactive graph visualization is needed for the first milestone. A short list of actionable findings will be more useful during the demo than a dense network picture.

Every consequential assertion should have its source locator, source revision or hash, relation type, scope, rationale, author/reviewer, timestamp, confidence, and review status. Use separate fields for implementation status and evidence freshness. Distinguish **unknown**, **not implemented**, **failed**, **passed for this build**, and **deliberately different**. Keep public evidence separate from private memory. A confidence label is a review judgment, not a calibrated probability.

### What to reuse from Didactopus

Reuse its learning-pack structure, prerequisite analysis, curriculum-path checks, and review ledger for the learning subgraph. Build a small adapter for software traceability and workflow checks rather than forcing controls and tests into concept-prerequisite edges.

Inspection of Didactopus revision `f0b0e8ac` found two integration hazards in `src/didactopus/graph_qa.py`: failed pack loading can produce zero graph warnings, and the “bottleneck” rule counts direct dependents with a fixed threshold of three. Therefore, require successful pack validation before graph analysis; expose invalid input as an error; label direct degree accurately; and treat flatness, depth, and bottleneck thresholds as advisory heuristics. Their usefulness for this curriculum has not been validated. Centrality alone must not determine port priority.

For the demo, implement only five useful queries:

- Which selected tutorial tasks lack an implementation or a current passing workflow test?
- Which learning objectives have no observation or explanation prompt?
- Which scientific measurements lack a reviewed definition or a suitable test oracle?
- Which evidence became stale after a relevant source, configuration, or code change?
- Which tasks, strings, help text, and tests need review if a particular contract changes?

For each finding, show the missing link or the typed path and its source. Test the analyzer with a missing edge, stale revision, invalid pack, and valid interaction loop. A deliberately injected defect must be labeled as a test fixture. Do not present it as a discovered production failure.

## The demonstration’s educational task

Use the existing population experiment: **How does the mutation setting affect the variation present in a population?** It is related to the introductory topic in the [official lab-manual sequence](https://avida-ed.github.io/curriculum/lab-manual/), but it is not a validated reproduction of that legacy exercise. That sequence also provides the longer-term progression toward selection, fitness, and independent inquiry.

Keep the bundled ancestor, 10 × 10 world, instruction profile, environment, and scheduling configuration fixed. Compare 0% and 1% per-site mutation settings using seeds 42, 43, and 44, with the same recording policy and target update. Audit whether generic settings controls can silently invalidate the preset. Lock incompatible edits or clearly mark the run as a modified experiment and retain its resolved configuration.

| Learner action | Evidence of reasoning to elicit | Software contract |
|---|---|---|
| Predict before running | Can new sequences appear? Must diversity increase continuously? | Preserve prediction separately from simulation output. |
| Run both treatments and inspect plots | Distinguish observations from predictions and compare replicate variation. | Keep treatment, seed, update, metric definitions, and configuration attached to records. |
| Inspect a sampled organism and ancestor | Explain exact sequence difference without confusing it with lineage or fitness. | Use exact sequence identity with the appropriate instruction profile, not genotype tracker IDs alone. |
| Save, reopen, and compare results | Another reader can reconstruct what was compared. | Versioned exports and round trips preserve scientific data and distinguish results from resumable checkpoints. |
| Explain the outcome and limitations | Richness is neither mutation count nor proof of adaptation; updates are not generations. | Provide prompts and metric help, not an automatic claim that learning occurred. |

Review the mutation implementation carefully: instruction randomization can choose the original instruction, so the setting is not an exact proportion of changed sites. The rewarded-task environment is not neutral evolution. Equal seeds across treatments do not imply paired random histories. Three replicates per treatment support an illustration, not a general effect-size claim. Numerical similarity across Avida versions is not an equivalence criterion unless their semantics and randomization are demonstrably comparable.

A small instructor-reviewed explanation rubric is enough for this milestone. Later, evaluate misconceptions and transfer to a new scenario with learners. Do not infer durable learning from a successful demonstration or a graph with complete links.

## Delivery plan to October 23

The dates below are proposed working windows, not estimates of measured productivity. Implementation can be delegated; the maintainer retains scientific, instructional, and release decisions. Obtain an instructor review and a qualified Spanish-language review where available. If either is unavailable, state the limitation and narrow the release claim.

### September 30–October 3: establish the task contracts

Select three workflows: configure/run/inspect; save/reopen/transfer; compare/explain. Identify the relevant legacy tutorial segments and record exact timestamps and version-specific exceptions. Start with about 15–25 reviewable requirements, not a full reverse engineering of the app. Correct guide text that still tells users to open “Compare runs” when the visible workspace is “Analysis.”

Deliver a scoped task inventory, metric definitions, instructor-review questions, and an evidence register. Record the live baseline before changes. Exit when every selected requirement has an observable expected result and an explicit evidence status. A tutorial not watched is marked unreviewed.

### October 4–9: make the analysis executable

Implement the typed records, five queries, a small Didactopus learning pack and adapter, and a generated report. Link existing meaningful tests before adding new ones. Run validation before graph QA. Add the malformed-input and stale-evidence checks described above.

Create a reviewed GroundRecall handoff with the selected task, current revision, invariants, pending decisions, and next verification commands; retrieve it in a fresh context and verify it against Git. The earlier bounded searches did not surface an exact Avida task record, which does not establish that none exists.

Exit with a reproducible report that identifies at least one real unresolved requirement and explains its evidence. If the graph work exceeds this window, retain the same records as a simple traceability table and finish the learner workflow first.

### October 10–15: close the chosen workflow

Resolve the highest-impact blocker found in those tasks. Verify target-update inputs survive Run, presets preserve their controlled variables, records are not duplicated, selected organisms load correctly, and saved results reopen with their metadata. Keep chart hover, zoom, reset, and export stable during live updates. Limit Analysis work to what this exercise requires; show its remaining v4 differences honestly.

Exercise the Spanish path using the documented local translation method, stable IDs, glossary, placeholder checks, and provenance. Inspect actual rendered controls, option replacement, tooltips, Freezer, settings, and Analysis in English and Spanish, plus expanded-string and RTL test modes. Do not equate catalog coverage with translation acceptance.

Exit with the complete activity working from a clean browser session and a portable container build. Freeze feature scope on October 15; do not add a general graph platform or broaden engine capabilities for the demo.

### October 16–19: verify and rehearse

Run the workflow on the presentation laptop and a second browser. Check desktop and narrow layouts, keyboard navigation, reload, data export/import, and error recovery. Use fixtures for deterministic record checks and scientific invariants; do not demand matching stochastic trajectories across engines. Rehearse the full 20-minute demonstration twice and record actual duration.

Exit with linked test executions, visual-review evidence, known limitations, a reviewed Spanish sample, and a selected build. If a review is unavailable, retain “development preview” labeling. Any failed core configure/run/save/reopen/compare path blocks the live-app claim.

### October 20–23: freeze, prepare fallbacks, demonstrate

On October 20, tag the candidate and archive its image digest, source revisions, configuration, exports, and report. On October 21–22, verify Pages and the local container server; cache the six-run data set and record a short walkthrough. Serve the local fallback over HTTP with the required browser headers. A recording is explicitly labeled as recorded, and cached results as previously generated.

On October 23, present a bounded engineering case: what worked, what the infrastructure helped expose, and what remains uncertain. Reserve approximately three minutes for context, twenty for the demonstration, and seven for discussion.

## A twenty-minute demonstration

| Minutes | Show | Point to establish |
|---|---|---|
| 0–3 | The educational question, prediction, familiar workspace, and pinned build recipe; use a prebuilt container. | A concrete teaching task and reproducible environment. |
| 3–6 | A fresh-context handoff retrieved through GroundRecall, then checked against repository state. | Decisions can be recovered with sources; memory is not current-state authority. |
| 6–10 | The task report, one real gap, its source path, and the reviewed repair and rerun. | Didactopus and traceability checks can make an omission actionable. Prepare the repair beforehand; no live coding dependency. |
| 10–17 | Run both mutation settings, inspect an organism, use the live plots, save/reopen, and compare with the archived replicates. Show a checked Spanish workflow. | The resulting software supports an interpretable activity. |
| 17–20 | The ClaimWright-guided review record, remaining gaps, and the next milestone. | Show exactly which checks changed a claim or action, without overstating enforcement or benefits. |

## Make infrastructure contributions observable

Record **retrieved information → decision changed → artifact changed → verification**, including cases where a tool adds no useful information. This is stronger evidence than listing installed repositories.

- **GroundRecall:** compare recovery using the same repository and handoff summary with and without retrieval. Record correct decisions recovered, incorrect/stale suggestions, repeated questions, and review time. A small demonstration is a case study, not a causal productivity experiment; balance order or use independent contexts if making a comparison.
- **Didactopus:** show actual pack validation and prerequisite or assessment-gap findings, their disposition, and the adapter’s separate traceability checks. Attribute each finding to the component that produced it.
- **ClaimWright:** apply its current public-artifact standards and preserve the review record. If an executable gate is connected, demonstrate a supported check and its result. Otherwise describe checklist use accurately; do not imply automatic enforcement. Log both useful catches and false alarms.
- **SciSiteForge/TalkOrigins methods:** document the specific reused source or workflow and the Avida adaptation. Demonstrate a terminology or overflow correction plus visual recheck. Method reuse is distinct from importing a package.
- **doclift and CiteGeist:** use them only where a selected source genuinely needs ingestion or citation verification. Record the converted passage or verified reference and its use. They need not appear live, and proposed use must not be reported as past contribution.

Start a prospective effort log separating maintainer direction/review, implementation work, automated runtime, and rework. There is no reliable historical human-hours measurement or controlled “Codex alone” comparison for this project. The earlier numeric estimate should not be used as evidence of savings. A comparison must give the baseline the same task, repository, and ordinary documentation access.

## After the demonstration: a functional Avida-ED 5

These phases have acceptance gates rather than promised dates. Estimate them after the scoped task audit and upstream review. Keep the educational application shell, engine adapter, measurements, storage, localization, and reports separate. The demo task and its tests remain a maintained regression example throughout.

### Phase 1: agree on the release scope and scientific contracts

Extend the task inventory across the lab manual and existing tutorial videos. Review priorities with the Avida-ED group; pin supported engine and instruction profiles, metric meanings, mutation behavior, and reproducibility limits. Classify requirements as preserved, redesigned with explanation, deferred, or unsupported. Generate a public gap report with denominators and evidence dates; do not count every graph edge as progress.

**Exit:** a reviewed functional-release scope and scientific oracle for each supported measure. Legacy parity and new lessons are separately identifiable. This scope review can begin before October 23 if it does not displace the demo work.

### Phase 2: Population, Freezer, and trustworthy persistence

Complete the supported dish configuration, ancestor placement, mutation and environment settings, run lifecycle, population measurements, and familiar color modes. Restore tutorial-relevant transfers among configured dishes, organisms, and populated dishes, with keyboard alternatives to drag-and-drop. Test invalid imports without damaging existing work.

Separate organism artifacts, configurations, recorded results, and resumable checkpoints. Version each format; test migrations, storage limits, download/import, immediate restore, and continuation where deterministic continuation is supported. Data-only results must not masquerade as complete engine checkpoints.

**Exit:** selected population/tutorial workflows survive save, reopen, and cross-browser transfer with scientific metadata intact. This is the foundation for full Analysis.

### Phase 3: Organism and Analysis continuity

Complete the supported Organism cycle controls and state displays in a recognizable arrangement; validate isolated organism evaluation without changing the live population or its random stream. Map fitness, viability, gestation, and task observations to reviewed definitions rather than substituting similarly named measures.

Implement the v4-style Analysis workflow for up to three populated dishes, supported left/right measures, trace colors, remove/replace operations, interactive plots, and export. Keep the newer lesson-run comparison as an explicitly identified capability. If a legacy measure cannot be supported, make that gap visible and revise affected lesson guidance.

**Exit:** instructor-selected Organism and Analysis tutorial walkthroughs pass with recorded exceptions; differential tests are used only where old and new semantics are comparable.

### Phase 4: curriculum, accessibility, localization, and classroom pilot

Broaden activities from mutation to selection, fitness, and independent experimental design using the reviewed learning graph. Align prompts and rubrics with concepts and misconceptions; check whether the software exposes the necessary evidence. Complete contextual help, reviewed Spanish, visual regression coverage, keyboard/screen-reader checks, and the supported browser/device matrix.

Pilot the supported curriculum with instructors and learners. Observe task completion, interpretation errors, data loss, and tutorial mismatches separately. Evaluate learning with appropriate consent and study design if making research claims; do not turn routine demo telemetry into learner research data.

**Exit:** no unresolved critical workflow or data-integrity defects within the agreed scope, instructor acceptance, published limitations and support instructions, and an explicit release decision by the responsible maintainers. A functional teaching release does not require every possible engine feature, but it does require complete supported activities and honest compatibility boundaries.

Alternate CPUs, continuous worlds, knockout experiments, and a general dashboard framework remain separate extensions unless a reviewed learning task makes them necessary. Follow Avida upstream through a pinned compatibility boundary and small reviewable changes; avoid tying the education release to every upstream design proposal.

## Evidence, stewardship, and limitations

Prepared with OpenAI Codex assistance for Wesley R. Elsberry’s review. Proposed project steward: Wesley R. Elsberry; instructional and language reviewers are not yet assigned. No external endorsement or completed instructor review is implied. Evidence inspection: **2026-09-30, approximately 11:01 UTC**. Roadmap dates are planning judgments, not measured effort estimates.

- **Current functionality and gaps — high confidence for documented source facts, moderate for tutorial impact.** Baseline [prototype revision 3dd5f8c](https://github.com/welsberr/avida-ed-5-poc/tree/3dd5f8cb0b36b048c297592265df7947ed964397), its [control map](https://github.com/welsberr/avida-ed-5-poc/blob/3dd5f8cb0b36b048c297592265df7947ed964397/docs/avida-ed4-consilience-map.md), experiment guide, and earlier first-experiment roadmap were inspected. This reassessment did not repeat the full application acceptance suite or watch every tutorial.
- **Educational sequence — high confidence in the published sequence, learning effectiveness untested here.** [Official lab manual](https://avida-ed.github.io/curriculum/lab-manual/) and [tutorial index](https://avida-ed.github.io/help/) establish source targets, not completed compatibility checks.
- **Didactopus implementation — high confidence in the inspected code behavior.** [Didactopus](https://git.cns.fyi/welsberr/Didactopus), local revision `f0b0e8ac55459e05bdf16604ec971ea266278b84`; graph QA, prerequisite-analysis, curriculum-path-quality, and review-workflow documentation. The new Avida adapter remains proposed.
- **Research rationale — moderate confidence in applicability.** Memon (2007), *An event-flow model of GUI-based applications for testing*, STVR 17:137–157; Goknil et al. (2014), *Change impact analysis for requirements: A metamodeling approach*, IST 56:950–972, [doi:10.1016/j.infsof.2014.03.002](https://doi.org/10.1016/j.infsof.2014.03.002). Author/institutional abstracts were consulted; neither establishes this project’s effectiveness.
- **Infrastructure impact and schedule — hypotheses to evaluate.** Bounded memory retrieval and repository evidence informed this plan. No quantitative productivity benefit, learning gain, or October completion guarantee has been established.

Keep future decisions, changes and reasons, test executions, and review dispositions in a versioned work record. Regenerate the public status after meaningful changes rather than allowing an old passing result to certify a new build.
