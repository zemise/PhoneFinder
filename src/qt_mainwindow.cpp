#include "phonefinder/qt_mainwindow.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QInputMethod>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QShowEvent>
#include <QScreen>

#include "phonefinder/hotkey_rules.hpp"
#include "phonefinder/util.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

namespace phonefinder {

namespace {
namespace fs = std::filesystem;

QString triggerDisplay(const std::string& def) {
  QString out = QString::fromStdString(def);
  out.replace("+", " + ");
  out.replace("*2", " x2");
  return out;
}

QIcon loadTrayIconFromDisk() {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      appDir + "/trayicon.png",
      appDir + "/icon.ico",
      "build/trayicon.png",
      "build/windows/icon.ico",
  };
  for (const auto& p : candidates) {
    if (!QFileInfo::exists(p)) {
      continue;
    }
    QIcon icon(p);
    if (!icon.isNull()) {
      return icon;
    }
  }
  return {};
}

std::string resolveSourceForQt(const std::string& configuredPath) {
  const QString appDir = QCoreApplication::applicationDirPath();
  std::vector<fs::path> candidates;
  auto addCandidate = [&](const std::string& raw) {
    const std::string path = trim(raw);
    if (path.empty()) return;
    fs::path p = fs::u8path(path);
    candidates.push_back(p);
    if (p.is_relative()) {
      candidates.push_back(fs::u8path(appDir.toStdString()) / p);
    }
  };

  addCandidate(configuredPath);
  addCandidate("医院科室电话数据模板_demo.csv");
  addCandidate("samples/医院科室电话数据模板_demo.csv");

  for (const auto& p : candidates) {
    std::error_code ec;
    if (fs::exists(p, ec) && !fs::is_directory(p, ec)) {
      return p.u8string();
    }
  }
  return "";
}

std::vector<std::string> splitPlus(const std::string& s) {
  return split_trimmed(s, '+');
}

#ifdef _WIN32
bool parseVk(const std::string& token, UINT& vk) {
  int key = 0;
  if (parse_alnum_hotkey_token(token, key)) {
    vk = static_cast<UINT>(key);
    return true;
  }

  int fn = 0;
  if (parse_function_hotkey_token(token, 1, 12, fn)) {
    vk = static_cast<UINT>(VK_F1 + (fn - 1));
    return true;
  }

  if (token == "ENTER" || token == "RETURN") {
    vk = VK_RETURN;
    return true;
  }
  if (token == "SPACE") {
    vk = VK_SPACE;
    return true;
  }
  if (token == "ESC" || token == "ESCAPE") {
    vk = VK_ESCAPE;
    return true;
  }
  return false;
}
#endif

#ifdef __APPLE__
bool parseCarbonKeyCode(const std::string& token, UInt32& keyCode) {
  if (token.size() == 1) {
    switch (token[0]) {
      case 'A': keyCode = kVK_ANSI_A; return true;
      case 'B': keyCode = kVK_ANSI_B; return true;
      case 'C': keyCode = kVK_ANSI_C; return true;
      case 'D': keyCode = kVK_ANSI_D; return true;
      case 'E': keyCode = kVK_ANSI_E; return true;
      case 'F': keyCode = kVK_ANSI_F; return true;
      case 'G': keyCode = kVK_ANSI_G; return true;
      case 'H': keyCode = kVK_ANSI_H; return true;
      case 'I': keyCode = kVK_ANSI_I; return true;
      case 'J': keyCode = kVK_ANSI_J; return true;
      case 'K': keyCode = kVK_ANSI_K; return true;
      case 'L': keyCode = kVK_ANSI_L; return true;
      case 'M': keyCode = kVK_ANSI_M; return true;
      case 'N': keyCode = kVK_ANSI_N; return true;
      case 'O': keyCode = kVK_ANSI_O; return true;
      case 'P': keyCode = kVK_ANSI_P; return true;
      case 'Q': keyCode = kVK_ANSI_Q; return true;
      case 'R': keyCode = kVK_ANSI_R; return true;
      case 'S': keyCode = kVK_ANSI_S; return true;
      case 'T': keyCode = kVK_ANSI_T; return true;
      case 'U': keyCode = kVK_ANSI_U; return true;
      case 'V': keyCode = kVK_ANSI_V; return true;
      case 'W': keyCode = kVK_ANSI_W; return true;
      case 'X': keyCode = kVK_ANSI_X; return true;
      case 'Y': keyCode = kVK_ANSI_Y; return true;
      case 'Z': keyCode = kVK_ANSI_Z; return true;
      case '0': keyCode = kVK_ANSI_0; return true;
      case '1': keyCode = kVK_ANSI_1; return true;
      case '2': keyCode = kVK_ANSI_2; return true;
      case '3': keyCode = kVK_ANSI_3; return true;
      case '4': keyCode = kVK_ANSI_4; return true;
      case '5': keyCode = kVK_ANSI_5; return true;
      case '6': keyCode = kVK_ANSI_6; return true;
      case '7': keyCode = kVK_ANSI_7; return true;
      case '8': keyCode = kVK_ANSI_8; return true;
      case '9': keyCode = kVK_ANSI_9; return true;
      default: break;
    }
  }

  int fn = 0;
  if (parse_function_hotkey_token(token, 1, 12, fn)) {
    switch (fn) {
      case 1: keyCode = kVK_F1; return true;
      case 2: keyCode = kVK_F2; return true;
      case 3: keyCode = kVK_F3; return true;
      case 4: keyCode = kVK_F4; return true;
      case 5: keyCode = kVK_F5; return true;
      case 6: keyCode = kVK_F6; return true;
      case 7: keyCode = kVK_F7; return true;
      case 8: keyCode = kVK_F8; return true;
      case 9: keyCode = kVK_F9; return true;
      case 10: keyCode = kVK_F10; return true;
      case 11: keyCode = kVK_F11; return true;
      case 12: keyCode = kVK_F12; return true;
      default: break;
    }
  }
  if (token == "ENTER" || token == "RETURN") {
    keyCode = kVK_Return;
    return true;
  }
  if (token == "SPACE") {
    keyCode = kVK_Space;
    return true;
  }
  if (token == "ESC" || token == "ESCAPE") {
    keyCode = kVK_Escape;
    return true;
  }
  return false;
}
#endif

}  // namespace

