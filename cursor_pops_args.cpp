#include "cursor_pops.h"
#include <vector>
#include <algorithm>

void ShowUsage() {
    MessageBoxW(NULL, 
        L"Usage: cursor-pops.exe <options> <text>\n\n"
        L"Options:\n"
        L"  -h, --help             Show this help message\n"
        L"  -t, --text <text>      Text to display\n"
        L"  -f, --follow           Follow cursor (default: static)\n"
        L"  -b, --block           Run in blocking mode\n"
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
        L"  cursor-pops.exe \"Hello World\"\n"
        L"  cursor-pops.exe \"Rising\" -v -1.5\n"
        L"  cursor-pops.exe \"Moving\" -v 1.0,-1.0\n"
        L"  cursor-pops.exe -t \"Float\" -v 0,-2 -d 5 -e 2",
        L"CursorPops Usage",
        MB_OK | MB_ICONINFORMATION);
}

// color parsing helper functions
Color ParseHexColor(const std::wstring& hex) {
    std::wstring cleanHex = hex;
    if (cleanHex[0] == L'#') {
        cleanHex = cleanHex.substr(1);
    }

    if (cleanHex.length() != 6) {
        return Color();
    }

    for (wchar_t c : cleanHex) {
        if (!iswxdigit(c)) {
            return Color();
        }
    }

    for (wchar_t& c : cleanHex) {
        c = towupper(c);
    }
    
    BYTE r = (BYTE)wcstoul(cleanHex.substr(0,2).c_str(), nullptr, 16);
    BYTE g = (BYTE)wcstoul(cleanHex.substr(2,2).c_str(), nullptr, 16);
    BYTE b = (BYTE)wcstoul(cleanHex.substr(4,2).c_str(), nullptr, 16);

    return Color(r, g, b);
}

Color ParseRGBColor(const std::wstring& rgb) {
    size_t pos1 = rgb.find(L',');
    size_t pos2 = rgb.find(L',', pos1 + 1);
    
    if (pos1 == std::wstring::npos || pos2 == std::wstring::npos) {
        return Color();
    }

    std::wstring r = rgb.substr(0, pos1);
    std::wstring g = rgb.substr(pos1 + 1, pos2 - pos1 - 1);
    std::wstring b = rgb.substr(pos2 + 1);

    if (r.find(L'.') != std::wstring::npos) {
        return Color(
            (BYTE)(std::stof(r) * 255),
            (BYTE)(std::stof(g) * 255),
            (BYTE)(std::stof(b) * 255)
        );
    }

    return Color(
        (BYTE)std::stoi(r),
        (BYTE)std::stoi(g),
        (BYTE)std::stoi(b)
    );
}

Color ParseColor(const std::wstring& colorStr) {
    try {
        if (colorStr.empty()) {
            return Color();
        }
        if (colorStr[0] == L'#') {
            return ParseHexColor(colorStr);
        }
        return ParseRGBColor(colorStr);
    }
    catch (...) {
        return Color();
    }
}

Color AdjustColorForVisibility(const Color& color) {
    if (color.r == 255 && color.g == 0 && color.b == 255) {
        return Color(255, 0, 254);
    }
    return color;
}

Velocity ParseVelocity(const std::wstring& velStr) {
    try {
        size_t commaPos = velStr.find(L',');
        if (commaPos == std::wstring::npos) {
            float y = std::stof(velStr);
            return Velocity(0, -y);
        } else {
            std::wstring xStr = velStr.substr(0, commaPos);
            std::wstring yStr = velStr.substr(commaPos + 1);
            float x = std::stof(xStr);
            float y = std::stof(yStr);
            return Velocity(x, y);
        }
    } catch (...) {
        return Velocity();
    }
}

CommandLineArgs ParseCommandLine(const std::wstring& cmdLine) {
    std::vector<std::wstring> args;
    std::wstring current;
    bool inQuotes = false;

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

    if (args.empty() || 
        std::find(args.begin(), args.end(), L"-h") != args.end() ||
        std::find(args.begin(), args.end(), L"--help") != args.end()) {
        ShowUsage();
        ExitProcess(0);
    }

    CommandLineArgs result = {L"Sample Text", false, false, false, false, 1000, 3000,
        Color(), Color(255, 255, 255), Velocity()};

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == L"-f" || args[i] == L"--follow") {
            result.follow = true;
        } else if (args[i] == L"-v" || args[i] == L"--velocity") {
            if (i + 1 < args.size()) {
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    CommandLineArgs args = ParseCommandLine(pCmdLine);

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
            if (args.easeOutDuration != 1000) {
                swprintf_s(easeStr, L"-e %.1f", args.easeOutDuration / 1000.0);
            } else {
                wcscpy_s(easeStr, L"-e");
            }
        }

        if (args.displayDuration != 3000) {
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

    return RunCursorPops(args);
} 