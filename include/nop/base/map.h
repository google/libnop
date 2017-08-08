#ifndef LIBNOP_INCLUDE_NOP_BASE_MAP_H_
#define LIBNOP_INCLUDE_NOP_BASE_MAP_H_

#include <map>
#include <unordered_map>

#include <nop/base/encoding.h>

namespace nop {

//
// std::map<Key, T> and std::unordered_map<Key, T> encoding format:
//
// +-----+---------+--------//---------+
// | MAP | INT64:N | N KEY/VALUE PAIRS |
// +-----+---------+--------//---------+
//
// Each pair must be a valid encoding of Key followed by a valid encoding of T.
//

template <typename Key, typename T, typename Compare, typename Allocator>
struct Encoding<std::map<Key, T, Compare, Allocator>>
    : EncodingIO<std::map<Key, T, Compare, Allocator>> {
  using Type = std::map<Key, T, Compare, Allocator>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;

    value->clear();
    for (std::uint64_t i = 0; i < size; i++) {
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

template <typename Key, typename T, typename Hash, typename KeyEqual,
          typename Allocator>
struct Encoding<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>>
    : EncodingIO<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> {
  using Type = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;

    value->clear();
    for (std::uint64_t i = 0; i < size; i++) {
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MAP_H_
