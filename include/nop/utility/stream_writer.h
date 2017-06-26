#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_

#include <cstdint>
#include <ostream>

#include <nop/base/encoding.h>
#include <nop/status.h>

namespace nop {

template <typename OStream>
class StreamWriter {
 public:
  template <typename... Args>
  StreamWriter(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamWriter(const StreamWriter&) = default;
  StreamWriter& operator=(const StreamWriter&) = default;

  Status<void> Prepare(std::size_t /*size*/) { return {}; }

  Status<void> Write(EncodingByte prefix) {
    stream_.put(static_cast<typename OStream::char_type>(prefix));

    return ReturnStatus();
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> WriteRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    stream_.write(reinterpret_cast<const typename OStream::char_type*>(&*begin),
                  length_bytes);

    return ReturnStatus();
  }

  const OStream& stream() const { return stream_; }
  OStream& stream() { return stream_; }
  OStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus(EIO);
    else
      return {};
  }

  OStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
