#include "core/indexer/index_update.h"

#include "core/indexer/indexer.h"
#include "core/usn/usn_event.h"
#include "core/usn/usn_watcher.h"

IndexUpdate::IndexUpdate(
    Indexer& indexer,
    std::vector<std::wstring> volumes)
    : m_indexer(indexer),
      m_volumes(std::move(volumes)) {}

IndexUpdate::~IndexUpdate() {
    stop();
}

void IndexUpdate::start() {
    if (m_running.exchange(true)) {
        return;
    }

    for (const auto& volume : m_volumes) {
        m_threads.emplace_back(
            &IndexUpdate::watcher_thread,
            this,
            volume
        );
    }
}

void IndexUpdate::stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_threads.clear();
}

void IndexUpdate::watcher_thread(std::wstring volume) {
    try {
        UsnWatcher watcher(volume);

        watcher.run([this](const UsnEvent& ev) {
            if (!m_running.load()) {
                return;
            }

            switch (ev.action) {
            case UsnAction::Create: {
                FileRecord record;
                record.file_id        = ev.file_id;
                record.parent_file_id = ev.parent_file_id;
                record.name           = ev.name;
                record.is_directory   = ev.is_directory;
                m_indexer.on_file_created(record);
                break;
            }
            case UsnAction::Delete:
                m_indexer.on_file_deleted(ev.file_id);
                break;

            case UsnAction::Rename:
            case UsnAction::Move:
                m_indexer.on_file_renamed(
                    ev.file_id,
                    ev.name,
                    ev.parent_file_id
                );
                break;

            default:
                break;
            }
        });
    } catch (...) {
        // 卷异常 / Journal 异常
    }
}
