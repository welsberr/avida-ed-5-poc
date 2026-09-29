#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace avida_web::education::json {

struct Value {
  enum class Type { OBJECT, ARRAY, STRING, NUMBER, BOOLEAN, NULL_VALUE } type = Type::NULL_VALUE;
  std::map<std::string, Value> object;
  std::vector<Value> array;
  std::string scalar;
  bool boolean = false;
};

class Parser {
  std::string_view source;
  std::size_t cursor = 0;
  std::size_t node_count = 0;

  void Whitespace() {
    while (cursor < source.size()
      && (source[cursor] == ' ' || source[cursor] == '\n'
        || source[cursor] == '\r' || source[cursor] == '\t')) ++cursor;
  }

  [[nodiscard]] std::expected<std::uint16_t, std::string> Hex4() {
    if (source.size() - cursor < 4) return std::unexpected("Truncated Unicode escape.");
    std::uint16_t code = 0;
    for (int i = 0; i < 4; ++i) {
      const char c = source[cursor++];
      const int digit = c >= '0' && c <= '9' ? c - '0'
        : c >= 'a' && c <= 'f' ? c - 'a' + 10
        : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
      if (digit < 0) return std::unexpected("Invalid Unicode escape.");
      code = static_cast<std::uint16_t>((code << 4) | digit);
    }
    return code;
  }

  static void AppendUtf8(std::string & out, std::uint32_t code) {
    if (code <= 0x7f) out.push_back(static_cast<char>(code));
    else if (code <= 0x7ff) {
      out.push_back(static_cast<char>(0xc0 | (code >> 6)));
      out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
    } else if (code <= 0xffff) {
      out.push_back(static_cast<char>(0xe0 | (code >> 12)));
      out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
    } else {
      out.push_back(static_cast<char>(0xf0 | (code >> 18)));
      out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
    }
  }

  [[nodiscard]] std::expected<std::string, std::string> String() {
    if (cursor >= source.size() || source[cursor++] != '"') {
      return std::unexpected("Expected a JSON string.");
    }
    std::string out;
    while (cursor < source.size()) {
      const unsigned char c = static_cast<unsigned char>(source[cursor++]);
      if (c == '"') return out;
      if (c < 0x20) return std::unexpected("Unescaped control byte in JSON string.");
      if (c != '\\') {
        out.push_back(static_cast<char>(c));
      } else {
        if (cursor >= source.size()) return std::unexpected("Truncated JSON escape.");
        switch (source[cursor++]) {
          case '"': out.push_back('"'); break;
          case '\\': out.push_back('\\'); break;
          case '/': out.push_back('/'); break;
          case 'b': out.push_back('\b'); break;
          case 'f': out.push_back('\f'); break;
          case 'n': out.push_back('\n'); break;
          case 'r': out.push_back('\r'); break;
          case 't': out.push_back('\t'); break;
          case 'u': {
            auto first = Hex4();
            if (!first) return std::unexpected(first.error());
            std::uint32_t code = *first;
            if (code >= 0xd800 && code <= 0xdbff) {
              if (source.size() - cursor < 6 || source[cursor] != '\\'
                  || source[cursor + 1] != 'u') {
                return std::unexpected("Missing low surrogate in Unicode escape.");
              }
              cursor += 2;
              auto second = Hex4();
              if (!second || *second < 0xdc00 || *second > 0xdfff) {
                return std::unexpected("Invalid low surrogate in Unicode escape.");
              }
              code = 0x10000 + ((code - 0xd800) << 10) + (*second - 0xdc00);
            } else if (code >= 0xdc00 && code <= 0xdfff) {
              return std::unexpected("Unexpected low surrogate in Unicode escape.");
            }
            AppendUtf8(out, code);
            break;
          }
          default: return std::unexpected("Invalid JSON escape.");
        }
      }
      if (out.size() > 16 * 1024 * 1024) return std::unexpected("JSON string exceeds size limit.");
    }
    return std::unexpected("Unterminated JSON string.");
  }

