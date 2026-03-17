#include <iostream>
#include "store.cpp"

int main(int argc, char* argv[]) {
  std::cout << "Here it is!";

  auto kv = kv::KVStore("store.dat");
  kv.set("a", "1"); 
  return 0; 
}