# Live graphs and color continuity

## Plotly in the Avida-ED prototype

The result panel has three live graphs: population size, exact sequence richness, and ancestor-sequence fraction. Avida samples the experiment on the main application workflow; the UI sends the recorded values to a small browser-side adapter, which updates Plotly. The graph containers stay mounted while the results table refreshes, and the adapter appends new samples to each existing trace instead of redrawing the complete history. A data table remains available so exact observations can still be read and copied without relying on graph interaction.

The Plotly controls provide hover values, pan/zoom, reset, and PNG export. Scroll-wheel zoom and double-click reset are also enabled. `uirevision` keeps a user's selected view stable as new samples arrive, then resets it for the next run.

The bundled Plotly.js is version 1.53.0, the version already used by the local Avida-ED 4 checkout. Its MIT notice is retained in the file. Before release, review whether a newer pinned release is appropriate; the current version is chosen for compatibility and reproducibility, not as a recommendation to freeze on it indefinitely.

## What an Empirical contribution might look like

This app currently uses an Avida-specific JavaScript adapter instead of changing Empirical. That lets the team first assess a real chart use in context. A possible Empirical contribution could be a small optional Plotly bridge for declaring a chart container, passing batches of series data, and connecting updates to the browser library. It should not make Plotly mandatory for Empirical users.

Before proposing a PR, the adapter should be tested across supported browsers and chart lifecycles; its behavior with Emscripten threading, live updates, localization, export, and large histories should be documented. It should also be compared with Empirical's existing D3 wrapper so the PR addresses a clear gap rather than adding a second way to create every chart. This prototype provides evidence and an integration seam, not an upstream contribution by itself.

## Avida-ED 4 palettes retained here

The numeric color maps and discrete group palette come from `colormap.js` in [Avida-ED 4](https://github.com/Avida-ED/Avida-ED4/blob/main/colormap.js), pinned in the source checkout at commit [`fcfc5c6`](https://github.com/Avida-ED/Avida-ED4/commit/fcfc5c60e03479c85eadad083ffb5e7d92a95781). That file explicitly describes its color choices as informed by color-vision accessibility references and points to the Gnuplot2 and Viridis sources. The palette values are retained to keep the visual cues consistent with Avida-ED 4 and its tutorial videos.

- **Gnuplot2** is the default numeric map, matching the Avida-ED 4 population view for fitness, offspring cost, and energy acquisition rate.
- **Viridis** and **Cubehelix** remain selectable numeric maps, also carried over from v4.
- **The 28-color categorical set** is the v4 `parentColorList` used for distinguishable groups.
- Plotly's three run-history series use the v4 named red, blue, and black colors.

The values are stored in [`Ed4ColorPalettes.hpp`](../source/web/Ed4ColorPalettes.hpp). [`import_ed4_color_palettes.py`](../scripts/import_ed4_color_palettes.py) regenerates that header from a supplied v4 `colormap.js`, which makes the transformation inspectable. The original Avida-ED 4 repository is MIT-licensed; its copyright and license are acknowledged in the generated header. Colors for general page decoration remain separate from these data palettes.