QtMainWindow::QtMainWindow(QWidget* parent) : QMainWindow(parent) {
  if (qApp && !qApp->windowIcon().isNull()) {
    setWindowIcon(qApp->windowIcon());
  }
  const std::string defaultSource = "";
  settings_ = load_settings(defaultSource);
  const std::string resolvedSource = resolveSourceForQt(settings_.source_path.empty()
                                                            ? "医院科室电话数据模板_demo.csv"
                                                            : settings_.source_path);
  if (!resolvedSource.empty()) {
    settings_.source_path = resolvedSource;
    try {
      service_ = std::make_unique<Service>(settings_.source_path);
    } catch (...) {
      service_.reset();
      settings_.source_path.clear();
    }
  } else {
    settings_.source_path.clear();
  }

  std::string normalized;
  std::string err;
  if (!normalize_hotkey(settings_.trigger_hotkey, normalized, err)) {
    normalized = kDefaultHotkey;
  }
  settings_.trigger_hotkey = normalized;
  confirmedHotkeyDisplay_ = triggerDisplay(settings_.trigger_hotkey);
  hasConfirmedHotkey_ = true;
  persistSettings();

  hotkeyTimer_ = new QTimer(this);
  hotkeyTimer_->setSingleShot(true);
  hotkeyTimer_->setInterval(500);
  connect(hotkeyTimer_, &QTimer::timeout, this, &QtMainWindow::commitPendingHotkey);

  buildUi();
  // This app captures direct key events (not text composition). Disabling IME
  // avoids first-hit stalls from macOS InputMethodKit initialization.
  setAttribute(Qt::WA_InputMethodEnabled, false);
  if (centralWidget()) {
    centralWidget()->setAttribute(Qt::WA_InputMethodEnabled, false);
  }
  qApp->inputMethod()->hide();
  buildTray();
  applyStatus();
  applyQueryState();
#ifdef _WIN32
  applyWindowsGlobalHotkey();
#endif
#ifdef __APPLE__
  applyMacGlobalHotkey();
#endif
  setFocusPolicy(Qt::StrongFocus);
}

QtMainWindow::~QtMainWindow() {
#ifdef _WIN32
  unregisterWindowsGlobalHotkey();
#endif
#ifdef __APPLE__
  unregisterMacGlobalHotkey();
#endif
}

static QWidget* buildResultRowWidget(int idx, const MatchResult& m, QWidget* parent) {
  auto* row = new QWidget(parent);
  row->setObjectName("resultRow");
  auto* layout = new QHBoxLayout(row);
  layout->setContentsMargins(12, 10, 12, 10);
  layout->setSpacing(10);

  auto* idxLabel = new QLabel(QString::number(idx), row);
  idxLabel->setObjectName("idxBadge");
  idxLabel->setAlignment(Qt::AlignCenter);
  idxLabel->setFixedSize(24, 24);

  auto* deptLabel = new QLabel(QString::fromStdString(m.entry.department), row);
  deptLabel->setObjectName("deptText");
  deptLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto* phoneLabel = new QLabel(QString::fromStdString(m.entry.phone), row);
  phoneLabel->setObjectName("phoneText");
  phoneLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  layout->addWidget(idxLabel);
  layout->addWidget(deptLabel, 1);
  layout->addWidget(phoneLabel);
  return row;
}

