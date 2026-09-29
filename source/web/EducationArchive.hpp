#pragma once

#include <charconv>
#include <iomanip>
#include <sstream>

namespace {
std::string EducationBase64Encode(std::string_view input) {
  constexpr char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((input.size() + 2) / 3 * 4);
  for (size_t i = 0; i < input.size(); i += 3) {
    const auto a = static_cast<unsigned char>(input[i]);
    const auto b = i + 1 < input.size() ? static_cast<unsigned char>(input[i + 1]) : 0;
    const auto c = i + 2 < input.size() ? static_cast<unsigned char>(input[i + 2]) : 0;
    const uint32_t value = (static_cast<uint32_t>(a) << 16)
      | (static_cast<uint32_t>(b) << 8) | c;
    out.push_back(alphabet[(value >> 18) & 63]);
    out.push_back(alphabet[(value >> 12) & 63]);
    out.push_back(i + 1 < input.size() ? alphabet[(value >> 6) & 63] : '=');
    out.push_back(i + 2 < input.size() ? alphabet[value & 63] : '=');
  }
  return out;
}

std::expected<std::string, std::string> EducationBase64Decode(std::string_view input) {
  if (input.size() % 4 || input.size() > 60 * 1024 * 1024) {
    return std::unexpected("Checkpoint data has invalid base64 size.");
  }
  std::array<int, 256> values{};
  values.fill(-1);
  const std::string_view alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (size_t i = 0; i < alphabet.size(); ++i) {
    values[static_cast<unsigned char>(alphabet[i])] = static_cast<int>(i);
  }
  std::string out;
  out.reserve(input.size() / 4 * 3);
  for (size_t i = 0; i < input.size(); i += 4) {
    const bool last = i + 4 == input.size();
    const char c0 = input[i], c1 = input[i + 1], c2 = input[i + 2], c3 = input[i + 3];
    if (values[static_cast<unsigned char>(c0)] < 0
        || values[static_cast<unsigned char>(c1)] < 0
        || (c2 == '=' && (!last || c3 != '='))
        || (c3 == '=' && !last)
        || (c2 != '=' && values[static_cast<unsigned char>(c2)] < 0)
        || (c3 != '=' && values[static_cast<unsigned char>(c3)] < 0)) {
      return std::unexpected("Checkpoint data contains invalid base64.");
    }
    const uint32_t value = (static_cast<uint32_t>(values[static_cast<unsigned char>(c0)]) << 18)
      | (static_cast<uint32_t>(values[static_cast<unsigned char>(c1)]) << 12)
      | (c2 == '=' ? 0u : static_cast<uint32_t>(values[static_cast<unsigned char>(c2)]) << 6)
      | (c3 == '=' ? 0u : static_cast<uint32_t>(values[static_cast<unsigned char>(c3)]));
    out.push_back(static_cast<char>((value >> 16) & 255));
    if (c2 != '=') out.push_back(static_cast<char>((value >> 8) & 255));
    if (c3 != '=') out.push_back(static_cast<char>(value & 255));
  }
  return out;
}

std::expected<double, std::string> EducationNumber(const avida_web::education::json::Value & value) {
  if (value.type != avida_web::education::json::Value::Type::NUMBER) {
    return std::unexpected("Expected numeric census value.");
  }
  double result = 0.0;
  const auto parsed = std::from_chars(value.scalar.data(), value.scalar.data() + value.scalar.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != value.scalar.data() + value.scalar.size()
      || !std::isfinite(result)) return std::unexpected("Invalid numeric census value.");
  return result;
}

std::expected<avida_web::education::CensusSample, std::string> ParseEducationSample(
  const avida_web::education::json::Value & value
) {
  using namespace avida_web::education::json;
  using avida_web::education::CensusSample;
  auto update = UnsignedField(value, "update");
  auto population = UnsignedField(value, "population_count");
  auto richness = UnsignedField(value, "sequence_richness");
  if (!update || !population || !richness || *population > 10000 || *richness > *population) {
    return std::unexpected("Invalid census row counts.");
  }
  const Value * fraction = Field(value, "ancestor_sequence_fraction");
  if (!fraction) return std::unexpected("Missing ancestor fraction.");
  std::optional<double> fraction_value;
  if (fraction->type != Value::Type::NULL_VALUE) {
    auto parsed = EducationNumber(*fraction);
    if (!parsed || *parsed < 0.0 || *parsed > 1.0 || *population == 0) {
      return std::unexpected("Invalid ancestor sequence fraction.");
    }
    fraction_value = *parsed;
  } else if (*population != 0) {
    return std::unexpected("Non-empty population cannot have a null ancestor fraction.");
  }
  return CensusSample{*update, *population, *richness, fraction_value};
}

std::string EducationSampleJson(const avida_web::education::CensusSample & sample) {
  std::string out = "{\"update\":" + std::to_string(sample.update)
    + ",\"population_count\":" + std::to_string(sample.population_count)
    + ",\"sequence_richness\":" + std::to_string(sample.sequence_richness)
    + ",\"ancestor_sequence_fraction\":";
  out += sample.ancestor_sequence_fraction ? std::to_string(*sample.ancestor_sequence_fraction) : "null";
  return out + "}";
}
}

