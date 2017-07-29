#include <errno.h>

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include <nop/base/table.h>
#include <nop/serializer.h>
#include <nop/status.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"
#include "string_to_hex.h"

using nop::ActiveEntry;
using nop::DeletedEntry;
using nop::Deserializer;
using nop::Entry;
using nop::ErrorStatus;
using nop::Serializer;
using nop::Status;
using nop::StreamReader;
using nop::StreamWriter;
using nop::StringToHex;

namespace {

// The first version of the table with a single member.
struct TableA0 {
  Entry<std::string, 0> a;

  NOP_TABLE("TableA", TableA0, a);
};

// The second version of the table that adds a member.
struct TableA1 {
  Entry<std::string, 0> a;
  Entry<std::vector<int>, 1> b;

  NOP_TABLE("TableA", TableA1, a, b);
};

// The third version of the table that deletes a member.
struct TableA2 {
  Entry<std::string, 0> a;
  Entry<std::vector<int>, 1, DeletedEntry> b;

  NOP_TABLE("TableA", TableA2, a, b);
};

template <typename T, std::uint64_t Id>
std::ostream& operator<<(std::ostream& stream,
                         const Entry<T, Id, ActiveEntry>& entry) {
  if (entry)
    stream << entry.get();
  else
    stream << "<empty>";

  return stream;
}

template <typename T, std::uint64_t Id>
std::ostream& operator<<(std::ostream& stream,
                         const Entry<T, Id, DeletedEntry>& entry) {
  stream << "<deleted>";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const TableA0& table) {
  stream << "TableA0{" << table.a << "}";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const TableA1& table) {
  stream << "TableA1{" << table.a << ", " << table.b << "}";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const TableA2& table) {
  stream << "TableA2{" << table.a << ", " << table.b << "}";
  return stream;
}

}  // anonymouse namespace

int main(int /*argc*/, char** /*argv*/) {
  Serializer<StreamWriter<std::stringstream>> serializer;

  TableA0 t0{"foo"};
  auto status = serializer.Write(t0);
  if (!status) {
    std::cerr << "Failed to write t0: " << status.GetErrorMessage()
              << std::endl;
    return -status.error();
  }
  const std::string t0_data = serializer.writer().stream().str();
  serializer.writer().stream().str("");

  std::cout << "Wrote t0: " << t0 << std::endl;
  std::cout << "Serialized data: " << StringToHex(t0_data) << std::endl;
  std::cout << t0_data.size() << " bytes" << std::endl << std::endl;

  TableA1 t1{"foo", {1, 2, 3, 4}};
  status = serializer.Write(t1);
  if (!status) {
    std::cerr << "Failed to write t1: " << status.GetErrorMessage()
              << std::endl;
    return -status.error();
  }
  const std::string t1_data = serializer.writer().stream().str();
  serializer.writer().stream().str("");

  std::cout << "Wrote t1: " << t1 << std::endl;
  std::cout << "Serialized data: " << StringToHex(t1_data) << std::endl;
  std::cout << t1_data.size() << " bytes" << std::endl << std::endl;

  TableA2 t2{"foo", {}};
  status = serializer.Write(t2);
  if (!status) {
    std::cerr << "Failed to write t2: " << status.GetErrorMessage()
              << std::endl;
    return -status.error();
  }
  const std::string t2_data = serializer.writer().stream().str();
  serializer.writer().stream().str("");

  std::cout << "Wrote t2: " << t2 << std::endl;
  std::cout << "Serialized data: " << StringToHex(t2_data) << std::endl;
  std::cout << t2_data.size() << " bytes" << std::endl << std::endl;

  Deserializer<StreamReader<std::stringstream>> deserializer{};

  {
    deserializer.reader().stream().str(t0_data);

    TableA0 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t0_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t0_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t1_data);

    TableA0 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t1_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t1_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t2_data);

    TableA0 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t2_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t2_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t0_data);

    TableA1 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t0_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t0_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t1_data);

    TableA1 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t1_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t1_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t2_data);

    TableA1 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t2_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t2_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t0_data);

    TableA2 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t0_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t0_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t1_data);

    TableA2 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t1_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t1_data: " << table << std::endl;
  }

  {
    deserializer.reader().stream().str(t2_data);

    TableA2 table;
    status = deserializer.Read(&table);
    if (!status) {
      std::cerr << "Failed to read t2_data: " << status.GetErrorMessage()
                << std::endl;
      return -status.error();
    }
    std::cout << "Read t2_data: " << table << std::endl;
  }

  return 0;
}
