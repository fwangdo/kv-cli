#pragma once
#include <string> 
#include <map> 
#include <ostream> 
#include <string> 
#include <memory> 

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

class JsonExporter : public Exporter {
public:
  void dump(const std::map<std::string, std::string> &data, std::ostream &out) override;
};

std::unique_ptr<Exporter> createExporter(const std::string& format); 

}