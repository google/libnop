#ifndef LIBNOP_EXAMPLES_STREAM_UTILITIES_H_
#define LIBNOP_EXAMPLES_STREAM_UTILITIES_H_

#include <array>
#include <iostream>
#include <vector>

namespace {

// Prints a std::vector<T> to the given stream. This template will work for any
// type T that has an operator<< overload.
template <typename T, typename Allocator>
std::ostream& operator<<(std::ostream& stream,
                         const std::vector<T, Allocator>& vector) {
  stream << "vector{";

  const std::size_t length = vector.size();
  std::size_t count = 0;

  for (const auto& element : vector) {
    stream << element;
    if (count < length - 1)
      stream << ", ";
    count++;
  }

  stream << "}";
  return stream;
}

// Prints a std::array<T, Size> to the given stream. This template will work for
// any type T that has an operator<< overload.
template <typename T, std::size_t Size>
std::ostream& operator<<(std::ostream& stream,
                         const std::array<T, Size>& array) {
  stream << "array{";

  const std::size_t length = array.size();
  std::size_t count = 0;

  for (const auto& element : array) {
    stream << element;
    if (count < length - 1)
      stream << ", ";
    count++;
  }

  stream << "}";
  return stream;
}

}  // anonymous namespace

#endif  // LIBNOP_EXAMPLES_STREAM_UTILITIES_H_
