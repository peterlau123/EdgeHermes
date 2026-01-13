#include <EdgeHermes/EdgeHermes-cpp.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

auto main(int argc, char** argv) -> int {
  int arg_num = argc;
  std::cout << "arg num:" << arg_num << std::endl;
  for (int i = 0; i < arg_num; i++) {
    std::cout << argv[i] << std::endl;
  }
  return 0;
}




