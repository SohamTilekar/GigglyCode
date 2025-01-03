#ifndef COMPILATION_STATE
#define COMPILATION_STATE
#include "compiler/enviornment/enviornment.hpp"
#include <string>
#include <variant>
#include <vector>


namespace compilationState {
class RecordFolder;
class RecordFile {
  public:
    std::string name;
    enviornment::Enviornment* env = nullptr;
    bool compiled = false;
    RecordFolder* parent = nullptr;
};

class RecordFolder {
  public:
    std::string name;
    std::vector<std::variant<RecordFile*, RecordFolder*>> files_or_folder = {};
    RecordFolder* parent = nullptr;

    ~RecordFolder() {
        for (auto& item : files_or_folder) {
            std::visit([](auto* ptr) { delete ptr; }, item);
        }
    }
};
}
#endif