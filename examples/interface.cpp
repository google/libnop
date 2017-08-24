#include <unistd.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nop/rpc/interface.h>
#include <nop/serializer.h>
#include <nop/types/result.h>
#include <nop/types/variant.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"

using nop::BindInterface;
using nop::Deserializer;
using nop::ErrorStatus;
using nop::Interface;
using nop::InterfaceBindings;
using nop::Result;
using nop::Serializer;
using nop::Status;
using nop::StreamReader;
using nop::StreamWriter;
using nop::Variant;

namespace {

// A simple class to represent phone numbers. This is not intended for
// production. In the real world phone numbers are much more complicated.
class PhoneNumber {
 public:
  enum class Type {
    Work,
    Home,
    Cell,
    Other,
  };

  PhoneNumber() = default;
  PhoneNumber(const std::string& number, Type type = Type::Other)
      : number_{number}, type_{type} {}
  PhoneNumber(const PhoneNumber&) = default;
  PhoneNumber& operator=(const PhoneNumber&) = default;

  std::string number() const { return number_; }
  Type type() const { return type_; }

  // Prints the phone number type to the debug stream as a string.
  friend std::ostream& operator<<(std::ostream& stream, Type type) {
    static const char* kTypeLabels[] = {
            [static_cast<int>(Type::Work)] = "Work",
            [static_cast<int>(Type::Home)] = "Home",
            [static_cast<int>(Type::Cell)] = "Cell",
            [static_cast<int>(Type::Other)] = "Other",
    };

    switch (type) {
      case Type::Work:
      case Type::Home:
      case Type::Cell:
      case Type::Other:
        stream << kTypeLabels[static_cast<int>(type)];
        break;
      default:
        stream << "Unknown";
        break;
    }

    return stream;
  }

  // Prints the phone number to the debug stream.
  friend std::ostream& operator<<(std::ostream& stream,
                                  const PhoneNumber& phone_number) {
    stream << "PhoneNumber{" << phone_number.number_ << ", "
           << phone_number.type_ << "}";
    return stream;
  }

 private:
  std::string number_;
  Type type_{Type::Other};

  NOP_MEMBERS(PhoneNumber, number_, type_);
};

// A simple customer record with basic identity and contact information,
// including a variable list of phone numbers.
class Customer {
 public:
  Customer() = default;
  Customer(const std::string& first_name, const std::string& last_name,
           const std::string& middle_name, const std::string& address,
           std::vector<PhoneNumber> phone_numbers)
      : last_name_{last_name},
        first_name_{first_name},
        middle_name_{middle_name},
        address_{address},
        phone_numbers_{phone_numbers} {}
  Customer(const Customer&) = default;
  Customer& operator=(const Customer&) = default;

  std::string last_name() const { return last_name_; }
  std::string first_name() const { return first_name_; }
  std::string middle_name() const { return middle_name_; }
  std::string address() const { return address_; }
  const std::vector<PhoneNumber>& phone_numbers() const {
    return phone_numbers_;
  }

  // In this example customers are uniquely defined by their name and address,
  // which is not realisitc in the real world.
  bool operator==(const Customer& other) const {
    return std::tie(last_name_, first_name_, middle_name_, address_) ==
           std::tie(other.last_name_, other.first_name_, other.middle_name_,
                    other.address_);
  }

  // Prints the customer to the debug stream.
  friend std::ostream& operator<<(std::ostream& stream,
                                  const Customer& customer) {
    stream << "Customer{" << customer.last_name_ << ", " << customer.first_name_
           << ", " << customer.middle_name_ << ", " << customer.address_ << ", "
           << customer.phone_numbers_ << "}";
    return stream;
  }

 private:
  std::string last_name_;
  std::string first_name_;
  std::string middle_name_;
  std::string address_;
  std::vector<PhoneNumber> phone_numbers_;

  NOP_MEMBERS(Customer, last_name_, first_name_, middle_name_, address_,
              phone_numbers_);
};

// Type for unique customer ids.
using CustomerId = std::uint64_t;

// Enumeration of error values to return from methods.
enum class CustomerError {
  // Required by Result<>.
  None,

