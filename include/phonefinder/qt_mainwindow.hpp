#pragma once

#include <memory>

#include <QMainWindow>

#include "phonefinder/service.hpp"
#include "phonefinder/settings.hpp"

class QCloseEvent;
class QEvent;
class QShowEvent;
class QLineEdit;
class QListWidget;
class QPushButton;
class QLabel;
class QStackedWidget;
class QSystemTrayIcon;
class QAction;
class QTimer;
class QKeyEvent;
class QListWidgetItem;

namespace phonefinder {

class QtMainWindow : public QMainWindow {
 public:
  explicit QtMainWindow(QWidget* parent = nullptr);
  ~QtMainWindow() override;
  void activateFromExternalTrigger();

 protected:
  void closeEvent(QCloseEvent* event) override;
  void changeEvent(QEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;
  bool event(QEvent* event) override;
#ifdef _WIN32
  bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif

 private:
  void onResultActivated(QListWidgetItem* item = nullptr);
  void chooseSourceFile();
  void openSettings();
  void backToFinder();
  void reloadData();
  void showFromTray();
  void hideToTray();
  void commitPendingHotkey();
  void onGlobalHotkeyPressed();

  void buildUi();
  void buildTray();
  void applyStatus();
  void renderResults(const std::string& query);
  bool trayReady() const;
  void persistSettings();
  void setHotkeyInputDisplay(const QString& text, bool muted);
  QString buildHotkeyFromKeyEvent(QKeyEvent* event) const;
  bool canAcceptQueryChar(QChar ch) const;
  void applyQueryChar(QChar ch);
  void applyQueryState();
  void clearQuery();
  void applyHotkeyAndPersist(const std::string& normalized);
  void rollbackHotkeyInputAfterSetError();
  QString modifierDraftFromKeyEvent(QKeyEvent* event) const;
#ifdef _WIN32
  void applyWindowsGlobalHotkey();
  void unregisterWindowsGlobalHotkey();
#endif
#ifdef __APPLE__
  void applyMacGlobalHotkey();
  void unregisterMacGlobalHotkey();
#endif

  AppSettings settings_;
  std::unique_ptr<Service> service_;
  std::vector<MatchResult> last_;
  QString queryBuffer_;

  QStackedWidget* pages_ = nullptr;

  QWidget* finderPage_ = nullptr;
  QWidget* settingsPage_ = nullptr;

  QLabel* bufferLabel_ = nullptr;
  QListWidget* resultsList_ = nullptr;
  QLabel* hintLabel_ = nullptr;

  QLabel* sourcePathLabel_ = nullptr;
  QLineEdit* hotkeyEdit_ = nullptr;
  QLabel* statusLabel_ = nullptr;

  QPushButton* openSettingsBtn_ = nullptr;
  QPushButton* backBtn_ = nullptr;
  QPushButton* pickBtn_ = nullptr;

  QSystemTrayIcon* tray_ = nullptr;
  QAction* trayShowAct_ = nullptr;
  QAction* trayHideAct_ = nullptr;
  QAction* trayReloadAct_ = nullptr;
  QAction* trayQuitAct_ = nullptr;
  bool quitting_ = false;

  QTimer* hotkeyTimer_ = nullptr;
  QString pendingHotkey_;
  QString pendingModifierDraft_;
  QString confirmedHotkeyDisplay_;
  bool hasConfirmedHotkey_ = false;
  bool suppressBlurHide_ = false;
  qint64 showGuardUntilMs_ = 0;
#ifdef _WIN32
  int winHotkeyId_ = 0x2A11;
  unsigned int winHotkeyMods_ = 0;
  unsigned int winHotkeyVk_ = 0;
  int winHotkeyTapCount_ = 1;
  qint64 winLastTapMs_ = 0;
#endif
#ifdef __APPLE__
  void* macHotkeyRef_ = nullptr;
  void* macEventHandlerRef_ = nullptr;
  void* macEventHandlerUPP_ = nullptr;
  int macHotkeyTapCount_ = 1;
  qint64 macLastTapMs_ = 0;
#endif
};

}  // namespace phonefinder
