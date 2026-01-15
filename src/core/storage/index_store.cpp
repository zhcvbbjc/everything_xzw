#include "core/storage/index_store.h"
#include "core/indexer/index_snapshot.h"
#include "utils/logger.h"

#include <fstream>
#include <filesystem>

IndexStore::IndexStore(const std::wstring& store_dir)
    : m_store_dir(store_dir) {
    std::filesystem::create_directories(m_store_dir);
}

IndexStore::~IndexStore() = default;

std::wstring IndexStore::snapshot_path() const {
    return m_store_dir + L"\\snapshot.bin";
}

std::wstring IndexStore::metadata_path() const {
    return m_store_dir + L"\\metadata.json";
}

bool IndexStore::save_snapshot(const IndexSnapshot& snapshot) {
    std::wstring path = snapshot_path();
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string path_utf8 = converter.to_bytes(path);
    std::ofstream ofs(path_utf8, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        Logger::instance().log(LogLevel::Error, L"Failed to open snapshot file for writing");
        return false;
    }

    const auto& records = snapshot.records();
    size_t count = records.size();
    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& r : records) {
        ofs.write(reinterpret_cast<const char*>(&r), sizeof(FileRecord));
    }

    Logger::instance().info(L"Saved snapshot with %llu records", count);
    return true;
}

bool IndexStore::load_snapshot(const IndexSnapshot& out_snapshot) {
    std::wstring path = snapshot_path();
    if (!std::filesystem::exists(path)) {
        Logger::instance().log(LogLevel::Warning, L"Snapshot file not found");
        return false;
    }
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string path_utf8 = converter.to_bytes(path);
    std::ifstream ifs(path_utf8, std::ios::binary);
    if (!ifs.is_open()) {
        Logger::instance().log(LogLevel::Error, L"Failed to open snapshot file for reading");
        return false;
    }

    size_t count = 0;
    ifs.read(reinterpret_cast<char*>(&count), sizeof(count));
    std::vector<FileRecord> records(count);
    ifs.read(reinterpret_cast<char*>(records.data()), count * sizeof(FileRecord));

    Logger::instance().info(L"Loaded snapshot with %llu records", count);
    return true;
}

void IndexStore::clear() {
    std::filesystem::remove(snapshot_path());
    std::filesystem::remove(metadata_path());
    Logger::instance().log(LogLevel::Info, L"Cleared snapshot and metadata");
}