  // Application errors.
  CustomerExists,
  InvalidCustomerId,

  // Transport errors.
  IoError,
};

template <typename T>
struct Return : public Result<CustomerError, T> {
  using Base = Result<CustomerError, T>;
  using Base::Base;

  std::string GetErrorMessage() const {
    switch (this->error()) {
      case CustomerError::None:
        return "No Error";
      case CustomerError::CustomerExists:
        return "Customer Exists";
      case CustomerError::InvalidCustomerId:
        return "Invalid Customer ID";
      case CustomerError::IoError:
        return "IO Error";
      default:
        return "Unknown Error";
    }
  }
};

// Interface used by client and service to communicate about customers.
struct CustomerInterface : public Interface<CustomerInterface> {
  NOP_INTERFACE("io.github.eieio.examples.interface.Customer");

  NOP_METHOD(Add, Return<CustomerId>(const Customer&));
  NOP_METHOD(Remove, CustomerError(CustomerId));
  NOP_METHOD(Update, CustomerError(CustomerId, const Customer&));
  NOP_METHOD(Get, Return<Customer>(CustomerId));

  NOP_API(Add, Remove, Update, Get);
};

// This example uses pipes to connect the service and client. Define the reader
// and writer types to support pipe communication.
using Reader = StreamReader<FdInputStream>;
using Writer = StreamWriter<FdOutputStream>;

class CustomerService {
 public:
  CustomerService(std::unique_ptr<Reader> reader,
                  std::unique_ptr<Writer> writer)
      : serializer_{std::move(writer)}, deserializer_{std::move(reader)} {}

  ~CustomerService() { Quit(); }

  void HandleMessages() {
    // Build a dispatch table with the handlers for each method.
    auto dispatch = BindInterface(
        CustomerInterface::Add::Bind(this, &CustomerService::OnAdd),
        CustomerInterface::Remove::Bind(this, &CustomerService::OnRemove),
        CustomerInterface::Update::Bind(this, &CustomerService::OnUpdate),
        CustomerInterface::Get::Bind(this, &CustomerService::OnGet));

    while (!quit_) {
      auto status = dispatch.Dispatch(&deserializer_, &serializer_);
      if (!status && !quit_) {
        std::cerr << "Failed to handle message: " << status.GetErrorMessage()
                  << std::endl;
      }
    }
  }

  void Quit() { quit_ = true; }

 private:
  Return<CustomerId> OnAdd(const Customer& customer) {
    for (const auto& search : customers_) {
      if (search.second == customer)
        return CustomerError::CustomerExists;
    }

    CustomerId customer_id = customer_id_counter_++;
    customers_[customer_id] = customer;

    return customer_id;
  }

  CustomerError OnRemove(CustomerId customer_id) {
    auto search = customers_.find(customer_id);
    if (search == customers_.end())
      return CustomerError::InvalidCustomerId;

    customers_.erase(search);
    return CustomerError::None;
  }

  CustomerError OnUpdate(CustomerId customer_id, const Customer& customer) {
    auto search = customers_.find(customer_id);
    if (search == customers_.end())
      return CustomerError::InvalidCustomerId;

    customers_[customer_id] = customer;
    return CustomerError::None;
  }

  Return<Customer> OnGet(CustomerId customer_id) {
    auto search = customers_.find(customer_id);
    if (search == customers_.end())
      return CustomerError::InvalidCustomerId;
    else
      return search->second;
  }

  Serializer<std::unique_ptr<Writer>> serializer_;
  Deserializer<std::unique_ptr<Reader>> deserializer_;

  std::unordered_map<CustomerId, Customer> customers_;
  CustomerId customer_id_counter_{0};
  std::atomic<bool> quit_{false};

  CustomerService(const CustomerService&) = delete;
  void operator=(const CustomerService&) = delete;
};

class CustomerClient {
 public:
  CustomerClient(std::unique_ptr<Reader> reader, std::unique_ptr<Writer> writer)
      : serializer_{std::move(writer)}, deserializer_{std::move(reader)} {}