void QtMainWindow::buildUi() {
  const QScreen* screen = QGuiApplication::primaryScreen();
  qreal uiScale = 1.0;
  if (screen) {
    uiScale = screen->logicalDotsPerInch() / 96.0;
  }
#ifdef _WIN32
  if (uiScale < 1.12) {
    uiScale = 1.12;
  }
#endif
  if (uiScale < 1.0) {
    uiScale = 1.0;
  }
  if (uiScale > 1.6) {
    uiScale = 1.6;
  }
  const auto s = [uiScale](int px) {
    return std::max(1, static_cast<int>(px * uiScale + 0.5));
  };

  setWindowTitle("一键即查");
  resize(s(760), s(460));

  auto* central = new QWidget(this);
  central->setObjectName("appRoot");
  setCentralWidget(central);

  auto* root = new QVBoxLayout(central);
  root->setContentsMargins(s(10), s(10), s(10), s(10));
  root->setSpacing(0);

  auto* panel = new QWidget(central);
  panel->setObjectName("panel");
  auto* panelLayout = new QVBoxLayout(panel);
  panelLayout->setContentsMargins(s(16), s(16), s(16), s(16));
  panelLayout->setSpacing(s(8));
  root->addWidget(panel);

  pages_ = new QStackedWidget(panel);
  pages_->setObjectName("pages");
  panelLayout->addWidget(pages_);

  // Finder page
  finderPage_ = new QWidget(pages_);
  auto* finderLayout = new QVBoxLayout(finderPage_);
  finderLayout->setSpacing(s(10));

  auto* headRow = new QHBoxLayout();
  headRow->setSpacing(s(10));
  openSettingsBtn_ = new QPushButton("设置", finderPage_);
  openSettingsBtn_->setProperty("ghost", true);
  headRow->addStretch(1);
  headRow->addWidget(openSettingsBtn_);
  finderLayout->addLayout(headRow);

  auto* queryPanel = new QWidget(finderPage_);
  queryPanel->setObjectName("queryPanel");
  auto* queryLayout = new QVBoxLayout(queryPanel);
  queryLayout->setContentsMargins(s(12), s(10), s(12), s(10));
  queryLayout->setSpacing(s(6));
  auto* qlabel = new QLabel("输入缓冲", queryPanel);
  qlabel->setObjectName("smallLabel");
  bufferLabel_ = new QLabel("_", queryPanel);
  bufferLabel_->setObjectName("bufferLabel");
  queryLayout->addWidget(qlabel);
  queryLayout->addWidget(bufferLabel_);
  finderLayout->addWidget(queryPanel);

  hintLabel_ = new QLabel("字母检索科室，纯数字反查电话，ESC 隐藏", finderPage_);
  hintLabel_->setObjectName("hintLabel");
  finderLayout->addWidget(hintLabel_);

  resultsList_ = new QListWidget(finderPage_);
  resultsList_->setObjectName("resultsList");
  resultsList_->setFrameShape(QFrame::NoFrame);
  resultsList_->setSpacing(s(4));
  resultsList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  resultsList_->setFocusPolicy(Qt::NoFocus);
  finderLayout->addWidget(resultsList_, 1);

  pages_->addWidget(finderPage_);

  // Settings page
  settingsPage_ = new QWidget(pages_);
  auto* settingsLayout = new QVBoxLayout(settingsPage_);
  settingsLayout->setSpacing(s(10));

  auto* settingsHead = new QHBoxLayout();
  settingsHead->setSpacing(s(10));
  auto* stTitle = new QLabel("设置", settingsPage_);
  stTitle->setObjectName("pageTitle");
  backBtn_ = new QPushButton("返回查询", settingsPage_);
  backBtn_->setProperty("ghost", true);
  settingsHead->addWidget(stTitle);
  settingsHead->addStretch(1);
  settingsHead->addWidget(backBtn_);
  settingsLayout->addLayout(settingsHead);

  auto* toolbar = new QWidget(settingsPage_);
  toolbar->setObjectName("toolbar");
  auto* toolbarLayout = new QVBoxLayout(toolbar);
  toolbarLayout->setContentsMargins(s(10), s(10), s(10), s(10));
  toolbarLayout->setSpacing(s(8));

  auto* hotkeyRowWrap = new QWidget(toolbar);
  hotkeyRowWrap->setObjectName("toolbarRow");
  auto* hotkeyRow = new QHBoxLayout(hotkeyRowWrap);
  hotkeyRow->setContentsMargins(0, 0, 0, 0);
  hotkeyRow->setSpacing(s(8));
  auto* hotkeyLabel = new QLabel("唤起热键", hotkeyRowWrap);
  hotkeyLabel->setObjectName("toolbarLabel");
  hotkeyRow->addWidget(hotkeyLabel);
  hotkeyEdit_ = new QLineEdit(hotkeyRowWrap);
  hotkeyEdit_->setObjectName("hotkeyInput");
  hotkeyEdit_->setReadOnly(true);
  hotkeyEdit_->setAlignment(Qt::AlignCenter);
  hotkeyEdit_->setFixedWidth(s(180));
  hotkeyEdit_->installEventFilter(this);
  auto* resetBtn = new QPushButton("恢复默认", hotkeyRowWrap);
  resetBtn->setFixedWidth(s(110));
  hotkeyRow->addWidget(hotkeyEdit_);
  hotkeyRow->addStretch(1);
  hotkeyRow->addWidget(resetBtn);
  toolbarLayout->addWidget(hotkeyRowWrap);

  auto* sourceRowWrap = new QWidget(toolbar);
  sourceRowWrap->setObjectName("toolbarRow");
  auto* sourceRow = new QHBoxLayout(sourceRowWrap);
  sourceRow->setContentsMargins(0, 0, 0, 0);
  sourceRow->setSpacing(s(8));
  auto* sourceLabel = new QLabel("数据源", sourceRowWrap);
  sourceLabel->setObjectName("toolbarLabel");
  sourceRow->addWidget(sourceLabel);
  sourcePathLabel_ = new QLabel(sourceRowWrap);
  sourcePathLabel_->setObjectName("pathValue");
  sourcePathLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  sourcePathLabel_->setMinimumWidth(0);
  sourcePathLabel_->setTextInteractionFlags(Qt::NoTextInteraction);
  pickBtn_ = new QPushButton("选择文件", sourceRowWrap);
  pickBtn_->setFixedWidth(s(110));
  sourceRow->addWidget(sourcePathLabel_, 1);
  sourceRow->addWidget(pickBtn_);
  toolbarLayout->addWidget(sourceRowWrap);

  statusLabel_ = new QLabel(toolbar);
  statusLabel_->setObjectName("statusLabel");
  toolbarLayout->addWidget(statusLabel_);

  settingsLayout->addWidget(toolbar);
  settingsLayout->addStretch(1);

  pages_->addWidget(settingsPage_);
  pages_->setCurrentWidget(finderPage_);

  sourcePathLabel_->setText(QString::fromStdString(settings_.source_path));
  setHotkeyInputDisplay(hasConfirmedHotkey_ ? confirmedHotkeyDisplay_ : "点击记录快捷键", false);

  connect(resultsList_, &QListWidget::itemDoubleClicked, this, &QtMainWindow::onResultActivated);
  connect(openSettingsBtn_, &QPushButton::clicked, this, &QtMainWindow::openSettings);
  connect(backBtn_, &QPushButton::clicked, this, &QtMainWindow::backToFinder);
  connect(pickBtn_, &QPushButton::clicked, this, &QtMainWindow::chooseSourceFile);
  connect(resetBtn, &QPushButton::clicked, this, [this]() {
    std::string normalized;
    std::string err;
    if (normalize_hotkey(kDefaultHotkey, normalized, err)) {
      settings_.trigger_hotkey = normalized;
      applyHotkeyAndPersist(normalized);
      hasConfirmedHotkey_ = true;
      applyStatus();
    }
  });

  const QString style = QString(R"(
    QWidget { font-family: 'PingFang SC', 'Hiragino Sans GB', 'Microsoft YaHei', sans-serif; color: #122550; }
    #appRoot { background: qradialgradient(cx:0.15, cy:0.2, radius:1.1, fx:0.15, fy:0.2, stop:0 #f8fcff, stop:0.45 #e8f1ff, stop:1 #eff6ff); }
    #panel { border: 1px solid #c7d7fb; border-radius: %1px; background: rgba(255,255,255,237); }
    #pages { background: transparent; }
    #pageTitle { font-size: %2px; font-weight: 700; color: #5c6a8e; }
    #queryPanel { border: 1px solid #c7d7fb; border-radius: %3px; background: white; }
    #smallLabel { color: #5c6a8e; font-size: %4px; }
    #bufferLabel { font-size: %5px; font-family: 'SF Mono', 'Menlo', 'Monaco', monospace; border-bottom: 1px solid #c7d7fb; min-height: %6px; letter-spacing: 0.12em; }
    #hintLabel { color: #5c6a8e; font-size: %7px; }
    #resultsList { border: none; background: transparent; outline: 0; }
    #resultsList::item { border: none; padding: 0; margin: 0; background: transparent; }
    QWidget#resultRow { background: #e6efff; border-radius: %8px; }
    QLabel#idxBadge { background: #ffffff; border-radius: %9px; color: #1d5ee4; font-weight: 700; font-size: %10px; }
    QLabel#deptText { font-weight: 600; font-size: %11px; }
    QLabel#phoneText { font-weight: 800; font-size: %12px; letter-spacing: 0.04em; }
    #toolbar { border: 1px solid #c7d7fb; border-radius: %13px; background: #f6f9ff; margin-bottom: %14px; }
    QWidget#toolbarRow { min-height: %15px; }
    #toolbarLabel { font-size: %16px; color: #5c6a8e; }
    #pathValue { color: #122550; font-size: %17px; padding-left: %18px; }
    #statusLabel { color: #5c6a8e; font-size: %19px; }
    QLineEdit#hotkeyInput { font-weight: 700; }
    QLineEdit#hotkeyInput[muted='true'] { color: #5c6a8e; font-weight: 500; }
    QLineEdit#hotkeyInput[muted='false'] { color: #122550; }
    QPushButton { min-height: %20px; border: 1px solid #c7d7fb; border-radius: %21px; padding: 0 %22px; background: #fff; color: #122550; font-weight: 600; }
    QPushButton[ghost='true'] { background: #f0f5ff; }
    QPushButton:hover { border-color: #1d5ee4; }
    QLineEdit { border: 1px solid #c7d7fb; border-radius: %23px; min-height: %24px; padding: 0 %25px; background: #fff; }
  )")
      .arg(s(20))  // 1
      .arg(s(14))  // 2
      .arg(s(12))  // 3
      .arg(s(12))  // 4
      .arg(s(30))  // 5
      .arg(s(40))  // 6
      .arg(s(13))  // 7
      .arg(s(10))  // 8
      .arg(s(8))   // 9
      .arg(s(13))  // 10
      .arg(s(15))  // 11
      .arg(s(16))  // 12
      .arg(s(12))  // 13
      .arg(s(10))  // 14
      .arg(s(34))  // 15
      .arg(s(12))  // 16
      .arg(s(13))  // 17
      .arg(s(2))   // 18
      .arg(s(12))  // 19
      .arg(s(34))  // 20
      .arg(s(10))  // 21
      .arg(s(10))  // 22
      .arg(s(10))  // 23
      .arg(s(34))  // 24
      .arg(s(10)); // 25
  setStyleSheet(style);
}

void QtMainWindow::buildTray() {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    tray_ = nullptr;
    return;
  }
  tray_ = new QSystemTrayIcon(this);
  QIcon trayIcon = loadTrayIconFromDisk();
  if (trayIcon.isNull()) {
    trayIcon = !windowIcon().isNull()
                   ? windowIcon()
                   : (qApp && !qApp->windowIcon().isNull()
                          ? qApp->windowIcon()
                          : QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
  }
  tray_->setIcon(trayIcon);

  auto* menu = new QMenu(this);
  trayShowAct_ = menu->addAction("显示");
  trayHideAct_ = menu->addAction("隐藏");
  trayReloadAct_ = menu->addAction("重载数据");
  menu->addSeparator();
  trayQuitAct_ = menu->addAction("退出");

  tray_->setContextMenu(menu);
  tray_->setToolTip("科室电话一键即查");
  tray_->show();

  connect(trayShowAct_, &QAction::triggered, this, &QtMainWindow::showFromTray);
  connect(trayHideAct_, &QAction::triggered, this, &QtMainWindow::hideToTray);
  connect(trayReloadAct_, &QAction::triggered, this, &QtMainWindow::reloadData);
  connect(trayQuitAct_, &QAction::triggered, this, [this]() {
    quitting_ = true;
    qApp->quit();
  });
  connect(tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
      if (isVisible()) hide();
      else showFromTray();
    }
  });
}

bool QtMainWindow::trayReady() const {
  return tray_ && QSystemTrayIcon::isSystemTrayAvailable() && tray_->isVisible();
}

void QtMainWindow::applyStatus() {
  const auto n = service_ ? service_->count() : 0;
  sourcePathLabel_->setText(settings_.source_path.empty() ? "未设置" : QString::fromStdString(settings_.source_path));
  if (!service_) {
    statusLabel_->setText(QString("未配置数据源，请先选择 CSV；当前热键：%1")
                             .arg(hasConfirmedHotkey_ ? confirmedHotkeyDisplay_ : "-"));
    return;
  }
  statusLabel_->setText(QString("已加载 %1 条，当前热键：%2")
                            .arg(static_cast<int>(n))
                            .arg(hasConfirmedHotkey_ ? confirmedHotkeyDisplay_ : "-"));
}

void QtMainWindow::setHotkeyInputDisplay(const QString& text, bool muted) {
  hotkeyEdit_->setText(text);
  hotkeyEdit_->setProperty("muted", muted ? "true" : "false");
  hotkeyEdit_->style()->unpolish(hotkeyEdit_);
  hotkeyEdit_->style()->polish(hotkeyEdit_);
}

QString QtMainWindow::buildHotkeyFromKeyEvent(QKeyEvent* event) const {
  const int k = event->key();
  const auto mods = event->modifiers();

  if (mods.testFlag(Qt::AltModifier) || mods.testFlag(Qt::MetaModifier)) {
    return {};
  }

  if (k == Qt::Key_Control && mods == Qt::ControlModifier) {
    return "CTRL";
  }
  if (k == Qt::Key_Shift && mods == Qt::ShiftModifier) {
    return "SHIFT";
  }

  if (k == Qt::Key_Control || k == Qt::Key_Shift) {
    return {};
  }

  QString main;
  if ((k >= Qt::Key_A && k <= Qt::Key_Z) || (k >= Qt::Key_0 && k <= Qt::Key_9)) {
    main = QString(QChar(k)).toUpper();
  } else if (k >= Qt::Key_F1 && k <= Qt::Key_F12) {
    main = QString("F%1").arg(k - Qt::Key_F1 + 1);
  } else if (k == Qt::Key_Space) {
    main = "SPACE";
  } else if (k == Qt::Key_Return || k == Qt::Key_Enter) {
    main = "ENTER";
  } else if (k == Qt::Key_Escape) {
    main = "ESC";
  } else {
    return {};
  }

  QStringList parts;
  if (mods.testFlag(Qt::ControlModifier)) parts << "CTRL";
  if (mods.testFlag(Qt::ShiftModifier)) parts << "SHIFT";
  parts << main;
  return parts.join('+');
}

QString QtMainWindow::modifierDraftFromKeyEvent(QKeyEvent* event) const {
  const auto mods = event->modifiers();
  QStringList parts;
  if (mods.testFlag(Qt::ControlModifier) || event->key() == Qt::Key_Control) {
    parts << "CTRL";
  }
  if (mods.testFlag(Qt::ShiftModifier) || event->key() == Qt::Key_Shift) {
    parts << "SHIFT";
  }
  return parts.join('+');
}

bool QtMainWindow::canAcceptQueryChar(QChar ch) const {
  if (!service_) {
    return false;
  }
  const bool allDigits =
      !queryBuffer_.isEmpty() &&
      std::all_of(queryBuffer_.cbegin(), queryBuffer_.cend(), [](QChar c) { return c.isDigit(); });
  if (ch.isLetter()) {
    return !allDigits;
  }
  if (ch.isDigit()) {
    return queryBuffer_.isEmpty() || allDigits;
  }
  return false;
}

void QtMainWindow::applyQueryChar(QChar ch) {
  if (ch.isLetter()) {
    queryBuffer_.append(ch.toUpper());
  } else {
    queryBuffer_.append(ch);
  }
  applyQueryState();
}

void QtMainWindow::applyQueryState() {
  bufferLabel_->setText(queryBuffer_.isEmpty() ? "_" : queryBuffer_);
  renderResults(queryBuffer_.toStdString());
  if (!service_) {
    hintLabel_->setText("未配置数据源，请到设置页选择 CSV");
    return;
  }
  if (queryBuffer_.isEmpty()) {
    hintLabel_->setText("字母检索科室，纯数字反查电话，ESC 隐藏");
    return;
  }
  if (last_.empty()) {
    hintLabel_->setText("未匹配到结果");
  } else if (last_.size() == 1) {
    hintLabel_->setText("唯一匹配：按 Enter 选中号码");
  } else {
    hintLabel_->setText(QString("匹配到 %1 条").arg(static_cast<int>(last_.size())));
  }
}

void QtMainWindow::clearQuery() {
  queryBuffer_.clear();
  applyQueryState();
}

void QtMainWindow::renderResults(const std::string& query) {
  resultsList_->clear();
  last_.clear();
  if (!service_ || query.empty()) {
    return;
  }
  last_ = service_->search(query, 9);
  int i = 1;
  for (const auto& m : last_) {
    auto* item = new QListWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    auto* rowWidget = buildResultRowWidget(i++, m, resultsList_);
    item->setSizeHint(rowWidget->sizeHint());
    resultsList_->addItem(item);
    resultsList_->setItemWidget(item, rowWidget);
  }
}

void QtMainWindow::persistSettings() {
  std::string err;
  if (!save_settings(settings_, err)) {
    QMessageBox::warning(this, "保存失败", QString::fromStdString(err));
  }
}

void QtMainWindow::applyHotkeyAndPersist(const std::string& normalized) {
  settings_.trigger_hotkey = normalized;
  confirmedHotkeyDisplay_ = triggerDisplay(normalized);
  hasConfirmedHotkey_ = true;
  setHotkeyInputDisplay(confirmedHotkeyDisplay_, false);
  persistSettings();
#ifdef _WIN32
  applyWindowsGlobalHotkey();
#endif
#ifdef __APPLE__
  applyMacGlobalHotkey();
#endif
}

void QtMainWindow::rollbackHotkeyInputAfterSetError() {
  pendingHotkey_.clear();
  pendingModifierDraft_.clear();
  hotkeyTimer_->stop();
  hasConfirmedHotkey_ = false;
  confirmedHotkeyDisplay_.clear();
  setHotkeyInputDisplay("点击记录快捷键", false);
}

void QtMainWindow::onResultActivated(QListWidgetItem* item) {
  int row = resultsList_->currentRow();
  if (item) {
    row = resultsList_->row(item);
  }
  if (row < 0 || static_cast<std::size_t>(row) >= last_.size()) {
    return;
  }
  hintLabel_->setText(
      QString("已选中号码: %1").arg(QString::fromStdString(last_[static_cast<std::size_t>(row)].entry.phone)));
  clearQuery();
}

void QtMainWindow::chooseSourceFile() {
  suppressBlurHide_ = true;
  const QString path = QFileDialog::getOpenFileName(this, "选择 CSV", QString::fromStdString(settings_.source_path), "CSV (*.csv)");
  QTimer::singleShot(300, this, [this]() { suppressBlurHide_ = false; });
  if (path.isEmpty()) {
    return;
  }

  try {
    service_ = std::make_unique<Service>(path.toStdString());
    settings_.source_path = path.toStdString();
    persistSettings();
    applyStatus();
    applyQueryState();
  } catch (const std::exception& ex) {
    QMessageBox::critical(this, "加载失败", QString::fromUtf8(ex.what()));
  }
}

void QtMainWindow::openSettings() {
  pages_->setCurrentWidget(settingsPage_);
  hotkeyEdit_->setFocus();
}

void QtMainWindow::backToFinder() {
  pages_->setCurrentWidget(finderPage_);
  setFocus();
}

void QtMainWindow::reloadData() {
  if (!service_) return;
  try {
    service_->reload();
    applyStatus();
    applyQueryState();
  } catch (const std::exception& ex) {
    QMessageBox::critical(this, "重载失败", QString::fromUtf8(ex.what()));
  }
}

void QtMainWindow::showFromTray() {
  showGuardUntilMs_ = QDateTime::currentMSecsSinceEpoch() + 300;
  pages_->setCurrentWidget(finderPage_);
  show();
  raise();
  activateWindow();
  clearQuery();
  setFocus();
}

void QtMainWindow::hideToTray() {
  if (!trayReady()) {
    showMinimized();
    return;
  }
  hide();
}

void QtMainWindow::onGlobalHotkeyPressed() {
#ifdef _WIN32
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (winHotkeyTapCount_ <= 1) {
    if (isVisible()) hideToTray();
    else showFromTray();
    return;
  }
  const qint64 gap = 350;
  if (winLastTapMs_ > 0 && now - winLastTapMs_ <= gap) {
    winLastTapMs_ = 0;
    if (isVisible()) hideToTray();
    else showFromTray();
  } else {
    winLastTapMs_ = now;
  }
  return;
#endif
#ifdef __APPLE__
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (macHotkeyTapCount_ <= 1) {
    if (isVisible()) hideToTray();
    else showFromTray();
    return;
  }
  const qint64 gap = 350;
  if (macLastTapMs_ > 0 && now - macLastTapMs_ <= gap) {
    macLastTapMs_ = 0;
    if (isVisible()) hideToTray();
    else showFromTray();
  } else {
    macLastTapMs_ = now;
  }
#endif
}

void QtMainWindow::commitPendingHotkey() {
  if (pendingHotkey_.isEmpty()) {
    return;
  }
  std::string normalized;
  std::string err;
  if (!normalize_hotkey(pendingHotkey_.toStdString(), normalized, err)) {
    rollbackHotkeyInputAfterSetError();
    return;
  }

  applyHotkeyAndPersist(normalized);
  pendingHotkey_.clear();
  applyStatus();
}

void QtMainWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    clearQuery();
    hide();
    return;
  }

  if (pages_->currentWidget() != finderPage_) {
    QMainWindow::keyPressEvent(event);
    return;
  }

  if (event->key() == Qt::Key_Backspace) {
    if (!queryBuffer_.isEmpty()) {
      queryBuffer_.chop(1);
      applyQueryState();
    }
    return;
  }

  if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && last_.size() == 1) {
    hintLabel_->setText(QString("已选中号码: %1").arg(QString::fromStdString(last_[0].entry.phone)));
    clearQuery();
    return;
  }

  const QString t = event->text();
  if (!service_ && !t.isEmpty()) {
    hintLabel_->setText("未配置数据源，请到设置页选择 CSV");
    return;
  }
  if (!t.isEmpty() && canAcceptQueryChar(t.front())) {
    applyQueryChar(t.front());
    return;
  }

  QMainWindow::keyPressEvent(event);
}

