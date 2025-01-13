#include <windows.h>
#include <string>
#include <vector>

struct Color {
    BYTE r, g, b;
    Color(BYTE r_ = 0, BYTE g_ = 0, BYTE b_ = 0) : r(r_), g(g_), b(b_) {}
};

struct Velocity {
    float x, y;
    Velocity(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
};

struct CommandLineArgs {
    std::wstring text;
    bool follow;            // will be true if -f/--follow is present
    bool block;             // will be true if -b/--block is present
    bool outline;           // will be true if -o/--outline is present
    bool ease;              // ease out effect flag
    DWORD easeOutDuration;  // duration in milliseconds
    DWORD displayDuration;  // duration in milliseconds before easing/disappearing
    Color outlineColor;     // default will be black (0,0,0)
    Color textColor;        // text color
    Velocity velocity;      // velocity for movement
};

// Window class name
const wchar_t* CLASS_NAME = L"CursorPopsClass";
// Global variables
POINT lastCursorPos = {0, 0};
ULONGLONG showStartTime = 0;
const DWORD DISPLAY_DURATION_MS = 3000;  // text will show for 3 seconds by default
bool followCursor = true;                // default behavior
bool useOutline = false;
Color outlineColor = Color(0, 0, 0);     // default outline color
Color textColor = Color(255, 255, 255);  // default white text
bool useEaseOut = false;
BYTE currentAlpha = 255;                 // for tracking current transparency
DWORD easeOutDuration = 1000;            // default 1 second
DWORD displayDuration = 3000;            // default 3 seconds
Velocity textVelocity(0, 0);
float currentOffsetX = 0;
float currentOffsetY = 0;

// forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateWindowPosition(HWND hwnd);
void ShowUsage();

// color parsing helper functions
Color ParseHexColor(const std::wstring& hex) {
    // femove # if present
    std::wstring cleanHex = hex;
    if (cleanHex[0] == L'#') {
        cleanHex = cleanHex.substr(1);
    }

    // validate hex string length
    if (cleanHex.length() != 6) {
        return Color(); // default black
    }

    // validate hex characters
    for (wchar_t c : cleanHex) {
        if (!iswxdigit(c)) {
            return Color(); // default black
        }
    }

    // convert to uppercase for consistency
    for (wchar_t& c : cleanHex) {
        c = towupper(c);
    }
    
    // parse each color component separately to avoid overflow
    BYTE r = (BYTE)wcstoul(cleanHex.substr(0,2).c_str(), nullptr, 16);
    BYTE g = (BYTE)wcstoul(cleanHex.substr(2,2).c_str(), nullptr, 16);
    BYTE b = (BYTE)wcstoul(cleanHex.substr(4,2).c_str(), nullptr, 16);

    return Color(r, g, b);
}

Color ParseRGBColor(const std::wstring& rgb) {
    size_t pos1 = rgb.find(L',');
    size_t pos2 = rgb.find(L',', pos1 + 1);
    
    if (pos1 == std::wstring::npos || pos2 == std::wstring::npos) {
        return Color();  // default black
    }

    std::wstring r = rgb.substr(0, pos1);
    std::wstring g = rgb.substr(pos1 + 1, pos2 - pos1 - 1);
    std::wstring b = rgb.substr(pos2 + 1);

    // check if values are in 0-1 range (float)
    if (r.find(L'.') != std::wstring::npos) {
        return Color(
            (BYTE)(std::stof(r) * 255),
            (BYTE)(std::stof(g) * 255),
            (BYTE)(std::stof(b) * 255)
        );
    }

    // integer values
    return Color(
        (BYTE)std::stoi(r),
        (BYTE)std::stoi(g),
        (BYTE)std::stoi(b)
    );
}

Color ParseColor(const std::wstring& colorStr) {
    try {
        if (colorStr.empty()) {
            return Color();  // default black
        }
        if (colorStr[0] == L'#') {
            return ParseHexColor(colorStr);
        }
        return ParseRGBColor(colorStr);
    }
    catch (...) {
        return Color();  // default black on any error
    }
}

Color AdjustColorForVisibility(const Color& color) {
    // if color matches our transparent color (magenta), adjust it slightly
    if (color.r == 255 && color.g == 0 && color.b == 255) {
        return Color(255, 0, 254);  // slightly different magenta
    }
    return color;
}

// velocity parsing helper function
Velocity ParseVelocity(const std::wstring& velStr) {
    try {
        size_t commaPos = velStr.find(L',');
        if (commaPos == std::wstring::npos) {
            // only Y velocity provided
            float y = std::stof(velStr);
            return Velocity(0, -y);  // negative Y in Windows means up
        } else {
            // both X and Y velocities provided
            std::wstring xStr = velStr.substr(0, commaPos);
            std::wstring yStr = velStr.substr(commaPos + 1);
            float x = std::stof(xStr);
            float y = std::stof(yStr);
            return Velocity(x, y);
        }
    } catch (...) {
        return Velocity();  // default to no movement
    }
}

CommandLineArgs ParseCommandLine(const std::wstring& cmdLine) {
    std::vector<std::wstring> args;
    std::wstring current;
    bool inQuotes = false;

    // split command line into args
    for (size_t i = 0; i < cmdLine.length(); i++) {
        if (cmdLine[i] == L'"') {
            inQuotes = !inQuotes;
        } else if (cmdLine[i] == L' ' && !inQuotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += cmdLine[i];
        }
    }
    if (!current.empty()) {
        args.push_back(current);
    }

    // show help if no args or help flag is present
    if (args.empty() || 
        std::find(args.begin(), args.end(), L"-h") != args.end() ||
        std::find(args.begin(), args.end(), L"--help") != args.end()) {
        ShowUsage();
        ExitProcess(0);
    }

    // default values
    CommandLineArgs result = {L"Sample Text", false, false, false, false, 1000, 3000,
        Color(), Color(255, 255, 255), Velocity()};

    // parse args
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == L"-f" || args[i] == L"--follow") {
            result.follow = true;
        } else if (args[i] == L"-v" || args[i] == L"--velocity") {
            if (i + 1 < args.size()) {
                // Always treat the next argument as velocity value, even if it starts with '-'
                result.velocity = ParseVelocity(args[i + 1]);
                i++;
            }
        } else if (args[i] == L"-t" || args[i] == L"--text") {
            if (i + 1 < args.size()) {
                result.text = args[i + 1];
                i++;
            }
        } else if (args[i] == L"-o" || args[i] == L"--outline") {
            result.outline = true;
            if (i + 1 < args.size() && args[i + 1][0] != L'-') {
                result.outlineColor = AdjustColorForVisibility(ParseColor(args[i + 1]));
                i++;
            }
        } else if (args[i] == L"-c" || args[i] == L"--color") {
            if (i + 1 < args.size() && args[i + 1][0] != L'-') {
                result.textColor = AdjustColorForVisibility(ParseColor(args[i + 1]));
                i++;
            }
        } else if (args[i] == L"-d" || args[i] == L"--duration") {
            if (i + 1 < args.size() && args[i + 1][0] != L'-') {
                try {
                    double seconds = std::stod(args[i + 1]);
                    result.displayDuration = (DWORD)(seconds * 1000);
                    i++;
                } catch (...) {}
            }
        } else if (args[i] == L"-e" || args[i] == L"--ease") {
            result.ease = true;
            if (i + 1 < args.size() && args[i + 1][0] != L'-') {
                try {
                    double seconds = std::stod(args[i + 1]);
                    result.easeOutDuration = (DWORD)(seconds * 1000);
                    i++;
                } catch (...) {}
            }
        } else if (args[i] == L"-b" || args[i] == L"--block") {
            result.block = true;
        } else if (!args[i].empty() && args[i][0] != L'-') {
            result.text = args[i];
        }
    }

    return result;
}

