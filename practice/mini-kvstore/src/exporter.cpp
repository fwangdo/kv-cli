#include "exporter.hpp"
#include <string>
#include <memory>
#include <map>
#include <ostream>

namespace kv {

void CsvExporter::dump(const std::map<std::string, std::string> &data, std::ostream& out) {
  for (const auto &[k, v] : data) {
    out << k << "," << v << '\n'; 
  } 
}

}