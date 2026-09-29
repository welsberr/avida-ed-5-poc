#pragma once

/*
 *  This file is part of the Avida Digital Evolution Research Platform, v5.0
 *  Copyright (C) 2026 Michigan State University & Dr. Charles Ofria
 *  Released under the MIT Public Licence.  See LICENSE.md for details.
 */

#include <array>
#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "SpanishCatalog.hpp"

// Localization data is plain text. Callers must insert formatted messages as text, never HTML.
// Numeric/date arguments are formatted by the browser Intl bridge before being passed here.
namespace avida::web::localization {

enum class Locale { ENGLISH, SPANISH, PSEUDO_EXPANDED, PSEUDO_RTL };
enum class Direction { LTR, RTL };
enum class Domain { SHELL, POPULATION, EXPERIMENT, LESSON, ERROR, METRIC };

namespace key {
inline constexpr std::string_view APP_TITLE = "shell.app_title";
inline constexpr std::string_view MODE_POPULATION = "shell.mode.population";
inline constexpr std::string_view MODE_SEQUENCES = "shell.mode.sequences";
inline constexpr std::string_view MODE_COMPARE = "shell.mode.compare";
inline constexpr std::string_view LANGUAGE_LABEL = "shell.language.label";
inline constexpr std::string_view RUN = "shell.run";
inline constexpr std::string_view PAUSE = "shell.pause";
inline constexpr std::string_view STEP = "shell.step";
inline constexpr std::string_view RESET = "shell.reset";
inline constexpr std::string_view SAVE = "shell.save";
inline constexpr std::string_view OPEN = "shell.open";
inline constexpr std::string_view EXPORT = "shell.export";
inline constexpr std::string_view STATUS_UPDATE = "shell.status.update";
inline constexpr std::string_view STATUS_PAUSED = "shell.status.paused";
inline constexpr std::string_view POPULATION_SIZE = "population.size";
inline constexpr std::string_view MUTATION_SETTING = "experiment.mutation.setting";
inline constexpr std::string_view MUTATION_HELP = "experiment.mutation.help";
inline constexpr std::string_view PRESET_ZERO_MUTATION = "experiment.preset.zero_mutation";
inline constexpr std::string_view PRESET_ONE_PERCENT = "experiment.preset.one_percent";
inline constexpr std::string_view LESSON_TITLE = "lessons.mutation_variation.title";
inline constexpr std::string_view LESSON_QUESTION = "lessons.mutation_variation.question";
inline constexpr std::string_view LESSON_PREDICTION = "lessons.mutation_variation.prediction";
inline constexpr std::string_view LESSON_RICHNESS = "lessons.mutation_variation.richness";
inline constexpr std::string_view LESSON_INTERPRETATION =
  "lessons.mutation_variation.interpretation";
inline constexpr std::string_view ERROR_MUTATION_RANGE = "errors.mutation.range";
inline constexpr std::string_view ERROR_RUN_FAILED = "errors.run.failed";
inline constexpr std::string_view METRIC_ORGANISMS = "metrics.population.organisms";
inline constexpr std::string_view METRIC_SEQUENCE_RICHNESS = "metrics.sequence.richness";
inline constexpr std::string_view METRIC_MUTATION_RATE = "metrics.mutation.per_site_rate";
} // namespace key

// These IDs are persisted/exported and must not be translated or reused for another meaning.
enum class ErrorId { INVALID_MUTATION_SETTING, RUN_FAILED };
enum class MetricId { ORGANISM_COUNT, DISTINCT_SEQUENCE_COUNT, PER_SITE_MUTATION_RATE };

[[nodiscard]] constexpr std::string_view StableId(ErrorId id) noexcept {
  switch (id) {
    case ErrorId::INVALID_MUTATION_SETTING: return "error.invalid_mutation_setting";
    case ErrorId::RUN_FAILED: return "error.run_failed";
  }
  return "error.unknown";
}

[[nodiscard]] constexpr std::string_view StableId(MetricId id) noexcept {
  switch (id) {
    case MetricId::ORGANISM_COUNT: return "metric.population.organism_count";
    case MetricId::DISTINCT_SEQUENCE_COUNT: return "metric.population.distinct_sequence_count";
    case MetricId::PER_SITE_MUTATION_RATE: return "metric.experiment.per_site_mutation_rate";
  }
  return "metric.unknown";
}

struct Entry {
  std::string_view id;
  Domain domain;
  std::string_view english;
};

inline constexpr std::array<Entry, 29> ENGLISH_CATALOG{{
  {key::APP_TITLE, Domain::SHELL, "Avida-ED"},
  {key::MODE_POPULATION, Domain::SHELL, "Population"},
  {key::MODE_SEQUENCES, Domain::SHELL, "Sequences"},
  {key::MODE_COMPARE, Domain::SHELL, "Compare runs"},
  {key::LANGUAGE_LABEL, Domain::SHELL, "Language"},
  {key::RUN, Domain::SHELL, "Run"},
  {key::PAUSE, Domain::SHELL, "Pause"},
  {key::STEP, Domain::SHELL, "Step"},
  {key::RESET, Domain::SHELL, "Reset"},
  {key::SAVE, Domain::SHELL, "Save"},
  {key::OPEN, Domain::SHELL, "Open"},
  {key::EXPORT, Domain::SHELL, "Export"},
  {key::STATUS_UPDATE, Domain::SHELL, "Update {update}"},
  {key::STATUS_PAUSED, Domain::SHELL, "Paused at update {update}"},
  {key::POPULATION_SIZE, Domain::POPULATION, "Population size"},
  {key::MUTATION_SETTING, Domain::EXPERIMENT, "Per-site mutation setting ({percent}%)"},
  {key::MUTATION_HELP, Domain::EXPERIMENT,
    "The setting is the chance of mutation at each copied genome site."},
  {key::PRESET_ZERO_MUTATION, Domain::EXPERIMENT, "0% mutation"},
  {key::PRESET_ONE_PERCENT, Domain::EXPERIMENT, "1% mutation"},
  {key::LESSON_TITLE, Domain::LESSON, "Mutation and genetic variation"},
  {key::LESSON_QUESTION, Domain::LESSON,
    "How does the mutation setting affect genetic variation over time?"},
  {key::LESSON_PREDICTION, Domain::LESSON, "Record your prediction"},
  {key::LESSON_RICHNESS, Domain::LESSON,
    "Sequence richness is the number of distinct genome sequences present."},
  {key::LESSON_INTERPRETATION, Domain::LESSON,
    "Compare runs as observations; one run does not establish a general outcome."},
  {key::ERROR_MUTATION_RANGE, Domain::ERROR,
    "Enter a per-site mutation setting from {minimum}% to {maximum}%."},
  {key::ERROR_RUN_FAILED, Domain::ERROR, "The run could not continue: {reason}"},
  {key::METRIC_ORGANISMS, Domain::METRIC, "Organisms"},
  {key::METRIC_SEQUENCE_RICHNESS, Domain::METRIC, "Distinct sequences"},
  {key::METRIC_MUTATION_RATE, Domain::METRIC, "Per-site mutation setting"}
}};

enum class FormatErrorCode {
  UNKNOWN_MESSAGE,
  MALFORMED_TEMPLATE,
  MISSING_ARGUMENT,
  UNUSED_ARGUMENT
};

struct NamedArgument {
  std::string_view name;
  std::string_view value;
};

struct FormatError {
  FormatErrorCode code;
  std::string message_id;
  std::string argument;
};

struct FormattedMessage {
  std::string text; // Plain text; insert via a text API such as textContent, never innerHTML.
  Direction direction = Direction::LTR;
  bool pseudolocale = false;
};

struct CoverageReport {
  size_t inventory = 0;
  size_t source = 0;
  size_t generated = 0;
  size_t machine_assisted = 0;
  size_t translated = 0;
  size_t fallback = 0;
  size_t missing = 0;
  size_t malformed = 0;
  [[nodiscard]] constexpr bool Complete() const noexcept {
    return inventory == source + generated + machine_assisted + translated + fallback
      && missing == 0 && malformed == 0;
  }
};

[[nodiscard]] constexpr std::span<const Entry> Inventory() noexcept {
  return ENGLISH_CATALOG;
}

[[nodiscard]] constexpr Direction TextDirection(Locale locale) noexcept {
  return locale == Locale::PSEUDO_RTL ? Direction::RTL : Direction::LTR;
}

[[nodiscard]] constexpr std::string_view LocaleLabel(Locale locale) noexcept {
  switch (locale) {
    case Locale::ENGLISH: return "English";
    case Locale::SPANISH: return "Español";
    case Locale::PSEUDO_EXPANDED: return "Pseudo: expanded strings";
    case Locale::PSEUDO_RTL: return "Pseudo: right-to-left layout";
  }
  return "Unknown locale";
}

[[nodiscard]] constexpr std::string_view LocaleTag(Locale locale) noexcept {
  switch (locale) {
    case Locale::ENGLISH: return "en";
    case Locale::SPANISH: return "es";
    case Locale::PSEUDO_EXPANDED: return "qps-ploc";
    case Locale::PSEUDO_RTL: return "qps-plocm";
  }
  return "und";
}

[[nodiscard]] constexpr const Entry * Find(std::string_view id) noexcept {
  for (const Entry & entry : ENGLISH_CATALOG) {
    if (entry.id == id) return &entry;
  }
  return nullptr;
}

[[nodiscard]] constexpr bool HasPlaceholder(std::string_view text, std::string_view name) noexcept {
  if (name.empty()) return false;
  for (size_t start = 0; start < text.size(); ++start) {
    if (text[start] != '{' || text.size() - start < name.size() + 2) continue;
    if (text.substr(start + 1, name.size()) == name && text[start + name.size() + 1] == '}') {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline bool HasWellFormedPlaceholders(std::string_view text) noexcept {
  bool inside = false;
  bool has_name = false;
  for (const char character : text) {
    if (character == '{') {
      if (inside) return false;
      inside = true;
      has_name = false;
    } else if (character == '}') {
      if (!inside || !has_name) return false;
      inside = false;
    } else if (inside) {
      const bool letter = (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z');
      const bool digit = character >= '0' && character <= '9';
      if (!(character == '_' || letter || (has_name && digit))) return false;
      has_name = true;
    }
  }
  return !inside;
}

[[nodiscard]] inline CoverageReport Coverage(Locale locale) noexcept {
  CoverageReport report;
  for (const Entry & entry : ENGLISH_CATALOG) {
    ++report.inventory;
    if (entry.id.empty() || entry.english.empty()) {
      ++report.missing;
    } else {
      if (locale == Locale::ENGLISH) ++report.source;
      else if (locale == Locale::SPANISH) {
        if (FindSpanish(entry.id)) ++report.machine_assisted;
        else ++report.fallback;
      } else ++report.generated;
    }
    if (!HasWellFormedPlaceholders(entry.english)) ++report.malformed;
  }
  // Generated pseudolocales are layout checks, not reviewed human translations.
  return report;
}

[[nodiscard]] inline std::string PseudoExpand(std::string_view source) {
  std::string result{"⟦"};
  for (size_t index = 0; index < source.size();) {
    if (source[index] == '{') {
      const size_t end = source.find('}', index + 1);
      if (end != std::string_view::npos) {
        result.append(source.substr(index, end - index + 1));
        index = end + 1;
        continue;
      }
    }
    const char character = source[index++];
    result.push_back(character);
    if ((character >= 'a' && character <= 'z')
      || (character >= 'A' && character <= 'Z')) {
      result.push_back('~');
      result.push_back(character);
    }
  }
  result.append("⟧");
  return result;
}

[[nodiscard]] inline std::string PseudoRtl(std::string_view source) {
  // Hebrew markers exercise RTL context/layout. The English source remains untranslated.
  std::string result{"\xD7\x90 "}; // U+05D0 ALEF
  result.append(source);
  result.append(" \xD7\x90");
  return result;
}

[[nodiscard]] inline std::expected<FormattedMessage, FormatError> Format(
  std::string_view id,
  Locale locale,
  std::span<const NamedArgument> arguments = {}
) {
  const Entry * entry = Find(id);
  const SpanishEntry * candidate = FindSpanish(id);
  if (!entry && !candidate) return std::unexpected(FormatError{
    FormatErrorCode::UNKNOWN_MESSAGE, std::string{id}, {}});

  std::string source{entry ? entry->english : candidate->source};
  if (locale == Locale::SPANISH) {
    if (candidate) source = candidate->value;
  }
  if (locale == Locale::PSEUDO_EXPANDED) source = PseudoExpand(source);
  if (locale == Locale::PSEUDO_RTL) source = PseudoRtl(source);

  std::string output;
  size_t cursor = 0;
  while (cursor < source.size()) {
    const size_t open = source.find('{', cursor);
    const size_t stray_close = source.find('}', cursor);
    if (stray_close != std::string::npos && (open == std::string::npos || stray_close < open)) {
      return std::unexpected(FormatError{
        FormatErrorCode::MALFORMED_TEMPLATE, std::string{id}, {}});
    }
    if (open == std::string::npos) {
      output.append(source.substr(cursor));
      cursor = source.size();
      break;
    }
    output.append(source.substr(cursor, open - cursor));
    const size_t close = source.find('}', open + 1);
    if (close == std::string::npos || source.find('{', open + 1) < close) {
      return std::unexpected(FormatError{
        FormatErrorCode::MALFORMED_TEMPLATE, std::string{id}, {}});
    }
    const std::string_view name{source.data() + open + 1, close - open - 1};
    if (name.empty()) {
      return std::unexpected(FormatError{
        FormatErrorCode::MALFORMED_TEMPLATE, std::string{id}, {}});
    }
    const NamedArgument * matched = nullptr;
    for (const NamedArgument & argument : arguments) {
      if (argument.name == name) {
        if (matched) {
          return std::unexpected(FormatError{FormatErrorCode::UNUSED_ARGUMENT, std::string{id},
            std::string{name}});
        }
        matched = &argument;
      }
    }
    if (!matched) {
      return std::unexpected(FormatError{FormatErrorCode::MISSING_ARGUMENT, std::string{id},
        std::string{name}});
    }
    output.append(matched->value);
    cursor = close + 1;
  }

  for (const NamedArgument & argument : arguments) {
    if (!HasPlaceholder(source, argument.name)) {
      return std::unexpected(FormatError{FormatErrorCode::UNUSED_ARGUMENT, std::string{id},
        std::string{argument.name}});
    }
  }
  return FormattedMessage{
    std::move(output), TextDirection(locale),
    locale == Locale::PSEUDO_EXPANDED || locale == Locale::PSEUDO_RTL
  };
}

} // namespace avida::web::localization
