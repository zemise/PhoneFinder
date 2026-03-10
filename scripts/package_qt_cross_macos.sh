#!/usr/bin/env bash
set -euo pipefail

# Cross-package Windows Qt app from macOS.
# Output:
#   - EXE + runtime stage dir:   PhoneFinder/build-win-qt/stage
#   - NSIS installer (if works): PhoneFinder/dist/PhoneFinder-installer.exe
#
# Environment variables:
#   QT_WIN_PREFIX   Windows Qt root (contains bin/Qt5*.dll), e.g.
#                   /opt/qt-win/5.15.2/win64_mingw81_64
#   BUILD_DIR       Optional build dir, default: PhoneFinder/build-win-qt
#   STAGE_DIR       Optional stage dir, default: <BUILD_DIR>/stage
#   DIST_DIR        Optional output dir, default: PhoneFinder/dist

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TOOLCHAIN_FILE="${ROOT_DIR}/cmake/toolchains/mingw64.cmake"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-win-qt}"
STAGE_DIR="${STAGE_DIR:-${BUILD_DIR}/stage}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist}"
NSI_FILE="${ROOT_DIR}/packaging/qt_installer.nsi"
INSTALLER_OUT="${DIST_DIR}/PhoneFinder-installer.exe"

die() {
  echo "[ERROR] $*" >&2
  exit 1
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "missing command: $1"
}

autodetect_qt_prefix() {
  local candidates=(
    "/opt/qt-win"
    "${HOME}/Qt"
    "/usr/local/qt-win"
  )
  local d
  for base in "${candidates[@]}"; do
    [[ -d "$base" ]] || continue
    # Pick newest Windows MinGW Qt folder under base.
    d="$(find "$base" -type d \( -name 'win64_mingw*' -o -name 'mingw*_64' \) 2>/dev/null | sort | tail -n 1 || true)"
    if [[ -n "${d}" ]]; then
      echo "$d"
      return 0
    fi
  done
  return 1
}

copy_if_exists() {
  local src="$1"
  local dst="$2"
  if [[ -f "$src" ]]; then
    cp -f "$src" "$dst"
  fi
}

echo "==> Check toolchain..."
require_cmd cmake
require_cmd x86_64-w64-mingw32-g++
GENERATOR="Ninja"
if ! command -v ninja >/dev/null 2>&1; then
  GENERATOR="Unix Makefiles"
fi

if [[ -z "${QT_WIN_PREFIX:-}" ]]; then
  QT_WIN_PREFIX="$(autodetect_qt_prefix || true)"
fi
[[ -n "${QT_WIN_PREFIX:-}" ]] || die "QT_WIN_PREFIX is not set and auto-detect failed"
[[ -d "${QT_WIN_PREFIX}" ]] || die "QT_WIN_PREFIX does not exist: ${QT_WIN_PREFIX}"
[[ -f "${QT_WIN_PREFIX}/bin/Qt5Core.dll" ]] || die "Qt5Core.dll not found in ${QT_WIN_PREFIX}/bin"
[[ -f "${QT_WIN_PREFIX}/lib/cmake/Qt5/Qt5Config.cmake" ]] || die "Qt5Config.cmake not found in ${QT_WIN_PREFIX}/lib/cmake/Qt5"
[[ -f "${TOOLCHAIN_FILE}" ]] || die "toolchain file missing: ${TOOLCHAIN_FILE}"

echo "==> ROOT_DIR       : ${ROOT_DIR}"
echo "==> QT_WIN_PREFIX  : ${QT_WIN_PREFIX}"
echo "==> BUILD_DIR      : ${BUILD_DIR}"
echo "==> STAGE_DIR      : ${STAGE_DIR}"
echo "==> DIST_DIR       : ${DIST_DIR}"

mkdir -p "${BUILD_DIR}" "${STAGE_DIR}" "${DIST_DIR}"
rm -f "${BUILD_DIR}/CMakeCache.txt"

echo "==> Configure CMake (cross MinGW + Qt5)..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_WIN_PREFIX}" \
  -DQt5Core_DIR="${QT_WIN_PREFIX}/lib/cmake/Qt5Core" \
  -DQt5Gui_DIR="${QT_WIN_PREFIX}/lib/cmake/Qt5Gui" \
  -DQt5Widgets_DIR="${QT_WIN_PREFIX}/lib/cmake/Qt5Widgets" \
  -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF \
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF \
  -DCMAKE_FIND_ROOT_PATH="/opt/homebrew;${QT_WIN_PREFIX}" \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH

