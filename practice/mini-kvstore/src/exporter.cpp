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

void JsonExporter::dump(const std::map<std::string, std::string> &data,
                        std::ostream &out) {
  out << "{";
  bool first = true; 
    for (const auto& [key, value] : data) {
      if (!first) {
          out << ',';
      }
      out << '"' << key << "\":\"" << value << '"';
      first = false;
    }
  out << "}"; 
}

std::unique_ptr<Exporter> createExporter(const std::string &format) {
  if (format == "json") return std::make_unique<JsonExporter>(); 
  if (format == "csv") return std::make_unique<CsvExporter>(); 
  return nullptr;
}

}