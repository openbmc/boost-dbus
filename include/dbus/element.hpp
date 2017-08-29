// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_ELEMENT_HPP
#define DBUS_ELEMENT_HPP

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

namespace dbus {

/// Message elements
/**
 * D-Bus Messages are composed of simple elements of one of these types
 */
// bool // is this simply valid? It might pack wrong...
// http://maemo.org/api_refs/5.0/5.0-final/dbus/api/group__DBusTypes.html
typedef boost::uint8_t byte;

typedef boost::int16_t int16;
typedef boost::uint16_t uint16;
typedef boost::int32_t int32;
typedef boost::uint32_t uint32;

typedef boost::int64_t int64;
typedef boost::uint64_t uint64;
// double
// unix_fd

typedef std::string string;

typedef boost::variant<std::string, bool, byte, int16, uint16, int32, uint32,
                       int64, uint64, double>
    dbus_variant;

struct object_path {
  string value;
};
struct signature {
  string value;
};

template <std::size_t... Is>
struct seq {};

template <std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {};

template <std::size_t N1, std::size_t... I1, std::size_t N2, std::size_t... I2>
constexpr std::array<char, N1 + N2 - 1> concat_helper(
    const std::array<char, N1>& a1, const std::array<char, N2>& a2, seq<I1...>,
    seq<I2...>) {
  return {{a1[I1]..., a2[I2]...}};
}

template <std::size_t N1, std::size_t N2>
// Initializer for the recursion
constexpr std::array<char, N1 + N2 - 1> concat(const std::array<char, N1>& a1,
                                               const std::array<char, N2>& a2) {
  // note, this function expects both character arrays to be null
  // terminated. The -1 below drops the null terminator on the first string
  return concat_helper(a1, a2, gen_seq<N1 - 1>{}, gen_seq<N2>{});
}

/**
 * D-Bus Message elements are identified by unique integer type codes.
 */

template <typename InvalidType>
struct element_signature;

template <>
struct element_signature<bool> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_BOOLEAN, 0}};
};

template <>
struct element_signature<byte> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_BYTE, 0}};
};

template <>
struct element_signature<int16> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_INT16, 0}};
};

template <>
struct element_signature<uint16> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_UINT16, 0}};
};

template <>
struct element_signature<int32> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_INT32, 0}};
};

template <>
struct element_signature<uint32> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_UINT32, 0}};
};

template <>
struct element_signature<int64> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_INT64, 0}};
};

template <>
struct element_signature<uint64> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_UINT64, 0}};
};

template <>
struct element_signature<double> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_DOUBLE, 0}};
};

template <>
struct element_signature<string> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_STRING, 0}};
};

template <>
struct element_signature<dbus_variant> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_VARIANT, 0}};
};

template <>
struct element_signature<object_path> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_OBJECT_PATH, 0}};
};

template <>
struct element_signature<signature> {
  static auto constexpr code = std::array<char, 2>{{DBUS_TYPE_SIGNATURE, 0}};
};

template <typename Key, typename Value>
struct element_signature<std::pair<Key, Value>> {
  static auto constexpr code =
      concat(std::array<char, 2>{{'{', 0}},
             concat(element_signature<Key>::code,
                    concat(element_signature<Value>::code,
                           std::array<char, 2>{{'}', 0}})));
  ;
};

template <typename Element>
struct element_signature<std::vector<Element>> {
  static auto constexpr code = concat(std::array<char, 2>{{DBUS_TYPE_ARRAY, 0}},
                                      element_signature<Element>::code);
};

template <typename InvalidType>
struct element {
  static const int code = DBUS_TYPE_INVALID;
};

template <>
struct element<bool> {
  static const int code = DBUS_TYPE_BOOLEAN;
};

template <>
struct element<byte> {
  static const int code = DBUS_TYPE_BYTE;
};

template <>
struct element<int16> {
  static const int code = DBUS_TYPE_INT16;
};

template <>
struct element<uint16> {
  static const int code = DBUS_TYPE_UINT16;
};

template <>
struct element<int32> {
  static const int code = DBUS_TYPE_INT32;
};

template <>
struct element<uint32> {
  static const int code = DBUS_TYPE_UINT32;
};

template <>
struct element<int64> {
  static const int code = DBUS_TYPE_INT64;
};

template <>
struct element<uint64> {
  static const int code = DBUS_TYPE_UINT64;
};

template <>
struct element<double> {
  static const int code = DBUS_TYPE_DOUBLE;
};

template <>
struct element<string> {
  static const int code = DBUS_TYPE_STRING;
};

template <>
struct element<dbus_variant> {
  static const int code = DBUS_TYPE_VARIANT;
};

template <>
struct element<object_path> {
  static const int code = DBUS_TYPE_OBJECT_PATH;
};

template <>
struct element<signature> {
  static const int code = DBUS_TYPE_SIGNATURE;
};

template <typename InvalidType>
struct is_fixed_type {
  static const int value = false;
};

template <>
struct is_fixed_type<bool> {
  static const int value = true;
};

template <>
struct is_fixed_type<byte> {
  static const int value = true;
};

template <>
struct is_fixed_type<int16> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint16> {
  static const int value = true;
};

template <>
struct is_fixed_type<int32> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint32> {
  static const int value = true;
};

template <>
struct is_fixed_type<int64> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint64> {
  static const int value = true;
};

template <>
struct is_fixed_type<double> {
  static const int value = true;
};

template <typename InvalidType>
struct is_string_type {
  static const bool value = false;
};

template <>
struct is_string_type<string> {
  static const bool value = true;
};

template <>
struct is_string_type<object_path> {
  static const bool value = true;
};

template <>
struct is_string_type<signature> {
  static const bool value = true;
};

}  // namespace dbus

#endif  // DBUS_ELEMENT_HPP
