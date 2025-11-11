#pragma once
#include <list>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"
#include "NovaLLM/memory/buffer_define.h"
#include "NovaLLM/utils/template.h"
#include "NovaLLM/utils/macros.h"

namespace nova_llm {

struct Size {
 private:
  uint64_t b_ = 0;
  uint64_t kb_ = 0;
  uint64_t mb_ = 0;
  uint64_t gb_ = 0;
  uint64_t total_bytes_ = 0;
  const uint64_t ratio_ = 1024;

  void convert_in_units(uint64_t bytes) {
    auto down_ratio = std::pow(ratio_, 3);

    gb_ = bytes / down_ratio;
    bytes -= gb_ * down_ratio;
    down_ratio /= ratio_;

    mb_ = bytes / down_ratio;
    bytes -= mb_ * down_ratio;
    down_ratio /= ratio_;

    kb_ = bytes / down_ratio;
    bytes -= kb_ * down_ratio;

    b_ = bytes;
  }

 public:
  Size() = default;

  explicit Size(uint64_t sz) {
    total_bytes_ = sz;
    convert_in_units(total_bytes_);
  }

  Size(uint64_t b, uint64_t kb, uint64_t mb, uint64_t gb) {
    b_ = b;
    kb_ = kb;
    mb_ = mb;
    gb_ = gb;

    if (ratio_ < b_) {
      auto kb_cnt = b_ / ratio_;
      b_ -= kb_cnt * ratio_;
      kb_ += kb_cnt;
    }

    if (ratio_ < kb_) {
      auto mb_cnt = kb_ / ratio_;
      kb_ -= mb_cnt * ratio_;
      mb_ += mb_cnt;
    }

    if (ratio_ < mb_) {
      auto gb_cnt = mb_ / ratio_;
      mb_ -= gb_cnt * ratio_;
      gb_ += gb_cnt;
    }

    total_bytes_ = b_ + kb_ * ratio_ + mb_ * ratio_ * ratio_ + gb_ * ratio_ * ratio_ * ratio_;
  }

  Size(const Size& rhs) {
    total_bytes_ = rhs.totalBytes();
    convert_in_units(total_bytes_);
  }

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
  bool operator()(const Size& lhs, const Size& rhs) const {
    return lhs.totalBytes() == rhs.totalBytes();
  }
};

struct Block {
  using DataPtr = uint8_t*;
  //using BlockPtr = Block*;
  DataPtr data = nullptr;
  uint64_t size = 0;
  int32_t ref_cnt = 0;

  bool isValid() const {
    // return data != nullptr && (prev != nullptr || next != nullptr) && 0 != size;
    return data != nullptr && 0 != size;
  }
};

using BlockPtr = Block*;

class DefaultSizeLevelStrategy {
 public:
  static std::vector<Size> byteSizes() ;

  static std::vector<Size> kiloByteSizes() ;

  static std::vector<Size> megaByteSizes() ;

  static std::vector<Size> gigaByteSizes() ;
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
  struct Config {
    DeviceType device_type;
    std::vector<Size> size_levels;  // ensure that levels are in ascending order
    Size size_limit {0, 0, 0, 8};   // Memory in buffer hub cannot exceed this limit
    float warning_level =
        0.95;  // Be cautious when memory in buffer hub exceeds size_limit*warning_level
    IAllocatorSharedPtr allocator;
  };

  struct Level {
   public:
    BlockPtr fetchOneFreeBlock();

    void putOneBlock(const BlockPtr& block_ptr);

    void refill(const Size& sz);

    ~Level();

    uint32_t index = -1;
    Size level_size {static_cast<uint64_t>(0)};  // each block size at this level

    //using BlockPtr = Block*;
    std::list<BlockPtr> block_list;
    using BlockIterator = std::list<BlockPtr>::iterator;
    std::unordered_map<Block::DataPtr, BlockIterator> free_map;
    std::unordered_map<Block::DataPtr, BlockIterator> busy_map;
    BufferHub* hub;
  };

  class Builder {
   public:
    static BufferHub* build(const Config& config);

    static void destroy(BufferHub** hub);
  };

  void initConfig(const Config& config);

  BlockPtr getBlock(const Size& sz);

  void putBlock(const BlockPtr& block);

  void putBlockFromBuffer(const Buffer& buffer);

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

  BufferHub() = default;

  std::unordered_map<Size, Level, SizeHash, SizeEqual> buffers_;
  DeviceType device_type_;
  std::vector<Size> size_levels_;  // ensure that levels are in ascending order
  Size size_limit_ {0, 0, 0, 4};   // Memory in buffer hub cannot exceed this limit

  // Be cautious when memory in buffer hub exceeds size_limit*warning_level
  float warning_level_ = 0.95;

  IAllocatorSharedPtr allocator_;
};

}  // namespace nova_llm
