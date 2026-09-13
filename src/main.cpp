#include "backend.h"
#include "mpris.h"
#include "videoitem.h"
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setOrganizationName("Yunjian");
  QCoreApplication::setApplicationName("yunjian");
  QCoreApplication::setApplicationVersion("0.1.0-dev");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Basic");
  const bool isolated = app.arguments().contains("--isolated") ||
                        app.arguments().contains("--smoke-test");
  auto bus = QDBusConnection::sessionBus();
  if (!isolated && bus.isConnected() &&
      !bus.registerService("org.mpris.MediaPlayer2.yunjian")) {
    bus.call(QDBusMessage::createMethodCall("org.mpris.MediaPlayer2.yunjian",
                                            "/org/mpris/MediaPlayer2",
                                            "org.mpris.MediaPlayer2", "Raise"));
    return 0;
  }
  qmlRegisterType<VideoItem>("Yunjian", 1, 0, "VideoSurface");
  Backend backend;
  if (!isolated) {
    new MprisRoot(&backend);
    new MprisPlayer(&backend);
    bus.registerObject("/org/mpris/MediaPlayer2", &backend,
                       QDBusConnection::ExportAdaptors);
  }
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("Backend", &backend);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.load(QUrl("qrc:/qml/Main.qml"));
  QObject::connect(
      &backend, &Backend::desktopAction, &app,
      [&engine](const QString &action) {
        if (action == "quit")
          QCoreApplication::quit();
        else if (action == "raise" && !engine.rootObjects().isEmpty()) {
          auto window =
              qobject_cast<QQuickWindow *>(engine.rootObjects().first());
          if (window) {
            window->showNormal();
            window->raise();
            window->requestActivate();
          }
        }
      });
  if (app.arguments().contains("--login"))
    QTimer::singleShot(250, &app, [&engine] {
      if (!engine.rootObjects().isEmpty())
        QMetaObject::invokeMethod(engine.rootObjects().first(), "login");
    });
  if (app.arguments().contains("--smoke-test"))
    QTimer::singleShot(2500, &app, &QCoreApplication::quit);
  return app.exec();
}
