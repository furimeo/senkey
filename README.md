# SenKey

Standalone Vietnamese input method daemon for Linux.  
Kernel-level `evdev` + `uinput` FIFO single-channel architecture.

## Requirements

- Linux 2.6+ (`evdev`, `uinput`)
- C++17 compiler (`g++` or `clang++`)
- `cmake` (>= 3.16)
- Optional: `libgtk-3-dev` (for `senkey-gui`)

## Building

Using CMake directly:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Or using Makefile wrapper:
```bash
make
```

Output binaries:
- `senkey` (standalone daemon)
- `senkey-gui` (GTK3 control panel)

## Testing & Memory Verification

Run unit tests and pipeline stress test:
```bash
make test
```

Run with AddressSanitizer and UndefinedBehaviorSanitizer (leak & bounds check):
```bash
make asan
```

## Installation

```bash
sudo ./install.sh
```

This installs:
- Binaries to `/usr/local/bin/senkey` and `/usr/local/bin/senkey-gui`
- Desktop launcher `/usr/share/applications/senkey.desktop`
- Udev rule `/etc/udev/rules.d/99-uinput.rules`
- Systemd user service `~/.config/systemd/user/senkey.service`

## Usage

```bash
# Run in foreground
senkey

# Run as background daemon
senkey -d

# Open control panel GUI
senkey -g

# Toggle mode between Vietnamese [V] and English [E]
senkey -t

# Check current mode
senkey -s

# Stop running daemon
senkey -q
```

## Shortcuts

- `Ctrl + Shift` or `Alt + Z`: Toggle Vietnamese / English mode
- Navigation keys (`Left`, `Right`, `Up`, `Down`, `Home`, `End`): Commit word and reset buffer
- Mouse click: Reset buffer

## Configuration

File location: `~/.config/senkey/senkey.conf`

```ini
# senkey configuration

input_method=telex
hotkey=ctrl_shift
modern_spelling=true
free_marking=true
spell_check=true
macro_enabled=true
micro_delay_us=1200
```

Macros file: `~/.config/senkey/macro.txt` (format `key:replacement`)

## Project Structure

```
.
├── CMakeLists.txt
├── Makefile
├── install.sh
├── README.md
├── Source/
│   ├── Config.hpp / Config.cpp
│   ├── Emitter.hpp
│   ├── EngineWrapper.hpp / EngineWrapper.cpp
│   ├── Grabber.hpp
│   ├── Hotplug.hpp / Hotplug.cpp
│   ├── Ipc.hpp / Ipc.cpp
│   ├── Keymap.hpp
│   ├── Logger.hpp / Logger.cpp
│   ├── Macro.hpp / Macro.cpp
│   ├── Main.cpp
│   ├── MouseWatcher.hpp
│   ├── Types.hpp
│   └── GUI/
│       ├── AboutDialog.hpp / AboutDialog.cpp
│       ├── AdvancedSection.hpp / AdvancedSection.cpp
│       ├── BasicSection.hpp / BasicSection.cpp
│       ├── ButtonBar.hpp / ButtonBar.cpp
│       ├── MainWindow.hpp / MainWindow.cpp
│       ├── SilkIcons.hpp / SilkIcons.cpp
│       └── MainGui.cpp
├── Tests/
│   ├── CMakeLists.txt
│   ├── EngineTest.cpp
│   └── PipelineMemoryTest.cpp
├── UniKeyCore/
│   ├── byteio.cpp / byteio.h
│   ├── charset.cpp / charset.h
│   ├── convert.cpp
│   ├── data.cpp / data.h
│   ├── inputproc.cpp / inputproc.h
│   ├── keycons.h
│   ├── mactab.cpp / mactab.h
│   ├── pattern.cpp / pattern.h
│   ├── ukengine.cpp / ukengine.h
│   └── vnconv.h
└── icons/

## Credits & License

- **Tác giả SenKey**: Lê Hùng Quang Minh
- **Tác giả lõi gõ UniKeyCore**: Phạm Kim Long
- **Giấy phép**: SenKey được phát hành theo giấy phép GNU GPLv3. Phần mã nguồn UniKeyCore tuân theo giấy phép gốc của tác giả Phạm Kim Long kèm theo tệp `NOTICE`.
- **Icons**: Silk Icons bởi Mark James (FAMFAMFAM) theo Creative Commons Attribution.

```
