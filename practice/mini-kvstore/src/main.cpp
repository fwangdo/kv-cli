#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include "command.hpp"
#include "store.hpp"
#include "exporter.hpp"

int main(int argc, char** argv) {
  kv::KVStore store("store.dat");
  store.set("name", "alice");
  store.set("city", "seoul");

  kv::CsvExporter csv;
  csv.dump(store.list(), std::cout);

  auto cmd = kv::parseCommand(argc, argv); 
  if (!cmd) return 1;
  // kv::KVStore store("store.dat");
  cmd->execute(store);

  return 0;
}