  [[nodiscard]] std::expected<Value, std::string> ParseValue(std::size_t depth) {
    Whitespace();
    if (depth > 16) return std::unexpected("JSON nesting exceeds the supported depth.");
    if (++node_count > 100000) return std::unexpected("JSON contains too many values.");
    if (cursor >= source.size()) return std::unexpected("Unexpected end of JSON input.");
    const char c = source[cursor];
    if (c == '"') {
      auto value = String();
      if (!value) return std::unexpected(value.error());
      Value parsed;
      parsed.type = Value::Type::STRING;
      parsed.scalar = std::move(*value);
      return parsed;
    }
    if (c == '{') {
      ++cursor;
      Value value;
      value.type = Value::Type::OBJECT;
      Whitespace();
      if (cursor < source.size() && source[cursor] == '}') { ++cursor; return value; }
      while (cursor < source.size()) {
        Whitespace();
        auto name = String();
        if (!name) return std::unexpected(name.error());
        Whitespace();
        if (cursor >= source.size() || source[cursor++] != ':') {
          return std::unexpected("Expected ':' after JSON object key.");
        }
        auto child = ParseValue(depth + 1);
        if (!child) return std::unexpected(child.error());
        if (!value.object.emplace(std::move(*name), std::move(*child)).second) {
          return std::unexpected("Duplicate JSON object key.");
        }
        Whitespace();
        if (cursor >= source.size()) break;
        const char separator = source[cursor++];
        if (separator == '}') return value;
        if (separator != ',') return std::unexpected("Expected ',' or '}' in JSON object.");
      }
      return std::unexpected("Unterminated JSON object.");
    }
    if (c == '[') {
      ++cursor;
      Value value;
      value.type = Value::Type::ARRAY;
      Whitespace();
      if (cursor < source.size() && source[cursor] == ']') { ++cursor; return value; }
      while (cursor < source.size()) {
        if (value.array.size() >= 20000) return std::unexpected("JSON array exceeds size limit.");
        auto child = ParseValue(depth + 1);
        if (!child) return std::unexpected(child.error());
        value.array.push_back(std::move(*child));
        Whitespace();
        if (cursor >= source.size()) break;
        const char separator = source[cursor++];
        if (separator == ']') return value;
        if (separator != ',') return std::unexpected("Expected ',' or ']' in JSON array.");
      }
      return std::unexpected("Unterminated JSON array.");
    }
    if (source.substr(cursor, 4) == "true" || source.substr(cursor, 5) == "false") {
      const bool truth = source[cursor] == 't';
      cursor += truth ? 4 : 5;
      Value value;
      value.type = Value::Type::BOOLEAN;
      value.boolean = truth;
      return value;
    }
    if (source.substr(cursor, 4) == "null") {
      cursor += 4;
      return Value{};
    }
    const std::size_t start = cursor;
    if (source[cursor] == '-') ++cursor;
    if (cursor >= source.size()) return std::unexpected("Invalid JSON number.");
    if (source[cursor] == '0') ++cursor;
    else {
      if (source[cursor] < '1' || source[cursor] > '9') return std::unexpected("Invalid JSON value.");
      while (cursor < source.size() && source[cursor] >= '0' && source[cursor] <= '9') ++cursor;
    }
    if (cursor < source.size() && source[cursor] == '.') {
      ++cursor;
      const std::size_t fractional = cursor;
      while (cursor < source.size() && source[cursor] >= '0' && source[cursor] <= '9') ++cursor;
      if (fractional == cursor) return std::unexpected("Invalid JSON fraction.");
    }
    if (cursor < source.size() && (source[cursor] == 'e' || source[cursor] == 'E')) {
      ++cursor;
      if (cursor < source.size() && (source[cursor] == '+' || source[cursor] == '-')) ++cursor;
      const std::size_t exponent = cursor;
      while (cursor < source.size() && source[cursor] >= '0' && source[cursor] <= '9') ++cursor;
      if (exponent == cursor) return std::unexpected("Invalid JSON exponent.");
    }
    Value value;
    value.type = Value::Type::NUMBER;
    value.scalar = std::string{source.substr(start, cursor - start)};
    return value;
  }

public:
  explicit Parser(std::string_view input) : source(input) { }

  [[nodiscard]] std::expected<Value, std::string> Parse() {
    if (source.size() > 64 * 1024 * 1024) return std::unexpected("JSON file exceeds 64 MiB.");
    auto value = ParseValue(0);
    if (!value) return value;
    Whitespace();
    if (cursor != source.size()) return std::unexpected("Unexpected trailing data after JSON value.");
    return value;
  }
};

[[nodiscard]] inline std::string Escape(std::string_view value) {
  std::string out{"\""};
  for (const unsigned char c : value) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          constexpr char hex[] = "0123456789abcdef";
          out += "\\u00";
          out.push_back(hex[c >> 4]);
          out.push_back(hex[c & 0x0f]);
        } else out.push_back(static_cast<char>(c));
    }
  }
  out.push_back('"');
  return out;
}

[[nodiscard]] inline const Value * Field(const Value & value, std::string_view name) {
  if (value.type != Value::Type::OBJECT) return nullptr;
  const auto found = value.object.find(std::string{name});
  return found == value.object.end() ? nullptr : &found->second;
}

[[nodiscard]] inline std::expected<std::string, std::string> StringField(
  const Value & value,
  std::string_view name
) {
  const Value * field = Field(value, name);
  if (!field || field->type != Value::Type::STRING) {
    return std::unexpected("Missing or invalid string field: " + std::string{name});
  }
  return field->scalar;
}

[[nodiscard]] inline std::expected<std::uint64_t, std::string> UnsignedField(
  const Value & value,
  std::string_view name
) {
  const Value * field = Field(value, name);
  if (!field || field->type != Value::Type::NUMBER || field->scalar.empty()
      || field->scalar.front() == '-' || field->scalar.find_first_of(".eE") != std::string::npos) {
    return std::unexpected("Missing or invalid integer field: " + std::string{name});
  }
  std::uint64_t result = 0;
  const auto parsed = std::from_chars(field->scalar.data(), field->scalar.data() + field->scalar.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != field->scalar.data() + field->scalar.size()) {
    return std::unexpected("Integer field is out of range: " + std::string{name});
  }
  return result;
}

} // namespace avida_web::education::json
