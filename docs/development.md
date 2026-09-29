# Development and reproducible builds

## Pinned inputs

- **Avida 5 base:** `c3fc79383dd418fdfd3cfd6d45ecab434d9fda63` from [Mercere99/Avida](https://github.com/mercere99/Avida). This proof of concept adds its educational web interface and experiment work on top.
- **Prototype snapshot:** the carried-forward interface work was assembled through local revision `52b4bb1b7a1cf16baaa815918143e1c7d8a9a4f4` before this repository was created. This repo begins with a focused project snapshot; the referenced Avida commit and preserved source make the engine lineage explicit.
- **Empirical:** submodule pinned to `f41efa29f047d81a56037fe9f6307c958dd6f3c7` from [devosoft/Empirical](https://github.com/devosoft/Empirical), including its submodules.
- **Web compiler:** Emscripten SDK 6.0.10, pinned by image digest in `Dockerfile`.
- **Graph library:** Plotly.js 1.53.0, bundled with its upstream license notice.
- **Cross-origin isolation helper:** `coi-serviceworker` 0.1.7, bundled from its MIT-licensed release for static hosting.

## Container build

From a checkout with submodules:

```sh
docker compose up --build
```

The page is served at <http://127.0.0.1:18505/Avida.html>. The server sends the cross-origin isolation headers needed for shared WebAssembly memory and worker execution. Compose binds only to loopback by default.

## GitHub Pages

The [Pages workflow](../.github/workflows/pages.yml) builds the app in the pinned Emscripten container and deploys a static artifact with a landing page at the site root and the app under `/app/`. It initializes Empirical's submodules before building. The published entry point is <https://welsberr.github.io/avida-ed-5-poc/>.

GitHub Pages serves static files and does not let this project set COOP/COEP response headers. Because the Avida app uses WebAssembly threads, it loads the locally bundled `coi-serviceworker` helper, which applies those policies through a service worker. The first app visit can reload once before the worker controls the page. The standalone landing page does not require isolation. The direct local server continues to set COOP/COEP headers itself.

To build without Docker, install the pinned Emscripten release and initialize Empirical:

```sh
git submodule update --init --recursive
make web EMP_DIR=vendor/Empirical CXX_web=em++
cd web
python3 serve.py
```

Open the address printed by the server. If you change the interface code, rebuild from the repository root first.

## Useful checks

```sh
python3 scripts/generate_es_runtime_catalog.py
python3 scripts/finalize_es_catalog_provenance.py
python3 scripts/validate_es_catalog.py
make test-education EMP_DIR=vendor/Empirical
make test-education-plots
```

If regenerating the Spanish draft from a local translation service, set `TALKORIGINS_TRANSLATION_COMPENDIUM` to a local compendium file first. The scripts record its content hash and a generic source label, not the machine-specific file path.

The Spanish catalog intentionally remains release-ineligible until a competent human review is recorded. The scripts validate its placeholders, hashes, and provenance; they do not establish translation quality or layout fit. A browser and visual pass are still needed for responsive layout, Plotly hover/export, English/Spanish labels, and the old Avida-ED visual landmarks.

## Architecture notes

The experiment measurements are collected by `EducationRecording.hpp`. That code serializes a small JSON payload and calls `window.AvidaEducationPlots`, defined by `web/education-plots.js`. The adapter owns only the chart presentation; it does not calculate or alter population measurements. An accessible HTML table carries the same observations.

The runtime package is built from this repository plus the pinned Empirical submodule. Avida 5 engine changes should be traced to the upstream code and kept separate from educational interface changes when practical. The expected upstream review boundary for Plotly is a future, optional Empirical helper; this repository currently contains no modifications to Empirical itself.
