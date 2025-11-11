#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <atomic>
#include "../common/device.h"
#include "../common/dtype.h"
#include "NovaLLM/utils/macros.h"

namespace nova_llm {

/**
 * @brief 张量类，用于表示和操作多维数组数据
 *
 * @details 支持多种数据类型(如INT8、FLOAT32等)和设备类型(CPU/GPU)，
 *          提供基本的张量运算操作，包括乘法和加法。
 */
class NOVA_LLM_API Tensor {
 public:
  /**
   * @brief 数据来源枚举
   */
  enum class DataSourceType {
    UNKNOWN = -1,
    MANUAL,  // Marked data managed by user including allocation and de-allocation
    AUTO     // Marked data managed by framework
  };

  /**
   * @brief 默认删除器
   */
  struct DefaultDeletor {
    void operator()(void** data) {}
  };

  using Deleter = std::function<void(void**)>;

  /**
   * @brief 默认构造函数
   */
  Tensor();

  /**
   * @brief 构造指定维度和类型的张量
   *
   * @param dims 张量的维度数组
   * @param dtype 数据类型
   * @param device 设备类型，默认为CPU
   */
  Tensor(const std::vector<uint32_t>& dims, DataType dtype, DeviceType device = DeviceType::CPU);

  /**
   * @brief 从现有数据构造张量
   *
   * @param data 指向数据的指针
   * @param dims 张量的维度数组
   * @param dtype 数据类型
   * @param device 设备类型
   * @param deleter 自定义删除器，默认使用DefaultDeletor
   */
  Tensor(const void* data,
         const std::vector<uint32_t>& dims,
         DataType dtype,
         DeviceType device,
         Deleter deleter = DefaultDeletor());

  /**
   * @brief 拷贝构造函数
   *
   * @param other 要拷贝的张量
   */
  Tensor(const Tensor& other);

  /**
   * @brief 赋值运算符
   *
   * @param other 要赋值的张量
   * @return Tensor& 返回自身引用
   */
  Tensor& operator=(const Tensor& other);

  /**
   * @brief 张量乘法运算
   *
   * @param rhs 右操作数
   * @return Tensor& 返回结果张量的引用
   */
  Tensor& operator*(const Tensor& rhs);

  /**
   * @brief 张量加法运算
   *
   * @param rhs 右操作数
   * @return Tensor& 返回结果张量的引用
   */
  Tensor& operator+(const Tensor& rhs);

  std::vector<uint32_t> dims() const { return dims_; }

  /**
   * @brief 获取指定维度的大小
   *
   * @param idx 维度索引
   * @return int 返回该维度的大小
   */
  uint32_t dimAt(uint32_t idx) const {
    ASSERT(idx < dims_.size(), "idx out of range");
    return dims_[idx];
  }

  /**
   * @brief 获取张量的元素总数
   *
   * @return int 元素总数
   */
  int totalElements() const { return ele_cnt_; }

  DataType dtype() const { return m_dtype_; }

  DeviceType device() const { return m_device_; }

  uint8_t* data() const { return reinterpret_cast<uint8_t*>(data_); }

  uint64_t capacity() const { return capacity_; }

  DataSourceType dataFrom() const { return m_data_source_; }

  uint32_t refCnt() const { return ref_cnt_?ref_cnt_->load():0; }

  Deleter deleter() const { return m_deleter_; }

  /**
   * @brief 析构函数
   */
  ~Tensor();

 private:
  std::atomic<uint32_t>* allocRefCnt() { return new std::atomic<uint32_t>; }

  void deallocRefCnt() {
    if (this->ref_cnt_) {
      delete this->ref_cnt_;
      this->ref_cnt_ = nullptr;
    }
  }

  std::vector<uint32_t> dims_;  ///< 张量的维度数组
  uint32_t ele_cnt_ {0};        ///< 元素总数
  void* data_ {nullptr};        ///< 数据缓冲区
  uint64_t capacity_ {0};  ///< 数据缓冲区大小，单位为字节，大于等于size_*sizeof(m_dtype_)
  DataSourceType m_data_source_ {DataSourceType::AUTO};
  DataType m_dtype_ {DataType::UNKNOWN};       ///< 数据类型
  DeviceType m_device_ {DeviceType::UNKNOWN};  ///< 设备类型
  std::atomic<uint32_t>* ref_cnt_ {nullptr};   ///< 引用计数,TODO:important!
  Deleter m_deleter_ = DefaultDeletor();       ///< 自定义删除器
};

}  // namespace nova_llm