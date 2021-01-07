#ifndef ALIA_COMMON_HPP
#define ALIA_COMMON_HPP

#include <cassert>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

// This file defines some generic functionality that's commonly used throughout
// alia.

namespace alia {

typedef long long counter_type;

// Inspired by Boost, inheriting from noncopyable disables copying for a type.
// The namespace prevents unintended ADL if used by applications.
namespace detail { namespace noncopyable_ {
/// \internal
struct noncopyable
{
    noncopyable()
    {
    }

 private:
    noncopyable(noncopyable const& other) = delete;
    noncopyable&
    operator=(noncopyable const& other)
        = delete;
};
}} // namespace detail::noncopyable_
typedef detail::noncopyable_::noncopyable noncopyable;

// general-purpose exception class for alia
struct exception : std::exception
{
    exception(std::string const& msg) : msg_(new std::string(msg))
    {
    }

    ~exception() noexcept(true)
    {
    }

    virtual char const*
    what() const noexcept(true)
    {
        return msg_->c_str();
    }

    // Add another level of context to the error messsage.
    void
    add_context(std::string const& str)
    {
        *msg_ += "\n" + str;
    }

 private:
    std::shared_ptr<std::string> msg_;
};

// ALIA_UNUSED is used to denote that a variable may be unused.
#ifdef __GNUC__
#define ALIA_UNUSED [[gnu::unused]]
#else
#define ALIA_UNUSED
#endif

// ALIA_LAMBDIFY(f) produces a lambda that calls f, which is essentially a
// version of f that can be passed as an argument and still allows normal
// overload resolution.
#define ALIA_LAMBDIFY(f)                                                      \
    [](auto&&... args) { return f(std::forward<decltype(args)>(args)...); }
#ifndef ALIA_STRICT_MACROS
#define alia_lambdify(f) ALIA_LAMBDIFY(f)
#endif

// ALIA_AGGREGATOR(f) produces a lambda that assembles its arguments into an
// aggregate expression (i.e., "{args...}") and passes that into f.
// This is useful, for example, when you want to explicitly refer to the
// aggregate constructor of a type as an invocable function.
#define ALIA_AGGREGATOR(f)                                                    \
    [](auto&&... args) { return f{std::forward<decltype(args)>(args)...}; }
#ifndef ALIA_STRICT_MACROS
#define alia_aggregator(f) ALIA_AGGREGATOR(f)
#endif

// function_view is the non-owning equivalent of std::function.
template<class Signature>
class function_view;
template<class Return, class... Args>
class function_view<Return(Args...)> final
{
 private:
    using signature_type = Return(void*, Args...);

    void* _ptr;
    Return (*_erased_fn)(void*, Args...);

 public:
    template<typename T>
    function_view(T&& x) noexcept : _ptr{(void*) std::addressof(x)}
    {
        _erased_fn = [](void* ptr, Args... xs) -> Return {
            return (*reinterpret_cast<std::add_pointer_t<T>>(ptr))(
                std::forward<Args>(xs)...);
        };
    }

    decltype(auto)
    operator()(Args... xs) const
        noexcept(noexcept(_erased_fn(_ptr, std::forward<Args>(xs)...)))
    {
        return _erased_fn(_ptr, std::forward<Args>(xs)...);
    }
};

// uncaught_exception_detector is a utility for detecting whether or not an
// object is being destructed due to an uncaught exception.
struct uncaught_exception_detector
{
    uncaught_exception_detector()
    {
        base_exception_count_ = std::uncaught_exceptions();
    }

    bool
    detect()
    {
        return base_exception_count_ != std::uncaught_exceptions();
    }

 private:
    int base_exception_count_;
};

} // namespace alia

#endif
