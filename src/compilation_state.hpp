#ifndef COMPILATION_STATE
#define COMPILATION_STATE
#include "compiler/enviornment/enviornment.hpp"
#include <string>
#include <variant>
#include <vector>
#include <filesystem>

#include <memory>

// =======================================
// Helper Function to Run External Commands
// =======================================
std::string runCommand(const std::string& command, int& exit_code);

// =======================================
// Utility Namespace
// =======================================
namespace Utils {
std::string readFileToString(const std::filesystem::path& filePath);
size_t computeHash(const std::string& content);
void createDirectories(const std::filesystem::path& path);
} // namespace Utils

namespace compilationState {

class RecordFolder;
class RecordFile {
  public:
    std::string name;
    std::unique_ptr<enviornment::Enviornment> env = nullptr;
    bool compiled = false;
    RecordFolder* parent = nullptr;

    ~RecordFile() = default;
};

class RecordFolder {
  public:
    std::string name;
    std::vector<std::variant<RecordFile*, RecordFolder*>> files_or_folder = {};
    RecordFolder* parent = nullptr;

        ~RecordFolder() {
            for (auto& item : files_or_folder) {
                if (std::holds_alternative<RecordFile*>(item)) {
                    delete std::get<RecordFile*>(item);
                } else if (std::holds_alternative<RecordFolder*>(item)) {
                    delete std::get<RecordFolder*>(item);
                }
            }
        }
};
}

// Global declaration for shared compilation record access
compilationState::RecordFile* findOrCreateFileRecord(compilationState::RecordFolder* rootFolder, const std::filesystem::path& relativePath);

#endif