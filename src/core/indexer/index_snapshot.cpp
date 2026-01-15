#include "core/indexer/index_snapshot.h"

IndexSnapshot::IndexSnapshot(std::vector<FileRecord>&& records)
    : m_records(std::move(records)) {}

const std::vector<FileRecord>& IndexSnapshot::records() const {
    return m_records;
}
