#include <alia/signals/lambdas.hpp>

#include <catch.hpp>

TEST_CASE("lambda input signal", "[signals]")
{
    using namespace alia;

    auto s = lambda_input(always_readable, []() { return 1; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("lambda input signal with ID", "[signals]")
{
    using namespace alia;

    auto s = lambda_input(
        always_readable, []() { return 1; }, []() { return unit_id; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(s.value_id() == unit_id);
}

TEST_CASE("lambda inout signal", "[signals]")
{
    using namespace alia;

    int x = 1;

    auto s = lambda_inout(
        always_readable,
        [&x]() { return x; },
        always_writable,
        [&x](int v) { x = v; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    owned_id original_id;
    original_id.store(s.value_id());
    write_signal(s, 0);
    REQUIRE(read_signal(s) == 0);
    REQUIRE(!original_id.matches(s.value_id()));
}

TEST_CASE("lambda inout signal with ID", "[signals]")
{
    using namespace alia;

    int x = 1;

    auto s = lambda_inout(
        always_readable,
        [&x]() { return x; },
        always_writable,
        [&x](int v) { x = v; },
        [&x]() { return make_id(x); });

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(s.value_id() == make_id(1));
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0);
    REQUIRE(read_signal(s) == 0);
    REQUIRE(s.value_id() == make_id(0));
}