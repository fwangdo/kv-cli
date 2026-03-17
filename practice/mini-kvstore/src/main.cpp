#include <iostream>
#include "store.hpp"

int main() {
  kv::KVStore store("store.dat");

  store.set("name", "alice");
  store.set("city", "seoul");

  auto name = store.get("name");
  if (name) {
    std::cout << "name: " << *name << '\n';
  }

  auto missing = store.get("missing");
  if (!missing) {
    std::cout << "missing key not found" << '\n';
  }

  std::cout << "all items:" << '\n';
  for (const auto& [key, value] : store.list()) {
    std::cout << key << " = " << value << '\n';
  }

  std::cout << "count: " << store.count() << '\n';
  return 0;
}
