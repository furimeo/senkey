# SenKey

Bộ gõ tiếng Việt độc lập dành cho Linux.  
Kiến trúc luồng xử lý kênh đơn FIFO tầng nhân kết hợp giữa `evdev` và `uinput`.

## Yêu cầu hệ thống

- Nhân Linux 2.6 trở lên hỗ trợ `evdev` và `uinput`
- Trình biên dịch C++17 (`g++` hoặc `clang++`)
- `cmake` (phiên bản >= 3.16)
- Thư viện giao diện (tùy chọn cho bảng điều khiển `senkey-gui`): `libgtk-3-dev`, `libglib2.0-dev-bin`

## Hướng dẫn biên dịch

Sử dụng trực tiếp CMake:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Hoặc sử dụng Makefile viết sẵn:
```bash
make
```

Các tệp thực thi đầu ra:
- `senkey` (tiến trình nền daemon xử lý gõ phím)
- `senkey-gui` (bảng điều khiển đồ họa GTK3)

## Kiểm thử & Xác minh bộ nhớ

Chạy toàn bộ các bài kiểm tra đơn vị và đường truyền gõ phím:
```bash
make test
```

Biên dịch và kiểm thử với AddressSanitizer và UndefinedBehaviorSanitizer (kiểm tra rò rỉ bộ nhớ và tràn mảng):
```bash
make asan
```

## Cài đặt vào hệ thống

Chạy tệp cài đặt tự động với quyền quản trị:
```bash
sudo ./install.sh
```

Quá trình cài đặt sẽ thiết lập:
- Tệp thực thi tại `/usr/local/bin/senkey` và `/usr/local/bin/senkey-gui`
- Lối tắt ứng dụng tại `/usr/share/applications/senkey.desktop`
- Quy tắc cấp quyền uinput tại `/etc/udev/rules.d/99-uinput.rules`
- Dịch vụ người dùng systemd tại `~/.config/systemd/user/senkey.service`

## Hướng dẫn sử dụng

```bash
# Chạy trực tiếp ở chế độ dòng lệnh
senkey

# Chạy dưới dạng tiến trình nền daemon
senkey -d

# Mở bảng điều khiển giao diện đồ họa
senkey -g

# Chuyển đổi qua lại giữa chế độ Tiếng Việt [V] và Tiếng Anh [E]
senkey -t

# Kiểm tra trạng thái hoạt động hiện tại
senkey -s

# Dừng tiến trình daemon đang chạy
senkey -q
```

## Phím tắt điều khiển

- `Ctrl + Shift` hoặc `Alt + Z`: Chuyển đổi chế độ Tiếng Việt / Tiếng Anh
- Các phím điều hướng (`Trái`, `Phải`, `Lên`, `Xuống`, `Home`, `End`): Xác nhận từ và đặt lại bộ đệm gõ
- Nhấp chuột (chuột trái / chuột phải): Tự động đặt lại bộ đệm từ

## Cấu hình

Đường dẫn tệp cấu hình: `~/.config/senkey/senkey.conf`

```ini
# Cấu hình SenKey

input_method=telex
hotkey=ctrl_shift
modern_spelling=true
free_marking=true
spell_check=true
macro_enabled=true
micro_delay_us=1200
```

Tệp định nghĩa từ gõ tắt: `~/.config/senkey/macro.txt` (định dạng `từ_viết_tắt:từ_thay_thế`).

## Cấu trúc thư mục mã nguồn

```
.
├── CMakeLists.txt              # Cấu hình xây dựng CMake toàn dự án
├── Makefile                    # Lệnh bọc tiện ích (make, test, asan, install)
├── install.sh                  # Kịch bản cài đặt tự động
├── COPYING                     # Giấy phép GNU GPLv2
├── NOTICE                      # Thông cáo bản quyền và nguồn gốc linh kiện
├── README.md                   # Tài liệu hướng dẫn sử dụng
├── Source/                     # Mã nguồn chính của dự án SenKey
│   ├── Config.hpp / .cpp       # Quản lý nạp và lưu tệp cấu hình
│   ├── Emitter.hpp             # Phát sự kiện phím qua kênh ảo uinput
│   ├── EngineWrapper.hpp / .cpp# Lớp bao bọc lõi xử lý tiếng Việt
│   ├── Grabber.hpp             # Bắt trực tiếp thiết bị đầu vào từ evdev
│   ├── Hotplug.hpp / .cpp      # Giám sát cắm/rút bàn phím thời gian thực
│   ├── Ipc.hpp / .cpp          # Giao tiếp tiến trình qua Unix domain socket
│   ├── Keymap.hpp              # Bảng ánh xạ mã phím Linux sang ký tự
│   ├── Logger.hpp / .cpp       # Hệ thống ghi nhật ký an toàn đa luồng
│   ├── Macro.hpp / .cpp        # Quản lý bảng gõ tắt
│   ├── Main.cpp                # Điểm khởi chạy của tiến trình daemon
│   ├── MouseWatcher.hpp        # Lắng nghe sự kiện chuột để đặt lại đệm
│   ├── Types.hpp               # Định nghĩa kiểu dữ liệu và cấu trúc chung
│   └── GUI/                    # Giao diện đồ họa bảng điều khiển GTK3
│       ├── AboutDialog.hpp / .cpp      # Hộp thoại thông tin tác giả và bản quyền
│       ├── AdvancedSection.hpp / .cpp  # Khung tùy chọn cài đặt mở rộng
│       ├── BasicSection.hpp / .cpp     # Lưới chọn bảng mã và kiểu gõ cơ bản
│       ├── ButtonBar.hpp / .cpp        # Thanh nút bấm tác vụ
│       ├── MainWindow.hpp / .cpp       # Cửa sổ chính bảng điều khiển
│       ├── SilkIcons.hpp / .cpp        # Bộ nạp biểu tượng nhúng từ GResource
│       └── MainGui.cpp                 # Điểm khởi chạy ứng dụng đồ họa
├── Tests/                      # Bộ kiểm thử tự động
│   ├── CMakeLists.txt          # Cấu hình kiểm thử CTest
│   ├── EngineTest.cpp          # Kiểm thử giải thuật gõ Telex, VNI
│   └── PipelineMemoryTest.cpp  # Kiểm thử áp lực và kiểm tra an toàn bộ nhớ
├── UniKeyCore/                 # Lõi thuật toán gõ tiếng Việt UniKey
│   ├── COPYING                 # Giấy phép LGPL cho lõi UniKey
│   ├── ukengine.cpp / .h       # Thuật toán xử lý bỏ dấu tiếng Việt
│   └── ...
└── icons/                      # Bộ biểu tượng Silk Icons đóng gói GResource
```

## Tác giả & Giấy phép bản quyền

- **Tác giả SenKey**: Lê Hùng Quang Minh
- **Tác giả lõi gõ UniKeyCore**: Phạm Kim Long
- **Giấy phép phần mềm**: SenKey được phát hành theo giấy phép GNU General Public License (GPL-2.0-or-later). Mã nguồn phần lõi `UniKeyCore` tuân thủ giấy phép gốc của tác giả Phạm Kim Long kèm theo tệp `NOTICE`.
- **Bộ biểu tượng**: Silk Icons sáng tác bởi Mark James (FAMFAMFAM), phát hành theo giấy phép Creative Commons Attribution.
