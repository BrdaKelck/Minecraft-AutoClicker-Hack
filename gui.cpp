#include <windows.h>
#include <thread>
#include <atomic>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

std::atomic<bool> g_pressed(false);
std::atomic<int> g_cps(18);

HWND g_hWndSlider;
HWND g_hWndLabel;

void updateLabel(HWND label, int cps) {
    char text[64];
    sprintf_s(text, "CPS: %d", cps);
    SetWindowTextA(label, text);
}

void control() {
    while (true) {
        if (g_pressed) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

            int delay = 1000 / g_cps.load();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        g_pressed = false;
        PostQuitMessage(0);
        return 0;

    case WM_HSCROLL:
        if ((HWND)lParam == g_hWndSlider) {
            int pos = SendMessage(g_hWndSlider, TBM_GETPOS, 0, 0);
            g_cps = pos;
            updateLabel(g_hWndLabel, pos);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);

    const char CLASS_NAME[] = "AutoClickerWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "AutoClicker GUI",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 160,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    g_hWndSlider = CreateWindow(
        TRACKBAR_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        50, 30, 200, 30,
        hwnd, NULL, hInstance, NULL
    );
    SendMessage(g_hWndSlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 60));
    SendMessage(g_hWndSlider, TBM_SETPOS, TRUE, g_cps);
    SendMessage(g_hWndSlider, TBM_SETTICFREQ, 1, 0);

    g_hWndLabel = CreateWindow(
        "STATIC", "",
        WS_CHILD | WS_VISIBLE,
        110, 70, 100, 20,
        hwnd, NULL, hInstance, NULL
    );
    updateLabel(g_hWndLabel, g_cps);

    ShowWindow(hwnd, nCmdShow);

    std::thread clickThread(control);

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        g_pressed = (GetAsyncKeyState('F') & 0x8000);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

quit:
    g_pressed = false;
    clickThread.detach();
    return 0;
}