bool QtMainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == hotkeyEdit_) {
    if (event->type() == QEvent::FocusIn) {
      pendingHotkey_.clear();
      pendingModifierDraft_.clear();
      hotkeyTimer_->stop();
      if (hasConfirmedHotkey_) {
        setHotkeyInputDisplay(confirmedHotkeyDisplay_, true);
      } else {
        setHotkeyInputDisplay("输入快捷键", true);
      }
      return false;
    }

    if (event->type() == QEvent::FocusOut) {
      pendingHotkey_.clear();
      pendingModifierDraft_.clear();
      hotkeyTimer_->stop();
      setHotkeyInputDisplay(hasConfirmedHotkey_ ? confirmedHotkeyDisplay_ : "点击记录快捷键", false);
      return false;
    }

    if (event->type() == QEvent::KeyPress) {
      auto* ke = static_cast<QKeyEvent*>(event);
      if (ke->isAutoRepeat()) return true;
      if (ke->key() == Qt::Key_Escape) {
        hotkeyEdit_->clearFocus();
        return true;
      }

      if (ke->key() == Qt::Key_Control || ke->key() == Qt::Key_Shift) {
        const QString draft = modifierDraftFromKeyEvent(ke);
        if (!draft.isEmpty()) {
          pendingModifierDraft_ = draft;
          pendingHotkey_.clear();
          hotkeyTimer_->stop();
          setHotkeyInputDisplay(draft, true);
        }
        return true;
      }

      const QString hk = buildHotkeyFromKeyEvent(ke);
      if (hk.isEmpty()) {
        return true;
      }
      pendingModifierDraft_.clear();

      if (!pendingHotkey_.isEmpty() && pendingHotkey_ == hk) {
        hotkeyTimer_->stop();
        pendingHotkey_.clear();
        const QString candidate = hk + "*2";

        std::string normalized;
        std::string err;
        if (!normalize_hotkey(candidate.toStdString(), normalized, err)) {
          rollbackHotkeyInputAfterSetError();
          return true;
        }

        setHotkeyInputDisplay(candidate, true);
        applyHotkeyAndPersist(normalized);
        applyStatus();
        hotkeyEdit_->clearFocus();
        return true;
      }

      pendingHotkey_ = hk;
      setHotkeyInputDisplay(hk, true);
      hotkeyTimer_->start();
      return true;
    }
  }

  return QMainWindow::eventFilter(watched, event);
}

