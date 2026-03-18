#pragma once
#include <string> 
#include <map> 
#include <ostream> 
#include <string> 

namespace kv {

class Exporter {
public:
  virtual ~Exporter() = default; 
  virtual void dump(const std::map<std::string, std::string> &data, std::ostream &out) = 0;
};

class CsvExporter : public Exporter {
public:
  void dump(const std::map<std::string, std::string> &data, std::ostream &out) override;
}; 

}