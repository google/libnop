#ifndef LIBNOP_TEST_TEST_WRITER_H_
#define LIBNOP_TEST_TEST_WRITER_H_

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>
#include <nop/types/file_handle.h>

namespace nop {

class TestWriter {
 public:
  TestWriter() = default;

  Status<void> Prepare(std::size_t size) {
    data_.reserve(data_.size() + size);
    return {};
  }

  Status<void> Write(EncodingByte prefix) {
    data_.push_back(static_cast<std::uint8_t>(prefix));
    return {};
  }

  Status<void> Write(std::uint8_t value) {
    data_.push_back(value);
    return {};
  }

  Status<void> Write(std::uint16_t value) {
    Write(static_cast<std::uint8_t>(value));
    Write(static_cast<std::uint8_t>(value >> 8));
    return {};
  }

  Status<void> Write(std::uint32_t value) {
    Write(static_cast<std::uint16_t>(value));
    Write(static_cast<std::uint16_t>(value >> 16));
    return {};
  }

  Status<void> Write(std::uint64_t value) {
    Write(static_cast<std::uint32_t>(value));
    Write(static_cast<std::uint32_t>(value >> 32));
    return {};
  }

  Status<void> Write(std::int8_t value) {
    return Write(static_cast<std::uint8_t>(value));
  }

  Status<void> Write(std::int16_t value) {
    return Write(static_cast<std::uint16_t>(value));
  }

  Status<void> Write(std::int32_t value) {
    return Write(static_cast<std::uint32_t>(value));
  }

  Status<void> Write(std::int64_t value) {
    return Write(static_cast<std::uint64_t>(value));
  }

  Status<void> Write(char value) {
    return Write(static_cast<std::uint8_t>(value));
  }

  // Writes an integer of type U as type T. The caller must ensure that the
  // value can fit in type T without truncation.
  template <typename T, typename U, typename Enabled = EnableIfIntegral<T, U>>
  Status<void> Write(U value) {
    return Write(static_cast<T>(value));
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> WriteRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    // Extend the buffer to accomodate the data.
    const std::size_t start_offset = data_.size();
    data_.resize(start_offset + length_bytes);

    std::copy(reinterpret_cast<const std::uint8_t*>(&*begin),
              reinterpret_cast<const std::uint8_t*>(&*end),
              &data_[start_offset]);
    return {};
  }

  template <typename HandleType>
  Status<HandleReference> PushHandle(const HandleType& handle) {
    if (handle) {
      HandleReference handle_reference = handles_.size();
      handles_.push_back(handle.get());
      return {handle_reference};
    } else {
      return {kEmptyHandleReference};
    }
  }

  void clear() {
    data_.clear();
    handles_.clear();
  }

  const std::vector<uint8_t>& data() const { return data_; }
  const std::vector<int>& handles() const { return handles_; }

 private:
  std::vector<std::uint8_t> data_;
  std::vector<int> handles_;

  TestWriter(const TestWriter&) = delete;
  void operator=(const TestWriter&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_TEST_TEST_WRITER_H_
