#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include "command.hpp"
#include "store.hpp"

int main(int argc, char** argv) {
  // std::remove("command_test.dat");
  // kv::KVStore store("command_test.dat");

  // std::cout << "=== direct command execution ===" << '\n';
  // kv::SetCommand set_name("name", "alice");
  // kv::SetCommand set_city("city", "seoul");
  // kv::GetCommand get_name("name");
  // kv::GetCommand get_missing("missing");

  // set_name.execute(store);
  // set_city.execute(store);
  // get_name.execute(store);
  // get_missing.execute(store);

  // std::cout << '\n' << "=== polymorphic execution ===" << '\n';
  // std::vector<std::unique_ptr<kv::Command>> commands;
  // commands.push_back(std::make_unique<kv::SetCommand>("lang", "cpp"));
  // commands.push_back(std::make_unique<kv::GetCommand>("lang"));
  // commands.push_back(std::make_unique<kv::GetCommand>("ghost"));

  // for (const auto& command : commands) {
  //   command->execute(store);
  // }

  // std::cout << '\n' << "=== final store state ===" << '\n';
  // for (const auto& [key, value] : store.list()) {
  //   std::cout << key << " = " << value << '\n';
  // }
  // std::cout << "count: " << store.count() << '\n';

  // auto cmd = kv::parseCommand(argc, argv);
  auto cmd = kv::parseCommand(argc, argv); 
  if (!cmd) return 1;
  kv::KVStore store("store.dat");
  cmd->execute(store);

  return 0;
}
