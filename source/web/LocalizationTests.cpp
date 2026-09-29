#include "Localization.hpp"

#include <array>
#include <cassert>
#include <string_view>

using namespace avida::web::localization;

namespace {

constexpr std::array<Locale, 4> locales{
  Locale::ENGLISH,
  Locale::SPANISH,
  Locale::PSEUDO_EXPANDED,
  Locale::PSEUDO_RTL
};

constexpr std::array<NamedArgument, 5> argument_pool{{
  {"update", "42"},
  {"percent", "1.5"},
  {"minimum", "0"},
  {"maximum", "100"},
  {"reason", "worker stopped"}
}};

bool IsUnique(std::string_view id, size_t before) {
  const auto entries = Inventory();
  for (size_t index = 0; index < before; ++index) {
    if (entries[index].id == id) return false;
  }
  return true;
}

void CheckCatalog() {
  const auto entries = Inventory();
  assert(entries.size() == 29);
  for (size_t index = 0; index < entries.size(); ++index) {
    const Entry & entry = entries[index];
    assert(!entry.id.empty());
    assert(!entry.english.empty());
    assert(IsUnique(entry.id, index));
    assert(HasWellFormedPlaceholders(entry.english));

    std::array<NamedArgument, argument_pool.size()> arguments{};
    size_t argument_count = 0;
    for (const NamedArgument & candidate : argument_pool) {
      if (HasPlaceholder(entry.english, candidate.name)) {
        arguments[argument_count++] = candidate;
      }
    }
    for (const Locale locale : locales) {
      const auto result = Format(entry.id, locale,
        std::span<const NamedArgument>{arguments.data(), argument_count});
      assert(result.has_value());
      assert(!result->text.empty());
      assert(result->direction == TextDirection(locale));
      assert(result->pseudolocale == (locale == Locale::PSEUDO_EXPANDED || locale == Locale::PSEUDO_RTL));
    }
  }

  for (const Locale locale : locales) {
    const CoverageReport coverage = Coverage(locale);
    assert(coverage.Complete());
    assert(coverage.inventory == entries.size());
    assert(coverage.translated == 0);
    assert(coverage.machine_assisted == (locale == Locale::SPANISH ? entries.size() : 0));
    assert(coverage.generated == (locale == Locale::PSEUDO_EXPANDED || locale == Locale::PSEUDO_RTL ? entries.size() : 0));
    assert(coverage.source == (locale == Locale::ENGLISH ? entries.size() : 0));
  }
}

void CheckArguments() {
  const NamedArgument update{"update", "17"};
  const auto valid = Format(key::STATUS_UPDATE, Locale::ENGLISH, {&update, 1});
  assert(valid.has_value() && valid->text == "Update 17");
  const auto spanish = Format(key::STATUS_UPDATE, Locale::SPANISH, {&update, 1});
  assert(spanish && spanish->text == "Actualización 17");
  assert(!spanish->pseudolocale);
  assert(Format("shell.sidepanel.run", Locale::SPANISH)->text == "Ejecución");
  assert(TextDirection(Locale::SPANISH) == Direction::LTR);
  const NamedArgument count{"count", "24"};
  const auto offspring = Format("organism.status.offspring", Locale::SPANISH, {&count, 1});
  assert(offspring && offspring->text.find("24 instrucciones") != std::string::npos);
  assert(SpanishText("Execution map") == "Mapa de ejecución");
  assert(SpanishText("Ancestor") == "Ancestro");

  const auto missing = Format(key::STATUS_UPDATE, Locale::ENGLISH);
  assert(!missing && missing.error().code == FormatErrorCode::MISSING_ARGUMENT);
  assert(missing.error().argument == "update");

  const std::array<NamedArgument, 2> extra_arguments{{
    {"update", "17"}, {"other", "unused"}
  }};
  const auto extra = Format(key::STATUS_UPDATE, Locale::ENGLISH, extra_arguments);
  assert(!extra && extra.error().code == FormatErrorCode::UNUSED_ARGUMENT);

  const std::array<NamedArgument, 2> duplicate{{
    {"update", "1"}, {"update", "2"}
  }};
  const auto duplicated = Format(key::STATUS_UPDATE, Locale::ENGLISH, duplicate);
  assert(!duplicated && duplicated.error().code == FormatErrorCode::UNUSED_ARGUMENT);
}

void CheckPseudolocalesAndStableIds() {
  const NamedArgument update{"update", "42"};
  const auto expanded = Format(key::STATUS_UPDATE, Locale::PSEUDO_EXPANDED, {&update, 1});
  assert(expanded && expanded->text.find("{update}") == std::string::npos);
  assert(expanded->text.find("42") != std::string::npos);
  assert(expanded->text.size() > std::string_view{"Update 42"}.size());
  assert(LocaleLabel(Locale::PSEUDO_EXPANDED).find("Pseudo:") == 0);

  const auto rtl = Format(key::LESSON_TITLE, Locale::PSEUDO_RTL);
  assert(rtl && rtl->direction == Direction::RTL);
  assert(rtl->text.starts_with("\xD7\x90 "));
  assert(rtl->text.ends_with(" \xD7\x90"));
  assert(LocaleLabel(Locale::PSEUDO_RTL).find("Pseudo:") == 0);
  assert(LocaleTag(Locale::PSEUDO_EXPANDED) == "qps-ploc");
  assert(LocaleTag(Locale::PSEUDO_RTL) == "qps-plocm");

  assert(StableId(ErrorId::INVALID_MUTATION_SETTING) == "error.invalid_mutation_setting");
  assert(StableId(MetricId::DISTINCT_SEQUENCE_COUNT)
    == "metric.population.distinct_sequence_count");
  assert(!Find("error.invalid_mutation_setting"));
}

} // namespace

int main() {
  CheckCatalog();
  CheckArguments();
  CheckPseudolocalesAndStableIds();
}