bool QtMainWindow::event(QEvent* event) {
  if (event->type() == QEvent::WindowDeactivate && isVisible() && pages_->currentWidget() == finderPage_) {
    if (!suppressBlurHide_ && QDateTime::currentMSecsSinceEpoch() >= showGuardUntilMs_) {
      if (trayReady()) {
        hide();
      }
    }
  }
  return QMainWindow::event(event);
}

void QtMainWindow::closeEvent(QCloseEvent* event) {
  if (quitting_) {
    event->accept();
    return;
  }
  if (!trayReady()) {
    quitting_ = true;
    event->accept();
    qApp->quit();
    return;
  }
  hide();
  event->ignore();
}

void QtMainWindow::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (pages_ && pages_->currentWidget() == finderPage_) {
    QTimer::singleShot(0, this, [this]() {
      activateWindow();
      raise();
      setFocus(Qt::OtherFocusReason);
    });
  }
}

void QtMainWindow::changeEvent(QEvent* event) {
  if (event && event->type() == QEvent::WindowStateChange) {
    if (isMinimized() && trayReady()) {
      QTimer::singleShot(0, this, [this]() {
        hideToTray();
      });
    }
  }
  QMainWindow::changeEvent(event);
}

#ifdef _WIN32
void QtMainWindow::unregisterWindowsGlobalHotkey() {
  UnregisterHotKey(reinterpret_cast<HWND>(winId()), winHotkeyId_);
  winHotkeyMods_ = 0;
  winHotkeyVk_ = 0;
  winHotkeyTapCount_ = 1;
  winLastTapMs_ = 0;
}

