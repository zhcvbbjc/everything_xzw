// include/gui/main_window.h
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class MainWindow {
public:
    explicit MainWindow(HINSTANCE hInstance);
    ~MainWindow() = default;

    [[nodiscard]] bool Create();
    void RunMessageLoop();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    HWND m_hwndEdit = nullptr;
    HWND m_hwndList = nullptr;
};