std::string AvidaWebApp::WriteEducationExperiment() {
  std::ostringstream engine;
  emp::SerialPod pod{engine};
  pod(Avida());
  const std::string checkpoint = BuildCheckpointFile(SnapshotConfiguration(), engine.str());
  std::string out = "{\"format\":\"avida-ed5-education-experiment\",\"schema_version\":1"
    + std::string{",\"metric_definition_version\":1,\"checkpoint_base64\":"}
    + avida_web::education::json::Escape(EducationBase64Encode(checkpoint))
    + ",\"locale\":" + avida_web::education::json::Escape(
      active_locale == avida::web::localization::Locale::ENGLISH ? "en"
        : active_locale == avida::web::localization::Locale::SPANISH ? "es"
          : active_locale == avida::web::localization::Locale::PSEUDO_EXPANDED ? "qps-ploc" : "qps-plocm")
    + ",\"prediction_note\":" + avida_web::education::json::Escape(prediction_note)
    + ",\"observation_note\":" + avida_web::education::json::Escape(observation_note)
    + ",\"ancestor_sequence\":[";
  for (size_t i = 0; i < education_ancestor_sequence.size(); ++i) {
    if (i) out += ',';
    out += std::to_string(education_ancestor_sequence[i]);
  }
  out += "],\"runs\":[";
  bool first_run = true;
  auto write_run = [&out, &first_run](const auto & run) {
    if (!first_run) out += ',';
    first_run = false;
    out += "{\"id\":" + avida_web::education::json::Escape(run.id)
      + ",\"preset_id\":" + avida_web::education::json::Escape(run.preset_id)
      + ",\"treatment_id\":" + avida_web::education::json::Escape(run.treatment_id)
      + ",\"status\":" + avida_web::education::json::Escape(run.status)
      + ",\"seed\":" + std::to_string(run.seed)
      + ",\"target_update\":" + std::to_string(run.target_update)
      + ",\"sample_interval\":" + std::to_string(run.sample_interval)
      + ",\"samples\":[";
    for (size_t i = 0; i < run.samples.size(); ++i) {
      if (i) out += ',';
      out += EducationSampleJson(run.samples[i]);
    }
    out += "]}";
  };
  for (const auto & run : completed_education_runs) write_run(run);
  if (!active_education_run_finalized && !active_education_run.id.empty()) {
    auto active = active_education_run;
    const auto samples = education_run_recorder.Samples();
    active.samples.assign(samples.begin(), samples.end());
    write_run(active);
  }
  out += "]}";
  return out;
}

