/* Plotly adapter for the recorded education-run metrics. */
(function () {
  "use strict";

  const plotSpecs = [
    {
      id: "population-history-plot",
      key: "population",
      color: "#000000",
      filename: "avida-ed5-population-history"
    },
    {
      id: "richness-history-plot",
      key: "richness",
      color: "#006ddb",
      filename: "avida-ed5-sequence-richness"
    },
    {
      id: "ancestor-history-plot",
      key: "ancestor",
      color: "#9c0a0a",
      filename: "avida-ed5-ancestor-sequence"
    }
  ];
  const plotLocales = new WeakMap();

  function update(payloadText) {
    if (!window.Plotly) return;
    const payload = JSON.parse(payloadText);
    const locale = payload.locale === "es" ? "es" : "en";
    for (const spec of plotSpecs) {
      const target = document.getElementById(spec.id);
      if (!target) continue;
      if (!payload.updates.length) {
        window.Plotly.purge(target);
        plotLocales.delete(target);
        continue;
      }
      if (plotLocales.has(target) && plotLocales.get(target) !== locale) {
        window.Plotly.purge(target);
      }
      plotLocales.set(target, locale);

      const y = payload[spec.key];
      const data = [{
        x: payload.updates,
        y,
        type: "scatter",
        mode: "lines",
        name: payload.labels[spec.key],
        line: { color: spec.color, width: 2.5 },
        connectgaps: false,
        hovertemplate: `${payload.labels.update}: %{x}<br>${payload.labels[spec.key]}: %{y}<extra></extra>`
      }];
      const layout = {
        title: { text: payload.labels[`${spec.key}_title`], x: 0.02, xanchor: "left" },
        autosize: true,
        margin: { l: 64, r: 24, t: 44, b: 52 },
        paper_bgcolor: "#ffffff",
        plot_bgcolor: "#ffffff",
        font: { family: "Arial, sans-serif", color: "#17212b", size: 12 },
        xaxis: {
          title: { text: payload.labels.update },
          rangemode: "tozero",
          showgrid: true,
          gridcolor: "#d9e0e6",
          zeroline: true
        },
        yaxis: {
          title: { text: payload.labels[`${spec.key}_axis`] },
          rangemode: "tozero",
          showgrid: true,
          gridcolor: "#d9e0e6",
          zeroline: true
        },
        uirevision: payload.run_id || "education-run"
      };
      const config = {
        responsive: true,
        locale,
        displaylogo: false,
        scrollZoom: true,
        doubleClick: "reset",
        toImageButtonOptions: {
          format: "png",
          filename: spec.filename,
          scale: 2
        },
        modeBarButtonsToRemove: ["sendDataToCloud"]
      };
      window.Plotly.react(target, data, layout, config);
    }
  }

  window.AvidaEducationPlots = { update };
})();
