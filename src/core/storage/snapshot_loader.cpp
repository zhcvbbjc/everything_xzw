#include "core/storage/snapshot_loader.h"
#include "core/indexer/indexer.h"
#include "core/indexer/index_snapshot.h"
#include "utils/logger.h"

bool SnapshotLoader::load(Indexer& indexer, IndexSnapshot& snapshot) {
    if (!validate(snapshot)) {
        Logger::instance().log(LogLevel::Error, L"Snapshot validation failed");
        return false;
    }

    if (!rebuild_runtime_structures(indexer, snapshot)) {
        Logger::instance().log(LogLevel::Error, L"Failed to rebuild runtime structures");
        return false;
    }

    Logger::instance().log(LogLevel::Info, L"Snapshot loaded successfully");
    return true;
}

bool SnapshotLoader::validate(const IndexSnapshot& snapshot) const {
    // 简单校验：非空
    return !snapshot.records().empty();
}

bool SnapshotLoader::rebuild_runtime_structures(Indexer& indexer, const IndexSnapshot& snapshot) {
    for (const auto& record : snapshot.records()) {
        indexer.on_file_created(record);
    }
    return true;
}
