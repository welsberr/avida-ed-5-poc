/* Plotly adapter for recorded education metrics; append live samples to mounted traces. */
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
  const plotStates = new WeakMap();

  function update(payloadText) {
    if (!window.Plotly) return Promise.resolve();
    const payload = JSON.parse(payloadText);
    const plotContainer = document.getElementById("education-plots");
    if (plotContainer) plotContainer.hidden = payload.updates.length === 0;
    const tasks = [];
    for (const spec of plotSpecs) {
      const target = document.getElementById(spec.id);
      if (!target) continue;
      let state = plotStates.get(target);
      if (!state) {
        state = { queue: Promise.resolve(), initialized: false, runId: null, locale: null, length: 0 };
        plotStates.set(target, state);
      }
      const task = state.queue.catch(() => {}).then(() => updatePlot(target, spec, payload, state));
      state.queue = task;
      tasks.push(task);
    }
    return Promise.all(tasks);
  }

  async function updatePlot(target, spec, payload, state) {
    if (!payload.updates.length) {
      if (state.initialized) window.Plotly.purge(target);
      state.initialized = false;
      state.runId = null;
      state.locale = null;
      state.length = 0;
      return;
    }

    const locale = payload.locale === "es" ? "es" : "en";
    const runId = payload.run_id || "education-run";
    const data = [{
      x: payload.updates,
      y: payload[spec.key],
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
      uirevision: runId
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

    if (!state.initialized || state.runId !== runId || state.locale !== locale) {
      if (state.initialized) window.Plotly.purge(target);
      await window.Plotly.newPlot(target, data, layout, config);
      state.initialized = true;
      state.runId = runId;
      state.locale = locale;
      state.length = payload.updates.length;
      return;
    }

    if (payload.updates.length < state.length) {
      window.Plotly.purge(target);
      await window.Plotly.newPlot(target, data, layout, config);
      state.length = payload.updates.length;
      return;
    }

    if (payload.updates.length === state.length) return;

    const start = state.length;
    await window.Plotly.extendTraces(target, {
      x: [payload.updates.slice(start)],
      y: [payload[spec.key].slice(start)]
    }, [0]);
    state.length = payload.updates.length;
  }

  window.AvidaEducationPlots = { update };
})();
