#include <iostream> 
#include <optional>
#include "store.hpp"
#include "command.hpp" 

namespace kv {

void SetCommand::execute(KVStore& store) {
  store.set(key_, value_); 
  std::cout << "OK\n"; 
}

void GetCommand::execute(KVStore& store) {
  auto check = store.get(key_);
  if (check) {
    std::cout << *check << std::endl; 
  } else {
    std::cout << "ERROR: this is no key like " << key_ << std::endl; 
  }
  return; 
}

}