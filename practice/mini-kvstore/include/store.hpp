#pragma once 

#include <map>
#include <string> 
#include <optional>

namespace kv {

class KVStore {
public:
  explicit KVStore(const std::string &path); 

  // utility. 
  void set(const std::string &key, const std::string &value);
  std::optional<std::string> get(const std::string &key); 
  bool remove(const std::string &key);

  // return. 
  const std::map<std::string, std::string>& list();
  size_t count(); 
  void save();

private:
  std::map<std::string, std::string> data_; // real data. 
  std::string path_; 
  void load(); 
}; 

}