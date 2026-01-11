// src/main.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>  // ListView, Edit 等控件支持
#include "gui/main_window.h"

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int
) {
    // 初始化 Common Controls
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    MainWindow window(hInstance);
    if (!window.Create()) {
        MessageBox(nullptr, L"Failed to create main window.", L"Error", MB_ICONERROR);
        return 1;
    }

    window.RunMessageLoop();
    return 0;
}
