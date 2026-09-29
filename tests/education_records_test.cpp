#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include "../source/web/EducationRecords.hpp"

int main() {
  using namespace avida_web::education;
  const InstructionSequence ancestor{1, 2, 3};
  const std::array<InstructionSequence, 4> present{{
    ancestor, ancestor, InstructionSequence{1, 2, 4}, InstructionSequence{4, 3, 2, 1}
  }};
  const CensusResult result = Census(5, present, ancestor);
  assert(result.metrics.update == 5);
  assert(result.metrics.population_count == 4);
  assert(result.metrics.sequence_richness == 3);
  assert(result.metrics.ancestor_sequence_fraction == 0.5);
  assert(result.sequence_abundance.at(ancestor) == 2);

  const std::array<InstructionSequence, 0> empty{};
  const CensusResult empty_result = Census(0, empty, ancestor);
  assert(empty_result.metrics.population_count == 0);
  assert(empty_result.metrics.sequence_richness == 0);
  assert(!empty_result.metrics.ancestor_sequence_fraction.has_value());

  RunRecorder recorder;
  assert(recorder.Append(empty_result.metrics) == AppendResult::APPENDED);
  auto update_one = result.metrics;
  update_one.update = 1;
  assert(recorder.Append(update_one) == AppendResult::APPENDED);
  assert(recorder.Append(update_one) == AppendResult::DUPLICATE_OR_REVERSED_UPDATE);
  assert(recorder.Size() == 2);
}
