#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>

#include "phonefinder/qt_mainwindow.hpp"

namespace {

QIcon loadAppIcon() {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      appDir + "/icon.ico",
      appDir + "/trayicon.png",
      "build/windows/icon.ico",
      "build/trayicon.png",
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

}  // namespace

int main(int argc, char** argv) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#ifdef __APPLE__
  qputenv("QT_IM_MODULE", QByteArray("none"));
#endif
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  const QIcon icon = loadAppIcon();
  if (!icon.isNull()) {
    app.setWindowIcon(icon);
  }

  phonefinder::QtMainWindow w;
  w.show();

  return app.exec();
}
