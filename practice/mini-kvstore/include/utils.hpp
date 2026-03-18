#pragma once
#include <iostream> 

template <typename MapLike>
void printEntries(const MapLike &data) {
  for (const auto &[k, v] : data) {
    std::cout << k << " = " << v << '\n'; 
  }
}