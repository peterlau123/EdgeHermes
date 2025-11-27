#include "NovaLLM/memory/buffer_manager.h"

#include "NovaLLM/memory/allocator.h"
#include "NovaLLM/memory/buffer_hub.h"
#include "NovaLLM/utils/log.h"
#include "NovaLLM/utils/macros.h"
// Disable C4251 warning on Windows (DLL interface for STL containers)

namespace nova_llm {


BufferManager BufferManager::Builder::buffer_manager;

BufferManager &BufferManager::Builder::build(const nova_llm::BufferManager::Config &config) {
  if (!buffer_manager.isInited()) {
    auto ret = buffer_manager.init(config);
    if (!ret) {
      LOG_ERROR("Failed to init buffer manager");
    }
  }
  return buffer_manager;
}

BufferManager &BufferManager::Builder::getInstance() { return buffer_manager; }

bool BufferManager::init(const nova_llm::BufferManager::Config &config) {
  if (is_init_) {
    return true;
  }
  bool ret = false;
  if (config.device_flags.has(DeviceType::CPU)) {
    BufferHubConfig cfg(DeviceType::CPU, config.cpu.alloc, Size(4UL*1024*1024*1024));
    buffer_hubs_[DeviceType::CPU] = BufferHub::Builder::build(cfg);
    ret |= true;
  }
  // TODO: other devices
  is_init_ = true;
  return ret;
}

void BufferManager::put(Buffer &buffer) {
  if (nullptr == buffer.data || 0 == buffer.size) {
    return;
  }
  auto device_type = buffer.device_type;
  auto &device_mem_hub = buffer_hubs_[device_type];
  device_mem_hub->putBlockFromBuffer(buffer);
}

Buffer BufferManager::fetch(size_t size, DeviceType device_type) {
  Buffer buffer;
  Size sz(size);
  auto block_ptr = buffer_hubs_[device_type]->getBlock(sz);
  if (nullptr != block_ptr) {
    buffer.data = block_ptr->data;
    buffer.size = block_ptr->size;
  }
  return buffer;
}

BufferManager::~BufferManager() { destroy(); }

void BufferManager::destroy() {
  for (auto& p : buffer_hubs_) {
    BufferHub::Builder::destroy(&(p.second));
  }
  buffer_hubs_.clear();
  is_init_ = false;
}


}  // namespace nova_llm