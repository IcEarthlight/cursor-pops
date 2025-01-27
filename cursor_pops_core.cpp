#include "cursor_pops.h"
#include <vector>

// window class name
const wchar_t* CLASS_NAME = L"CursorPopsClass";

// global variables for window state
POINT lastCursorPos = {0, 0};
ULONGLONG showStartTime = 0;
bool followCursor = true;
bool useOutline = false;
Color outlineColor(0, 0, 0);
Color textColor(255, 255, 255);
bool useEaseOut = false;
BYTE currentAlpha = 255;
DWORD easeOutDuration = 1000;
DWORD displayDuration = 3000;
Velocity textVelocity(0, 0);
float currentOffsetX = 0;
float currentOffsetY = 0;

// helper functions
SIZE GetTextDimensions(HDC hdc, const wchar_t* text) {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    SIZE textSize;
    GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &textSize);
    
    SelectObject(hdc, hOldFont);
    return textSize;
}

DWORD GetMonitorRefreshRate(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int refreshRate = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(hwnd, hdc);
    
    return (refreshRate <= 0) ? 60 : (DWORD)refreshRate;
}

void UpdateWindowPosition(HWND hwnd) {
    static bool initialPositionSet = false;
    static POINT initialCursorPos = {0, 0};
    
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if (!initialPositionSet) {
        initialCursorPos = cursorPos;
        initialPositionSet = true;
    }

    if (textVelocity.x != 0 || textVelocity.y != 0) {
        ULONGLONG elapsedTime = GetTickCount64() - showStartTime;
        float seconds = elapsedTime / 1000.0f;
        currentOffsetX = textVelocity.x * seconds;
        currentOffsetY = textVelocity.y * seconds;
    }

    HDC hdc = GetDC(hwnd);
    const wchar_t* text = (const wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    SIZE textSize = GetTextDimensions(hdc, text);
    ReleaseDC(hwnd, hdc);

    int xPos, yPos;
    if (!followCursor && initialPositionSet) {
        xPos = initialCursorPos.x - (textSize.cx / 2) + (int)currentOffsetX;
        yPos = initialCursorPos.y - textSize.cy - 20 + (int)currentOffsetY;
    } else {
        xPos = cursorPos.x - (textSize.cx / 2) + (int)currentOffsetX;
        yPos = cursorPos.y - textSize.cy - 20 + (int)currentOffsetY;
    }

    SetWindowPos(hwnd, HWND_TOPMOST,
        xPos, yPos,
        textSize.cx + 40,
        textSize.cy + 20,
        SWP_NOACTIVATE | SWP_NOZORDER);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        const wchar_t* text = (const wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        SetBkMode(hdc, TRANSPARENT);

        if (useOutline) {
            SetTextColor(hdc, RGB(outlineColor.r, outlineColor.g, outlineColor.b));
            for(int i = -1; i <= 1; i++) {
                for(int j = -1; j <= 1; j++) {
                    RECT shadowRect = rect;
                    OffsetRect(&shadowRect, i, j);
                    DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
        }
        
        SetTextColor(hdc, RGB(textColor.r, textColor.g, textColor.b));
        DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        ULONGLONG elapsedTime = GetTickCount64() - showStartTime;
        
        if (elapsedTime > displayDuration) {
            if (!useEaseOut) {
                DestroyWindow(hwnd);
                return 0;
            }
            if (elapsedTime > displayDuration + easeOutDuration) {
                DestroyWindow(hwnd);
                return 0;
            }
            currentAlpha = (BYTE)(255 * (displayDuration + easeOutDuration - elapsedTime) / easeOutDuration);
            SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), currentAlpha, LWA_COLORKEY | LWA_ALPHA);
            InvalidateRect(hwnd, NULL, TRUE);
        }

        UpdateWindowPosition(hwnd);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int RunCursorPops(const CommandLineArgs& args) {
    // initialize global state from args
    followCursor = args.follow;
    useOutline = args.outline;
    outlineColor = args.outlineColor;
    textColor = args.textColor;
    useEaseOut = args.ease;
    easeOutDuration = args.easeOutDuration;
    displayDuration = args.displayDuration;
    textVelocity = args.velocity;

    // register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // get initial text dimensions
    HDC hdc = GetDC(NULL);
    SIZE textSize = GetTextDimensions(hdc, args.text.c_str());
    ReleaseDC(NULL, hdc);

    // create window
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Cursor Pops",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        textSize.cx + 40,
        textSize.cy + 20,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // setup window appearance
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 255, LWA_COLORKEY);

    // create and set window region
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    HRGN region = CreateRectRgn(0, 0, clientRect.right, clientRect.bottom);
    SetWindowRgn(hwnd, region, TRUE);
    DeleteObject(region);

    // show window and start timer
    ShowWindow(hwnd, SW_SHOW);
    DWORD refreshRate = GetMonitorRefreshRate(hwnd);
    DWORD timerInterval = 1000 / refreshRate;
    SetTimer(hwnd, 1, timerInterval, NULL);
    showStartTime = GetTickCount64();

    // store text as window data
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)args.text.c_str());

    // message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
} 