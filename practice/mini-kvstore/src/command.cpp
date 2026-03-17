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

void DeleteCommand::execute(KVStore &store) {
  auto res = store.remove(key_);
  if (!res) {
    std::cout << "[ERROR]: " << key_ << " does not exist" << std::endl;
  } else {
    std::cout << "OK" << std::endl; 
  }
  return; 
} 

void ListCommand::execute(KVStore &store) {
  // print all elements in data_. 
  auto list = store.list();
  for (auto &[k, v] : list) {
    std::cout << k << "=" << v << std::endl; 
  }
  return; 
}

void CountCommand::execute(KVStore &store) {
  auto cnt = store.count(); 
  std::cout << cnt << std::endl; 
}

void ExportCommand::execute(KVStore &store) {
  std::cout << "TODO" << std::endl; 
}

}