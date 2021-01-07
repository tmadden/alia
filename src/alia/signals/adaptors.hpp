#ifndef ALIA_SIGNALS_ADAPTORS_HPP
#define ALIA_SIGNALS_ADAPTORS_HPP

#include <alia/signals/basic.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

// signalize(x) turns x into a signal if it isn't already one.
// Or, in other words...
// signalize(s), where s is a signal, returns s.
// signalize(v), where v is a raw value, returns a value signal carrying v.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, Signal>
signalize(Signal s)
{
    return s;
}
template<class Value, std::enable_if_t<!is_signal_type<Value>::value, int> = 0>
auto
signalize(Value v)
{
    return value(std::move(v));
}

// fake_readability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have read capabilities. It will never actually have a value, but
// it will type-check as a readable signal.
template<class Wrapped>
struct readability_faker final : signal_wrapper<
                                     readability_faker<Wrapped>,
                                     Wrapped,
                                     typename Wrapped::value_type,
                                     typename signal_capabilities_union<
                                         read_only_signal,
                                         typename Wrapped::capabilities>::type>
{
    readability_faker(Wrapped wrapped)
        : readability_faker::signal_wrapper(std::move(wrapped))
    {
    }
    id_interface const&
    value_id() const override
    {
        return null_id;
    }
    bool
    has_value() const override
    {
        return false;
    }
    // Since this is only faking readability, read() should never be called.
    // LCOV_EXCL_START
    typename Wrapped::value_type const&
    read() const override
    {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
        return *(typename Wrapped::value_type const*) nullptr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
readability_faker<Wrapped>
fake_readability(Wrapped wrapped)
{
    return readability_faker<Wrapped>(std::move(wrapped));
}

// fake_writability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have write capabilities. It will never actually be ready to
// write, but it will type-check as a writable signal.
template<class Wrapped>
struct writability_faker final : signal_wrapper<
                                     writability_faker<Wrapped>,
                                     Wrapped,
                                     typename Wrapped::value_type,
                                     typename signal_capabilities_union<
                                         write_only_signal,
                                         typename Wrapped::capabilities>::type>
{
    writability_faker(Wrapped wrapped)
        : writability_faker::signal_wrapper(std::move(wrapped))
    {
    }
    bool
    ready_to_write() const override
    {
        return false;
    }
    // Since this is only faking writability, write() should never be called.
    // LCOV_EXCL_START
    void write(typename Wrapped::value_type) const override
    {
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
writability_faker<Wrapped>
fake_writability(Wrapped wrapped)
{
    return writability_faker<Wrapped>(std::move(wrapped));
}

// signal_cast<Value>(x), where :x is a signal, yields a proxy for :x with
// the value type :Value. The proxy will apply static_casts to convert its
// own values to and from :x's value type.
template<class Wrapped, class To>
struct casting_signal final
    : casting_signal_wrapper<casting_signal<Wrapped, To>, Wrapped, To>
{
    casting_signal(Wrapped wrapped)
        : casting_signal::casting_signal_wrapper(std::move(wrapped))
    {
    }
    To const&
    read() const override
    {
        value_ = this->movable_value();
        return value_;
    }
    To
    movable_value() const override
    {
        return static_cast<To>(forward_signal(this->wrapped_));
    }
    void
    write(To value) const override
    {
        return this->wrapped_.write(
            static_cast<typename Wrapped::value_type>(value));
    }

 private:
    mutable To value_;
};
// signal_caster is just another level of indirection that allows us to
// eliminate the casting_signal entirely if it's just going to cast to the same
// type.
template<class Wrapped, class To>
struct signal_caster
{
    typedef casting_signal<Wrapped, To> type;
    static type
    apply(Wrapped wrapped)
    {
        return type(std::move(wrapped));
    }
};
template<class Wrapped>
struct signal_caster<Wrapped, typename Wrapped::value_type>
{
    typedef Wrapped type;
    static type
    apply(Wrapped wrapped)
    {
        return wrapped;
    }
};
template<class To, class Wrapped>
typename signal_caster<Wrapped, To>::type
signal_cast(Wrapped wrapped)
{
    return signal_caster<Wrapped, To>::apply(std::move(wrapped));
}

// has_value(x) yields a signal to a boolean which indicates whether or not :x
// has a value. (The returned signal itself always has a value.)
template<class Wrapped>
struct value_presence_signal final
    : regular_signal<value_presence_signal<Wrapped>, bool, read_only_signal>
{
    value_presence_signal(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    bool const&
    read() const override
    {
        value_ = wrapped_.has_value();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
has_value(Wrapped wrapped)
{
    return value_presence_signal<Wrapped>(std::move(wrapped));
}

// ready_to_write(x) yields a signal to a boolean that indicates whether or not
// :x is ready to write. (The returned signal always has a value.)
template<class Wrapped>
struct write_readiness_signal final
    : regular_signal<write_readiness_signal<Wrapped>, bool, read_only_signal>
{
    write_readiness_signal(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    bool const&
    read() const override
    {
        value_ = wrapped_.ready_to_write();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
ready_to_write(Wrapped wrapped)
{
    return write_readiness_signal<Wrapped>(std::move(wrapped));
}

// add_default(primary, default), where :primary and :default are both
// signals, yields another signal whose value is that of :primary if it has one
// and that of :default otherwise.
// All writes go directly to :primary.
template<class Primary, class Default>
struct default_value_signal final
    : signal_wrapper<default_value_signal<Primary, Default>, Primary>
{
    default_value_signal()
    {
    }
    default_value_signal(Primary primary, Default default_value)
        : default_value_signal::signal_wrapper(std::move(primary)),
          default_(std::move(default_value))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() || default_.has_value();
    }
    typename Primary::value_type const&
    read() const override
    {
        return this->wrapped_.has_value() ? this->wrapped_.read()
                                          : default_.read();
    }
    id_interface const&
    value_id() const override
    {
        id_ = combine_ids(
            make_id(this->wrapped_.has_value()),
            this->wrapped_.has_value() ? ref(this->wrapped_.value_id())
                                       : ref(default_.value_id()));
        return id_;
    }

 private:
    mutable id_pair<simple_id<bool>, id_ref> id_;
    Default default_;
};
template<class Primary, class Default>
default_value_signal<Primary, Default>
make_default_value_signal(Primary primary, Default default_)
{
    return default_value_signal<Primary, Default>(
        std::move(primary), std::move(default_));
}
template<class Primary, class Default>
auto
add_default(Primary primary, Default default_)
{
    return make_default_value_signal(
        signalize(std::move(primary)), signalize(std::move(default_)));
}

// simplify_id(s), where :s is a signal, yields a wrapper for :s with the exact
// same read/write behavior but whose value ID is a simple_id (i.e., it is
// simply the value of the signal).
//
// The main utility of this is in cases where you have a signal carrying a
// small value with a complicated value ID (because it was picked from the
// signal of a larger data structure, for example). The more complicated ID
// might change superfluously.
//
template<class Wrapped>
struct simplified_id_wrapper
    : signal_wrapper<simplified_id_wrapper<Wrapped>, Wrapped>
{
    simplified_id_wrapper(Wrapped wrapped)
        : simplified_id_wrapper::signal_wrapper(std::move(wrapped))
    {
    }
    id_interface const&
    value_id() const override
    {
        if (this->has_value())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return null_id;
    }

 private:
    mutable simple_id_by_reference<typename Wrapped::value_type> id_;
};
template<class Wrapped>
simplified_id_wrapper<Wrapped>
simplify_id(Wrapped wrapped)
{
    return simplified_id_wrapper<Wrapped>(std::move(wrapped));
}

template<class Primary, class Mask>
struct masking_signal final
    : signal_wrapper<masking_signal<Primary, Mask>, Primary>
{
    masking_signal()
    {
    }
    masking_signal(Primary primary, Mask mask)
        : masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    has_value() const override
    {
        return mask_.has_value() && mask_.read() && this->wrapped_.has_value();
    }
    id_interface const&
    value_id() const override
    {
        if (mask_.has_value() && mask_.read())
            return this->wrapped_.value_id();
        else
            return null_id;
    }
    bool
    ready_to_write() const override
    {
        return mask_.has_value() && mask_.read()
               && this->wrapped_.ready_to_write();
    }

 private:
    Mask mask_;
};
template<class Signal, class AvailabilityFlag>
auto
make_masking_signal(Signal signal, AvailabilityFlag availability_flag)
{
    return masking_signal<Signal, AvailabilityFlag>(
        std::move(signal), std::move(availability_flag));
}

/// Mask a signal.
///
/// This does the equivalent of bit-masking on an individual signal. If
/// \p availibility_flag evaluates to true, the mask evaluates to a signal
/// equivalent to \p signal. Otherwise, it evaluates to an empty signal of the
/// same type.
///
template<class Signal, class AvailabilityFlag>
auto
mask(Signal signal, AvailabilityFlag availability_flag)
{
    return make_masking_signal(
        signalize(std::move(signal)), signalize(std::move(availability_flag)));
}

// mask_writes(signal, writability_flag) masks writes to :signal according to
// the value of :writability_flag.
//
// :writability_flag can be either a signal or a raw value. If it evaluates to
// true (in a boolean context), the mask evaluates to a signal equivalent to
// :signal. Otherwise, it evaluates to one with equivalent reading behavior but
// with writing disabled.
//
// Note that in either case, the masked version has the same capabilities as
// :signal.
//
template<class Primary, class Mask>
struct write_masking_signal final
    : signal_wrapper<write_masking_signal<Primary, Mask>, Primary>
{
    write_masking_signal()
    {
    }
    write_masking_signal(Primary primary, Mask mask)
        : write_masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    ready_to_write() const override
    {
        return mask_.has_value() && mask_.read()
               && this->wrapped_.ready_to_write();
    }

 private:
    Mask mask_;
};
template<class Signal, class WritabilityFlag>
auto
make_write_masking_signal(Signal signal, WritabilityFlag writability_flag)
{
    return write_masking_signal<Signal, WritabilityFlag>(
        std::move(signal), std::move(writability_flag));
}
template<class Signal, class WritabilityFlag>
auto
mask_writes(Signal signal, WritabilityFlag writability_flag)
{
    return make_write_masking_signal(
        std::move(signal), signalize(std::move(writability_flag)));
}

// disable_writes(s), where :s is a signal, yields a wrapper for :s where
// writes are disabled. Like mask_writes, this doesn't change the capabilities
// of :s.
template<class Signal>
auto
disable_writes(Signal s)
{
    return mask_writes(std::move(s), false);
}

// mask_reads(signal, readality_flag) masks reads to :signal according to the
// value of :readality_flag.
//
// :readality_flag can be either a signal or a raw value. If it evaluates to
// true (in a boolean context), the mask evaluates to a signal equivalent to
// :signal. Otherwise, it evaluates to one with equivalent writing behavior but
// with reading disabled.
//
// Note that in either case, the masked version has the same capabilities as
// :signal.
//
template<class Primary, class Mask>
struct read_masking_signal final
    : signal_wrapper<read_masking_signal<Primary, Mask>, Primary>
{
    read_masking_signal()
    {
    }
    read_masking_signal(Primary primary, Mask mask)
        : read_masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    has_value() const override
    {
        return mask_.has_value() && mask_.read() && this->wrapped_.has_value();
    }

 private:
    Mask mask_;
};
template<class Signal, class ReadabilityFlag>
auto
make_read_masking_signal(Signal signal, ReadabilityFlag readability_flag)
{
    return read_masking_signal<Signal, ReadabilityFlag>(
        std::move(signal), std::move(readability_flag));
}
template<class Signal, class ReadabilityFlag>
auto
mask_reads(Signal signal, ReadabilityFlag readability_flag)
{
    return make_read_masking_signal(
        std::move(signal), signalize(std::move(readability_flag)));
}

// disable_reads(s), where :s is a signal, yields a wrapper for :s where reads
// are disabled. Like mask_reads, this doesn't change the capabilities of :s.
template<class Signal>
auto
disable_reads(Signal s)
{
    return mask_reads(std::move(s), false);
}

// unwrap(signal), where :signal is a signal carrying a std::optional value,
// yields a signal that directly carries the value wrapped inside the optional.
template<class Wrapped>
struct unwrapper_signal final : casting_signal_wrapper<
                                    unwrapper_signal<Wrapped>,
                                    Wrapped,
                                    typename Wrapped::value_type::value_type>
{
    unwrapper_signal()
    {
    }
    unwrapper_signal(Wrapped wrapped)
        : unwrapper_signal::casting_signal_wrapper(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() && this->wrapped_.read().has_value();
    }
    typename Wrapped::value_type::value_type const&
    read() const override
    {
        return this->wrapped_.read().value();
    }
    typename Wrapped::value_type::value_type
    movable_value() const override
    {
        return this->wrapped_.movable_value().value();
    }
    id_interface const&
    value_id() const override
    {
        if (this->has_value())
            return this->wrapped_.value_id();
        else
            return null_id;
    }
    void
    write(typename Wrapped::value_type::value_type value) const override
    {
        this->wrapped_.write(std::move(value));
    }
};
template<class Signal>
auto
unwrap(Signal signal)
{
    return unwrapper_signal<Signal>(std::move(signal));
}

// move(signal) returns a signal with movement activated (if possible).
//
// If the input signal supports movement, the returned signal's value can be
// moved out with move_signal() or forward_signal().
//
// If the input signal doesn't support movement, it's returned unchanged.
//
template<class Wrapped>
struct signal_movement_activator
    : signal_wrapper<
          signal_movement_activator<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          signal_capabilities<
              signal_movable,
              typename Wrapped::capabilities::writing>>
{
    signal_movement_activator()
    {
    }
    signal_movement_activator(Wrapped wrapped)
        : signal_movement_activator::signal_wrapper(std::move(wrapped))
    {
    }
};
template<
    class Signal,
    std::enable_if_t<is_copyable_signal_type<Signal>::value, int> = 0>
auto
move(Signal signal)
{
    return signal_movement_activator<Signal>(std::move(signal));
}
template<
    class Signal,
    std::enable_if_t<
        is_signal_type<Signal>::value && signal_is_readable<Signal>::value
            && !signal_is_copyable<Signal>::value,
        int> = 0>
auto
move(Signal signal)
{
    return signal;
}

} // namespace alia

#endif
