#include "NovaLLM/memory/buffer_hub.h"

#include <algorithm>

#include "NovaLLM/utils/log.h"

namespace nova_llm {

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

BlockPtr BufferHub::Level::fetchOneFreeBlock() {
  BlockPtr ret_block {nullptr};
  if (!free_map.empty()) {
    auto it = free_map.begin();
    auto block_it = it->second;
    (*block_it)->ref_cnt++;
    busy_map.insert({it->first, it->second});
    free_map.erase(it);
    ret_block = *block_it;
  } else {
    auto level_bytes = level_size.totalBytes();

    auto kb_level = Size(0, 1, 0, 0);
    auto mb_level = Size(0, 0, 1, 0);
    auto gb_level = Size(0, 0, 0, 1);
    auto kb_bytes = kb_level.totalBytes();
    auto mb_bytes = mb_level.totalBytes();
    auto gb_bytes = gb_level.totalBytes();

    if (level_bytes < kb_bytes) {
      refill(kb_level);
    } else if (level_bytes < mb_bytes) {
      refill(mb_level);
    } else if (level_bytes < gb_bytes) {
      refill(gb_level);
    }
    ret_block = *(free_map.begin()->second);
  }
  return ret_block;
}

void BufferHub::Level::refill(const nova_llm::Size& sz) {
  auto dst_size = sz.totalBytes();
  auto level_bytes = this->level_size.totalBytes();
  uint64_t cnt = dst_size / level_bytes;

  auto* data = this->hub->allocData(dst_size);
  for (uint64_t i = 0; i < cnt; i++) {
    auto* one_block = hub->allocBlock();
    one_block->data = data + i * level_bytes;
    one_block->size = level_bytes;
    one_block->ref_cnt = 1;// set ref_cnt to 1 when allocated
    auto it = this->block_list.insert(this->block_list.end(), one_block);
    this->free_map[one_block->data] = it;
  }
}

void BufferHub::Level::putOneBlock(const BlockPtr& block_ptr) {
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
      LOG_WARN("Block %p already in block list at level %d",
               static_cast<void*>(dst_block->data),
               index);
    } else {  // in_busy_m is true
      auto& it = busy_map[dst_block->data];
      auto& busy_block = *it;
      if (0 == --busy_block->ref_cnt) {
        busy_map.erase(busy_block->data);
        busy_block->ref_cnt = 0;
        free_map[dst_block->data] = it;
      } else {
        busy_block->ref_cnt--;
      }
    }
  }
}

BufferHub::Level::~Level() {
  free_map.clear();
  busy_map.clear();
  for (auto& block_ptr : block_list) {
    hub->tearDownBlock(block_ptr);
  }
}

BufferHub* BufferHub::Builder::build(const Config& config) {
  auto* hub = new BufferHub;
  hub->initConfig(config);
  int index = 0;
  for (auto v : config.size_levels) {
    hub->addSizeLevel(index, v);
    ++index;
  }
  return hub;
}

void BufferHub::Builder::destroy(nova_llm::BufferHub** hub) {
  for (auto& p : (*hub)->buffers_) {
    p.second.~Level();
  }
}

void BufferHub::initConfig(const Config& config) {
  device_type_ = config.device_type;
  size_levels_ = config.size_levels;
  // sort size levels in ascending order
  std::sort(size_levels_.begin(), size_levels_.end(), [](const Size& a, const Size& b) {
    return a.totalBytes() < b.totalBytes();
  });
  size_limit_ = config.size_limit;
  warning_level_ = config.warning_level;
  allocator_ = config.allocator;
}

Block::DataPtr BufferHub::allocData(uint64_t sz) {
  return static_cast<Block::DataPtr>(this->allocator_->allocate(sz));
}

void BufferHub::deallocData(Block::DataPtr& data_ptr) {
  if (data_ptr) {
    this->allocator_->deallocate(data_ptr);
    data_ptr = nullptr;
  }
}

BlockPtr BufferHub::allocBlock() {
  return static_cast<BlockPtr>(this->allocator_->allocate(sizeof(Block)));
}

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

void BufferHub::addSizeLevel(uint32_t index, const Size& level_sz) {
  auto& level = buffers_[level_sz];
  level.level_size = level_sz;
  level.index = index;
  level.hub = this;
}

void BufferHub::eraseSizeLevel(const Size& level_sz) {
  // NOTE:cautious,make sure the size level is not in use
  if (buffers_.count(level_sz)) {
    if (buffers_[level_sz].busy_map.empty()) {
      auto& level = buffers_[level_sz];
      level.~Level();
    } else {
      LOG_WARN("Level with size %d is in use,cannot erase now,please try some time later",
               level_sz.totalBytes());
    }
  } else {
    LOG_WARN("Level with size %d is not found!", level_sz.totalBytes());  // TODO:optimize
  }
}

BlockPtr BufferHub::getBlock(const Size& sz){
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
    LOG_ERROR("Level with size %d is not found!", level_size.totalBytes());
  }
}

void BufferHub::putBlockFromBuffer(const Buffer& buffer) {
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
}

Size BufferHub::gradeLevel(const Size& sz) const {
  Size ret;
  uint32_t level_index = 0;
  size_t i = 0;
  for (; i < size_levels_.size(); i++) {
    if (sz.totalBytes() < size_levels_[i].totalBytes()) {
      level_index = i;
      break;
    }
  }
  if (size_levels_.size() == i) {
    LOG_ERROR("Cannot grade to current levels for size %d", sz.totalBytes());
    return Size {};
  }
  return size_levels_[level_index];
}

}  // namespace nova_llm