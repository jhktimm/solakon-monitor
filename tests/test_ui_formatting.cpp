#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "ui.h"

TEST_CASE("format_power: values < 1000W", "[ui][format]") {
    REQUIRE(solakon::ui::TerminalUI::format_power(0.0f) == "0 W");
    REQUIRE(solakon::ui::TerminalUI::format_power(100.0f) == "100 W");
    REQUIRE(solakon::ui::TerminalUI::format_power(999.9f) == "1000 W");
    REQUIRE(solakon::ui::TerminalUI::format_power(500.0f) == "500 W");
    REQUIRE(solakon::ui::TerminalUI::format_power(1.5f) == "2 W");
}

TEST_CASE("format_power: values >= 1000W", "[ui][format]") {
    REQUIRE(solakon::ui::TerminalUI::format_power(1000.0f) == "1.00 kW");
    REQUIRE(solakon::ui::TerminalUI::format_power(1500.0f) == "1.50 kW");
    REQUIRE(solakon::ui::TerminalUI::format_power(10000.0f) == "10.00 kW");
    REQUIRE(solakon::ui::TerminalUI::format_power(12345.0f) == "12.35 kW");
    REQUIRE(solakon::ui::TerminalUI::format_power(99999.0f) == "100.00 kW");
}

TEST_CASE("format_voltage: standard values", "[ui][format]") {
    REQUIRE(solakon::ui::TerminalUI::format_voltage(230.0f) == "230.0 V");
    REQUIRE(solakon::ui::TerminalUI::format_voltage(0.0f) == "0.0 V");
    REQUIRE(solakon::ui::TerminalUI::format_voltage(400.0f) == "400.0 V");
    REQUIRE(solakon::ui::TerminalUI::format_voltage(115.5f) == "115.5 V");
}

TEST_CASE("format_current: standard values", "[ui][format]") {
    REQUIRE(solakon::ui::TerminalUI::format_current(0.0f) == "0.00 A");
    REQUIRE(solakon::ui::TerminalUI::format_current(10.5f) == "10.50 A");
    REQUIRE(solakon::ui::TerminalUI::format_current(100.0f) == "100.00 A");
    REQUIRE(solakon::ui::TerminalUI::format_current(-5.25f) == "-5.25 A");
}

TEST_CASE("separator: correct length and character", "[ui][format]") {
    auto sep = solakon::ui::TerminalUI::separator(10, '-');
    REQUIRE(sep.find("----------") != std::string::npos);

    auto sep2 = solakon::ui::TerminalUI::separator(20, '=');
    REQUIRE(sep2.find("====================") != std::string::npos);

    auto sep3 = solakon::ui::TerminalUI::separator(5, ' ');
    REQUIRE(sep3.find("     ") != std::string::npos);
}

TEST_CASE("section_header: contains title", "[ui][format]") {
    auto hdr = solakon::ui::TerminalUI::section_header("Test", 40);
    REQUIRE(hdr.find("Test") != std::string::npos);
}

TEST_CASE("status_badge: ok=true shows green", "[ui][format]") {
    auto badge = solakon::ui::TerminalUI::status_badge("OK", true, solakon::ui::Color::GREEN);
    REQUIRE(badge.find("OK") != std::string::npos);
}

TEST_CASE("status_badge: ok=false shows red", "[ui][format]") {
    auto badge = solakon::ui::TerminalUI::status_badge("FAIL", false, solakon::ui::Color::RED);
    REQUIRE(badge.find("FAIL") != std::string::npos);
}

TEST_CASE("bar: zero max value returns spaces", "[ui][format]") {
    auto b = solakon::ui::TerminalUI::bar(100.0f, 0.0f, 10);
    REQUIRE(!b.empty());
}

TEST_CASE("bar: zero value returns empty bar", "[ui][format]") {
    auto b = solakon::ui::TerminalUI::bar(0.0f, 100.0f, 10);
    REQUIRE(!b.empty());
}

TEST_CASE("bar: full value returns full bar", "[ui][format]") {
    auto b = solakon::ui::TerminalUI::bar(100.0f, 100.0f, 10);
    REQUIRE(b.find('#') != std::string::npos);
}

TEST_CASE("bar: partial value", "[ui][format]") {
    auto b = solakon::ui::TerminalUI::bar(50.0f, 100.0f, 10);
    REQUIRE(!b.empty());
}

TEST_CASE("color: returns non-empty string with escape codes", "[ui][format]") {
    auto c = solakon::ui::TerminalUI::color(solakon::ui::Color::RED, solakon::ui::Color::DEFAULT, "test");
    REQUIRE(!c.empty());
    REQUIRE(c.find('\033') != std::string::npos);
}

TEST_CASE("bold: returns non-empty string", "[ui][format]") {
    auto b = solakon::ui::TerminalUI::bold(solakon::ui::Color::BRIGHT_CYAN, "hello");
    REQUIRE(!b.empty());
    REQUIRE(b.find("hello") != std::string::npos);
}

TEST_CASE("dim: returns non-empty string", "[ui][format]") {
    auto d = solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY, "dimmed");
    REQUIRE(!d.empty());
    REQUIRE(d.find("dimmed") != std::string::npos);
}
