#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

// BoundedWriter is a writer type that wraps another writer pointer and tracks
// the number of bytes written. Writer operations are transparently passed to
// the underlying writer unless the requested operation would exceed the size
// limit set at construction. BufferWriter can also pad the output up to the
// size limit in situations that require specific output payload size.
template <typename Writer>
class BoundedWriter {
 public:
  BoundedWriter() = default;
  BoundedWriter(const BoundedWriter&) = default;
  BoundedWriter(Writer* writer, std::size_t size)
      : writer_{writer}, size_{size} {}

  BoundedWriter& operator=(const BoundedWriter&) = default;

  Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return writer_->Prepare(size);
  }

  Status<void> Write(EncodingByte prefix) {
    if (index_ < size_) {
      auto status = writer_->Write(prefix);
      if (!status)
        return status;

      index_ += 1;
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

    auto status = writer_->WriteRaw(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  // Fills out any remaining bytes with the given padding value.
  Status<void> WritePadding(std::uint8_t padding_value = 0x00) {
    const std::size_t padding_bytes = size_ - index_;
    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  Status<HandleType> PushHandle(const HandleType& handle) {
    return writer_->PushHandle(handle);
  }

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  Writer* writer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_
