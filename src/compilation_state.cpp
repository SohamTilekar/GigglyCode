#include "compilation_state.hpp"
#include "errors/errors.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <system_error>

// =======================================
// Helper Function to Run External Commands
// =======================================
std::string runCommand(const std::string& command, int& exit_code) {
    std::array<char, 128> buffer;
    std::string result;

    // Open pipe to file
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) { errors::raiseCompilationError("popen() failed while executing command: " + command); }

    try {
        // Read till end of process
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) { result += buffer.data(); }
    } catch (...) {
        pclose(pipe);
        throw;
    }

    // Get the exit code
    exit_code = pclose(pipe);
    return result;
}

// =======================================
// Utility Namespace
// =======================================
namespace Utils {

std::string readFileToString(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file) { errors::raiseFileNotFoundError("Error: Could not open file " + filePath.string()); }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

size_t computeHash(const std::string& content) {
    return std::hash<std::string>{}(content);
}

void createDirectories(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::create_directories(path, ec) && ec) { 
        errors::raiseCompilationError("Error: Could not create directories " + path.string() + ": " + ec.message()); 
    }
}

} // namespace Utils

// =======================================
// Global File Record Helper
// =======================================
static std::mutex fileRecordMutex;

compilationState::RecordFile* findOrCreateFileRecord(compilationState::RecordFolder* rootFolder, const std::filesystem::path& relativePath) {
    std::lock_guard<std::mutex> lock(fileRecordMutex);
    compilationState::RecordFolder* currentFolder = rootFolder;
    for (const auto& part : relativePath.parent_path()) {
        bool found = false;
        for (auto& item : currentFolder->files_or_folder) {
            if (auto folder = std::get_if<compilationState::RecordFolder*>(&item)) {
                if ((*folder)->name == part.string()) {
                    currentFolder = *folder;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            auto newFolder = new compilationState::RecordFolder();
            newFolder->name = part.string();
            newFolder->parent = currentFolder;
            currentFolder->files_or_folder.push_back(newFolder);
            currentFolder = newFolder;
        }
    }

    for (auto& item : currentFolder->files_or_folder) {
        if (auto file = std::get_if<compilationState::RecordFile*>(&item)) {
            if ((*file)->name == relativePath.filename().string()) { return *file; }
        }
    }

    auto newFile = new compilationState::RecordFile();
    newFile->name = relativePath.filename().string();
    newFile->parent = currentFolder;
    currentFolder->files_or_folder.push_back(newFile);
    return newFile;
}
