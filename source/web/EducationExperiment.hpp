#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace avida_web::education {

enum class MetricValueType { COUNT, FRACTION, REAL };
enum class MissingValueRule { ZERO_FOR_EMPTY, UNDEFINED_FOR_EMPTY, UNDEFINED_IF_INVALID };

struct MetricDefinition {
  std::string_view id;
  std::string_view definition;
  std::string_view unit;
  std::string_view label_key;
  std::string_view definition_key;
  MetricValueType value_type;
  MissingValueRule missing_value;
};

struct Treatment {
  std::string_view id;
  std::string_view label_key;
  double substitution_probability;
};

struct PinnedFile {
  std::string_view path;
  std::string_view sha256;
};

struct ExperimentPreset {
  std::string_view id;
  std::string_view title_key;
  std::string_view question_key;
  std::string_view mutation_setting_label_key;
  std::string_view mutation_semantics_key;
  std::size_t grid_width;
  std::size_t grid_height;
  std::size_t ancestor_cell;
  std::array<std::uint32_t, 3> seeds;
  std::array<Treatment, 2> treatments;
  std::uint64_t target_update;
  std::uint64_t sample_interval;
  PinnedFile environment;
  PinnedFile ancestor;
  std::string_view avida_revision;
  std::string_view population_profile;
  std::string_view cpu_profile;
  std::string_view instruction_profile;
  std::string_view module_pack_definition;
  std::array<MetricDefinition, 4> metrics;
};

// IDs, formulae, and units are locale-independent. Catalogs own all presentation text.
inline constexpr std::array<MetricDefinition, 4> METRICS{{
  {
    "population_count",
    "count(occupied_cells(snapshot))",
    "organism",
    "metrics.population_count.label",
    "metrics.population_count.definition",
    MetricValueType::COUNT,
    MissingValueRule::ZERO_FOR_EMPTY
  },
  {
    "sequence_richness",
    "count(distinct(ordered_instruction_sequence(profile)))",
    "sequence",
    "metrics.sequence_richness.label",
    "metrics.sequence_richness.definition",
    MetricValueType::COUNT,
    MissingValueRule::ZERO_FOR_EMPTY
  },
  {
    "ancestor_sequence_fraction",
    "count(sequence == ancestor_sequence) / population_count",
    "fraction",
    "metrics.ancestor_sequence_fraction.label",
    "metrics.ancestor_sequence_fraction.definition",
    MetricValueType::FRACTION,
    MissingValueRule::UNDEFINED_FOR_EMPTY
  },
  {
    "fitness",
    "scheduling_metabolic_rate / gestation_cost",
    "scheduling_rate_per_gestation_cost",
    "metrics.fitness.label",
    "metrics.fitness.definition",
    MetricValueType::REAL,
    MissingValueRule::UNDEFINED_IF_INVALID
  }
}};

inline constexpr std::array<Treatment, 2> MUTATION_TREATMENTS{{
  {"mutation-0", "treatments.mutation_zero.label", 0.0},
  {"mutation-1", "treatments.mutation_one_percent.label", 0.01}
}};

inline constexpr ExperimentPreset MUTATION_VARIATION{
  "mutation-variation",
  "experiments.mutation_variation.title",
  "experiments.mutation_variation.question",
  "experiments.mutation_variation.mutation_setting_label",
  "experiments.mutation_variation.mutation_semantics",
  10,
  10,
  55, // Center cell (5,5) in the preset's 10x10 grid; the config default is overridden.
  {42, 43, 44},
  MUTATION_TREATMENTS,
  250,
  1,
  {
    "/config/Avida-web.cfg",
    "f2fe47a8e94b455fc1d2c2b351de334b7b95d95b7d83ab1df18cde8dc22d3a06"
  },
  {
    "/config/ancestor.org",
    "cb6c5528c2592c52559b84adb3cadd26c8a765f8e170960206a0280f82ecab08"
  },
  "c3fc79383dd418fdfd3cfd6d45ecab434d9fda63",
  "PopGrid-v1",
  "AvidaVM-v1",
  "AvidaVM-standard-v2-no-xor",
  "OrgTypeAvidian+PopGrid+DriverBuffered+MutationsDivideSub+TrackGeneration+TrackGenotypes+"
  "EventManager+EnvironmentLogic+ReactionsManager+TrackMetabolism+WebInterfaceBridge",
  METRICS
};

