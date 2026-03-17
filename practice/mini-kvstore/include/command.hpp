#pragma once 
#include <string>
#include "store.hpp"

namespace kv {

class Command {
public:
  // q1. why we cannot make constructor with virtual?
  // a1. virtual function uses vtable on the generated instance. so, we cannot use constructor on not-generated instance. 
  // q2. what is the meaning of virtual destructor? 
  virtual ~Command() = default;
  virtual void execute(KVStore &store) = 0; 
};

class SetCommand : public Command {
public:
  explicit SetCommand(const std::string &key, const std::string &value): key_(key), value_(value) {};
  void execute(KVStore &store) override;

private: 
  std::string key_; 
  std::string value_; 
}; 

class GetCommand : public Command {
public:
  explicit GetCommand(const std::string &key): key_(key) {};
  void execute(KVStore &store) override;

private:
  std::string key_; 
};

}