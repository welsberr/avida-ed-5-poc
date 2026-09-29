#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace avida_web::education {

using InstructionSequence = std::vector<std::uint8_t>;

struct CensusSample {
  std::uint64_t update = 0;
  std::uint64_t population_count = 0;
  std::uint64_t sequence_richness = 0;
  std::optional<double> ancestor_sequence_fraction;
};

struct CensusResult {
  CensusSample metrics;
  std::map<InstructionSequence, std::uint64_t> sequence_abundance;
};

struct EducationRun {
  std::string id;
  std::string preset_id;
  std::string treatment_id;
  std::string status;
  std::uint64_t seed = 0;
  std::uint64_t target_update = 0;
  std::uint64_t sample_interval = 0;
  std::vector<CensusSample> samples;
};

[[nodiscard]] inline CensusResult Census(
  std::uint64_t update,
  std::span<const InstructionSequence> sequences,
  const InstructionSequence & ancestor
) {
  CensusResult result;
  result.metrics.update = update;
  std::uint64_t ancestor_matches = 0;
  for (const InstructionSequence & sequence : sequences) {
    ++result.metrics.population_count;
    ++result.sequence_abundance[sequence];
    if (sequence == ancestor) ++ancestor_matches;
  }
  result.metrics.sequence_richness = result.sequence_abundance.size();
  if (result.metrics.population_count) {
    result.metrics.ancestor_sequence_fraction =
      static_cast<double>(ancestor_matches) / result.metrics.population_count;
  }
  return result;
}

enum class AppendResult { APPENDED, DUPLICATE_OR_REVERSED_UPDATE };

class RunRecorder {
  std::vector<CensusSample> samples;

public:
  [[nodiscard]] AppendResult Append(CensusSample sample) {
    if (!samples.empty() && sample.update <= samples.back().update) {
      return AppendResult::DUPLICATE_OR_REVERSED_UPDATE;
    }
    samples.push_back(std::move(sample));
    return AppendResult::APPENDED;
  }

  [[nodiscard]] std::span<const CensusSample> Samples() const noexcept {
    return samples;
  }

  [[nodiscard]] std::size_t Size() const noexcept { return samples.size(); }
  void Clear() { samples.clear(); }
};

} // namespace avida_web::education
