#ifndef LIBNOP_INCLUDE_NOP_BASE_CONTAINER_READER_H_
#define LIBNOP_INCLUDE_NOP_BASE_CONTAINER_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

template <typename Reader>
class ContainerReader {
 public:
  ContainerReader(Reader* reader, std::size_t size)
      : reader_{reader}, size_{size} {}

  Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus(ENOBUFS);
    else
      return reader_->Ensure(size);
  }

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < size_) {
      auto status = reader_->Read(prefix);
      if (!status)
        return status;

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

    auto status = reader_->ReadRaw(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  Status<void> ReadPadding() {
    const std::size_t padding_bytes = size_ - index_;
    auto status = reader_->Skip(padding_bytes);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    return reader_->GetHandle(handle_reference);
  }

  bool empty() const { return index_ == size_; }

 private:
  Reader* reader_;
  std::size_t size_;
  std::size_t index_{0};

  ContainerReader(const ContainerReader&) = delete;
  void operator=(const ContainerReader&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_CONTAINER_READER_H_
