#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class BufferReader {
 public:
  BufferReader() = default;
  BufferReader(const BufferReader&) = default;
  BufferReader(const void* buffer, std::size_t size)
      : buffer_{static_cast<const std::uint8_t*>(buffer)}, size_{size} {}

  BufferReader& operator=(const BufferReader&) = default;

  Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus(ENOBUFS);
    else
      return {};
  }

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < size_) {
      *prefix = buffer_[index_];
      index_ += 1;
      return {};
    } else {
      return ErrorStatus(ENOBUFS);
    }
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> ReadRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    if (length_bytes > (size_ - index_))
      return ErrorStatus(ENOBUFS);

    std::copy(&buffer_[index_], &buffer_[index_ + length_bytes],
              reinterpret_cast<std::uint8_t*>(&*begin));

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus(ENOBUFS);

    index_ += padding_bytes;
    return {};
  }

  bool empty() const { return index_ == size_; }

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  const std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
