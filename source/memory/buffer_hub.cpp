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

BlockRawPtr BufferHubLevel::fetchOneFreeBlock() {
  BlockRawPtr ret_block {nullptr};

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
    ret_block = block_it->get();  // Return non-owning pointer
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
    auto one_block = hub->setUpBlock(Size(block_bytes));
    one_block->ref_cnt = 0;  // free blocks have ref_cnt == 0
    auto* block_ptr = one_block.get();
    auto it = this->block_list.insert(this->block_list.end(), std::move(one_block));
    this->free_map[block_ptr->data] = it;
  }
}

void BufferHubLevel::putOneBlock(BlockRawPtr block_ptr) {
  if (block_ptr == nullptr) {
    return;
  }
  
  if (block_list.empty()) {
    LOG_WARN("putOneBlock called on empty block_list at level %d", index);
    return;
  }
  
  bool in_free_m = free_map.count(block_ptr->data);
  bool in_busy_m = busy_map.count(block_ptr->data);
  
  if (!in_free_m && !in_busy_m) {
    LOG_WARN("Block %p not found in level %d", static_cast<void*>(block_ptr->data), index);
    return;
  } else if (in_free_m) {
    LOG_WARN("Block %p already in free list at level %d", static_cast<void*>(block_ptr->data), index);
  } else {  // in_busy_m is true
    auto it = busy_map[block_ptr->data];
    auto& busy_block = *it;
    // Decrease ref count once; when it reaches zero, move block back to free_map
    if (busy_block->ref_cnt > 0) {
      busy_block->ref_cnt--;
    }
    if (busy_block->ref_cnt == 0) {
      free_map[block_ptr->data] = it;  // NOTE: Be cautious about the order of operations here
      busy_map.erase(busy_block->data);
    }
  }
}

BufferHubLevel::~BufferHubLevel() {
  free_map.clear();
  busy_map.clear();
  // Blocks are automatically cleaned up when unique_ptrs are destroyed
  // but we need to manually free the data
  for (auto& block_ptr : block_list) {
    if (block_ptr && block_ptr->data) {
      hub->deallocData(block_ptr->data);
    }
  }
  block_list.clear();  // unique_ptrs will deallocate Block structs
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

BlockPtr BufferHub::allocBlock() {
  auto* raw_ptr = static_cast<Block*>(this->allocator_->allocate(sizeof(Block)));
  return BlockPtr(raw_ptr);
}

void BufferHub::deallocateBlock(BlockPtr block) {
  if (block) {
    Block* raw = block.release();
    this->allocator_->deallocate(raw);
  }
}

BlockPtr BufferHub::setUpBlock(const Size& sz) {
  auto block = allocBlock();
  block->data = allocData(sz.totalBytes());
  block->size = sz.totalBytes();
  block->ref_cnt = 0;
  return block;
}

void BufferHub::tearDownBlock(BlockPtr block) {
  if (block) {
    deallocData(block->data);
    block->size = 0;
    block->ref_cnt = 0;
    deallocateBlock(std::move(block));
  }
}

void BufferHub::addSizeLevel(uint32_t index, const Size& level_block_sz) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  
  auto& level = buffers_[level_block_sz];
  level.block_size = level_block_sz;
  level.index = index;
  level.hub = this;
}

void BufferHub::eraseSizeLevel(const Size& level_sz) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  
  auto it = buffers_.find(level_sz);
  if (it == buffers_.end()) {
    LOG_WARN("Level with size %llu is not found!", level_sz.totalBytes());
    return;
  }

  auto& level = it->second;
  if (!level.busy_map.empty()) {
    LOG_ERROR("Level with size %llu has %zu busy blocks, cannot erase now", 
              level_sz.totalBytes(), level.busy_map.size());
    return;
  }

  // Free all blocks in the block_list before erasing
  // The destructor will be called, but let's be explicit about cleanup
  LOG_INFO("Erasing level with size %llu, freeing %zu blocks", 
           level_sz.totalBytes(), level.block_list.size());
  
  // Erasing from the map will call BufferHubLevel destructor,
  // which properly frees all blocks via tearDownBlock
  buffers_.erase(it);
}

BlockRawPtr BufferHub::getBlock(const Size& sz) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  
  // round it to ceil level
  auto level_sz = gradeLevel(sz);
  if (!level_sz.isValid()) {
    return nullptr;
  }
  // search the block list
  BlockRawPtr ret_block {nullptr};
  if (buffers_.count(level_sz)) {
    auto& level = buffers_[level_sz];
    auto block = level.fetchOneFreeBlock();
    if (block && block->isValid()) {
      ret_block = block;
    }
  }
  if (nullptr == ret_block) {
    LOG_WARN("Unable to find available block of size %d", sz.totalBytes());
  }
  return ret_block;
}

void BufferHub::putBlock(BlockRawPtr block_ptr) {
  if (!block_ptr) {
    return;
  }
  
  std::unique_lock<std::shared_mutex> lock(mutex_);
  
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
  std::unique_lock<std::shared_mutex> lock(mutex_);
  
  if (0 == buffer.size || nullptr == buffer.data) {
    return;
  }
  Size level_sz(buffer.size);
  if (buffers_.count(level_sz)) {
    auto& level = buffers_[level_sz];
    auto* data = static_cast<Block::DataPtr>(buffer.data);
    if (level.busy_map.count(data)) {
      auto block_it = level.busy_map[data];
      level.putOneBlock(block_it->get());  // Get raw pointer from unique_ptr
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