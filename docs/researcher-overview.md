# Research-group overview

## Purpose

This repository is a practical Avida-ED 5 user-interface proof of concept. It tests whether a new browser interface can run an Avida 5 experiment while retaining recognizable organization and visual cues from Avida-ED 2, 3, and 4. The goal is to support discussion with the research and teaching teams, not to announce a finished successor.

## Why this experiment?

The prototype includes a repeatable mutation-and-variation lesson. It compares 0% and 1% substitution mutation in a fixed 10 × 10 world, using seeds 42, 43, and 44. The experiment captures population count, exact sequence richness, and the fraction of the population that still has the ancestor sequence. These measurements give the new UI a concrete task: configure a run, monitor it, inspect organisms, preserve runs, and compare outcomes.

The experiment is intentionally bounded. Its setup does not isolate every causal pathway or substitute for a designed study. Runs are stochastic; the same numeric seed across treatments does not guarantee paired random events. Its measures are descriptive, and the preset’s active selection regime means this is not a neutral-evolution demonstration.

## Interface continuity

Avida-ED 3 and 4 aimed to preserve the interface structure of Avida-ED 2. Avida-ED 5 should respect that history because existing tutorials teach users where controls are and what they look like. This prototype therefore uses familiar side-by-side population and inspection areas, retains the supplied Avida-ED 4 graphics, and restores the legacy numeric maps and categorical color set. The goal is continuity where it aids recognition and learning, while leaving room to adapt layouts for mobile screens and new Avida 5 capabilities.

## Architecture at a glance

- **Avida 5** provides the digital-evolution engine, organisms, instruction execution, and experiment settings.
- **Empirical** supplies the C++ web interface and its Emscripten bridge to browser controls.
- **Plotly.js** renders the three live, sampled time series in the browser. The C++ recorder owns the measurements; a small JavaScript adapter sends those values to Plotly.
- **The browser Freezer** stores saved items locally. Export/import files support transfer between browsers.
- **Localization** has English and a Spanish development-preview catalog. Spanish text is not release-reviewed yet.

Plotly is integrated at the Avida-ED layer rather than added to Empirical itself. This keeps the initial change focused on an actual learning application. The adapter is isolated so its boundary can be assessed for reuse in Empirical. See [graphs and colors](graphs-and-colors.md).

## Current limits and review needs

- The application is a prototype and has not passed a formal accessibility or classroom usability review.
- The Spanish catalog is a draft pending competent Spanish-language and scientific terminology review.
- Plotly is currently pinned to the copy already used by Avida-ED 4. A future update should review the Plotly version, image-export behavior, supported browser targets, and bundle size.
- The current experiment is a teaching demonstration. It is not intended to support broad claims about mutation, evolution, or model outcomes.
- UI position and control styling should continue to be compared with Avida-ED 4 and its tutorial videos as the work advances.

The upstream Avida base revision and Empirical revision are pinned in the project history and [development notes](development.md). This repository contains a runnable snapshot so reviewers can build and inspect one concrete workflow.
