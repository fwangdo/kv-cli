#include <map>
#include <string> 
#include <optional>
#include "store.hpp"

namespace kv {

// public
KVStore::KVStore(const std::string &path) : path_(path) {
  load(); 
}

void KVStore::set(const std::string &key, const std::string &value) {
  data_[key] = value; 
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
  if (res == 1) {
    return true;
  } else {
    return false; 
  }
}

// return
const std::map<std::string, std::string> &KVStore::list() {
  return data_; 
}

size_t KVStore::count() {
  return data_.size(); 
}

void KVStore::save() {

}

// private.
void KVStore::load() {

}

}