  Return<CustomerId> Add(const Customer& customer) {
    auto status = CustomerInterface::Add::InvokeAndReturn(
        &serializer_, &deserializer_, customer);
    if (!status)
      return CustomerError::IoError;

    return status.take();
  }

  Return<Customer> Get(CustomerId customer_id) {
    auto status = CustomerInterface::Get::InvokeAndReturn(
        &serializer_, &deserializer_, customer_id);
    if (!status)
      return CustomerError::IoError;

    return status.take();
  }

 private:
  Serializer<std::unique_ptr<Writer>> serializer_;
  Deserializer<std::unique_ptr<Reader>> deserializer_;
};

// Builds a reader/writer pair connected by a pipe.
Status<std::pair<std::unique_ptr<Reader>, std::unique_ptr<Writer>>> MakePipe() {
  int pipe_fds[2];
  const int ret = pipe(pipe_fds);
  if (ret < 0)
    return ErrorStatus::SystemError;

  return {{std::make_unique<Reader>(pipe_fds[0]),
           std::make_unique<Writer>(pipe_fds[1])}};
}

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  // Build the client-to-service streams connected by a pipe.
  auto pipe_status = MakePipe();
  if (!pipe_status) {
    std::cerr << "Failed to build pipe: " << pipe_status.GetErrorMessage()
              << std::endl;
    return -1;
  }
  std::unique_ptr<Reader> service_reader;
  std::unique_ptr<Writer> client_writer;
  std::tie(service_reader, client_writer) = pipe_status.take();

  // Build the service-to-client streams connected by a pipe.
  pipe_status = MakePipe();
  if (!pipe_status) {
    std::cerr << "Failed to build pipe: " << pipe_status.GetErrorMessage()
              << std::endl;
    return -1;
  }
  std::unique_ptr<Reader> client_reader;
  std::unique_ptr<Writer> service_writer;
  std::tie(client_reader, service_writer) = pipe_status.take();

  // Build the service and client with the connecting pipes.
  CustomerService service{std::move(service_reader), std::move(service_writer)};
  CustomerClient client{std::move(client_reader), std::move(client_writer)};

  // Start the service message handler in a thread.
  std::thread service_thread{&CustomerService::HandleMessages, &service};
  service_thread.detach();

  // Exercise the customer API.
  Customer customer_a{"John",
                      "David",
                      "Doe",
                      "100 First St., Somewhere, CA 12345",
                      {{"408-555-5555", PhoneNumber::Type::Home}}};

  Customer customer_b{"Ronald",
                      "Trevor",
                      "Johnson",
                      "200 Second St., Somewhere, CA 12345",
                      {{"980-555-5555", PhoneNumber::Type::Cell}}};

  std::cout << "Adding customer_a: " << customer_a << std::endl;
  auto status_add = client.Add(customer_a);
  if (!status_add) {
    std::cerr << "Failed to add customer: " << status_add.GetErrorMessage()
              << std::endl;
    return -1;
  }
  CustomerId customer_id_a = status_add.get();
  std::cout << "Added customer: id=" << customer_id_a << std::endl << std::endl;

  std::cout << "Adding customer_b: " << customer_b << std::endl;
  status_add = client.Add(customer_b);
  if (!status_add) {
    std::cerr << "Failed to add customer: " << status_add.GetErrorMessage()
              << std::endl;
    return -1;
  }
  CustomerId customer_id_b = status_add.get();
  std::cout << "Added customer: id=" << customer_id_b << std::endl << std::endl;

  std::cout << "Adding customer_a: " << customer_a << std::endl;
  status_add = client.Add(customer_a);
  if (!status_add) {
    std::cerr << "Failed to add customer: " << status_add.GetErrorMessage()
              << std::endl;
  }

  std::cout << std::endl;
  std::cout << "Fetching customer id=" << customer_id_a << std::endl;

  auto status_get = client.Get(customer_id_a);
  if (!status_get) {
    std::cerr << "Failed to get customer for id=" << customer_id_a << ":"
              << status_get.GetErrorMessage() << std::endl;
    return -1;
  }
  std::cout << "Customer "
            << (status_get.get() == customer_a ? "matches" : "does not match")
            << " customer_a" << std::endl;

  return 0;
}