void AvidaWebApp::SaveEducationExperiment() {
  if (SimulationWorkerBusy()) {
    pending_education_save = true;
    SetRunMode(RunMode::PAUSED);
    education_archive_status = "Pausing at an update boundary before saving.";
    RefreshEducationStatus();
    return;
  }
  if (!Avida().IsCheckpointSafe()) {
    education_archive_status = "Save is available at a paused update boundary.";
    RefreshEducationStatus();
    return;
  }
  const std::string contents = WriteEducationExperiment();
  DownloadBrowserFile("avida-education-experiment-v1.json", "application/json", contents.data(), contents.size());
  pending_education_save = false;
  education_archive_status = "Experiment, notes, and run history downloaded as JSON.";
  RefreshEducationStatus();
}

void AvidaWebApp::ExportEducationData() {
  std::string csv = "run_id,preset_id,treatment_id,seed,status,update,population_count,sequence_richness,ancestor_sequence_fraction\n";
  for (const auto & run : completed_education_runs) {
    for (const auto & sample : run.samples) {
      csv += run.id + "," + run.preset_id + "," + run.treatment_id + ","
        + std::to_string(run.seed) + "," + run.status + "," + std::to_string(sample.update)
        + "," + std::to_string(sample.population_count) + ","
        + std::to_string(sample.sequence_richness) + ",";
      if (sample.ancestor_sequence_fraction) csv += std::to_string(*sample.ancestor_sequence_fraction);
      csv += '\n';
    }
  }
  DownloadBrowserFile("avida-education-results.csv", "text/csv;charset=utf-8", csv.data(), csv.size());
  education_archive_status = "Completed run samples downloaded as CSV.";
  RefreshEducationStatus();
}

