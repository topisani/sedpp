#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <fmt/format.h>

#include <charconv>
#include <string>
#include <string_view>

namespace sed {

  using namespace std::literals;

  bool eat_blank(std::string_view& sv)
  {
    bool res = false;
    while (sv.size() > 0 && std::isblank(sv.front())) {
      sv = sv.substr(1);
      res = true;
    }
    return res;
  }

  bool eat(std::string_view& sv, char c)
  {
    if (sv.empty()) return false;
    if (sv.front() == c) {
      sv = sv.substr(1);
      return true;
    }
    return false;
  }

  bool eat_n(std::string_view& sv, int n)
  {
    if (sv.size() < n) return false;
    sv = sv.substr(n);
    return true;
  }

  std::string_view get_until(std::string_view& input, char c)
  {
    std::size_t i = 0;
    for (; i < input.size(); i++) {
      if (input[i] == c) {
        auto result = input.substr(0, i);
        input = input.substr(i + 1);
        return result;
      }
    }
    auto result = input;
    input = input.substr(i);
    return result;
  }

  std::string_view get_line(std::string_view& input)
  {
    return get_until(input, '\n');
  }

  struct Address {
    std::size_t line_number = {};
    ;

    bool operator==(const Address&) const = default;
  };

  enum struct Function {
    none,
    d,
    equals,
    p,
  };

  struct Command {
    Function function = Function::none;
    std::optional<Address> address = std::nullopt;

    bool operator==(const Command&) const = default;
  };

  std::optional<Address> parse_address(std::string_view& script)
  {
    std::size_t result;
    if (auto [p, ec] = std::from_chars(script.data(), script.data() + script.size(), result); ec == std::errc()) {
      script = script.substr(p - script.data());
      return Address{result};
    }
    return std::nullopt;
  }

  std::optional<Command> parse_command(std::string_view& input)
  {
    while (input.size() > 0 && (std::isblank(input.front()) || input.front() == ';')) {
      input = input.substr(1);
    }

    Command result;
    result.address = parse_address(input);
    eat_blank(input);
    char cmd_char = input.front();
    eat_n(input, 1);
    switch (cmd_char) {
      case 'd': result.function = Function::d; break;
      case '=': result.function = Function::equals; break;
      case 'p': result.function = Function::p; break;
      default: return std::nullopt;
    }
    return {result};
  }

  std::vector<Command> parse_script(std::string_view script)
  {
    std::vector<Command> result;
    while (true) {
      auto cmd = parse_command(script);
      if (!cmd) break;
      result.emplace_back(std::move(*cmd));
    }
    return result;
  }

  struct StreamEditor {
    StreamEditor(std::vector<Command> cmds) noexcept : commands(std::move(cmds)) {}

    void run_cycle(std::string_view& input)
    {
      line_number++;
      pattern_space = get_line(input);
      fmt::print("Processing line {}: '{}'\n", line_number, pattern_space);
      for (auto& command : commands) {
        switch (command.function) {
          case Function::none: break;
          case Function::d: {
            if (!address_selects(command)) break;
            pattern_space.clear();
            return;
          } break;
          case Function::equals: {
            if (!address_selects(command)) break;
            write_line(std::to_string(line_number));
          } break;
          case Function::p: {
            if (!address_selects(command)) break;
            write_line(pattern_space);
          } break;
        }
      }
      write_line(pattern_space);
    }

    void write_line(std::string_view line)
    {
      output += line;
      output += '\n';
    }

    bool address_selects(const Command& cmd) const
    {
      if (!cmd.address) return true;
      if (cmd.address->line_number == line_number) return true;
      return false;
    }

    std::vector<Command> commands;
    std::string output = "";
    std::string pattern_space = "";
    std::string hold_space = "";
    std::size_t line_number = 0;
  };

  std::string execute(std::string_view input, std::string_view script)
  {
    auto sed = StreamEditor(parse_script(script));
    while (!input.empty()) {
      sed.run_cycle(input);
    }
    return sed.output;
  }

  TEST_CASE ("get_line") {
    SUBCASE ("Empty input") {
      auto input = ""sv;
      REQUIRE(get_line(input) == "");
      REQUIRE(input == "");
    }
    SUBCASE ("Single line, no trailing newline") {
      auto input = "line"sv;
      REQUIRE(get_line(input) == "line");
      REQUIRE(input == "");
    }
    SUBCASE ("Single line, trailing newline") {
      auto input = "line\n"sv;
      REQUIRE(get_line(input) == "line");
      REQUIRE(input == "");
    }
    SUBCASE ("Multiline") {
      auto input = "line1\nline2"sv;
      REQUIRE(get_line(input) == "line1");
      REQUIRE(input == "line2");
    }
  }

  TEST_CASE ("parse_script") {
    SUBCASE ("2d") {
      REQUIRE(parse_script("2d")[0] == Command{Function::d, Address{2}});
    }
  }

  TEST_CASE ("execute()") {
    SUBCASE ("Empty script") {
      REQUIRE(execute("input", "") == "input\n");
    }

    SUBCASE ("script: d") {
      REQUIRE(execute("input", "d") == "");
    }

    SUBCASE ("commands can be preceded by <blank> or ;") {
      REQUIRE(execute("input", " ;\td") == "");
    }

    SUBCASE ("script: 2d") {
      REQUIRE(execute("line1\nline2\nline3", "2d") == "line1\nline3\n");
    }
    SUBCASE ("Function can be preceded by blank characters") {
      REQUIRE(execute("line1\nline2\nline3", "2 \td") == "line1\nline3\n");
    }

    SUBCASE ("Multi-command script: '2d; 3d'") {
      REQUIRE(execute("line1\nline2\nline3\nline4\n", "2d; 3d") == "line1\nline4\n");
    }

    SUBCASE ("script: =") {
      REQUIRE(execute("line1\nline2\nline3", "=") == "1\nline1\n2\nline2\n3\nline3\n");
    }
    SUBCASE ("script: 2=") {
      REQUIRE(execute("line1\nline2\nline3", "2=") == "line1\n2\nline2\nline3\n");
    }
    SUBCASE ("script: 1p") {
      REQUIRE(execute("line1\nline2\nline3\n", "1p") == "line1\nline1\nline2\nline3\n");
    }
  }

} // namespace sed

namespace doctest {
  template<typename T>
  struct StringMaker<std::optional<T>> {
    static doctest::String convert(std::optional<T> const& value)
    {
      if (value == std::nullopt) return "nullopt";
      return String("{") + StringMaker<std::decay_t<T>>::convert(*value) + "}";
    }
  };
} // namespace doctest
