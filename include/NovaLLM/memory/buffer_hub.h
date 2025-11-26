#pragma once

// Disable C4251 warning on Windows (DLL interface for STL containers)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

#include <cmath>
#include <list>
#include <memory>
#include <mutex>
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

struct NOVA_LLM_API Size {
 private:
  uint64_t bytes_ = 0;

 public:
  Size() = default;

  explicit Size(uint64_t bytes) : bytes_(bytes) {}

  Size(const Size& rhs) = default;

  Size& operator=(const Size& rhs) = default;

  [[nodiscard]] uint64_t totalBytes() const { return bytes_; }

  bool operator==(const Size& rhs) const { return bytes_ == rhs.bytes_; }

  [[nodiscard]] bool isValid() const { return bytes_ != 0; }
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

// BlockPtr for owning pointers (used in collections)
using BlockPtr = std::unique_ptr<Block>;
// Raw non-owning pointer for temporary access
using BlockRawPtr = Block*;

class LevelAssignStrategy {
 public:
  virtual std::vector<Size> assignLevels();
};

class NOVA_LLM_API BufferHubConfig {
 public:
  BufferHubConfig(DeviceType device_type, IAllocatorSharedPtr allocator, Size size_limit=Size(4UL*1024*1024*1024), LevelAssignStrategy strategy = LevelAssignStrategy(), float warning_level = 0.95f)
      : device_type_(device_type),
        size_limit_(size_limit),
        warning_level_(warning_level),
        allocator_(allocator),
        level_assign_strategy_(strategy) {
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
class NOVA_LLM_API BufferHubLevel {
 public:
  // Default constructor required for unordered_map
  BufferHubLevel() = default;

  // Move constructor and assignment for unique_ptr compatibility
  BufferHubLevel(BufferHubLevel&&) = default;
  BufferHubLevel& operator=(BufferHubLevel&&) = default;

  // Copy operations deleted to prevent unique_ptr copying
  BufferHubLevel(const BufferHubLevel&) = delete;
  BufferHubLevel& operator=(const BufferHubLevel&) = delete;

  void initialize(uint32_t index, const Size& block_size, BufferHub* hub);

  // Returns non-owning pointer since pool retains ownership
  BlockRawPtr fetchOneFreeBlock();

  // Accepts non-owning pointer for blocks already in the pool
  void putOneBlock(BlockRawPtr block_ptr);
  
  // Attempts to put a block back by its data pointer. Returns true if successful.
  bool tryPutBlock(Block::DataPtr data);

  size_t busyBlockCount() const;

  size_t totalBlocks() const;
  
  ~BufferHubLevel();

 private:
  void refill(const Size& sz);

  uint32_t index_ = static_cast<uint32_t>(-1); // level index in buffer hub
  Size block_size_ {static_cast<uint64_t>(0)}; // each block size at this level
  uint32_t expand_factor_ = 2;
  
  std::list<BlockPtr> block_list_; // Owns the blocks
  using BlockIterator = std::list<BlockPtr>::iterator;
  
  std::unordered_map<Block::DataPtr, BlockIterator> free_map_;
  std::unordered_map<Block::DataPtr, BlockIterator> busy_map_;
  
  BufferHub* hub_ = nullptr;
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

  // Returns non-owning pointer to block managed by pool
  BlockRawPtr getBlock(const Size& sz);

  // Accepts non-owning pointer to block managed by pool
  void putBlock(BlockRawPtr block);

  // Return a buffer to the pool and clear the Buffer to avoid dangling pointers.
  void putBlockFromBuffer(Buffer& buffer);

  void addSizeLevel(uint32_t index, const Size& level_sz);

  void eraseSizeLevel(const Size& level_sz);

 private:
  Block::DataPtr allocData(uint64_t sz);
  void deallocData(Block::DataPtr& data_ptr);

  // Creates a new block with ownership
  BlockPtr allocBlock();
  void deallocateBlock(BlockPtr block);

  // Creates and initializes a new block
  BlockPtr setUpBlock(const Size& sz);

  // Cleans up and destroys a block
  void tearDownBlock(BlockPtr block);

  [[nodiscard]] Size gradeLevel(const Size& sz) const;

  BufferHub();

  ~BufferHub();

  // Thread safety: protects all mutable state
  mutable std::mutex mutex_;

  std::unordered_map<Size, std::unique_ptr<BufferHubLevel>, SizeHash, SizeEqual> buffers_;

  DeviceType device_type_;

  std::vector<Size> size_levels_;  // ensure that levels are in ascending order

  Size size_limit_;  // Memory in buffer hub cannot exceed this limit

  float warning_level_ = 0.95f; // Be cautious when memory in buffer hub exceeds size_limit*warning_level

  IAllocatorSharedPtr allocator_;

};

}  // namespace nova_llm

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