DWORD GetMonitorRefreshRate(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int refreshRate = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(hwnd, hdc);
    
    // If we couldn't get the refresh rate or it's invalid, default to 60
    if (refreshRate <= 0) {
        return 60;
    }
    return (DWORD)refreshRate;
}

// Add this helper function to calculate the window size
SIZE GetTextDimensions(HDC hdc, const wchar_t* text) {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    SIZE textSize;
    GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &textSize);
    
    SelectObject(hdc, hOldFont);
    return textSize;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // parse command line
    CommandLineArgs args = ParseCommandLine(pCmdLine);
    followCursor = args.follow;
    useOutline = args.outline;
    outlineColor = args.outlineColor;
    textColor = args.textColor;
    useEaseOut = args.ease;
    easeOutDuration = args.easeOutDuration;
    displayDuration = args.displayDuration;
    textVelocity = args.velocity;

    // if non-blocking, create a new process
    if (!args.block) {
        WCHAR cmdLine[MAX_PATH * 2];
        WCHAR exePath[MAX_PATH];
        WCHAR easeStr[32] = L"";
        WCHAR outlineColorStr[32] = L"";
        WCHAR textColorStr[32] = L"";
        WCHAR durationStr[32] = L"";
        WCHAR velocityStr[32] = L"";
        
        GetModuleFileNameW(NULL, exePath, MAX_PATH);

        if (args.outline && !(args.outlineColor.r == 0 && args.outlineColor.g == 0 && args.outlineColor.b == 0)) {
            swprintf_s(outlineColorStr, L"-o %d,%d,%d", args.outlineColor.r, args.outlineColor.g, args.outlineColor.b);
        }
        
        if (args.textColor.r != 255 || args.textColor.g != 255 || args.textColor.b != 255) {
            swprintf_s(textColorStr, L"-c %d,%d,%d", args.textColor.r, args.textColor.g, args.textColor.b);
        }

        if (args.ease) {
            if (args.easeOutDuration != 1000) {  // if not default duration
                swprintf_s(easeStr, L"-e %.1f", args.easeOutDuration / 1000.0);
            } else {
                wcscpy_s(easeStr, L"-e");
            }
        }

        if (args.displayDuration != 3000) {  // if not default duration
            swprintf_s(durationStr, L"-d %.1f", args.displayDuration / 1000.0);
        }

        if (args.velocity.x != 0 || args.velocity.y != 0) {
            if (args.velocity.x == 0) {
                swprintf_s(velocityStr, L"-v %.1f", -args.velocity.y);
            } else {
                swprintf_s(velocityStr, L"-v %.1f,%.1f", args.velocity.x, args.velocity.y);
            }
        }

        swprintf_s(cmdLine, L"\"%s\" \"%s\" %s %s %s %s %s %s -b", 
            exePath, 
            args.text.c_str(),
            args.follow ? L"-f" : L"",
            args.outline ? (outlineColorStr[0] ? outlineColorStr : L"-o") : L"",
            textColorStr[0] ? textColorStr : L"",
            easeStr,
            durationStr[0] ? durationStr : L"",
            velocityStr[0] ? velocityStr : L"");

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
    }

    // register the window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // Get initial text dimensions for window size
    HDC hdc = GetDC(NULL);
    SIZE textSize = GetTextDimensions(hdc, args.text.c_str());
    ReleaseDC(NULL, hdc);

    // create the window with calculated size
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, // extended window style
        CLASS_NAME,                    // window class
        L"CursorPops",                  // window text
        WS_POPUP,                      // window style
        CW_USEDEFAULT, CW_USEDEFAULT,  // position
        textSize.cx + 40,              // paddinged width
        textSize.cy + 20,              // paddinged height
        NULL,                          // parent window    
        NULL,                          // menu
        hInstance,                     // instance handle
        NULL                           // additional data
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Make the background color transparent
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 255, LWA_COLORKEY);  // magenta will be transparent

    // create and set the region to make background transparent
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    HRGN region = CreateRectRgn(0, 0, clientRect.right, clientRect.bottom);
    SetWindowRgn(hwnd, region, TRUE);
    DeleteObject(region);

    // show the window
    ShowWindow(hwnd, nCmdShow);

    // get monitor refresh rate and calculate timer interval
    DWORD refreshRate = GetMonitorRefreshRate(hwnd);
    DWORD timerInterval = 1000 / refreshRate;  // convert refresh rate to milliseconds
    SetTimer(hwnd, 1, timerInterval, NULL);
    showStartTime = GetTickCount64();

    // store the text as window data
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)args.text.c_str());

    // message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // fill background with the transparent color
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // get the text from window data
        const wchar_t* text = (const wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        // set up text format
        SetBkMode(hdc, TRANSPARENT);

        if (useOutline) {
            // draw shadow/outline effect with current alpha
            SetTextColor(hdc, RGB(outlineColor.r, outlineColor.g, outlineColor.b));
            for(int i = -1; i <= 1; i++) {
                for(int j = -1; j <= 1; j++) {
                    RECT shadowRect = rect;
                    OffsetRect(&shadowRect, i, j);
                    DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
        }
        
        // draw main text with custom color
        SetTextColor(hdc, RGB(textColor.r, textColor.g, textColor.b));
        DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        ULONGLONG elapsedTime = GetTickCount64() - showStartTime;
        
        if (elapsedTime > displayDuration) {  // use displayDuration instead of DISPLAY_DURATION_MS
            if (!useEaseOut) {
                DestroyWindow(hwnd);
                return 0;
            }
            // handle easing out effect
            if (elapsedTime > displayDuration + easeOutDuration) {
                DestroyWindow(hwnd);
                return 0;
            }
            // calculate alpha value based on custom duration
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

void UpdateWindowPosition(HWND hwnd)
{
    static bool initialPositionSet = false;
    static POINT initialCursorPos = {0, 0};
    
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if (!initialPositionSet) {
        initialCursorPos = cursorPos;
        initialPositionSet = true;
    }

    // Update movement offsets based on velocity and time
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

void ShowUsage() {
    MessageBoxW(NULL, 
        L"Usage: CursorPops.exe <options> <text>\n\n"
        L"Options:\n"
        L"  -h, --help             Show this help message\n"
        L"  -t, --text <text>      Text to display\n"
        L"  -f, --follow           Follow cursor (default: static)\n"
        L"  -b, --block            Run in blocking mode\n"
        L"  -d, --duration <sec>   Display duration in seconds (default: 3)\n"
        L"  -e, --ease <seconds>   Enable fade out effect (optional duration, default: 1s)\n"
        L"  -o, --outline <color>  Add outline effect (optional color)\n"
        L"  -c, --color <color>    Set text color (default: white)\n"
        L"  -v, --velocity <x,y>   Movement velocity in pixels/second\n"
        L"                         Can be single number for vertical movement only\n"
        L"Color formats for -o and -c:\n"
        L"  #RRGGBB, R,G,B, or R.R,G.G,B.B\n"
        L"  Note: #FF00FF (magenta) will be adjusted slightly\n\n"
        L"Examples:\n"
        L"  CursorPops.exe \"Hello World\"\n"
        L"  CursorPops.exe \"Rising\" -v -1.5\n"
        L"  CursorPops.exe \"Moving\" -v 1.0,-1.0\n"
        L"  CursorPops.exe -t \"Float\" -v 0,-2 -d 5 -e 2",
        L"CursorPops Usage",
        MB_OK | MB_ICONINFORMATION);
}
