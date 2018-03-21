#pragma once

/**
 *	\file utils.h
 *	Various utility functions and classes, mostly related to meta-programming of some kind
 */


#include <atomic>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace tx {
    // strong typedefs from http://stackoverflow.com/a/28928606/2415419
#define STRONG_TYPEDEF(target_type, new_type) \
    auto name_seed_##new_type = [](){}; \
    using new_type = strong_typedef<target_type, decltype(name_seed_##new_type)>;

    template <typename T, typename NameSeed,
        typename Enable = typename std::conditional<!std::is_fundamental<T>::value,
        std::true_type,
        std::false_type>::type>
        struct strong_typedef;

    //specialization for non fundamental types
    template <typename T, typename NameSeed>
    struct strong_typedef<T, NameSeed, std::true_type> : public T
    {
        template <typename... Args>
        strong_typedef(Args&&... args) : T(std::forward<Args>(args)...) {}
    };

    //specialization for fundamental types
    template <typename T, typename NameSeed>
    struct strong_typedef<T, NameSeed, std::false_type>
    {
        T t;
        strong_typedef(const T t_) : t(t_) {};
        strong_typedef() {};
        strong_typedef(const strong_typedef & t_) : t(t_.t) {}
        strong_typedef & operator=(const strong_typedef & rhs) { t = rhs.t; return *this; }
        strong_typedef & operator=(const T & rhs) { t = rhs; return *this; }
        /*operator const T & () const {return t; }*/
        operator T & () { return t; }
        bool operator==(const strong_typedef & rhs) const { return t == rhs.t; }
        bool operator<(const strong_typedef & rhs) const { return t < rhs.t; }
    };


    // constexpr string, from Scott Schurr's presentation at CppNow! 2012
    class str_const {
    private:
        const char* const p_;
        const std::size_t sz_;
    public:
        template<std::size_t N>
        constexpr str_const(const char(&a)[N]) : // ctor
            p_(a), sz_(N - 1) {}
        constexpr char operator[](std::size_t n) const { // []
            return n < sz_ ? p_[n] :
                throw std::out_of_range("");
        }
        constexpr std::size_t size() const { return sz_; } // size()
    };

    // function traits implementation, from http://stackoverflow.com/a/7943765/2415419
    template <typename T>
    struct function_traits
        : public function_traits<decltype(&T::operator())>
    {};
    // For generic types, directly use the result of the signature of its 'operator()'

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const>
        // we specialize for pointers to member function
    {
        enum { arity = sizeof...(Args) };
        // arity is the number of arguments.

        typedef ReturnType result_type;

        template <size_t i>
        struct arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
            // the i-th argument is equivalent to the i-th tuple element of a tuple
            // composed of those arguments.
        };
    };

    // bool pack and all_true for sequences of booleans
    template<bool...> struct bool_pack;
    template<bool... bs>
    using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;



    // helper class to execute a functional when the current scope is closed
    template <class Func>
    class ExecAtScopeExit {
    public:
        explicit ExecAtScopeExit(Func fn_) :
            fn(fn_) {};

        ~ExecAtScopeExit() {
            fn();
        }
    private:
        const Func fn;
    };

    /**
    *  Helper class to set a variable temporarily, guaranteeing exception-safe reset
    *  after this helper object is destroyed.
    */
    template<typename VariableType, typename ValueType = VariableType>
    class ResetAfterScope {
    public:
        ResetAfterScope(VariableType& tgt) : target(tgt), valueBefore(tgt) {}
        ResetAfterScope(VariableType& tgt, const ValueType& value) : target(tgt), valueBefore(tgt) { target = value; }
        ~ResetAfterScope() { target = valueBefore; }
        // Todo: Think about moving operations, could be handled with either a flag or storing target as a pointer instead

        template<typename ValueType2> ResetAfterScope& operator=(const ValueType2& rhs) { target = rhs; }
        template<typename ValueType2> ResetAfterScope& operator=(ValueType2&& rhs) { target = rhs; }

    private:
        VariableType& target;
        ValueType valueBefore;
    };
    template<typename U, typename V>
    static ResetAfterScope<U, V> setTemporarily(U& tgt, const V& value) {
        return ResetAfterScope<U, V>(tgt, value);
    }
#define SET_TEMPORARILY(variable, value) auto temp_##variable = tx::setTemporarily((variable), (value))


    /**
     *  Assertion function, based on inviwo assertion
     */
    void txAssertion(const char* fileName, const char* functionName, long lineNumber,
        const char* condition, const std::string& message) {
        std::cout << "Assertion \"" << condition << "\" in (" << fileName << ", " << functionName << ", Ln " << lineNumber
            << "): ";
        std::cout << message << std::endl;

#ifdef WIN32
        __debugbreak();
#else
        raise(SIGTRAP);
#endif
        std::quick_exit(-1);
    }

#if defined(_DEBUG)
#define txAssert(condition, message)                                                             \
    {                                                                                             \
        std::ostringstream stream__;                                                              \
        stream__ << message;                                                                      \
        if (!(bool(condition))) tx::txAssertion(__FILE__, __FUNCTION__, __LINE__, #condition, (stream__.str())); \
    }
#else
#define txAssert(condition, message)
#endif

}
