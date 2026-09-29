#include <cassert>
#include <string>

#include "source/web/EducationJson.hpp"

int main() {
  using namespace avida_web::education::json;
  const std::string source =
    R"({"name":"Avida \u0394","rows":[0,1.25,null],"ok":true})";
  auto parsed = Parser{source}.Parse();
  assert(parsed);
  assert(StringField(*parsed, "name").value() == "Avida \xce\x94");
  const auto * rows = Field(*parsed, "rows");
  assert(rows && rows->type == Value::Type::ARRAY && rows->array.size() == 3);
  assert(Field(*parsed, "ok")->boolean);

  assert(!Parser{R"({"x":1,"x":2})"}.Parse());
  assert(!Parser{R"({"x":"bad\q"})"}.Parse());
  assert(!Parser{R"([1,])"}.Parse());
  assert(!Parser{R"(true false)"}.Parse());
  assert(Escape("quote \" and newline\n") == R"("quote \" and newline\n")");
}