void AvidaWebApp::ImportEducationExperiment(const std::string & contents) {
  using namespace avida_web::education;
  using namespace avida_web::education::json;
  auto reject = [this](std::string reason) {
    education_archive_status = emp::MakeString("Experiment not loaded: ", reason).str();
    RefreshEducationStatus();
  };
  if (SimulationWorkerBusy()) {
    pending_education_import = contents;
    SetRunMode(RunMode::PAUSED);
    education_archive_status = "Pausing at an update boundary before opening the experiment.";
    RefreshEducationStatus();
    return;
  }
  auto root = Parser{contents}.Parse();
  if (!root || root->type != Value::Type::OBJECT) return reject(root ? "expected a JSON object." : root.error());
  auto format = StringField(*root, "format");
  auto version = UnsignedField(*root, "schema_version");
  auto checkpoint_encoded = StringField(*root, "checkpoint_base64");
  auto locale = StringField(*root, "locale");
  auto prediction = StringField(*root, "prediction_note");
  auto observation = StringField(*root, "observation_note");
  const Value * runs_value = Field(*root, "runs");
  const Value * ancestor_value = Field(*root, "ancestor_sequence");
  if (!format || *format != "avida-ed5-education-experiment" || !version || *version != 1
      || !checkpoint_encoded || !locale || (*locale != "en" && *locale != "es" && *locale != "qps-ploc" && *locale != "qps-plocm")
      || !prediction || !observation || prediction->size() > 100000
      || observation->size() > 100000 || !runs_value || runs_value->type != Value::Type::ARRAY
      || runs_value->array.size() > 50 || !ancestor_value || ancestor_value->type != Value::Type::ARRAY
      || ancestor_value->array.size() > 10000) return reject("unsupported schema or invalid metadata.");
  auto checkpoint = EducationBase64Decode(*checkpoint_encoded);
  if (!checkpoint) return reject(checkpoint.error());
  auto parsed_checkpoint = avida_checkpoint::Parse(*checkpoint);
  if (!parsed_checkpoint) return reject(parsed_checkpoint.error());
  if (auto compatible = avida_checkpoint::ValidateCompatibility(
        parsed_checkpoint->compatibility, CheckpointCompatibility()); !compatible) {
    return reject(compatible.error());
  }
  FrozenConfiguration candidate_configuration;
  auto candidate = BuildCheckpointCandidate(*parsed_checkpoint, candidate_configuration);
  if (!candidate) return reject(candidate.error());
  const std::uint64_t checkpoint_update = (*candidate)->GetUpdate();

  InstructionSequence ancestor;
  for (const auto & item : ancestor_value->array) {
    auto instruction = EducationNumber(item);
    if (!instruction || *instruction < 0 || *instruction > 255 || std::floor(*instruction) != *instruction) {
      return reject("ancestor sequence contains an invalid instruction.");
    }
    ancestor.push_back(static_cast<std::uint8_t>(*instruction));
  }
  std::vector<EducationRun> imported_runs;
  std::uint64_t max_run_id = 0;
  for (const auto & item : runs_value->array) {
    auto id = StringField(item, "id");
    auto preset = StringField(item, "preset_id");
    auto treatment = StringField(item, "treatment_id");
    auto status = StringField(item, "status");
    auto seed = UnsignedField(item, "seed");
    auto target = UnsignedField(item, "target_update");
    auto interval = UnsignedField(item, "sample_interval");
    const Value * samples = Field(item, "samples");
    if (!id || !preset || !treatment || !status || !seed || !target || !interval || !samples
        || samples->type != Value::Type::ARRAY || samples->array.size() > 10001
        || id->size() > 64 || (*preset != "mutation-variation" && *preset != "mutation-variation-quick")
        || (*treatment != "mutation-0" && *treatment != "mutation-1")
        || (*status != "active" && *status != "complete" && *status != "finished" && *status != "interrupted")
        || (*seed < 42 || *seed > 44) || *target == 0 || *target > 10000 || *interval == 0 || *interval > 10000) {
      return reject("run metadata failed validation.");
    }
    EducationRun run{*id, *preset, *treatment, *status, *seed, *target, *interval, {}};
    std::uint64_t last_update = 0;
    bool first = true;
    for (const auto & row : samples->array) {
      auto sample = ParseEducationSample(row);
      if (!sample || (!first && sample->update <= last_update) || sample->update > *target) {
        return reject(sample ? "run samples are out of order or past their target." : sample.error());
      }
      first = false;
      last_update = sample->update;
      run.samples.push_back(std::move(*sample));
    }
    if (!run.samples.empty() && run.samples.front().update != 0) return reject("run history must begin at update 0.");
    if (!id->starts_with("run-") || id->size() <= 4) return reject("run identifier is invalid.");
    std::uint64_t numeric_id = 0;
    const auto parsed_id = std::from_chars(id->data() + 4, id->data() + id->size(), numeric_id);
    if (parsed_id.ec != std::errc{} || parsed_id.ptr != id->data() + id->size() || numeric_id == 0) {
      return reject("run identifier is invalid.");
    }
    max_run_id = std::max(max_run_id, numeric_id);
    if (*status == "active" && *target < checkpoint_update) {
      return reject("active run target is earlier than the saved checkpoint.");
    }
    imported_runs.push_back(std::move(run));
  }

  ImportCheckpointFile(*checkpoint);
  completed_education_runs.clear();
  education_run_recorder.Clear();
  active_education_run = {};
  active_education_run_finalized = true;
  for (auto & run : imported_runs) {
    if (run.status == "active") {
      active_education_run = run;
      active_education_run_finalized = false;
      education_run_recorder.Clear();
      for (const auto & sample : run.samples) (void) education_run_recorder.Append(sample);
      education_seed = run.seed;
      education_target_update = run.target_update;
      education_sample_interval = run.sample_interval;
      education_uses_quick_preset = run.preset_id == "mutation-variation-quick";
      education_treatment_id = run.treatment_id;
    } else completed_education_runs.push_back(std::move(run));
  }
  next_education_run_id = max_run_id + 1;
  education_ancestor_sequence = std::move(ancestor);
  prediction_note = std::move(*prediction);
  observation_note = std::move(*observation);
  active_locale = *locale == "es"
    ? avida::web::localization::Locale::SPANISH
    : *locale == "qps-ploc"
    ? avida::web::localization::Locale::PSEUDO_EXPANDED
    : *locale == "qps-plocm"
      ? avida::web::localization::Locale::PSEUDO_RTL
      : avida::web::localization::Locale::ENGLISH;
  published_education_update.store(Avida().GetUpdate(), std::memory_order_release);
  pending_education_import.reset();
  education_archive_status = "Experiment and validated checkpoint opened.";
  RequestInterfaceRebuild();
}

