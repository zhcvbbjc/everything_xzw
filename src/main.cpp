#include "core/usn/usn_watcher.h"
#include "core/usn/usn_parser.h"
#include <iostream>
#include <thread>
#include <io.h>
#include <fcntl.h>

int main() {

    // 启用 UTF-8 输出（对 wcout 生效）
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    try {
        std::wstring volume = L"\\\\.\\C:";  // 可以改为 D: 或 E:
        UsnWatcher watcher(volume);

        std::thread t([&]() {
            watcher.run([](const UsnEvent& ev) {
                std::wcout << L"Event: ";
                switch (ev.action) {
                    case UsnAction::Create: std::wcout << L"Create"; break;
                    case UsnAction::Delete: std::wcout << L"Delete"; break;
                    case UsnAction::Rename: std::wcout << L"Rename"; break;
                    case UsnAction::Move:   std::wcout << L"Move"; break;
                    case UsnAction::Modify: std::wcout << L"Modify"; break;
                }
                std::wcout << L" | FRN=" << ev.file_id 
                           << L" | ParentFRN=" << ev.parent_file_id
                           << L" | Name=" << ev.name
                           << L" | IsDir=" << ev.is_directory
                           << std::endl;
            });
        });

        std::wcout << L"Watching volume " << volume << L" ... Press Enter to stop.\n";
        std::wstring dummy;
        std::getline(std::wcin, dummy);

        watcher.stop();
        t.join();
    } catch (const std::exception& ex) {
        std::cerr << "USN test failed: " << ex.what() << std::endl;
    }

    return 0;
}
