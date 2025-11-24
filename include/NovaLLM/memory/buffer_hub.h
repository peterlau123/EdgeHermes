#pragma once
#include <cmath>
#include <list>
#include <unordered_map>
#include <vector>

#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"
#include "NovaLLM/memory/buffer_define.h"
#include "NovaLLM/utils/macros.h"
#include "NovaLLM/utils/template.h"

namespace nova_llm {

// Forward declaration
class BufferHub;

struct Size {
 private:
  uint64_t b_ = 0;
  uint64_t kb_ = 0;
  uint64_t mb_ = 0;
  uint64_t gb_ = 0;
  uint64_t total_bytes_ = 0;
  const uint64_t ratio_ = 1 << 10;

  void convert_in_units(uint64_t bytes);

 public:
  Size() = default;

  explicit Size(uint64_t sz) {
    total_bytes_ = sz;
    convert_in_units(total_bytes_);
  }

  Size(uint64_t b, uint64_t kb, uint64_t mb, uint64_t gb);

  Size(const Size& rhs) {
    total_bytes_ = rhs.totalBytes();
    convert_in_units(total_bytes_);
  }

  uint64_t gb() const { return this->gb_; }

  uint64_t mb() const { return this->mb_; }

  uint64_t kb() const { return this->kb_; }

  uint64_t b() const { return this->b_; }

  Size& operator=(const Size& rhs) {
    total_bytes_ = rhs.totalBytes();
    convert_in_units(total_bytes_);
    return *this;
  }

  [[nodiscard]] uint64_t totalBytes() const { return total_bytes_; }

  bool operator==(const Size& rhs) const { return totalBytes() == rhs.totalBytes(); }

  [[nodiscard]] bool isValid() const { return totalBytes() != 0; }
};

struct SizeHash {
  std::size_t operator()(const Size& s) const { return std::hash<uint64_t>()(s.totalBytes()); }
};

struct SizeEqual {
  bool operator()(const Size& lhs, const Size& rhs) const { return lhs.totalBytes() == rhs.totalBytes(); }
};

struct Block {
  using DataPtr = uint8_t*;
  DataPtr data = nullptr;
  uint64_t size = 0;
  int32_t ref_cnt = 0;

  bool isValid() const { return data != nullptr && 0 != size; }
};

using BlockPtr = Block*;

class LevelAssignStrategy {
 public:
  virtual std::vector<Size> assignLevels();
};

class BufferHubConfig {
 public:
  BufferHubConfig(DeviceType device_type, IAllocatorSharedPtr allocator, Size size_limit, LevelAssignStrategy strategy = LevelAssignStrategy(), float warning_level = 0.95)
      : device_type_(device_type), allocator_(allocator), size_limit_(size_limit), warning_level_(warning_level), level_assign_strategy_(strategy) {
    size_levels_ = strategy.assignLevels();
  };

  void setLevelAssignStrategy(LevelAssignStrategy strategy) { size_levels_ = strategy.assignLevels(); }

  void setWarningLevel(float warning_level) { warning_level_ = warning_level; }

  DeviceType deviceType() const { return device_type_; }

  const std::vector<Size>& sizeLevels() const { return size_levels_; }

  Size sizeLimit() const { return size_limit_; }

  float warningLevel() const { return warning_level_; }

  IAllocatorSharedPtr allocator() const { return allocator_; }

 private:
  DeviceType device_type_;
  std::vector<Size> size_levels_;  // ensure that levels are in ascending order
  Size size_limit_;                // Memory in buffer hub cannot exceed this limit
  float warning_level_;            // Be cautious when memory in buffer hub exceeds size_limit*warning_level
  IAllocatorSharedPtr allocator_;
  LevelAssignStrategy level_assign_strategy_;
};

class BufferHub;
/**
 * @brief Buffers at the specified size level
 *
 */
class BufferHubLevel {
 public:
  BlockPtr fetchOneFreeBlock();
  void putOneBlock(const BlockPtr& block_ptr);
  void refill(const Size& sz);
  ~BufferHubLevel();
  uint32_t index = -1;                         // level index in buffer hub
  Size block_size {static_cast<uint64_t>(0)};  // each block size at this level
  uint32_t expand_factor = 2;
  std::list<BlockPtr> block_list;
  using BlockIterator = std::list<BlockPtr>::iterator;
  std::unordered_map<Block::DataPtr, BlockIterator> free_map;
  std::unordered_map<Block::DataPtr, BlockIterator> busy_map;
  BufferHub* hub;
};

/*
 * @Brief: Memory block hub
 * Initially we use segregated free list to manage memory block. It has the following features:
 * 1) each level is independent
 * 2) coalesce and split is not allowed between levels
 * 3) for levels below 1kb, we allocate 1kb for each level when no free block at this level
 *    for levels below 1mb, we allocate 1mb for each level
 *    for levels below 1gb, we allocate 1gb for each level
 *    for levels above 1gb, we allocate 4gb for the current level
 * */
class NOVA_LLM_API BufferHub {
 public:
  friend class BufferHubConfig;
  friend class BufferHubLevel;

  class Builder {
   public:
    NOVA_LLM_API static BufferHub* build(const BufferHubConfig& config);

    NOVA_LLM_API static void destroy(BufferHub** hub);
  };

  void initConfig(const BufferHubConfig& config);

  BlockPtr getBlock(const Size& sz);

  void putBlock(const BlockPtr& block);

  // Return a buffer to the pool and clear the Buffer to avoid dangling pointers.
  void putBlockFromBuffer(Buffer& buffer);

 private:
  Block::DataPtr allocData(uint64_t sz);
  void deallocData(Block::DataPtr& data_ptr);

  BlockPtr allocBlock();
  void deallocateBlock(BlockPtr& block_ptr);

  BlockPtr setUpBlock(const Size& sz);  // alloc and init block

  void tearDownBlock(BlockPtr& block);

  void addSizeLevel(uint32_t index, const Size& level_sz);

  void eraseSizeLevel(const Size& level_sz);

  [[nodiscard]] Size gradeLevel(const Size& sz) const;

  BufferHub();

  ~BufferHub();

  std::unordered_map<Size, BufferHubLevel, SizeHash, SizeEqual> buffers_;

  DeviceType device_type_;

  std::vector<Size> size_levels_;  // ensure that levels are in ascending order

  Size size_limit_ {0, 0, 0, 4};  // Memory in buffer hub cannot exceed this limit

  float warning_level_ = 0.95;  // Be cautious when memory in buffer hub exceeds size_limit*warning_level

  IAllocatorSharedPtr allocator_;
};

}  // namespace nova_llm
