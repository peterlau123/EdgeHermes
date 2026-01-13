#pragma once
#include <string>
#include <vector>

#include "EdgeHermes/parser/parser.h"
#include "EdgeHermes/pipeline/pipeline.h"

namespace edgehermes {

class EngineImpl {
 public:
  using EngineImplPtr = std::shared_ptr<EngineImpl>;

  static EngineImplPtr build();

  bool init();

  bool parse(const std::string& modelPath);

  std::string chat(const std::string& prompt);

  EngineImpl() = default;

 private:
  bool init_;
  std::vector<std::string> history_;
  ParserSharedPtr parser_;
  PipelineSharedPtr pipeline_;
};

using EngineImplPtr = EngineImpl::EngineImplPtr;

}  // namespace edgehermes



