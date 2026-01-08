// src/main.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>  // ListView, Edit 等控件支持
#include "gui/main_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // 初始化 Common Controls（必须！否则 ListView 不显示）
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    if (!InitCommonControlsEx(&icc)) {
        MessageBox(nullptr, L"Failed to initialize common controls.", L"Error", MB_ICONERROR);
        return 1;
    }

    MainWindow window(hInstance);
    if (!window.Create()) {
        MessageBox(nullptr, L"Failed to create main window.", L"Error", MB_ICONERROR);
        return 1;
    }

    window.RunMessageLoop();
    return 0;
}