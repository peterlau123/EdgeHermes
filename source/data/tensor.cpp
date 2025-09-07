#include "NovaLLM/data/tensor.h"

#include <utility>

#include "NovaLLM/memory/buffer_manager.h"
#include "NovaLLM/utils/macros.h"

namespace nova_llm {

uint64_t getByteSize(DataType dtype) {
  switch (dtype) {
    case DataType::INT8:
    case DataType::UINT8:
      return sizeof(uint8_t);
    case DataType::INT16:
    case DataType::UINT16:
      return sizeof(uint16_t);
    case DataType::INT32:
    case DataType::UINT32:
      return sizeof(uint32_t);
    case DataType::FLOAT32:
      return sizeof(float);
    case DataType::FLOAT64:
      return sizeof(double);
    case DataType::BOOL:
      return sizeof(bool);
    default:
      ASSERT(false, "Unsupported data type");
      return 0;
  }
}

namespace {

uint32_t checkDims(const std::vector<uint32_t>& dims) {
  ASSERT(0 != dims.size(), "dim size cannot be zero!");
  uint32_t cnt = 1;
  for (auto dim : dims) {
    ASSERT(0 < dim, "dimension cannot be below zero!");
    cnt *= dim;
  }
  return cnt;
}


}  // namespace

Tensor::Tensor()
    : dims_()
    , ele_cnt_(0)
    , data_ {nullptr}
    , capacity_(0)
    , m_data_source_(DataSourceType::AUTO)
    , m_dtype_(DataType::UNKNOWN)
    , m_device_(DeviceType::UNKNOWN)
    , ref_cnt_ {nullptr}
    , m_deleter_(DefaultDeletor()) {}

Tensor::Tensor(const std::vector<uint32_t>& dims, DataType dtype, DeviceType device)
    : dims_(dims)
    , ele_cnt_(0)
    , data_(nullptr)
    , capacity_(0)
    , m_data_source_(DataSourceType::AUTO)
    , m_dtype_(dtype)
    , m_device_(device)
    , ref_cnt_(0)
    , m_deleter_(DefaultDeletor()) {
  // Check if the data type is valid
  ASSERT(dtype >= DataType::INT8 && dtype < DataType::TOTAL, "Invalid data type");
  // Check if the device type is valid
  ASSERT(device >= DeviceType::CPU && device <= DeviceType::METAL, "Invalid device type");
  // Calculate the total elements of the tensor
  ele_cnt_ = checkDims(dims_);
  auto& buffer_manager = BufferManager::Builder::getInstance();
  Buffer buffer = buffer_manager.fetch(ele_cnt_ * sizeof(m_dtype_), m_device_);
  this->data_ = buffer.data;
  this->capacity_ = buffer.size;
  m_deleter_ = [&](void** data) {
    Buffer buffer {static_cast<decltype(std::declval<Buffer>().data)>(*data),
                   static_cast<decltype(std::declval<Buffer>().size)>(capacity_),
                   m_device_};
    buffer_manager.put(buffer);
    *data = nullptr;
  };
  ref_cnt_ = allocRefCnt();
  *ref_cnt_ = 1;
}

Tensor::Tensor(const void* data,
               const std::vector<uint32_t>& dims,
               DataType dtype,
               DeviceType device,
               Deleter deleter) {
  ASSERT(nullptr != data, "data cannot be null!");
  ASSERT(DataType::UNKNOWN < dtype, "data type must be specified!");
  ASSERT(DeviceType::UNKNOWN < device, "device type must be specified!");

  ele_cnt_ = checkDims(dims);
  dims_ = dims;
  data_ = const_cast<void*>(data);
  capacity_ = ele_cnt_ * sizeof(dtype);
  m_data_source_ = DataSourceType::MANUAL;
  m_dtype_ = dtype;
  m_device_ = device;
  m_deleter_ = std::move(deleter);
  ref_cnt_ = new std::atomic<uint32_t>;
  *ref_cnt_ = 1;
}

Tensor::Tensor(const Tensor& other)
    : dims_(other.dims())
    , ele_cnt_(other.totalElements())
    , data_(other.data())
    , capacity_(other.capacity())
    , m_data_source_(other.dataFrom())
    , m_dtype_(other.dtype())
    , m_device_(other.device())
    , ref_cnt_(other.ref_cnt_)
    , m_deleter_(other.deleter()) {
  if (ref_cnt_) {
    this->ref_cnt_->fetch_add(1);
  }
}

Tensor& Tensor::operator=(const Tensor& other) {
  if (this->data_ != static_cast<decltype(data_)>(other.data())) {
    this->~Tensor();
    dims_ = other.dims();
    ele_cnt_ = other.totalElements();
    data_ = other.data();
    capacity_ = other.capacity();
    m_data_source_ = other.dataFrom();
    m_dtype_ = other.dtype();
    m_device_ = other.device();
    ref_cnt_ = other.ref_cnt_;//TODO:notice here
    m_deleter_ = other.deleter();
  }
  return *this;
}

Tensor::~Tensor() {
  if (nullptr != ref_cnt_ && 0 == *ref_cnt_) {
    m_deleter_(&data_);
    this->deallocRefCnt();
  }
}

Tensor& Tensor::operator*(const Tensor& rhs) {
  // TODO
  return *this;
}

Tensor& Tensor::operator+(const Tensor& rhs) {
  // TODO
  return *this;
}

}  // namespace nova_llm
