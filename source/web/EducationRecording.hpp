#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace {
std::string EducationJSONQuote(std::string_view value) {
  std::string out{"\""};
  for (const unsigned char character : value) {
    switch (character) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (character < 0x20) {
          static constexpr char HEX[] = "0123456789abcdef";
          out += "\\u00";
          out += HEX[character >> 4];
          out += HEX[character & 0x0f];
        } else out += static_cast<char>(character);
    }
  }
  out += '"';
  return out;
}
}

// Definitions included after AvidaWebApp; the education recorder observes only
// completed update boundaries and never uses the engine's genotype tracker IDs.

void AvidaWebApp::InitializeEducationRunRecord() {
  active_education_run = {};
  active_education_run.id = std::string{"run-"} + std::to_string(next_education_run_id++);
  active_education_run.preset_id = education_uses_quick_preset
    ? "mutation-variation-quick" : "mutation-variation";
  active_education_run.treatment_id = education_treatment_id;
  active_education_run.status = "active";
  active_education_run.seed = education_seed;
  active_education_run.target_update = education_target_update;
  active_education_run.sample_interval = education_sample_interval;
  active_education_run_finalized = false;
}

bool AvidaWebApp::HasCompletedEducationCombination() const {
  const std::string_view preset_id = education_uses_quick_preset
    ? "mutation-variation-quick" : "mutation-variation";
  return std::any_of(
    completed_education_runs.begin(), completed_education_runs.end(),
    [this, preset_id](const auto & run) {
      return run.status == "complete" && run.preset_id == preset_id
        && run.treatment_id == education_treatment_id && run.seed == education_seed;
    }
  );
}

void AvidaWebApp::FinalizeEducationRun(const std::string & status) {
  if (active_education_run_finalized || active_education_run.id.empty()) return;
  const auto samples = education_run_recorder.Samples();
  active_education_run.samples.assign(samples.begin(), samples.end());
  active_education_run.status = status;
  active_education_run_finalized = true;
  completed_education_runs.push_back(active_education_run);
  education_archive_status = status == "complete"
    ? "Run saved on the comparison shelf."
    : emp::MakeString("Run retained with status: ", status, ".").str();
  RequestInterfaceRebuild();
}

avida_web::education::CensusSample AvidaWebApp::CaptureEducationSample() {
  using namespace avida_web::education;
  std::vector<InstructionSequence> sequences;
  const auto cells = Population().GetCells();
  sequences.reserve(cells.size());
  for (const size_t organism_id : cells) {
    if (organism_id == avida_web::EMPTY_CELL || !Avida().IsOccupied(organism_id)) continue;
    const auto & genome = Avida().GetOrg(organism_id).GetGenome();
    InstructionSequence sequence;
    sequence.reserve(genome.size());
    for (const auto instruction : genome) {
      sequence.push_back(static_cast<std::uint8_t>(instruction));
    }
    sequences.push_back(std::move(sequence));
  }
  return Census(Avida().GetUpdate(), sequences, education_ancestor_sequence).metrics;
}

void AvidaWebApp::CommitEducationSamples() {
  for (const auto & sample : pending_education_samples) {
    (void) education_run_recorder.Append(sample);
    published_education_update.store(sample.update, std::memory_order_release);
  }
  pending_education_samples.clear();
  education_results_text.Redraw();
  RefreshEducationStatus();
  UpdateEducationPlots();
}

std::string AvidaWebApp::BuildEducationPlotPayload() const {
  const auto samples = education_run_recorder.Samples();
  std::string out = "{\"run_id\":" + EducationJSONQuote(active_education_run.id)
    + ",\"updates\":[";
  for (size_t index = 0; index < samples.size(); ++index) {
    if (index) out += ',';
    out += std::to_string(samples[index].update);
  }
  out += "],\"population\":[";
  for (size_t index = 0; index < samples.size(); ++index) {
    if (index) out += ',';
    out += std::to_string(samples[index].population_count);
  }
  out += "],\"richness\":[";
  for (size_t index = 0; index < samples.size(); ++index) {
    if (index) out += ',';
    out += std::to_string(samples[index].sequence_richness);
  }
  out += "],\"ancestor\":[";
  for (size_t index = 0; index < samples.size(); ++index) {
    if (index) out += ',';
    if (samples[index].ancestor_sequence_fraction) {
      out += std::to_string(*samples[index].ancestor_sequence_fraction * 100.0);
    } else {
      out += "null";
    }
  }
  out += "],\"labels\":{\"update\":";
  out += EducationJSONQuote(PresentationText("Update").str());
  out += ",\"population\":";
  out += EducationJSONQuote(PresentationText("Population").str());
  out += ",\"population_title\":";
  out += EducationJSONQuote(PresentationText("Population size over time").str());
  out += ",\"population_axis\":";
  out += EducationJSONQuote(PresentationText("Organisms").str());
  out += ",\"richness\":";
  out += EducationJSONQuote(PresentationText("Sequence richness").str());
  out += ",\"richness_title\":";
  out += EducationJSONQuote(PresentationText("Sequence richness over time").str());
  out += ",\"richness_axis\":";
  out += EducationJSONQuote(PresentationText("Distinct sequences").str());
  out += ",\"ancestor\":";
  out += EducationJSONQuote(PresentationText("Ancestor sequence").str());
  out += ",\"ancestor_title\":";
  out += EducationJSONQuote(PresentationText("Ancestor sequence over time").str());
  out += ",\"ancestor_axis\":";
  out += EducationJSONQuote(PresentationText("Percent of population").str());
  out += "}}";
  return out;
}

