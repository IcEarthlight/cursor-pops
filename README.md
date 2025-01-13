# CursorPops

A lightweight utility that displays customizable pop-up text near your cursor. Perfect for visual feedback, notifications, or creating engaging UI effects. Currently implemented for Windows, with potential for other platforms in the future.

## Features

- Display text near the cursor with customizable duration
- Static or cursor-following text
- Smooth fade-out effects
- Text outline/shadow effects
- Custom text and outline colors
- Velocity-based text movement
- Non-blocking operation by default
- Easy integration with other applications through simple command-line interface
- Minimal dependencies for straightforward embedding

## Integration

CursorPops is designed to be easily integrated into other applications:

```cpp
// C++ example using CreateProcess
STARTUPINFOW si = { sizeof(si) };
PROCESS_INFORMATION pi;
CreateProcessW(NULL, L"CursorPops.exe \"Your Text\" -c #FF0000", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

// Remember to close handles if you need to track the process
CloseHandle(pi.hProcess);
CloseHandle(pi.hThread);
```

```python
# Python example
import subprocess
subprocess.Popen(['CursorPops.exe', 'Your Text', '-c', '#FF0000'])
```

```javascript
// Node.js example
const { spawn } = require('child_process');
spawn('CursorPops.exe', ['Your Text', '-c', '#FF0000']);
```

## Usage

```bash
CursorPops.exe [options] <text>
```

### Options

- `-h, --help`: Show help message
- `-t, --text <text>`: Text to display
- `-f, --follow`: Follow cursor (default: static)
- `-b, --block`: Run in blocking mode
- `-d, --duration <sec>`: Display duration in seconds (default: 3)
- `-e, --ease <seconds>`: Enable fade out effect (optional duration, default: 1s)
- `-o, --outline <color>`: Add outline effect (optional color)
- `-c, --color <color>`: Set text color (default: white)
- `-v, --velocity <x,y>`: Movement velocity in pixels/second (can be single number for vertical movement only)

### Color Formats

Colors for `-o` and `-c` can be specified in these formats:
- Hex: `#RRGGBB`
- RGB integers: `R,G,B` (0-255)
- RGB floats: `R.R,G.G,B.B` (0.0-1.0)

Note: `#FF00FF` (magenta) will be adjusted slightly as it's used for transparency.

### Examples

```bash
# Simple text display
CursorPops.exe "Hello World"

# Rising text with blue color
CursorPops.exe -v 20 -c "#0000FF" "Rising"

# Moving text with custom duration and fade
CursorPops.exe -v 50,0 -d 0.5 -e 2 "Moving"

# Floating text with outline
CursorPops.exe -t "Float" -v 0,-2 -o "#000000" -c "#FFFFFF"
```

## Building

The project requires:
- Windows OS
- C++ 17 compiler
- Windows SDK

## Technical Details

- Uses Windows API for lightweight window management
- Supports transparency and layered windows
- Monitor refresh rate aware for smooth animations
- Efficient memory usage with minimal dependencies
