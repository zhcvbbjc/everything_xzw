// src/gui/main_window.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "gui/main_window.h"
#include "utils/resource.h"  // 包含 IDC_SEARCH_EDIT 等定义

namespace {
    constexpr LPCTSTR WINDOW_CLASS_NAME = TEXT("EverythingCloneMainWindow");
    constexpr LPCTSTR WINDOW_TITLE      = TEXT("Everything Clone");
} // anonymous namespace

MainWindow::MainWindow(HINSTANCE hInstance) : m_hInstance(hInstance) {}

bool MainWindow::Create() {
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = m_hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    // wc.hIcon      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); // 可选图标

    if (!RegisterClass(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 500,
        nullptr,
        nullptr,
        m_hInstance,
        this  // 传递 this 指针用于 WM_CREATE 时绑定
    );

    return m_hwnd != nullptr;
}

void MainWindow::RunMessageLoop() {
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static MainWindow* pThis = nullptr;

    if (msg == WM_CREATE) {
        // 从 CREATESTRUCT 获取 this 指针
        CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<MainWindow*>(pcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        // 创建搜索框
        pThis->m_hwndEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_EDIT,
            TEXT(""),
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 10, 680, 26,
            hwnd,
            (HMENU)IDC_SEARCH_EDIT,
            pThis->m_hInstance,
            nullptr
        );

        // 创建列表视图
        pThis->m_hwndList = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            TEXT(""),
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_VSCROLL | WS_HSCROLL,
            10, 45, 680, 410,
            hwnd,
            (HMENU)IDC_FILE_LIST,
            pThis->m_hInstance,
            nullptr
        );

        // 设置 ListView 列
        LVCOLUMN col = {0};
        col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        col.fmt = LVCFMT_LEFT;

        col.cx = 300;
        col.pszText = const_cast<LPTSTR>(TEXT("Name"));
        ListView_InsertColumn(pThis->m_hwndList, 0, &col);

        col.cx = 370;
        col.pszText = const_cast<LPTSTR>(TEXT("Full Path"));
        ListView_InsertColumn(pThis->m_hwndList, 1, &col);

        // 示例数据（可删除）
        LVITEM item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.pszText = const_cast<LPTSTR>(TEXT("readme.txt"));
        ListView_InsertItem(pThis->m_hwndList, &item);
        ListView_SetItemText(pThis->m_hwndList, 0, 1, const_cast<LPTSTR>(TEXT("C:\\Projects\\EverythingClone")));

        return 0;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (msg) {
            case WM_COMMAND:
                if (LOWORD(wParam) == IDC_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
                    // TODO: 延迟触发搜索（避免频繁查询）
                }
                break;

            case WM_SIZE:
                if (pThis->m_hwndEdit && pThis->m_hwndList) {
                    int width = LOWORD(lParam);
                    int height = HIWORD(lParam);
                    SetWindowPos(pThis->m_hwndEdit, nullptr, 10, 10, width - 20, 26, SWP_NOZORDER);
                    SetWindowPos(pThis->m_hwndList, nullptr, 10, 45, width - 20, height - 55, SWP_NOZORDER);
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}