#ifndef LIBNOP_INCLUDE_NOP_BASE_CONTAINER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_BASE_CONTAINER_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

template <typename Writer>
class ContainerWriter {
 public:
  ContainerWriter(Writer* writer, std::size_t size)
      : writer_{writer}, size_{size} {}

  Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus(ENOBUFS);
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
      return ErrorStatus(ENOBUFS);
    }
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> WriteRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    if (length_bytes > (size_ - index_))
      return ErrorStatus(ENOBUFS);

    auto status = writer_->WriteRaw(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  Status<void> WritePadding() {
    const std::size_t padding_bytes = size_ - index_;
    auto status = writer_->Skip(padding_bytes, 0x5a);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  Status<HandleType> PushHandle(const HandleType& handle) {
    return writer_->PushHandle(handle);
  }

 private:
  Writer* writer_;
  std::size_t size_;
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_CONTAINER_WRITER_H_
