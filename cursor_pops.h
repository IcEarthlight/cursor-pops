#pragma once
#include <windows.h>
#include <string>

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

// Core function that runs the cursor pops window
int RunCursorPops(const CommandLineArgs& args); 