#include <alia/signals/operators.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/utilities.hpp>

TEST_CASE("signal operators", "[signals]")
{
    using namespace alia;

    REQUIRE(is_true(value(2) == value(2)));
    REQUIRE(is_false(value(6) == value(2)));
    REQUIRE(is_true(value(6) != value(2)));
    REQUIRE(is_false(value(2) != value(2)));
    REQUIRE(is_true(value(6) > value(2)));
    REQUIRE(is_false(value(6) < value(2)));
    REQUIRE(is_true(value(6) >= value(2)));
    REQUIRE(is_true(value(2) >= value(2)));
    REQUIRE(is_false(value(2) >= value(6)));
    REQUIRE(is_true(value(2) < value(6)));
    REQUIRE(is_false(value(6) < value(2)));
    REQUIRE(is_true(value(2) <= value(6)));
    REQUIRE(is_true(value(2) <= value(2)));
    REQUIRE(is_false(value(6) <= value(2)));

    REQUIRE(is_true(value(6) + value(2) == value(8)));
    REQUIRE(is_true(value(6) - value(2) == value(4)));
    REQUIRE(is_true(value(6) * value(2) == value(12)));
    REQUIRE(is_true(value(6) / value(2) == value(3)));
    REQUIRE(is_true(value(6) % value(2) == value(0)));
    REQUIRE(is_true((value(6) ^ value(2)) == value(4)));
    REQUIRE(is_true((value(6) & value(2)) == value(2)));
    REQUIRE(is_true((value(6) | value(2)) == value(6)));
    REQUIRE(is_true(value(6) << value(2) == value(24)));
    REQUIRE(is_true(value(6) >> value(2) == value(1)));

    REQUIRE(is_true(-value(2) == value(-2)));
    REQUIRE(is_false(!(value(2) == value(2))));
}

TEST_CASE("select_signal", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), value(1), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    owned_id captured_id;
    captured_id.store(s.value_id());
    condition = true;
    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(captured_id.get() != s.value_id());
}

TEST_CASE("select_signal with different directions", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), empty<int>(), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_is_readable(s));
}

TEST_CASE("select_signal value ID", "[signals]")
{
    // Test that select_signal's ID changes when the condition changes, even
    // if both of its input signals are producing the same value ID.

    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), value(2), value(2));

    owned_id captured_id;
    captured_id.store(s.value_id());
    condition = true;
    REQUIRE(captured_id.get() != s.value_id());
}

TEST_CASE("writable select_signal", "[signals]")
{
    using namespace alia;

    bool condition = false;
    int x = 1;
    int y = 2;
    auto s = select_signal(direct(&condition), direct(&x), direct(&y));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(read_signal(s) == 1);
    write_signal(s, 4);
    REQUIRE(x == 4);
    REQUIRE(y == 2);
    REQUIRE(read_signal(s) == 4);
    condition = false;
    write_signal(s, 3);
    REQUIRE(x == 4);
    REQUIRE(y == 3);
    REQUIRE(read_signal(s) == 3);
}

TEST_CASE("field signal", "[signals]")
{
    using namespace alia;

    struct foo
    {
        int x;
        double y;
    };
    foo f = {2, 1.5};
    auto f_signal = lambda_inout(
        always_readable,
        [&]() { return f; },
        always_writable,
        [&](foo const& v) { f = v; },
        [&]() { return combine_ids(make_id(f.x), make_id(f.y)); });

    auto x_signal = f_signal->*&foo::x;

    typedef decltype(x_signal) x_signal_t;
    REQUIRE((std::is_same<x_signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<x_signal_t>::value);
    REQUIRE(signal_can_write<x_signal_t>::value);

    REQUIRE(signal_is_readable(x_signal));
    REQUIRE(read_signal(x_signal) == 2);
    REQUIRE(signal_is_writable(x_signal));
    write_signal(x_signal, 1);
    REQUIRE(f.x == 1);

    auto y_signal = alia_field(f_signal, y);

    typedef decltype(y_signal) y_signal_t;
    REQUIRE((std::is_same<y_signal_t::value_type, double>::value));
    REQUIRE(signal_can_read<y_signal_t>::value);
    REQUIRE(signal_can_write<y_signal_t>::value);

    REQUIRE(y_signal.value_id() != x_signal.value_id());
    REQUIRE(signal_is_readable(y_signal));
    REQUIRE(read_signal(y_signal) == 1.5);
    REQUIRE(signal_is_writable(y_signal));
    owned_id captured_y_id;
    captured_y_id.store(y_signal.value_id());
    write_signal(y_signal, 0.5);
    REQUIRE(y_signal.value_id() != captured_y_id.get());
    REQUIRE(f.y == 0.5);
}