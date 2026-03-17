#include <iostream> 
#include <memory>
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

std::unique_ptr<Command> parseCommand(int argc, char **argv) {
  if (argc < 2) {
    return nullptr; 
  }

  std::string cmd = argv[1]; 
  if (cmd == "set") {
    if (argc != 4) {
      return nullptr; 
    }
    return std::make_unique<SetCommand>(argv[2], argv[3]); 
  }
  // get
  if (cmd == "get") {
    if (argc != 3) {
      return nullptr; 
    }
    return std::make_unique<GetCommand>(argv[2]); 
  }
  // delete 
  if (cmd == "delete") {
    if (argc != 3) {
      return nullptr; 
    }
    return std::make_unique<DeleteCommand>(argv[2]); 
  }
  // list 
  if (cmd == "list") {
    if (argc != 2) {
      return nullptr; 
    }
    return std::make_unique<ListCommand>(); 
  }
  // count
  if (cmd == "count") {
    if (argc != 2) {
      return nullptr; 
    }
    return std::make_unique<CountCommand>(); 
  }
  // export 
  if (cmd == "export") {
    if (argc != 3) {
      return nullptr; 
    }
    return std::make_unique<ExportCommand>(); 
  }

  return nullptr; 
}

}