// A bounded extension fixture proves target and sampling policy live in preset data.
inline constexpr ExperimentPreset MUTATION_VARIATION_QUICK{
  "mutation-variation-quick",
  "experiments.mutation_variation_quick.title",
  "experiments.mutation_variation.question",
  "experiments.mutation_variation.mutation_setting_label",
  "experiments.mutation_variation.mutation_semantics",
  10,
  10,
  55,
  {42, 43, 44},
  MUTATION_TREATMENTS,
  100,
  10,
  MUTATION_VARIATION.environment,
  MUTATION_VARIATION.ancestor,
  MUTATION_VARIATION.avida_revision,
  MUTATION_VARIATION.population_profile,
  MUTATION_VARIATION.cpu_profile,
  MUTATION_VARIATION.instruction_profile,
  MUTATION_VARIATION.module_pack_definition,
  METRICS
};

enum class ValidationError {
  NONE,
  MISSING_ID_OR_LOCALIZATION_KEY,
  INVALID_GRID_OR_ANCESTOR_CELL,
  INVALID_SEED,
  DUPLICATE_SEED,
  INVALID_TREATMENT,
  DUPLICATE_TREATMENT_ID,
  INVALID_TARGET_OR_SAMPLE_INTERVAL,
  MISSING_PINNED_CONTEXT,
  INVALID_SHA256,
  INVALID_METRIC,
  DUPLICATE_METRIC_ID
};

constexpr bool IsSha256(std::string_view value) {
  if (value.size() != 64) return false;
  for (char c : value) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

constexpr ValidationError Validate(const ExperimentPreset & preset) {
  if (preset.id.empty() || preset.title_key.empty() || preset.question_key.empty()
      || preset.mutation_setting_label_key.empty()) {
    return ValidationError::MISSING_ID_OR_LOCALIZATION_KEY;
  }
  if (preset.grid_width == 0 || preset.grid_height == 0
      || preset.ancestor_cell / preset.grid_width >= preset.grid_height) {
    return ValidationError::INVALID_GRID_OR_ANCESTOR_CELL;
  }
  for (std::size_t i = 0; i < preset.seeds.size(); ++i) {
    if (preset.seeds[i] == 0) return ValidationError::INVALID_SEED;
    for (std::size_t j = i + 1; j < preset.seeds.size(); ++j) {
      if (preset.seeds[i] == preset.seeds[j]) return ValidationError::DUPLICATE_SEED;
    }
  }
  for (std::size_t i = 0; i < preset.treatments.size(); ++i) {
    const auto & treatment = preset.treatments[i];
    if (treatment.id.empty() || treatment.label_key.empty()
        || !(treatment.substitution_probability >= 0.0
             && treatment.substitution_probability <= 1.0)) {
      return ValidationError::INVALID_TREATMENT;
    }
    for (std::size_t j = i + 1; j < preset.treatments.size(); ++j) {
      if (treatment.id == preset.treatments[j].id) {
        return ValidationError::DUPLICATE_TREATMENT_ID;
      }
    }
  }
  if (preset.target_update == 0 || preset.sample_interval == 0
      || preset.sample_interval > preset.target_update
      || preset.target_update % preset.sample_interval != 0) {
    return ValidationError::INVALID_TARGET_OR_SAMPLE_INTERVAL;
  }
  if (preset.environment.path.empty() || preset.ancestor.path.empty()
      || preset.avida_revision.empty() || preset.population_profile.empty()
      || preset.cpu_profile.empty() || preset.instruction_profile.empty()
      || preset.module_pack_definition.empty()) {
    return ValidationError::MISSING_PINNED_CONTEXT;
  }
  if (!IsSha256(preset.environment.sha256) || !IsSha256(preset.ancestor.sha256)) {
    return ValidationError::INVALID_SHA256;
  }
  for (std::size_t i = 0; i < preset.metrics.size(); ++i) {
    const auto & metric = preset.metrics[i];
    if (metric.id.empty() || metric.definition.empty() || metric.unit.empty()
        || metric.label_key.empty() || metric.definition_key.empty()) {
      return ValidationError::INVALID_METRIC;
    }
    for (std::size_t j = i + 1; j < preset.metrics.size(); ++j) {
      if (metric.id == preset.metrics[j].id) return ValidationError::DUPLICATE_METRIC_ID;
    }
  }
  return ValidationError::NONE;
}

} // namespace avida_web::education
