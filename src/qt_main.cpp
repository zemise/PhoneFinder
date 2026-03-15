#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>

#include "phonefinder/qt_mainwindow.hpp"

namespace {

constexpr auto kSingleInstanceServerName = "PhoneFinder.SingleInstance";

QIcon loadAppIcon() {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      appDir + "/icon.ico",
      appDir + "/trayicon.png",
      "resources/windows/icon.ico",
      "resources/trayicon.png",
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

bool notifyRunningInstance() {
  QLocalSocket socket;
  socket.connectToServer(QString::fromLatin1(kSingleInstanceServerName), QIODevice::WriteOnly);
  if (!socket.waitForConnected(250)) {
    return false;
  }
  socket.write("show");
  socket.flush();
  socket.waitForBytesWritten(250);
  socket.disconnectFromServer();
  return true;
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
  if (notifyRunningInstance()) {
    return 0;
  }
  QLocalServer::removeServer(QString::fromLatin1(kSingleInstanceServerName));
  const QIcon icon = loadAppIcon();
  if (!icon.isNull()) {
    app.setWindowIcon(icon);
  }

  phonefinder::QtMainWindow w;
  QLocalServer singleInstanceServer;
  QObject::connect(&singleInstanceServer, &QLocalServer::newConnection, &w, [&]() {
    while (QLocalSocket* socket = singleInstanceServer.nextPendingConnection()) {
      QObject::connect(socket, &QLocalSocket::readyRead, socket, [&w, socket]() {
        socket->readAll();
        w.activateFromExternalTrigger();
        socket->disconnectFromServer();
      });
      QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
  });
  if (!singleInstanceServer.listen(QString::fromLatin1(kSingleInstanceServerName))) {
    return 1;
  }
  w.show();

  return app.exec();
}
