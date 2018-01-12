/*
 * Copyright 2017 The Native Object Protocols Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_DIE_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_DIE_H_

#include <cstdlib>
#include <functional>
#include <type_traits>

#include <nop/base/utility.h>
#include <nop/status.h>

//
// The Status<T> || Die(...) idiom provides a simple way to handle errors when
// robust error handling and fallback paths are not necessary. When the
// Status<T> on the left-hand side of the expression contains an error, the
// error is reported using the stream-like object passed to nop::Die() and the
// process exits. Otherwise the expression evaluates to the Status<T>, which is
// guaranteed to contain a value.
//
// Example of using the idiom with deserialization:
//
//  deserializer.Read(&first_value) || nop::Die(std::cerr);
//  deserializer.Read(&second_value) || nop::Die(std::cerr);
//
//  auto value = (GetResult() || nop::Die(std::cerr)).take();
//
namespace nop {

namespace detail {

// Traits type that evaluates to true if the stream operator "<<" is correctly
// defined for the given Stream and Value types.
template <typename Stream, typename Value, typename Enabled = void>
struct IsStreamable : std::false_type {};
template <typename Stream, typename Value>
struct IsStreamable<
    Stream, Value,
    Void<decltype(std::declval<Stream&&>() << std::declval<Value>())>>
    : std::true_type {};

// Utility wrapper type that accepts a reference to a stream-like object to use
// to output an error message before exiting the process. An instance of this
// object is passed to the right-hand side of a Status<T> || Die(...) expression
// to handle the error output if the left-hand side evaluates to an error.
//
// The template type Stream may be any type with appropriately defined overloads
// of the stream "<<" operator that can accept std::string as right-hand
// arguments.
template <typename Stream>
class StreamWrapper {
 private:
  using StreamType = std::remove_reference_t<Stream>;

 public:
  StreamWrapper(StreamWrapper&&) = default;
  explicit StreamWrapper(StreamType& stream) : stream_{stream} {}
  StreamWrapper(StreamType& stream, const char* error_message)
      : stream_{stream}, error_message_{error_message} {}

  StreamWrapper(const StreamWrapper&) = delete;
  void operator=(const StreamWrapper&) = delete;
  void operator=(StreamWrapper&&) = delete;

  // Operator overload that handles Status<T> || Die(...) expressions. If the
  // Status<T> argument contains an error the StreamWrapper is used to report an
  // error before exiting the process. Otherwise the Status<T> argument is
  // returned.
  template <typename T, typename Enabled = std::enable_if_t<
                            IsStreamable<Stream, std::string>::value>>
  friend Status<T> operator||(Status<T>&& status,
                                const StreamWrapper<Stream>& wrapper) {
    if (!status) {
      wrapper.PrintError(status.GetErrorMessage());
      std::exit(-1);
    } else {
      return std::move(status);
    }
  }

 private:
  void PrintError(const std::string& error_message) const {
    stream_.get() << error_message_ << ": " << error_message << std::endl;
  }

  std::reference_wrapper<StreamType> stream_;
  const char* error_message_{"Error"};
};

}  // namespace detail

// Wraps the given stream-like object in a StreamWrapper and returns it for use
// in a Status<T> || Die(...) expression.
template <typename Stream, typename... Args>
detail::StreamWrapper<Stream> Die(Stream&& stream, Args&&... args) {
  return detail::StreamWrapper<Stream>{std::forward<Stream>(stream),
                                       std::forward<Args>(args)...};
}

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_DIE_H_
