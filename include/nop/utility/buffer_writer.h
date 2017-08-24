#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class BufferWriter {
 public:
  BufferWriter() = default;
  BufferWriter(const BufferWriter&) = default;
  BufferWriter(void* buffer, std::size_t size)
      : buffer_{static_cast<std::uint8_t*>(buffer)}, size_{size} {}

  BufferWriter& operator=(const BufferWriter&) = default;

  Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return {};
  }

  Status<void> Write(EncodingByte prefix) {
    if (index_ < size_) {
      buffer_[index_++] = static_cast<std::uint8_t>(prefix);
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> WriteRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    std::copy(reinterpret_cast<const std::uint8_t*>(&*begin),
              reinterpret_cast<const std::uint8_t*>(&*end), &buffer_[index_]);

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    auto status = Prepare(padding_bytes);
    if (!status)
      return status;

    while (padding_bytes) {
      buffer_[index_++] = padding_value;
      padding_bytes--;
    }

    return {};
  }

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
