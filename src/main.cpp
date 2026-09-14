#include "backend.h"
#include "mpris.h"
#include "videoitem.h"
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QTextStream>
int main(int argc, char **argv) {
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationDisplayName("Cloudlane");
  QGuiApplication::setDesktopFileName("io.github.charleszheng44.Cloudlane");
  // Preserve pre-rename settings, SQLite paths and the saved account.
  QCoreApplication::setOrganizationName("Yunjian");
  QCoreApplication::setApplicationName("yunjian");
  QCoreApplication::setApplicationVersion("0.1.0-dev");
  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Cloudlane · Native NetEase Cloud Music for Omarchy");
  parser.addHelpOption();
  parser.addOption({{"v", "version"}, "Display the application version"});
  parser.addOption({"login", "Show consumer QR login"});
  parser.addOption({"isolated", "Do not register the desktop media service"});
  parser.addOption({"smoke-test", "Launch briefly for a UI smoke check"});
  parser.addPositionalArgument(
      "urls", "Local media files or NetEase resource links", "[urls...]");
  parser.process(app);
  if (parser.isSet("version")) {
    QTextStream(stdout) << "Cloudlane " << app.applicationVersion() << Qt::endl;
    return 0;
  }
  const auto urls = parser.positionalArguments();
  const bool showLogin = parser.isSet("login");
  const bool smokeTest = parser.isSet("smoke-test");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Basic");
  const bool isolated = parser.isSet("isolated") || smokeTest;
  auto bus = QDBusConnection::sessionBus();
  if (!isolated && bus.isConnected() &&
      !bus.registerService("org.mpris.MediaPlayer2.cloudlane")) {
    bus.call(QDBusMessage::createMethodCall("org.mpris.MediaPlayer2.cloudlane",
                                            "/org/mpris/MediaPlayer2",
                                            "org.mpris.MediaPlayer2", "Raise"));
    for (const auto &uri : urls) {
      auto message = QDBusMessage::createMethodCall(
          "org.mpris.MediaPlayer2.cloudlane", "/org/mpris/MediaPlayer2",
          "org.mpris.MediaPlayer2.Player", "OpenUri");
      message << uri;
      bus.call(message);
    }
    return 0;
  }
  qmlRegisterType<VideoItem>("Cloudlane", 1, 0, "VideoSurface");
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
  QTimer::singleShot(350, &app, [&backend, urls] {
    for (const auto &uri : urls)
      emit backend.openRequested(uri);
  });
  if (showLogin)
    QTimer::singleShot(250, &app, [&engine] {
      if (!engine.rootObjects().isEmpty())
        QMetaObject::invokeMethod(engine.rootObjects().first(), "login");
    });
  if (smokeTest)
    QTimer::singleShot(2500, &app, &QCoreApplication::quit);
  return app.exec();
}
