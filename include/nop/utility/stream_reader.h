#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_

#include <cstdint>
#include <istream>

#include <nop/base/encoding.h>
#include <nop/status.h>

namespace nop {

//
// Reader template type that wraps STL input streams.
//
// Implements the basic reader interface on top of an STL input stream type.
//

template <typename IStream>
class StreamReader {
 public:
  template <typename... Args>
  StreamReader(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamReader(const StreamReader&) = default;
  StreamReader& operator=(const StreamReader&) = default;

  Status<void> Ensure(std::size_t /*size*/) { return {}; }

  Status<void> Read(EncodingByte* prefix) {
    stream_.read(reinterpret_cast<typename IStream::char_type*>(prefix),
                 sizeof(EncodingByte));

    return ReturnStatus();
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> ReadRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    stream_.read(reinterpret_cast<typename IStream::char_type*>(&*begin),
                 length_bytes);

    return ReturnStatus();
  }

  Status<void> Skip(std::size_t padding_bytes) {
    stream_.seekg(padding_bytes, std::ios_base::cur);
    return ReturnStatus();
  }

  const IStream& stream() const { return stream_; }
  IStream& stream() { return stream_; }
  IStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus(EIO);
    else
      return {};
  }

  IStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
