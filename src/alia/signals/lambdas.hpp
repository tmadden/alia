#ifndef ALIA_SIGNALS_LAMBDAS_HPP
#define ALIA_SIGNALS_LAMBDAS_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines utilities for constructing custom signals via lambda
// functions.

namespace alia {

// lambda_reader(is_readable, read) creates a read-only signal whose value is
// determined by calling :is_readable and :read.
template<class Value, class IsReadable, class Read>
struct lambda_reader_signal : regular_signal<
                                  lambda_reader_signal<Value, IsReadable, Read>,
                                  Value,
                                  read_only_signal>
{
    lambda_reader_signal(IsReadable is_readable, Read read)
        : is_readable_(is_readable), read_(read)
    {
    }
    bool
    is_readable() const
    {
        return is_readable_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    IsReadable is_readable_;
    Read read_;
    mutable decltype(read_()) value_;
};
template<class IsReadable, class Read>
auto
lambda_reader(IsReadable is_readable, Read read)
{
    return lambda_reader_signal<
        std::decay_t<decltype(read())>,
        IsReadable,
        Read>(is_readable, read);
}

// lambda_reader(is_readable, read, generate_id) creates a read-only signal
// whose value is determined by calling :is_readable and :read and whose ID is
// determined by calling :generate_id.
template<class Value, class IsReadable, class Read, class GenerateId>
struct lambda_reader_signal_with_id
    : signal<
          lambda_reader_signal_with_id<Value, IsReadable, Read, GenerateId>,
          Value,
          read_only_signal>
{
    lambda_reader_signal_with_id(
        IsReadable is_readable, Read read, GenerateId generate_id)
        : is_readable_(is_readable), read_(read), generate_id_(generate_id)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = generate_id_();
        return id_;
    }
    bool
    is_readable() const
    {
        return is_readable_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    IsReadable is_readable_;
    Read read_;
    mutable decltype(read_()) value_;
    GenerateId generate_id_;
    mutable decltype(generate_id_()) id_;
};
template<class IsReadable, class Read, class GenerateId>
auto
lambda_reader(IsReadable is_readable, Read read, GenerateId generate_id)
{
    return lambda_reader_signal_with_id<
        std::decay_t<decltype(read())>,
        IsReadable,
        Read,
        GenerateId>(is_readable, read, generate_id);
}

// lambda_bidirectional(is_readable, read, is_writable, write) creates a
// bidirectional signal whose value is read by calling :is_readable and :read
// and written by calling :is_writable and :write.
template<
    class Value,
    class IsReadable,
    class Read,
    class IsWritable,
    class Write>
struct lambda_bidirectional_signal : regular_signal<
                                         lambda_bidirectional_signal<
                                             Value,
                                             IsReadable,
                                             Read,
                                             IsWritable,
                                             Write>,
                                         Value,
                                         bidirectional_signal>
{
    lambda_bidirectional_signal(
        IsReadable is_readable, Read read, IsWritable is_writable, Write write)
        : is_readable_(is_readable),
          read_(read),
          is_writable_(is_writable),
          write_(write)
    {
    }
    bool
    is_readable() const
    {
        return is_readable_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }
    bool
    is_writable() const
    {
        return is_writable_();
    }
    void
    write(Value const& value) const
    {
        write_(value);
    }

 private:
    IsReadable is_readable_;
    Read read_;
    mutable decltype(read_()) value_;
    IsWritable is_writable_;
    Write write_;
};
template<class IsReadable, class Read, class IsWritable, class Write>
auto
lambda_bidirectional(
    IsReadable is_readable, Read read, IsWritable is_writable, Write write)
{
    return lambda_bidirectional_signal<
        std::decay_t<decltype(read())>,
        IsReadable,
        Read,
        IsWritable,
        Write>(is_readable, read, is_writable, write);
}

// lambda_bidirectional(is_readable, read, is_writable, write, generate_id)
// creates a bidirectional signal whose value is read by calling :is_readable
// and :read and written by calling :is_writable and :write. Its ID is
// determined by calling :generate_id.
template<
    class Value,
    class IsReadable,
    class Read,
    class IsWritable,
    class Write,
    class GenerateId>
struct lambda_bidirectional_signal_with_id
    : signal<
          lambda_bidirectional_signal_with_id<
              Value,
              IsReadable,
              Read,
              IsWritable,
              Write,
              GenerateId>,
          Value,
          bidirectional_signal>
{
    lambda_bidirectional_signal_with_id(
        IsReadable is_readable,
        Read read,
        IsWritable is_writable,
        Write write,
        GenerateId generate_id)
        : is_readable_(is_readable),
          read_(read),
          is_writable_(is_writable),
          write_(write),
          generate_id_(generate_id)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = generate_id_();
        return id_;
    }
    bool
    is_readable() const
    {
        return is_readable_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }
    bool
    is_writable() const
    {
        return is_writable_();
    }
    void
    write(Value const& value) const
    {
        write_(value);
    }

 private:
    IsReadable is_readable_;
    Read read_;
    mutable decltype(read_()) value_;
    IsWritable is_writable_;
    Write write_;
    GenerateId generate_id_;
    mutable decltype(generate_id_()) id_;
};
template<
    class IsReadable,
    class Read,
    class IsWritable,
    class Write,
    class GenerateId>
auto
lambda_bidirectional(
    IsReadable is_readable,
    Read read,
    IsWritable is_writable,
    Write write,
    GenerateId generate_id)
{
    return lambda_bidirectional_signal_with_id<
        std::decay_t<decltype(read())>,
        IsReadable,
        Read,
        IsWritable,
        Write,
        GenerateId>(is_readable, read, is_writable, write, generate_id);
}

// This is just a clear and concise way of indicating that a lambda signal is
// always readable.
inline bool
always_readable()
{
    return true;
}

// This is just a clear and concise way of indicating that a lambda signal is
// always writable.
inline bool
always_writable()
{
    return true;
}

} // namespace alia

#endif
