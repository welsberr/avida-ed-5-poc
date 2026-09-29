---
title: "Mutation and genetic variation: student walkthrough"
lang: en
status: validated
---

# Mutation and genetic variation

> **Validated in the English UI:** The current worktree includes prediction
> and observation notes, completed-run comparison, experiment JSON Save/Open,
> completed-data CSV export, and resuming a paused run from a fresh page. The
> browser walkthrough also checked the long-string and RTL pseudolocales for
> layout and preservation of simulation state. The Spanish catalog remains an
> unreviewed draft and is not enabled in the application. This verifies the
> implemented controls and archive path; instructors should review lesson
> wording and supervise student interpretation before classroom use.

## Question

How does the per-site mutation setting affect the genetic variation present in a
population over time?

Before you run anything, write a prediction for each question:

- Will any new genome sequences appear at 0% mutation? At 1%?
- Must the number of sequences rise at every update?
- Will two runs that start with the same seed follow the same path?

Use the **Prediction** field in **Learning record**. After the experiment, use
**Observations** to compare your prediction with all six runs and note what the
experiment can and cannot establish. Both notes travel with the saved JSON file.

## Set up the experiment

Choose **Mutation variation · every update**. The standard run uses a
10 × 10 population, the bundled ancestor, the same instruction set and reward
environment, and a target of update 250. The three starting seeds are 42, 43,
and 44. Leave the other settings fixed.

The treatment is a **per-site mutation setting**. At reproduction, each copied
genome site has the stated chance of being selected for instruction
randomization. The replacement instruction can match the instruction that was
already there. Therefore, 1% is not exactly 1% changed sites, a per-genome
probability, or a count of mutation events.

The environment rewards logic tasks, so selection is active. This is not a
neutral-evolution experiment.

## Run all six treatment–seed combinations

Complete each row once. Do not reroll a run because its result looks unexpected.

| Run | Treatment | Seed | Target |
|---:|---:|---:|---:|
| 1 | 0% | 42 | update 250 |
| 2 | 0% | 43 | update 250 |
| 3 | 0% | 44 | update 250 |
| 4 | 1% | 42 | update 250 |
| 5 | 1% | 43 | update 250 |
| 6 | 1% | 44 | update 250 |

For each run, select the treatment and seed, start a new population, and run to
the target. The recorder includes update 0 and each completed update. The run
shelf identifies the treatment, seed, final update, sample count, and completion
status. Check those labels before moving to the next row.

Using the same seed in both treatments gives each treatment the same initial
seed value; it does **not** pair later random events. A mutation can change how
many random numbers the simulation consumes, so later trajectories can diverge.

## Inspect and compare

Select an occupied cell and open the sequence view. Compare its ordered
instruction sequence and length with the bundled ancestor. Positions are
compared directly; this is not sequence alignment, and an insertion or deletion
can shift later positions. The view reports sequence differences only; it is not
a parent–offspring execution trace or a complete history of mutations.

Choose **Compare completed runs** after at least two runs are complete. Compare
all six runs when available. The view overlays completed trajectories for
population size and sequence richness and lists each run's endpoint with its
treatment and seed. It does not include incomplete runs in the comparison. Keep
the 0% and 1% runs separate and retain the seed labels. Look at the time series
and endpoint values for:

- **Population size (`N`):** number of occupied cells in the recorded snapshot.
- **Sequence richness (`S`):** number of distinct, ordered genome sequences
  present in the population at that update. If the population is empty, `S = 0`.
- **Ancestor-sequence fraction:** share of the population whose current sequence
  exactly matches the original ancestor. This is not a measure of ancestry.

Sequence richness is not the number of mutation events and does not include
sequences that appeared and disappeared before a sample. A higher value does not
show higher fitness, adaptation, or beneficial mutation. A fitness display, if
shown, describes the engine's scheduling rate relative to gestation cost; it is
not a direct measure of reproductive success in the recorded runs.

An update is a simulation scheduling unit, not one generation. Equal updates do
not guarantee equal numbers of births or mutation opportunities. With only three
seeds per treatment, describe the results as observations from these runs. Do
not pool incomplete runs into a mean or make a significance or universal-cause
claim.

## Save, reopen, and continue

The integrated application preserves the experiment notes, settings, completed
run history, and one resumable active run. To check that workflow:

1. During a standard run, pause at update 100 and choose **Save experiment
   JSON**. If the simulation is still finishing an update, saving waits for the
   completed update boundary.
2. Open a fresh application page and use **Open experiment JSON** to select the
   downloaded file.
3. Confirm the active run still identifies the right treatment and seed and is
   paused at update 100. Confirm the saved update-100 row appears only once.
4. Continue that same run to update 250. Its final metrics should match an
   uninterrupted run from the same saved state in this Avida build.

The JSON file contains the paused engine checkpoint, experiment notes, and run
history. This save is resumable experiment work. A CSV export is not resumable.

## Export and reflect

Choose **Export completed data CSV** after retaining the six completed runs. Its
columns identify the run, preset, treatment, seed, status, update, population
count, sequence richness, and ancestor-sequence fraction. This file contains
completed run samples; it does not contain the resumable checkpoint or lesson
notes. Check that the exported rows match the values shown in the application.
An empty ancestor fraction should remain an empty CSV field, not become zero.

Return to your prediction. Which observations were consistent with it? Did
sequence richness rise continuously, or did it rise and fall? What can these six
runs say about the populations observed here, and what additional experiments
would be needed to support a broader claim?
