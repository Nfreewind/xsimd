/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_UTILS_HPP
#define XSIMD_UTILS_HPP

#include <cstdint>

namespace xsimd
{

    template <class T, size_t N>
    class batch;

    /**************
     * as_integer *
     **************/

    template <class T>
    struct as_integer;

    template <>
    struct as_integer<float>
    {
        using type = int32_t;
    };

    template <>
    struct as_integer<double>
    {
        using type = int64_t;
    };

    template <class T, std::size_t N>
    struct as_integer<batch<T, N>>
    {
        using type = batch<typename as_integer<T>::type, N>;
    };

    template <class T>
    using as_integer_t = typename as_integer<T>::type;

    /***********************
     * as_unsigned_integer *
     ***********************/

    template <class T>
    struct as_unsigned_integer;

    template <>
    struct as_unsigned_integer<float>
    {
        using type = uint32_t;
    };

    template <>
    struct as_unsigned_integer<double>
    {
        using type = uint64_t;
    };

    template <>
    struct as_unsigned_integer<int32_t>
    {
        using type = uint32_t;
    };

    template <>
    struct as_unsigned_integer<int64_t>
    {
        using type = uint64_t;
    };

    template <class T, std::size_t N>
    struct as_unsigned_integer<batch<T, N>>
    {
        using type = batch<typename as_unsigned_integer<T>::type, N>;
    };

    template <class T>
    using as_unsigned_integer_t = typename as_unsigned_integer<T>::type;

    /***********
     * as_float *
     ************/

    template <class T>
    struct as_float;

    template <>
    struct as_float<int32_t>
    {
        using type = float;
    };

    template <>
    struct as_float<int64_t>
    {
        using type = double;
    };

    template <class T, std::size_t N>
    struct as_float<batch<T, N>>
    {
        using type = batch<typename as_float<T>::type, N>;
    };

    template <class T>
    using as_float_t = typename as_float<T>::type;

    /**************
     * as_logical *
     **************/

    template <class T>
    struct as_logical;

    template <class T, std::size_t N>
    struct as_logical<batch<T, N>>
    {
        using type = batch_bool<T, N>;
    };

    template <class T>
    using as_logical_t = typename as_logical<T>::type;

    /********************
     * primitive caster *
     ********************/

    namespace detail
    {
        template <class UI, class I, class F>
        union generic_caster {
            UI ui;
            I i;
            F f;

            constexpr generic_caster(UI t)
                : ui(t) {}
            constexpr generic_caster(I t)
                : i(t) {}
            constexpr generic_caster(F t)
                : f(t) {}
        };

        using caster32_t = generic_caster<uint32_t, int32_t, float>;
        using caster64_t = generic_caster<uint64_t, int64_t, double>;

        template <class T>
        struct caster;

        template <>
        struct caster<float>
        {
            using type = caster32_t;
        };

        template <>
        struct caster<double>
        {
            using type = caster64_t;
        };

        template <class T>
        using caster_t = typename caster<T>::type;
    }

    /****************************
     * to/from_unsigned_integer *
     ****************************/

    namespace detail
    {
        template <typename T>
        union unsigned_convertor {
            T data;
            as_unsigned_integer_t<T> bits;
        };

        template <typename T>
        as_unsigned_integer_t<T> to_unsigned_integer(const T& input) {
            unsigned_convertor<T> convertor;
            convertor.data = input;
            return convertor.bits;
        }

        template <typename T>
        T from_unsigned_integer(const as_unsigned_integer_t<T>& input) {
            unsigned_convertor<T> convertor;
            convertor.bits = input;
            return convertor.data;
        }
    }

    /*****************************************
     * Backport of index_sequence from c++14 *
     *****************************************/

    // TODO: Remove this once we drop C++11 support
    namespace detail
    {
        template <typename T>
        struct identity { using type = T; };

        #ifdef __cpp_lib_integer_sequence
            using std::integer_sequence;
            using std::index_sequence;
            using std::make_index_sequence;
            using std::index_sequence_for;
        #else
            template <typename T, T... Is>
            struct integer_sequence {
            using value_type = T;
            static constexpr std::size_t size() noexcept { return sizeof...(Is); }
            };

            template <std::size_t... Is>
            using index_sequence = integer_sequence<std::size_t, Is...>;

            template <typename Lhs, typename Rhs>
            struct make_index_sequence_concat;

            template <std::size_t... Lhs, std::size_t... Rhs>
            struct make_index_sequence_concat<index_sequence<Lhs...>,
                                            index_sequence<Rhs...>>
              : identity<index_sequence<Lhs..., (sizeof...(Lhs) + Rhs)...>> {};

            template <std::size_t N>
            struct make_index_sequence_impl;

            template <std::size_t N>
            using make_index_sequence = typename make_index_sequence_impl<N>::type;

            template <std::size_t N>
            struct make_index_sequence_impl
              : make_index_sequence_concat<make_index_sequence<N / 2>,
                                           make_index_sequence<N - (N / 2)>> {};

            template <>
            struct make_index_sequence_impl<0> : identity<index_sequence<>> {};

            template <>
            struct make_index_sequence_impl<1> : identity<index_sequence<0>> {};

            template <typename... Ts>
            using index_sequence_for = make_index_sequence<sizeof...(Ts)>;
        #endif
    }

    /*****************************************
     * Supplementary std::array constructors *
     *****************************************/

    namespace detail
    {
        // std::array constructor from scalar value ("broadcast")
        template <typename T, std::size_t... Is>
        constexpr std::array<T, sizeof...(Is)>
        array_from_scalar_impl(const T& scalar, index_sequence<Is...>) {
            // You can safely ignore this silly ternary, the "scalar" is all
            // that matters. The rest is just a dirty workaround...
            return std::array<T, sizeof...(Is)>{ (Is+1) ? scalar : T() ... };
        }

        template <typename T, std::size_t N>
        constexpr std::array<T, N>
        array_from_scalar(const T& scalar) {
            return array_from_scalar_impl(scalar, make_index_sequence<N>());
        }

        // std::array constructor from C-style pointer (handled as an array)
        template <typename T, std::size_t... Is>
        constexpr std::array<T, sizeof...(Is)>
        array_from_pointer_impl(const T* c_array, index_sequence<Is...>) {
            return std::array<T, sizeof...(Is)>{ c_array[Is]... };
        }

        template <typename T, std::size_t N>
        constexpr std::array<T, N>
        array_from_pointer(const T* c_array) {
            return array_from_pointer_impl(c_array, make_index_sequence<N>());
        }
    }

    /************************
     * is_array_initializer *
     ************************/

    namespace detail
    {
        template <bool...> struct bool_pack;

        template <bool... bs>
        using all_true = std::is_same<
            bool_pack<bs..., true>, bool_pack<true, bs...>
        >;

        template <typename T, typename... Args>
        using is_only_this_type = all_true<std::is_same<Args, T>::value...>;

        template <typename T, std::size_t N, typename... Args>
        using is_array_initializer = std::enable_if<
            (sizeof...(Args) == N) && is_only_this_type<T, Args...>::value
        >;

        // Check that a variadic argument pack is a list of N values of type T,
        // as usable for instantiating a value of type std::array<T, N>.
        template <typename T, std::size_t N, typename... Args>
        using is_array_initializer_t = typename is_array_initializer<T, N, Args...>::type;
    }
}

#endif
