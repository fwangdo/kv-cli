#include <map>
#include <string> 
#include <optional>
#include <fstream> // what is the role of this lib?
#include "store.hpp"

namespace kv {

// public
KVStore::KVStore(const std::string &path) : path_(path) {
  load(); 
}

void KVStore::set(const std::string &key, const std::string &value) {
  data_[key] = value; 
  KVStore::save(); 
}

std::optional<std::string> KVStore::get(const std::string &key) {
  auto res = data_.find(key);
  if (res != data_.end()) {
    return res->second; 
  }
  // what is this? 
  return std::nullopt; 
}

bool KVStore::remove(const std::string &key) {
  auto res = data_.erase(key);
  bool ret = false; 
  if (res == 1) {
    ret = true;
  }
  KVStore::save(); 
  return ret;
}

// return
const std::map<std::string, std::string> &KVStore::list() {
  return data_; 
}

size_t KVStore::count() {
  return data_.size(); 
}

void KVStore::save() {
  auto todo = std::ofstream(path_);
  for (const auto &[k, v] : data_) {
    todo << k << "=" <<  v << '\n'; 
  }
}

// private.
void KVStore::load() {
  auto in = std::ifstream(path_);
  if (!in) return;

  std::string line;
  while (std::getline(in, line)) {
    std::string::size_type idx = line.find("=");
    // npos is the value which is returned when find method does not find the element. 
    if (idx == std::string::npos) {
      continue;
    }
    
    std::string left = line.substr(0, idx); 
    std::string right = line.substr(idx + 1); 
    data_[left] = right; 
  }
}

}
