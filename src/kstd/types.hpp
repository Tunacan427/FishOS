#pragma once

#include <stdint.h>
#include <stddef.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = size_t;
using uptr = uintptr_t;

using nullptr_t = decltype(nullptr);

namespace kstd {
    template<typename T, T v>
    struct IntegralConstant {
        static constexpr T value = v;
    };

    template<bool v>
    using BooleanConstant = IntegralConstant<bool, v>;
    
    using True = BooleanConstant<true>;
    using False = BooleanConstant<false>;

    template<typename> struct IsIntegral : public False {};
    template<> struct IsIntegral<bool> : public True {};
    template<> struct IsIntegral<wchar_t> : public True {};
    template<> struct IsIntegral<char8_t> : public True {};
    template<> struct IsIntegral<char16_t> : public True {};
    template<> struct IsIntegral<char32_t> : public True {};
    template<> struct IsIntegral<u8> : public True {};
    template<> struct IsIntegral<u16> : public True {};
    template<> struct IsIntegral<u32> : public True {};
    template<> struct IsIntegral<u64> : public True {};
    template<> struct IsIntegral<i8> : public True {};
    template<> struct IsIntegral<i16> : public True {};
    template<> struct IsIntegral<i32> : public True {};
    template<> struct IsIntegral<i64> : public True {};
    
    template<typename T> concept Integral = IsIntegral<T>::value;

    template<usize _bits>
    struct __numeric_limits_helper {
        static constexpr usize bits = _bits;
    };

    template<typename> struct NumericLimits {};
    template<> struct NumericLimits<bool> : public __numeric_limits_helper<8> {};
    template<> struct NumericLimits<wchar_t> : public __numeric_limits_helper<16> {};
    template<> struct NumericLimits<char8_t> : public __numeric_limits_helper<8> {};
    template<> struct NumericLimits<char16_t> : public __numeric_limits_helper<16> {};
    template<> struct NumericLimits<char32_t> : public __numeric_limits_helper<32> {};
    template<> struct NumericLimits<u8> : public __numeric_limits_helper<8> {};
    template<> struct NumericLimits<u16> : public __numeric_limits_helper<16> {};
    template<> struct NumericLimits<u32> : public __numeric_limits_helper<32> {};
    template<> struct NumericLimits<u64> : public __numeric_limits_helper<64> {};
    template<> struct NumericLimits<i8> : public __numeric_limits_helper<8> {};
    template<> struct NumericLimits<i16> : public __numeric_limits_helper<16> {};
    template<> struct NumericLimits<i32> : public __numeric_limits_helper<32> {};
    template<> struct NumericLimits<i64> : public __numeric_limits_helper<64> {};

    template<typename T> struct RemoveReference { using type = T; };
    template<typename T> struct RemoveReference<T&> { using type = T; };
    template<typename T> struct RemoveReference<T&&> { using type = T; };

    template<typename T>
    constexpr auto move(T&& t) noexcept { 
        return static_cast<typename RemoveReference<T>::type&&>(t); 
    }

    template<typename T>
    constexpr inline void swap(T &a, T &b) {
        T tmp = move(a);
        a = move(b);
        b = move(tmp);
    }
}