emp::String AvidaWebApp::BuildComparisonHTML() const {
  std::vector<const avida_web::education::EducationRun *> runs;
  for (const auto & run : completed_education_runs) if (run.status == "complete") runs.push_back(&run);
  if (runs.size() < 2) return emp::MakeString("<p>", emp::MakeWebSafe(PresentationText("Complete at least two target runs before comparing.")), "</p>");
  auto chart = [this, &runs](bool richness) {
    std::string out = "<svg viewBox='0 0 320 150' role='img' aria-label='";
    out += PresentationText(richness ? "Sequence richness across completed runs" : "Population size across completed runs").str();
    out += "'><line x1='0' y1='140' x2='320' y2='140' class='chart-axis'/>";
    const double y_max = richness ? 100.0 : 100.0;
    for (const auto * run : runs) {
      std::string points;
      const double max_update = std::max<double>(1.0, run->target_update);
      for (const auto & sample : run->samples) {
        const double x = static_cast<double>(sample.update) / max_update * 320.0;
        const double value = richness ? sample.sequence_richness : sample.population_count;
        const double y = 140.0 - value / y_max * 130.0;
        if (!points.empty()) points.push_back(' ');
        points += std::to_string(x) + "," + std::to_string(y);
      }
      out += "<polyline points='" + points + "' class='";
      out += run->treatment_id == "mutation-1" ? "chart-richness" : "chart-population";
      out += "' data-run='" + run->id + "'/>";
    }
    return out + "</svg>";
  };
  std::string out = "<section class='education-results'><p>" + PresentationText("Only completed runs are overlaid; colors identify treatment. Each line is a single stochastic trajectory.").str() + "</p>";
  out += "<ul class='comparison-legend' aria-label='" + PresentationText("Run lines").str() + "'>";
  for (const auto * run : runs) {
    out += "<li class='";
    out += run->treatment_id == "mutation-1" ? "chart-richness" : "chart-population";
    out += "'><strong>" + run->id + "</strong> · ";
    out += PresentationText(run->treatment_id == "mutation-1" ? "1% mutation" : "0% mutation").str();
    out += " · seed " + std::to_string(run->seed) + "</li>";
  }
  out += "</ul>";
  out += "<figure class='result-chart'><figcaption>" + PresentationText("Population size (N)").str() + "</figcaption>" + chart(false) + "</figure>";
  out += "<figure class='result-chart'><figcaption>" + PresentationText("Sequence richness (S)").str() + "</figcaption>" + chart(true) + "</figure>";
  out += "<div class='results-table-wrap'><table class='results-table'><caption>" + PresentationText("Completed run endpoints").str() + "</caption><thead><tr><th>" + Localized("compare.table.run").str() + "</th><th>" + PresentationText("Treatment").str() + "</th><th>" + PresentationText("Seed").str() + "</th><th>" + PresentationText("Update").str() + "</th><th>N</th><th>S</th><th>" + PresentationText("Ancestor (%)").str() + "</th></tr></thead><tbody>";
  for (const auto * run : runs) {
    const auto * sample = run->samples.empty() ? nullptr : &run->samples.back();
    out += "<tr><th>" + run->id + "</th><td>" + (run->treatment_id == "mutation-1" ? "1%" : "0%")
      + "</td><td>" + std::to_string(run->seed) + "</td><td>" + (sample ? std::to_string(sample->update) : "—")
      + "</td><td>" + (sample ? std::to_string(sample->population_count) : "—")
      + "</td><td>" + (sample ? std::to_string(sample->sequence_richness) : "—") + "</td><td>";
    if (sample && sample->ancestor_sequence_fraction) out += std::to_string(*sample->ancestor_sequence_fraction * 100.0);
    out += "</td></tr>";
  }
  return emp::String{out + "</tbody></table></div></section>"};
}
