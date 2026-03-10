# PhoneFinder

This folder is the C++ implementation of PhoneFinder:

- `PhoneFinderCLI`: CLI/core engine validation
- `PhoneFinder`: cross-platform Qt Widgets GUI (macOS/Windows/Linux with Qt5)

## Core features completed

- CSV data loading (`科室/细分/电话`)
- Missing `首字母/全拼` auto-derive for CSV templates
- Search scoring and ranking (initials/full_pinyin/name/phone)
- Hotkey rule normalization and validation
- Settings load/save (JSON-lite)

## Build

### macOS prerequisites (for cross-build to Windows)

When using this Mac to cross-build Windows packages, install these tools first:

```bash
# base build tools
brew install cmake ninja mingw-w64

# NSIS (optional on macOS; may crash in some environments, see notes below)
brew install --cask nsis

# Python tool for downloading Windows Qt
python3 -m pip install --upgrade aqtinstall
```

Install Windows Qt runtime/toolchain payload (used by `package_qt_cross_macos.sh`):

```bash
sudo mkdir -p /opt/qt-win
aqt install-qt windows desktop 5.15.2 win64_mingw81 -O /opt/qt-win
```

Expected Qt prefix for this project:

```bash
/opt/qt-win/5.15.2/mingw81_64
```

Quick self-check:

```bash
test -f /opt/qt-win/5.15.2/mingw81_64/bin/Qt5Core.dll && echo "Qt payload OK"
command -v x86_64-w64-mingw32-g++ && echo "MinGW OK"
command -v cmake && echo "CMake OK"
```

### macOS/Linux (core CLI)

```bash
cd PhoneFinder
cmake -S . -B build
cmake --build build
./build/PhoneFinderCLI repl samples/医院科室电话数据模板_demo.csv
./build/PhoneFinderCLI lookup samples/医院科室电话数据模板_demo.csv RSK
```

### macOS Qt GUI (test now)

```bash
cd PhoneFinder
cmake -S . -B build-qt -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
cmake --build build-qt --target phonefinder_qt
./build-qt/PhoneFinder
```

Qt GUI parity updates:
- Finder/Settings two-page flow aligned with Go UI.
- Keyboard buffer search (letters for dept, digits for phone reverse lookup).
- Auto-hide finder on window deactivate (with file picker guard).
- Windows Qt build supports native global hotkey toggle (`RegisterHotKey`, including `*2` double-tap detection).

### Windows (Qt)

Use Visual Studio Developer Prompt:

```bat
cd PhoneFinder
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target phonefinder_qt
```

Output binary:
- `build\\Release\\PhoneFinder.exe`

### Windows Qt Installer (UI parity with macOS)

Package `PhoneFinder.exe`:

```bat
cd PhoneFinder
scripts\package_qt_windows.bat
```

PowerShell alternative (Win11 VM):

```powershell
cd PhoneFinder
powershell -ExecutionPolicy Bypass -File .\scripts\win11_build_qt_installer.ps1
```

### macOS Cross Packaging (Qt -> Windows)

If you must package Windows Qt app from macOS only:

```bash
cd PhoneFinder
QT_WIN_PREFIX=/opt/qt-win/5.15.2/mingw81_64 ./scripts/package_qt_cross_macos.sh
```

Result:
- Stage directory (runnable on Windows): `PhoneFinder/build-win-qt/stage`
- Installer (if `makensis` succeeds): `PhoneFinder/dist/PhoneFinder-installer.exe`

Generated installer:
- `PhoneFinder\\dist\\PhoneFinder-installer.exe`

Notes:
- This flow runs on macOS cross toolchain (`mingw-w64` + Windows Qt payload via `aqt`).
- On macOS, `makensis` may fail with `std::bad_alloc` in some environments. If this happens, use the generated `build-win-qt/stage` directory directly on Windows, or build the installer in a Windows VM.


## Notes

- Hotkey `*2` is normalized/validated by rules; registration currently maps to base key for OS hotkey API.
- `PhoneFinder` currently focuses on cross-platform UI and core search/settings behavior.
