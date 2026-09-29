#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "source/web/SequenceComparison.hpp"

struct FakeInstructionSet {
  std::array<std::string_view, 4> names{
    "Nop-A",
    "<img src=x onerror=alert(1)>",
    "Nop-C",
    "Nop-D"
  };

  std::string_view GetName(std::uint8_t id) const { return names[id]; }
};

int main() {
  const std::vector<std::uint8_t> selected{0, 1, 2, 3};
  const std::vector<std::uint8_t> ancestor{0, 2, 2};
  const auto counts = avida_web::education::CountSequenceDifferences(selected, ancestor);
  assert(counts.positions == 4);
  assert(counts.same == 2);
  assert(counts.different == 1);
  assert(counts.selected_only == 1);
  assert(counts.ancestor_only == 0);

  const std::vector<std::uint8_t> shorter_selected{0};
  const std::vector<std::uint8_t> longer_ancestor{0, 2, 3};
  const auto deletion_counts =
    avida_web::education::CountSequenceDifferences(shorter_selected, longer_ancestor);
  assert(deletion_counts.same == 1);
  assert(deletion_counts.different == 0);
  assert(deletion_counts.selected_only == 0);
  assert(deletion_counts.ancestor_only == 2);

  const FakeInstructionSet instructions;
  const auto html = avida_web::education::BuildSequenceComparisonHTML(
    selected,
    ancestor,
    instructions,
    [](std::string_view source){ return std::string{source}; }
  );
  assert(html.find("&lt;img src=x onerror=alert(1)&gt;") != std::string::npos);
  assert(html.find("<img src=x onerror=alert(1)>") == std::string::npos);
  assert(html.find("Only in selected") != std::string::npos);
  assert(html.find("not sequence alignment") != std::string::npos);

  const std::vector<std::uint8_t> empty;
  const auto unavailable = avida_web::education::BuildSequenceComparisonHTML(
    selected,
    empty,
    instructions,
    [](std::string_view source){ return std::string{source}; }
  );
  assert(unavailable.find("reference ancestor is unavailable") != std::string::npos);
  assert(unavailable.find("<tbody>") == std::string::npos);

  std::vector<std::uint8_t> long_sequence(
    avida_web::education::MAX_DISPLAYED_SEQUENCE_POSITIONS + 5,
    std::uint8_t{0}
  );
  const auto bounded = avida_web::education::BuildSequenceComparisonHTML(
    long_sequence,
    long_sequence,
    instructions,
    [](std::string_view source){ return std::string{source}; }
  );
  size_t table_rows = 0;
  size_t position = 0;
  while ((position = bounded.find(
            "<tr class='sequence-position-", position)) != std::string::npos) {
    ++table_rows;
    ++position;
  }
  assert(table_rows == avida_web::education::MAX_DISPLAYED_SEQUENCE_POSITIONS);
  assert(bounded.find("Showing the first") != std::string::npos);
  assert(bounded.find("Counts above include both complete sequences.") != std::string::npos);
}
