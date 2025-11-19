#include "NovaLLM/memory/buffer_hub.h"

#include <algorithm>

#include "NovaLLM/utils/log.h"

namespace nova_llm {

void Size::convert_in_units(uint64_t bytes) {
  auto down_ratio = ratio_ * ratio_ * ratio_;  // std::pow(ratio_, 3);

  // number of gb units
  gb_ = bytes / down_ratio;
  bytes -= gb_ * down_ratio;
  down_ratio /= ratio_;

  // number of mb units
  mb_ = bytes / down_ratio;
  bytes -= mb_ * down_ratio;
  down_ratio /= ratio_;

  // number of kb units
  kb_ = bytes / down_ratio;
  bytes -= kb_ * down_ratio;

  b_ = bytes;
}

Size::Size(uint64_t b, uint64_t kb, uint64_t mb, uint64_t gb) {
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

namespace {
class DefaultSizeLevelStrategy {
 public:
  static std::vector<Size> byteSizes();

  static std::vector<Size> kiloByteSizes();

  static std::vector<Size> megaByteSizes();

  static std::vector<Size> gigaByteSizes();
};

std::vector<Size> DefaultSizeLevelStrategy::byteSizes() {
  std::vector<Size> ret;
  uint32_t base = 64;
  uint32_t ratio = 2;
  for (uint64_t i = base; i < 1024;) {
    ret.push_back(Size(i, 0, 0, 0));
    i *= ratio;
  }
  return ret;
}

std::vector<Size> DefaultSizeLevelStrategy::kiloByteSizes() {
  std::vector<Size> ret;
  uint32_t base = 4;
  uint32_t ratio = 2;
  for (uint64_t i = base; i < 1024;) {
    ret.push_back(Size(0, i, 0, 0));
    i *= ratio;
  }
  return ret;
}

std::vector<Size> DefaultSizeLevelStrategy::megaByteSizes() {
  std::vector<Size> ret;
  uint32_t base = 2;
  uint32_t ratio = 2;
  for (uint64_t i = base; i < 1024;) {
    ret.push_back(Size(0, 0, i, 0));
    i *= ratio;
  }
  return ret;
}

std::vector<Size> DefaultSizeLevelStrategy::gigaByteSizes() {
  std::vector<Size> ret;
  uint32_t base = 1;
  uint32_t ratio = 2;
  for (uint64_t i = base; i < 10;) {
    ret.push_back(Size(0, 0, 0, i));
    i *= ratio;
  }
  return ret;
}
}  // namespace

std::vector<Size> LevelAssignStrategy::assignLevels() {
  std::vector<Size> ret;
  ret.insert(ret.end(), DefaultSizeLevelStrategy::byteSizes().begin(), DefaultSizeLevelStrategy::byteSizes().end());
  ret.insert(ret.end(), DefaultSizeLevelStrategy::kiloByteSizes().begin(), DefaultSizeLevelStrategy::kiloByteSizes().end());
  ret.insert(ret.end(), DefaultSizeLevelStrategy::megaByteSizes().begin(), DefaultSizeLevelStrategy::megaByteSizes().end());
  ret.insert(ret.end(), DefaultSizeLevelStrategy::gigaByteSizes().begin(), DefaultSizeLevelStrategy::gigaByteSizes().end());
  return ret;
}

BlockPtr BufferHubLevel::fetchOneFreeBlock() {
  BlockPtr ret_block {nullptr};

  if (free_map.empty()) {
    LOG_INFO("No free block at level %d,refilling...", index);
    auto block_bytes = this->block_size.totalBytes();
    refill(Size(expand_factor * block_bytes));  // allocate expand_factor blocks
  }

  if (!free_map.empty()) {
    LOG_INFO("Found free block at level %d", index);
    auto it = free_map.begin();
    auto block_it = it->second;
    // Transition from free to busy: increment ref_cnt from 0 to 1
    (*block_it)->ref_cnt++;
    busy_map.insert({it->first, it->second});
    free_map.erase(it);
    ret_block = *block_it;
  } else {
    LOG_WARN("Unable to fetch free block at level %d even after refill", index);
  }

  return ret_block;
}

void BufferHubLevel::refill(const nova_llm::Size& dst_sz) {
  auto dst_total_bytes = dst_sz.totalBytes();
  auto block_bytes = this->block_size.totalBytes();
  uint64_t cnt = dst_total_bytes / block_bytes;

  // Allocate data per block so that each pointer we free was directly allocated
  // Blocks start in the free list with ref_cnt == 0.
  for (uint64_t i = 0; i < cnt; i++) {
    auto* one_block = hub->allocBlock();
    one_block->data = hub->allocData(block_bytes);
    one_block->size = block_bytes;
    one_block->ref_cnt = 0;  // free blocks have ref_cnt == 0
    auto it = this->block_list.insert(this->block_list.end(), one_block);
    this->free_map[one_block->data] = it;
  }
}

void BufferHubLevel::putOneBlock(const BlockPtr& block_ptr) {
  BlockPtr dst_block(block_ptr);
  if (block_list.empty()) {
    auto ret_it = block_list.insert(block_list.end(), dst_block);
    (*ret_it)->ref_cnt = 0;
    free_map.insert({dst_block->data, ret_it});
  } else {
    bool in_free_m = free_map.count(dst_block->data);
    bool in_busy_m = busy_map.count(dst_block->data);
    if (!in_free_m && !in_busy_m) {
      auto it = block_list.insert(block_list.end(), dst_block);
      (*it)->ref_cnt = 0;
      free_map.insert({(*it)->data, it});
    } else if (in_free_m) {
      LOG_WARN("Block %p already in block list at level %d", static_cast<void*>(dst_block->data), index);
    } else {  // in_busy_m is true
      auto& it = busy_map[dst_block->data];
      auto& busy_block = *it;
      // Decrease ref count once; when it reaches zero, move block back to free_map
      if (busy_block->ref_cnt > 0) {
        busy_block->ref_cnt--;
      }
      if (busy_block->ref_cnt == 0) {
        busy_map.erase(busy_block->data);
        free_map[dst_block->data] = it;
      }
    }
  }
}

BufferHubLevel::~BufferHubLevel() {
  free_map.clear();
  busy_map.clear();
  for (auto& block_ptr : block_list) {
    hub->tearDownBlock(block_ptr);
  }
}

BufferHub::BufferHub() {}

BufferHub::~BufferHub() {
  // Let the map manage BufferHubLevel destruction
  buffers_.clear();
  // Clear configuration metadata
  size_levels_.clear();
}

BufferHub* BufferHub::Builder::build(const BufferHubConfig& config) {
  auto* hub = new BufferHub;
  hub->initConfig(config);
  int index = 0;
  for (auto v : config.sizeLevels()) {
    hub->addSizeLevel(index, v);
    ++index;
  }
  return hub;
}

void BufferHub::Builder::destroy(nova_llm::BufferHub** hub) {
  if (hub && *hub) {
    // Deleting the BufferHub will call destructors of its members (including Level),
    // which will in turn call tearDownBlock to free internal allocations.
    //(*hub)->~BufferHub();

    delete *hub;
    *hub = nullptr;
  }
}

void BufferHub::initConfig(const BufferHubConfig& config) {
  device_type_ = config.deviceType();
  this->size_levels_ = config.sizeLevels();
  std::sort(size_levels_.begin(), size_levels_.end(), [](const Size& a, const Size& b) { return a.totalBytes() < b.totalBytes(); });
  this->size_limit_ = config.sizeLimit();
  this->warning_level_ = config.warningLevel();
  this->allocator_ = config.allocator();
}

Block::DataPtr BufferHub::allocData(uint64_t sz) { return static_cast<Block::DataPtr>(this->allocator_->allocate(sz)); }

void BufferHub::deallocData(Block::DataPtr& data_ptr) {
  if (data_ptr) {
    this->allocator_->deallocate(data_ptr);
    data_ptr = nullptr;
  }
}

BlockPtr BufferHub::allocBlock() { return static_cast<BlockPtr>(this->allocator_->allocate(sizeof(Block))); }

void BufferHub::deallocateBlock(BlockPtr& block_ptr) {
  if (block_ptr) {
    this->allocator_->deallocate(block_ptr);
    block_ptr = nullptr;
  }
}

BlockPtr BufferHub::setUpBlock(const Size& sz) {
  auto block = allocBlock();
  block->data = allocData(sz.totalBytes());
  block->size = sz.totalBytes();
  block->ref_cnt = 0;
  return block;
}

void BufferHub::tearDownBlock(BlockPtr& block) {
  if (block) {
    deallocData(block->data);
    block->size = 0;
    block->ref_cnt = 0;
    deallocateBlock(block);
  }
}

void BufferHub::addSizeLevel(uint32_t index, const Size& level_block_sz) {
  auto& level = buffers_[level_block_sz];
  level.block_size = level_block_sz;
  level.index = index;
  level.hub = this;
}

void BufferHub::eraseSizeLevel(const Size& level_sz) {
  auto it = buffers_.find(level_sz);
  if (it == buffers_.end()) {
    LOG_WARN("Level with size %d is not found!", level_sz.totalBytes());  // TODO:optimize
    return;
  }

  auto& level = it->second;
  if (!level.busy_map.empty()) {
    LOG_WARN("Level with size %d is in use,cannot erase now,please try some time later", level_sz.totalBytes());
    return;
  }

  // Erasing from the map will automatically destroy the BufferHubLevel
  buffers_.erase(it);
}

BlockPtr BufferHub::getBlock(const Size& sz) {
  // round it to ceil level
  auto level_sz = gradeLevel(sz);
  if (!level_sz.isValid()) {
    return nullptr;
  }
  // search the block list
  BlockPtr ret_block {nullptr};
  if (buffers_.count(level_sz)) {
    auto& level = buffers_[level_sz];
    auto block = level.fetchOneFreeBlock();
    if (block->isValid()) {
      ret_block = block;
    }
  }
  if (nullptr == ret_block) {
    LOG_WARN("Unable to find available block of size %d", sz.totalBytes());
  }
  return ret_block;
}

void BufferHub::putBlock(const BlockPtr& block_ptr) {
  auto size = block_ptr->size;
  Size level_size(size);
  if (buffers_.count(level_size)) {
    auto& level = buffers_[level_size];
    level.putOneBlock(block_ptr);
  } else {
    LOG_ERROR("Level size %d is not found in buffers!", level_size.totalBytes());
  }
}

void BufferHub::putBlockFromBuffer(Buffer& buffer) {
  if (0 == buffer.size || nullptr == buffer.data) {
    return;
  }
  Size level_sz(buffer.size);
  if (buffers_.count(level_sz)) {
    auto& level = buffers_[level_sz];
    auto* data = static_cast<Block::DataPtr>(buffer.data);
    if (level.busy_map.count(data)) {
      auto block_it = level.busy_map[data];
      level.putOneBlock(*block_it);
    }
  } else {
    LOG_ERROR("Level with size %d cannot be found in this memory hub", level_sz.totalBytes());
  }

  // Clear the Buffer to avoid dangling pointers for callers.
  buffer.data = nullptr;
  buffer.size = 0;
}

// TODO: optim the level selection algorithm
Size BufferHub::gradeLevel(const Size& sz) const {
  Size ret;
  uint32_t level_index = 0;
  size_t i = 0;
  for (; i < this->size_levels_.size(); i++) {
    if (sz.totalBytes() <= this->size_levels_[i].totalBytes()) {
      level_index = i;
      break;
    }
  }
  if (this->size_levels_.size() == i) {
    LOG_ERROR("Cannot grade to current levels for size %d", sz.totalBytes());
    return Size {};
  }
  return size_levels_[level_index];
}

}  // namespace nova_llm