void QtMainWindow::applyWindowsGlobalHotkey() {
  unregisterWindowsGlobalHotkey();

  auto parts = splitPlus(settings_.trigger_hotkey);
  if (parts.empty()) {
    return;
  }
  std::string main = parts.back();
  winHotkeyTapCount_ = 1;
  if (main.size() > 2 && main.substr(main.size() - 2) == "*2") {
    main = main.substr(0, main.size() - 2);
    winHotkeyTapCount_ = 2;
  }
  if (main.empty()) {
    return;
  }

  UINT mods = MOD_NOREPEAT;
  for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
    if (parts[i] == "CTRL") {
      mods |= MOD_CONTROL;
    } else if (parts[i] == "SHIFT") {
      mods |= MOD_SHIFT;
    }
  }

  UINT vk = 0;
  if (!parseVk(main, vk)) {
    return;
  }

  if (RegisterHotKey(reinterpret_cast<HWND>(winId()), winHotkeyId_, mods, vk)) {
    winHotkeyMods_ = mods;
    winHotkeyVk_ = vk;
    winLastTapMs_ = 0;
  }
}

bool QtMainWindow::nativeEvent(const QByteArray&, void* message, long* result) {
  MSG* msg = static_cast<MSG*>(message);
  if (!msg) {
    return false;
  }

  if (msg->message == WM_HOTKEY && static_cast<int>(msg->wParam) == winHotkeyId_) {
    onGlobalHotkeyPressed();
    if (result) {
      *result = 0;
    }
    return true;
  }
  return false;
}
#endif

