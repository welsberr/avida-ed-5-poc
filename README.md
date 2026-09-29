# Avida-ED 5 proof of concept

An early, working web prototype for exploring Avida 5 through an educational interface inspired by Avida-ED 2, 3, and 4.

**For a quick orientation:** Avida is a computer world populated by small programs. Each program is a digital organism. They use instructions, reproduce, and can change over generations. This prototype lets you run a small experiment and watch the population change.

## What you can do

- Run a fixed 10 × 10 population with either 0% or 1% substitution mutation.
- Repeat runs with seeds 42, 43, and 44, then compare the results.
- Watch population size, distinct genome sequences, and the original ancestor’s share change in live Plotly graphs. Hover for values, zoom and reset the view, and save a graph as a PNG.
- Inspect organisms and their instruction sequences, place and save organisms, and save configurations or checkpoints in the browser’s Freezer.
- Switch between English and a Spanish preview. The Spanish catalog is still a draft and needs qualified language review.

The included lesson asks: **How does the mutation setting affect genetic variation?** Mutation can introduce differences; selection and the rest of the environment also shape which organisms remain. The graphs show observations from individual runs. They do not, by themselves, establish a general result.

## Project status

This is a proof of concept, not a completed Avida-ED 5 release. It demonstrates a functional Avida 5 web experiment, a familiar workspace direction, and a path for testing interface and teaching choices. It has not been validated as a replacement for Avida-ED 4 or reviewed for classroom release. See [the experiment guide](docs/experiment-guide.md) and [the researcher overview](docs/researcher-overview.md).

The project builds on the public [Avida 5 repository](https://github.com/mercere99/Avida) and [Empirical](https://github.com/devosoft/Empirical). It reuses selected Avida-ED 4 interface assets and color palettes for continuity with existing tutorial videos. It is an independent prototype and does not imply endorsement by Michigan State University, the Avida-ED team, or the Empirical maintainers.

## Try it

The container setup pins the Emscripten image and Empirical revision so the build does not depend on installing a compiler on your computer. You need Git, Docker, and Docker Compose.

```sh
git clone --recurse-submodules https://github.com/welsberr/avida-ed-5-poc.git
cd avida-ed-5-poc
docker compose up --build
```

Then open <http://127.0.0.1:18505/Avida.html>. The web build needs a server that sends cross-origin isolation headers; opening the HTML as a local file will not work. See [development and build notes](docs/development.md) for the direct Emscripten route and tests.

## Where to look next

- [Experiment guide](docs/experiment-guide.md) — what the controls and measurements mean.
- [Researcher overview](docs/researcher-overview.md) — scope, design intent, and current limitations.
- [Graph and color choices](docs/graphs-and-colors.md) — Plotly behavior, v4 palette provenance, and the Empirical integration seam.
- [Development notes](docs/development.md) — dependencies, reproducible builds, and checks.

## Acknowledgement

OpenAI Codex assisted with implementation and documentation. The project maintainer directs the work and remains responsible for design decisions, review, and release scope.
