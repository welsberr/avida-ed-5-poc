"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

const targets = new Map([
  "population-history-plot",
  "richness-history-plot",
  "ancestor-history-plot"
].map(id => [id, { id }]));
targets.set("education-plots", { id: "education-plots", hidden: true });
const rendered = [];
const extended = [];
const purged = [];
const browser = {
  document: { getElementById: id => targets.get(id) || null },
  Plotly: {
    newPlot: (target, data, layout, config) => rendered.push({ target, data, layout, config }),
    extendTraces: (target, update, indices) => extended.push({ target, update, indices }),
    purge: target => purged.push(target)
  }
};
browser.window = browser;

vm.runInNewContext(
  fs.readFileSync(new URL("../web/education-plots.js", `file://${__filename}`), "utf8"),
  browser,
  { filename: "education-plots.js" }
);

const payload = (runId, locale, updates) => ({
  run_id: runId,
  locale,
  updates,
  population: updates.map(update => update * 2 + 1),
  richness: updates.map(update => update + 1),
  ancestor: updates.map(update => 100 - update),
  labels: {
    update: "Update",
    population: "Population",
    population_title: "Population size over time",
    population_axis: "Organisms",
    richness: "Sequence richness",
    richness_title: "Sequence richness over time",
    richness_axis: "Distinct sequences",
    ancestor: "Ancestor sequence",
    ancestor_title: "Ancestor sequence over time",
    ancestor_axis: "Percent of population"
  }
});

async function main() {
await browser.AvidaEducationPlots.update(JSON.stringify(payload("run-1", "es", [0, 1])));

assert.equal(targets.get("education-plots").hidden, false);
assert.equal(rendered.length, 3);
assert.equal(extended.length, 0);
assert.deepEqual(
  JSON.parse(JSON.stringify(rendered.map(chart => chart.data[0].y))),
  [[1, 3], [1, 2], [100, 99]]
);
for (const chart of rendered) {
  assert.equal(chart.layout.uirevision, "run-1");
  assert.equal(chart.config.responsive, true);
  assert.equal(chart.config.locale, "es");
  assert.equal(chart.config.scrollZoom, true);
  assert.equal(chart.config.doubleClick, "reset");
  assert.equal(chart.config.toImageButtonOptions.format, "png");
  assert.equal(typeof chart.data[0].hovertemplate, "string");
}

await browser.AvidaEducationPlots.update(JSON.stringify(payload("run-1", "es", [0, 1, 2, 3])));
assert.equal(rendered.length, 3, "live samples append without redrawing the graph");
assert.equal(extended.length, 3);
assert.deepEqual(
  JSON.parse(JSON.stringify(extended.map(item => item.update))),
  [
    { x: [[2, 3]], y: [[5, 7]] },
    { x: [[2, 3]], y: [[3, 4]] },
    { x: [[2, 3]], y: [[98, 97]] }
  ]
);
assert.ok(extended.every(item => item.indices[0] === 0));

await browser.AvidaEducationPlots.update(JSON.stringify(payload("run-1", "en", [0, 1, 2, 3])));
assert.equal(rendered.length, 6, "a language change recreates modebar tooltips once");
assert.equal(purged.length, 3);
assert.equal(rendered[3].config.locale, "en");

await browser.AvidaEducationPlots.update(JSON.stringify(payload("run-2", "en", [0])));
assert.equal(rendered.length, 9, "a new run starts a fresh graph");
assert.equal(purged.length, 6);

await browser.AvidaEducationPlots.update(JSON.stringify({ updates: [] }));
assert.equal(purged.length, 9);
assert.equal(targets.get("education-plots").hidden, true);
console.log("education Plotly adapter: persistent streaming charts, locale and run resets, export, and empty reset passed");
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
