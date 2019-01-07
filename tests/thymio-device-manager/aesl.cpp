#include <catch2/catch.hpp>
#include <aseba/thymio-device-manager/aesl_parser.h>
#include <aseba/common/consts.h>
#include <fmt/format.h>

TEST_CASE("Aesl can be parsed", "[aesl]") {
    SECTION("empty file") {
        std::string aesl = R"(
                       <!DOCTYPE aesl-source>
                       <network></network>)";
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE((res->constants() && res->constants()->empty()));
        REQUIRE((res->nodes() && res->nodes()->empty()));
        REQUIRE((res->global_events() && res->global_events()->empty()));
    }
}

TEST_CASE("constants", "[aesl]") {

    SECTION("required fields for constants") {
        std::string test_template = R"(
                       <!DOCTYPE aesl-source> <network>{}
                       </network>)";

        auto input = GENERATE(values<std::string>({
            R"(<constant name="test" />)",
            R"(<constant value="0" />)",
            R"(<constant />)",
        }));
        std::string aesl = fmt::format(test_template, input);
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(!res->constants());
    }

    SECTION("constants") {
        std::string aesl = R"(
                       <!DOCTYPE aesl-source>
                       <network><constant name="test" value="42"/></network>)";
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(res->constants());
        auto constants = *res->constants();
        REQUIRE(constants.size() == 1);
        REQUIRE(constants.count("test") == 1);
        REQUIRE(constants["test"] == 42);
    }

    SECTION("valid constants") {
        std::string test_template = R"(
                       <!DOCTYPE aesl-source>
                       <network><constant name="ok" value="1"/>
                                <constant name="test" value="{}"/>
                       </network>)";
        auto [input, expected] = GENERATE(table<std::string, bool>({
            {"-327", true},     // ok
            {"-32769", false},  // to small
            {"32768", false},   // to big
            {"aaa", false},     // NaN
            {"1a", false},      // NaN
            {"0.1", false},     // no double
            {"-1", true},       // ok
            {"32767", true},    // ok
            {"-32768", true},   // ok

        }));
        CAPTURE(input);
        std::string aesl = fmt::format(test_template, input);
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        CHECK(res->constants().has_value() == expected);
    }
}


TEST_CASE("events", "[aesl]") {

    SECTION("required fields for events") {
        std::string test_template = R"(
                       <!DOCTYPE aesl-source> <network>{}
                       </network>)";

        auto input = GENERATE(values<std::string>({
            R"(<event name="test" />)",
            R"(<event value="0" />)",
            R"(<event />)",
        }));
        std::string aesl = fmt::format(test_template, input);
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(!res->global_events());
    }

    SECTION("events") {
        std::string aesl = R"(
                       <!DOCTYPE aesl-source>
                       <network><event name="test" size="2"/></network>)";
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(res->global_events());
        auto events = *res->global_events();
        REQUIRE(events.size() == 1);
        REQUIRE(events[0].name == "test");
        REQUIRE(events[0].size == 2);
        REQUIRE(events[0].type == mobsya::event_type::aseba);
    }

    SECTION("valid events") {
        std::string test_template = R"(
                       <!DOCTYPE aesl-source>
                       <network><event name="ok" size="1"/>
                                <event name="test" size="{}"/>
                       </network>)";
        auto [input, expected] = GENERATE(table<std::string, bool>({
            {"0", true},                                                // ok
            {"-1", false},                                              // to small
            {fmt::format("{}", ASEBA_MAX_EVENT_ARG_COUNT), true},       // ok
            {fmt::format("{}", ASEBA_MAX_EVENT_ARG_COUNT + 1), false},  // to big
            {"aaa", false},                                             // NaN
            {"0.1", false}                                              // NaN

        }));
        CAPTURE(input);
        std::string aesl = fmt::format(test_template, input);
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(res->global_events().has_value() == expected);
    }
}


TEST_CASE("nodes", "[aesl]") {

    SECTION("required fields for nodes") {
        std::string test_template = R"(
                       <!DOCTYPE aesl-source> <network>{}
                       </network>)";

        auto input = GENERATE(values<std::string>({
            R"(<node/>)",
        }));
        std::string aesl = fmt::format(test_template, input);
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(!res->nodes());
    }

    SECTION("nodes") {
        std::string aesl = R"(
                       <!DOCTYPE aesl-source>
                       <network>
                           <node name="foo" nodeId="2">test</node>
                           <node name="uuid_test" nodeId="{7d7fe12a-01cd-4d0a-9d70-7c6ec0b97094}">test</node>
                           <node name="uuid_test2" nodeId="7d7fe12a-01cd-4d0a-9d70-7c6ec0b97094">test</node>
                           <node name="bar" nodeId="2"><![CDATA[test]]></node>
                       </network>)";
        auto res = mobsya::load_aesl(aesl);
        REQUIRE(res);
        REQUIRE(res->nodes());
        auto nodes = *res->nodes();
        REQUIRE(nodes.size() == 4);
        REQUIRE(nodes[0].name == "foo");
        REQUIRE(variant_ns::holds_alternative<uint16_t>(nodes[0].id));
        REQUIRE(variant_ns::holds_alternative<mobsya::node_id>(nodes[1].id));
        REQUIRE(variant_ns::holds_alternative<mobsya::node_id>(nodes[2].id));
        REQUIRE(variant_ns::get<uint16_t>(nodes[0].id) == 2);
        REQUIRE(nodes[0].code == "test");
        REQUIRE(nodes[3].code == "test");
    }
}