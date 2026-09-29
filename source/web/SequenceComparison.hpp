#pragma once

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace avida_web::education {

inline constexpr std::size_t MAX_DISPLAYED_SEQUENCE_POSITIONS = 128;

enum class SequencePositionChange { SAME, DIFFERENT, SELECTED_ONLY, ANCESTOR_ONLY };

struct SequenceComparisonCounts {
  std::size_t positions = 0;
  std::size_t same = 0;
  std::size_t different = 0;
  std::size_t selected_only = 0;
  std::size_t ancestor_only = 0;
};

[[nodiscard]] inline std::string EscapeSequenceComparisonHTML(std::string_view value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char character : value) {
    switch (character) {
      case '&': escaped += "&amp;"; break;
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&#39;"; break;
      default: escaped += character; break;
    }
  }
  return escaped;
}

template <typename SELECTED_T, typename ANCESTOR_T>
[[nodiscard]] SequenceComparisonCounts CountSequenceDifferences(
  const SELECTED_T & selected,
  const ANCESTOR_T & ancestor
) {
  SequenceComparisonCounts counts;
  counts.positions = std::max(selected.size(), ancestor.size());
  for (std::size_t position = 0; position < counts.positions; ++position) {
    const bool in_selected = position < selected.size();
    const bool in_ancestor = position < ancestor.size();
    if (!in_ancestor) ++counts.selected_only;
    else if (!in_selected) ++counts.ancestor_only;
    else if (selected[position] == ancestor[position]) ++counts.same;
    else ++counts.different;
  }
  return counts;
}

template <typename SELECTED_T, typename ANCESTOR_T, typename INST_SET_T, typename TEXT_FN>
[[nodiscard]] std::string BuildSequenceComparisonHTML(
  const SELECTED_T & selected,
  const ANCESTOR_T & ancestor,
  const INST_SET_T & instruction_set,
  TEXT_FN && presentation_text
) {
  const auto counts = CountSequenceDifferences(selected, ancestor);
  const auto safe = [](std::string_view value) {
    return EscapeSequenceComparisonHTML(value);
  };
  const auto text = [&presentation_text, &safe](std::string_view source) {
    const auto localized = presentation_text(source);
    return safe(std::string_view{localized.data(), localized.size()});
  };
  std::ostringstream out;

  out << "<section class='organism-visual-panel sequence-comparison-panel' "
      << "aria-labelledby='seq-compare'>"
      << "<div class='panel-heading'><div><span class='eyebrow'>"
      << text("Genome comparison") << "</span><h2 id='seq-compare'>"
      << text("Selected sequence and ancestor") << "</h2></div></div>"
      << "<p class='sequence-comparison-note'>"
      << text("Positions are compared directly; this is not sequence alignment. "
              "An insertion or deletion can shift later positions.")
      << "</p>";

  if (!ancestor.size()) {
    out << "<p class='sequence-comparison-unavailable'>"
        << text("The lesson reference ancestor is unavailable for this run.")
        << "</p></section>";
    return out.str();
  }

  out << "<dl class='sequence-comparison-counts'>"
      << "<div><dt>" << text("Same instruction") << "</dt><dd>" << counts.same
      << "</dd></div><div><dt>" << text("Different instruction") << "</dt><dd>"
      << counts.different << "</dd></div><div><dt>" << text("Only in selected")
      << "</dt><dd>" << counts.selected_only << "</dd></div><div><dt>"
      << text("Only in ancestor") << "</dt><dd>" << counts.ancestor_only
      << "</dd></div></dl>";

  const auto displayed = std::min(counts.positions, MAX_DISPLAYED_SEQUENCE_POSITIONS);
  out << "<div class='sequence-comparison-table-wrap'><table class='sequence-comparison-table'>"
      << "<thead><tr><th scope='col'>" << text("Position") << "</th><th scope='col'>"
      << text("Ancestor") << "</th><th scope='col'>" << text("Selected")
      << "</th><th scope='col'>" << text("Difference") << "</th></tr></thead><tbody>";
  for (std::size_t position = 0; position < displayed; ++position) {
    const bool in_selected = position < selected.size();
    const bool in_ancestor = position < ancestor.size();
    SequencePositionChange change = SequencePositionChange::SAME;
    if (!in_ancestor) change = SequencePositionChange::SELECTED_ONLY;
    else if (!in_selected) change = SequencePositionChange::ANCESTOR_ONLY;
    else if (selected[position] != ancestor[position]) change = SequencePositionChange::DIFFERENT;

    std::string_view change_label;
    switch (change) {
      case SequencePositionChange::SAME: change_label = "Same instruction"; break;
      case SequencePositionChange::DIFFERENT: change_label = "Different instruction"; break;
      case SequencePositionChange::SELECTED_ONLY: change_label = "Only in selected"; break;
      case SequencePositionChange::ANCESTOR_ONLY: change_label = "Only in ancestor"; break;
    }
    out << "<tr class='sequence-position-"
        << (change == SequencePositionChange::SAME ? "same" : "changed")
        << "'><th scope='row'>" << position << "</th><td>";
    if (in_ancestor) {
      const std::string name{instruction_set.GetName(ancestor[position])};
      out << "<code>" << safe(name) << "</code>";
    } else out << "<span aria-label='" << text("Not present") << "'>&mdash;</span>";
    out << "</td><td>";
    if (in_selected) {
      const std::string name{instruction_set.GetName(selected[position])};
      out << "<code>" << safe(name) << "</code>";
    } else out << "<span aria-label='" << text("Not present") << "'>&mdash;</span>";
    out << "</td><td>" << text(change_label) << "</td></tr>";
  }
  out << "</tbody></table></div>";
  if (displayed < counts.positions) {
    out << "<p class='sequence-comparison-truncated'>" << text("Showing the first") << " "
        << displayed << " " << text("of") << " " << counts.positions << " "
        << text("positions.") << " " << text("Counts above include both complete sequences.")
        << "</p>";
  }
  out << "</section>";
  return out.str();
}

} // namespace avida_web::education