#ifdef __APPLE__
static QtMainWindow* gMacHotkeyWindow = nullptr;
static const UInt32 kMacHotkeySignature = 'PFQT';
static const UInt32 kMacHotkeyId = 1;

static OSStatus macHotkeyHandler(EventHandlerCallRef, EventRef event, void*) {
  EventHotKeyID hotKeyID{};
  if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
                        sizeof(hotKeyID), nullptr, &hotKeyID) != noErr) {
    return noErr;
  }
  if (hotKeyID.signature != kMacHotkeySignature || hotKeyID.id != kMacHotkeyId) {
    return noErr;
  }
  if (gMacHotkeyWindow) {
    QMetaObject::invokeMethod(gMacHotkeyWindow, "onGlobalHotkeyPressed", Qt::QueuedConnection);
  }
  return noErr;
}

void QtMainWindow::unregisterMacGlobalHotkey() {
  if (macHotkeyRef_) {
    UnregisterEventHotKey(reinterpret_cast<EventHotKeyRef>(macHotkeyRef_));
    macHotkeyRef_ = nullptr;
  }
  if (macEventHandlerRef_) {
    RemoveEventHandler(reinterpret_cast<EventHandlerRef>(macEventHandlerRef_));
    macEventHandlerRef_ = nullptr;
  }
  if (macEventHandlerUPP_) {
    DisposeEventHandlerUPP(reinterpret_cast<EventHandlerUPP>(macEventHandlerUPP_));
    macEventHandlerUPP_ = nullptr;
  }
  if (gMacHotkeyWindow == this) {
    gMacHotkeyWindow = nullptr;
  }
  macHotkeyTapCount_ = 1;
  macLastTapMs_ = 0;
}