void AvidaWebApp::UpdateEducationPlots() const {
  const std::string payload = BuildEducationPlotPayload();
  EM_ASM({
    const serialized = UTF8ToString($0);
    window.requestAnimationFrame(() => {
      if (window.AvidaEducationPlots) window.AvidaEducationPlots.update(serialized);
    });
  }, payload.c_str());
}

emp::String AvidaWebApp::BuildEducationResultsHTML() const {
  const auto text = [this](std::string_view source) {
    return PresentationText(source).str();
  };
  const auto samples = education_run_recorder.Samples();
  std::string out;
  out += "<section class='education-results' aria-labelledby='results-heading'>";
  out += "<h2 id='results-heading'>" + text("Recorded observations") + "</h2>";
  if (samples.empty()) {
    out += "<p>" + text("Start the population to record update 0 and each completed update.") + "</p></section>";
    return out;
  }
  const std::string treatment = run_preset_selector.GetSelectID() == 0 ? "0%" : "1%";
  const std::string seed = std::to_string(education_seed);
  const std::string interval = std::to_string(education_sample_interval);
  const std::array<avida::web::localization::NamedArgument, 3> context_args{{
    {"treatment", treatment}, {"seed", seed}, {"interval", interval}
  }};
  const auto context = avida::web::localization::Format(
    "results.sample_context", active_locale, context_args
  );
  out += "<p class='results-context'>" + (context ? context->text : text("Run")) + "</p>";
  out += "<figure class='result-chart'><figcaption>" + text("Population size (organisms)") + "</figcaption>";
  out += "<div id='population-history-plot' class='result-chart-plot' role='img' aria-describedby='results-table' aria-label='" + text("Population size by update") + "'></div></figure>";
  out += "<figure class='result-chart'><figcaption>" + text("Sequence richness (distinct sequences)") + "</figcaption>";
  out += "<div id='richness-history-plot' class='result-chart-plot' role='img' aria-describedby='results-table' aria-label='" + text("Sequence richness by update") + "'></div></figure>";
  out += "<figure class='result-chart'><figcaption>" + text("Ancestor sequence (% of population)") + "</figcaption>";
  out += "<div id='ancestor-history-plot' class='result-chart-plot' role='img' aria-describedby='results-table' aria-label='" + text("Ancestor sequence fraction by update") + "'></div></figure>";
  out += "<div class='results-table-wrap'><table id='results-table' class='results-table'><caption>";
  const std::string count = std::to_string(samples.size());
  const avida::web::localization::NamedArgument row_count{"count", count};
  const auto rows_message = avida::web::localization::Format(
    samples.size() == 1 ? "results.rows.one" : "results.rows.other",
    active_locale, {&row_count, 1}
  );
  out += rows_message ? rows_message->text : count;
  out += "</caption><thead><tr><th scope='col'>" + text("Update") + "</th>";
  out += "<th scope='col'>" + text("Population (N)") + "</th><th scope='col'>" + text("Sequence richness (S)") + "</th>";
  out += "<th scope='col'>" + text("Ancestor sequence (%)") + "</th></tr></thead><tbody>";
  for (const auto & sample : samples) {
    out += "<tr><th scope='row' data-metric='update'>" + std::to_string(sample.update) + "</th><td data-metric='population_count'>";
    out += std::to_string(sample.population_count) + "</td><td data-metric='sequence_richness'>";
    out += std::to_string(sample.sequence_richness) + "</td><td data-metric='ancestor_sequence_fraction'>";
    if (sample.ancestor_sequence_fraction) {
      out += std::to_string(*sample.ancestor_sequence_fraction * 100.0);
    }
    out += "</td></tr>";
  }
  out += "</tbody></table></div></section>";
  return emp::String{out};
}

emp::String AvidaWebApp::BuildRunHistoryHTML() const {
  const auto text = [this](std::string_view source) {
    return PresentationText(source).str();
  };
  std::string out{"<section class='run-history' id='saved-runs'><h2>"};
  out += text("Run shelf") + "</h2><p>";
  out += text("Completed observations remain here when you start another run. A completed run is result-only.") + "</p>";
  if (completed_education_runs.empty()) {
    out += "<p>" + text("No completed runs yet. Use the three listed seeds in both mutation treatments.") + "</p>";
  } else {
    out += "<ol class='run-history-list'>";
    for (const auto & run : completed_education_runs) {
      out += "<li data-run-id='" + run.id + "' data-status='" + run.status + "'><strong>" + run.id + "</strong> · ";
      out += text(run.treatment_id == "mutation-1" ? "1% mutation" : "0% mutation");
      out += " · seed " + std::to_string(run.seed) + " · ";
      out += run.status + " · " + text("Update") + " ";
      out += run.samples.empty() ? "0" : std::to_string(run.samples.back().update);
      out += " · " + std::to_string(run.samples.size()) + " samples</li>";
    }
    out += "</ol>";
  }
  out += "<p class='run-shelf-guidance'>" + text("Complete seeds 42, 43, and 44 at both 0% and 1%. The same seed does not pair later random trajectories.") + "</p></section>";
  return emp::String{out};
}
