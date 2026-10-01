# A short guide to the prototype experiment

## What is being modeled?

Avida represents living systems with digital organisms: short instruction sequences that run on a virtual computer. In this prototype, organisms occupy cells in a 10 by 10 world, use a shared instruction set, and reproduce. Mutation can change an instruction when an offspring is made. The organisms also compete under the configured resource and task rules.

The lesson changes one setting: the probability of a substitution mutation. It compares 0% and 1%. The packaged experiment keeps the ancestor, computer, instruction set, world, task rewards, and other configuration fixed. Its nine rewarded logic tasks and reward-strength tiers match the Avida-ED 4 default workspace; the POC applies those factors to metabolic rate rather than using v4's energy-reward mechanism. This makes the comparison easier to follow, while still leaving the usual stochastic variation between runs.

## A suggested first run

1. Choose the 0% mutation treatment and seed 42.
2. Use **Run to target** or advance the population one update at a time.
3. Watch the population and the three charts. Hover over the lines to read the update and value. Use the Plotly toolbar to zoom or reset; its camera button saves the displayed graph as a PNG.
4. Repeat with 1% mutation, then try seeds 43 and 44 for both settings.
5. Open **Compare runs** to inspect completed runs together.

Each seed starts a run; it does not promise that a 0% run and a 1% run will follow paired random histories. A run is one stochastic trajectory. Compare patterns across the displayed runs and keep that limitation in mind.

## What the measurements mean

- **Population size** counts occupied cells at the sampled update.
- **Sequence richness** counts distinct, ordered instruction sequences present at that update. It is not the number of mutations and does not count every sequence that ever appeared.
- **Ancestor sequence fraction** is the share of occupied cells whose current sequence exactly matches the starting ancestor. It is undefined when the population is empty.

These are descriptions of what the model contains. More sequence richness is not, by itself, evidence of greater fitness, adaptation, or a universal effect of mutation. Fitness can depend on the model’s scoring and environment.

## Other controls

The population view lets you select a color mode. Numeric modes can use the Gnuplot2, Viridis, or Cubehelix map. Group colors follow the palette used in Avida-ED 4. Select a cell to inspect an organism. Organisms can be saved to the browser-local Freezer and placed into an unstarted population. Configuration and checkpoint actions are also available there.

The Freezer stores data in the current browser. It is not a cloud account or a backup service. Export files when you need to move data to another browser or computer.

## Interpreting the prototype

The charts show results from the configured model, not experiments on biological populations. The interface is designed to make the settings and measured quantities inspectable. The prototype is not yet a curriculum evaluation, and its small set of replicates should not be treated as a statistical study.
