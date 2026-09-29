"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

const targets = new Map([
  "population-history-plot",
  "richness-history-plot",
  "ancestor-history-plot"
].map(id => [id, { id }]));
const rendered = [];
const purged = [];
const browser = {
  document: { getElementById: id => targets.get(id) || null },
  Plotly: {
    react: (target, data, layout, config) => rendered.push({ target, data, layout, config }),
    purge: target => purged.push(target)
  }
};
browser.window = browser;

vm.runInNewContext(
  fs.readFileSync(new URL("../web/education-plots.js", `file://${__filename}`), "utf8"),
  browser,
  { filename: "education-plots.js" }
);

browser.AvidaEducationPlots.update(JSON.stringify({
  run_id: "run-1",
  updates: [0, 1],
  population: [1, 3],
  richness: [1, 2],
  ancestor: [100, 66.7],
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
}));

assert.equal(rendered.length, 3);
assert.deepEqual(
  JSON.parse(JSON.stringify(rendered.map(chart => chart.data[0].y))),
  [[1, 3], [1, 2], [100, 66.7]]
);
for (const chart of rendered) {
  assert.equal(chart.layout.uirevision, "run-1");
  assert.equal(chart.config.responsive, true);
  assert.equal(chart.config.scrollZoom, true);
  assert.equal(chart.config.doubleClick, "reset");
  assert.equal(chart.config.toImageButtonOptions.format, "png");
  assert.equal(typeof chart.data[0].hovertemplate, "string");
}

rendered.length = 0;
browser.AvidaEducationPlots.update(JSON.stringify({ updates: [] }));
assert.equal(purged.length, 3);
console.log("education Plotly adapter: live series, interaction config, export, and empty reset passed");