echo "==> Build PhoneFinder.exe..."
cmake --build "${BUILD_DIR}" --target phonefinder_qt -j 8

EXE_PATH="${BUILD_DIR}/PhoneFinder.exe"
[[ -f "${EXE_PATH}" ]] || die "build finished but missing ${EXE_PATH}"

echo "==> Stage runtime files..."
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}" "${STAGE_DIR}/platforms"

cp -f "${EXE_PATH}" "${STAGE_DIR}/PhoneFinder.exe"

# Project sample data for first-run convenience.
if [[ -f "${ROOT_DIR}/samples/医院科室电话数据模板_demo.csv" ]]; then
  cp -f "${ROOT_DIR}/samples/医院科室电话数据模板_demo.csv" "${STAGE_DIR}/"
fi
if [[ -f "${ROOT_DIR}/resources/windows/icon.ico" ]]; then
  cp -f "${ROOT_DIR}/resources/windows/icon.ico" "${STAGE_DIR}/icon.ico"
fi
if [[ -f "${ROOT_DIR}/resources/trayicon.png" ]]; then
  cp -f "${ROOT_DIR}/resources/trayicon.png" "${STAGE_DIR}/trayicon.png"
fi

# Qt runtime dlls.
copy_if_exists "${QT_WIN_PREFIX}/bin/Qt5Core.dll" "${STAGE_DIR}/"
copy_if_exists "${QT_WIN_PREFIX}/bin/Qt5Gui.dll" "${STAGE_DIR}/"
copy_if_exists "${QT_WIN_PREFIX}/bin/Qt5Widgets.dll" "${STAGE_DIR}/"

# Qt platform plugin.
copy_if_exists "${QT_WIN_PREFIX}/plugins/platforms/qwindows.dll" "${STAGE_DIR}/platforms/"

# Optional style plugin.
if [[ -f "${QT_WIN_PREFIX}/plugins/styles/qwindowsvistastyle.dll" ]]; then
  mkdir -p "${STAGE_DIR}/styles"
  cp -f "${QT_WIN_PREFIX}/plugins/styles/qwindowsvistastyle.dll" "${STAGE_DIR}/styles/"
fi

# MinGW runtime dlls.
cp -f "$(x86_64-w64-mingw32-g++ -print-file-name=libgcc_s_seh-1.dll)" "${STAGE_DIR}/"
cp -f "$(x86_64-w64-mingw32-g++ -print-file-name=libstdc++-6.dll)" "${STAGE_DIR}/"
if [[ -f "/opt/homebrew/Cellar/mingw-w64/13.0.0_2/toolchain-x86_64/x86_64-w64-mingw32/bin/libwinpthread-1.dll" ]]; then
  cp -f "/opt/homebrew/Cellar/mingw-w64/13.0.0_2/toolchain-x86_64/x86_64-w64-mingw32/bin/libwinpthread-1.dll" "${STAGE_DIR}/"
else
  # Fallback search if version changes.
  WINPTHREAD="$(find /opt/homebrew/Cellar/mingw-w64 -path '*x86_64-w64-mingw32/bin/libwinpthread-1.dll' | sort | tail -n 1 || true)"
  [[ -n "${WINPTHREAD}" ]] || die "libwinpthread-1.dll not found"
  cp -f "${WINPTHREAD}" "${STAGE_DIR}/"
fi

# Ensure plugin lookup from exe directory.
cat > "${STAGE_DIR}/qt.conf" <<'EOF'
[Paths]
Plugins=platforms
EOF

echo "==> Stage ready: ${STAGE_DIR}"
ls -1 "${STAGE_DIR}"

if command -v makensis >/dev/null 2>&1; then
  echo "==> Build NSIS installer..."
  if makensis -DAPP_STAGE_DIR="${STAGE_DIR}" -DAPP_OUTFILE="${INSTALLER_OUT}" "${NSI_FILE}"; then
    echo "==> Installer built: ${INSTALLER_OUT}"
  else
    echo "[WARN] makensis failed on this macOS environment."
    echo "[WARN] You can still run stage dir directly on Windows: ${STAGE_DIR}"
  fi
else
  echo "[WARN] makensis not found, skip installer build."
  echo "[WARN] Stage dir is ready: ${STAGE_DIR}"
fi

echo "Done."