void QtMainWindow::applyMacGlobalHotkey() {
  unregisterMacGlobalHotkey();

  auto parts = splitPlus(settings_.trigger_hotkey);
  if (parts.empty()) {
    return;
  }
  std::string main = parts.back();
  macHotkeyTapCount_ = 1;
  if (main.size() > 2 && main.substr(main.size() - 2) == "*2") {
    main = main.substr(0, main.size() - 2);
    macHotkeyTapCount_ = 2;
  }
  if (main.empty()) {
    return;
  }

  UInt32 modifiers = 0;
  for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
    if (parts[i] == "CTRL") {
      modifiers |= controlKey;
    } else if (parts[i] == "SHIFT") {
      modifiers |= shiftKey;
    }
  }

  UInt32 keyCode = 0;
  if (!parseCarbonKeyCode(main, keyCode)) {
    return;
  }

  EventTypeSpec eventType{};
  eventType.eventClass = kEventClassKeyboard;
  eventType.eventKind = kEventHotKeyPressed;

  auto upp = NewEventHandlerUPP(macHotkeyHandler);
  if (!upp) {
    return;
  }
  macEventHandlerUPP_ = reinterpret_cast<void*>(upp);

  if (InstallApplicationEventHandler(upp, 1, &eventType, nullptr,
                                     reinterpret_cast<EventHandlerRef*>(&macEventHandlerRef_)) != noErr) {
    unregisterMacGlobalHotkey();
    return;
  }

  EventHotKeyID hotKeyID{};
  hotKeyID.signature = kMacHotkeySignature;
  hotKeyID.id = kMacHotkeyId;
  if (RegisterEventHotKey(keyCode, modifiers, hotKeyID, GetApplicationEventTarget(), 0,
                          reinterpret_cast<EventHotKeyRef*>(&macHotkeyRef_)) != noErr) {
    unregisterMacGlobalHotkey();
    return;
  }

  gMacHotkeyWindow = this;
  macLastTapMs_ = 0;
}
#endif

}  // namespace phonefinder
