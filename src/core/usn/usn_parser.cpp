#include "core/usn/usn_parser.h"
#include <Windows.h>

bool UsnParser::is_relevant(const USN_RECORD* record) {
    if (!record) return false;

    // 过滤系统噪声事件（可按需扩展）
    constexpr DWORD interested =
        USN_REASON_FILE_CREATE |
        USN_REASON_FILE_DELETE |
        USN_REASON_RENAME_NEW_NAME |
        USN_REASON_RENAME_OLD_NAME |
        USN_REASON_DATA_OVERWRITE |
        USN_REASON_DATA_EXTEND |
        USN_REASON_BASIC_INFO_CHANGE;

    return (record->Reason & interested) != 0;
}

UsnEvent UsnParser::parse(const USN_RECORD* record) {
    UsnEvent evt{};

    evt.action = decode_action(record->Reason);
    evt.file_id = record->FileReferenceNumber;
    evt.parent_file_id = record->ParentFileReferenceNumber;
    evt.is_directory = decode_is_directory(record);

    if (record->FileNameLength > 0) {
        const wchar_t* name =
            reinterpret_cast<const wchar_t*>(
                reinterpret_cast<const uint8_t*>(record) +
                record->FileNameOffset);

        evt.name.assign(name, record->FileNameLength / sizeof(wchar_t));
    }

    return evt;
}

UsnAction UsnParser::decode_action(DWORD reason) {
    if (reason & USN_REASON_FILE_CREATE)
        return UsnAction::Create;

    if (reason & USN_REASON_FILE_DELETE)
        return UsnAction::Delete;

    if (reason & USN_REASON_RENAME_NEW_NAME)
        return UsnAction::Rename;

    if (reason & USN_REASON_RENAME_OLD_NAME)
        return UsnAction::Move;

    if (reason & (USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND))
        return UsnAction::Modify;

    return UsnAction::Unknown;
}

bool UsnParser::decode_is_directory(const USN_RECORD* record) {
    return (record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
