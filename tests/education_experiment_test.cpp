#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string_view>

#include "source/web/EducationExperiment.hpp"

using namespace avida_web::education;

static_assert(Validate(MUTATION_VARIATION) == ValidationError::NONE);
static_assert(Validate(MUTATION_VARIATION_QUICK) == ValidationError::NONE);
static_assert(MUTATION_VARIATION.target_update != MUTATION_VARIATION_QUICK.target_update);
static_assert(MUTATION_VARIATION.sample_interval != MUTATION_VARIATION_QUICK.sample_interval);

int main() {
  constexpr std::array<std::string_view, 4> expected_ids{
    "population_count",
    "sequence_richness",
    "ancestor_sequence_fraction",
    "fitness"
  };
  for (std::size_t i = 0; i < expected_ids.size(); ++i) {
    assert(MUTATION_VARIATION.metrics[i].id == expected_ids[i]);
    assert(MUTATION_VARIATION_QUICK.metrics[i].id == expected_ids[i]);
  }

  assert(MUTATION_VARIATION.grid_width == 10);
  assert(MUTATION_VARIATION.grid_height == 10);
  assert(MUTATION_VARIATION.ancestor_cell == 55);
  constexpr std::array<std::uint32_t, 3> expected_seeds{42, 43, 44};
  assert(MUTATION_VARIATION.seeds == expected_seeds);
  assert(MUTATION_VARIATION.treatments[0].substitution_probability == 0.0);
  assert(MUTATION_VARIATION.treatments[1].substitution_probability == 0.01);
  assert(!MUTATION_VARIATION.mutation_semantics_key.empty());
  assert(MUTATION_VARIATION.target_update == 250);
  assert(MUTATION_VARIATION.sample_interval == 1);
  assert(MUTATION_VARIATION_QUICK.target_update == 100);
  assert(MUTATION_VARIATION_QUICK.sample_interval == 10);
  assert(MUTATION_VARIATION.environment.sha256.size() == 64);
  assert(MUTATION_VARIATION.ancestor.sha256.size() == 64);
  assert(METRICS[1].missing_value == MissingValueRule::ZERO_FOR_EMPTY);
  assert(METRICS[2].missing_value == MissingValueRule::UNDEFINED_FOR_EMPTY);
  assert(METRICS[3].missing_value == MissingValueRule::UNDEFINED_IF_INVALID);

  auto invalid = MUTATION_VARIATION;
  invalid.seeds[1] = invalid.seeds[0];
  assert(Validate(invalid) == ValidationError::DUPLICATE_SEED);

  invalid = MUTATION_VARIATION;
  invalid.treatments[1].substitution_probability = std::numeric_limits<double>::quiet_NaN();
  assert(Validate(invalid) == ValidationError::INVALID_TREATMENT);

  invalid = MUTATION_VARIATION;
  invalid.sample_interval = 7;
  assert(Validate(invalid) == ValidationError::INVALID_TARGET_OR_SAMPLE_INTERVAL);

  invalid = MUTATION_VARIATION;
  invalid.environment.sha256 = "not-a-sha256";
  assert(Validate(invalid) == ValidationError::INVALID_SHA256);

  invalid = MUTATION_VARIATION;
  invalid.metrics[1].id = invalid.metrics[0].id;
  assert(Validate(invalid) == ValidationError::DUPLICATE_METRIC_ID);
}
