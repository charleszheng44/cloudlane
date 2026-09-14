#include "backend.h"
#include "videoitem.h"
#include <QGuiApplication>
#include <QLocale>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

// Test-only presentation fixture. Requests/account restoration never reach
// the service; the normal app has no demo account or synthetic library.
class ScreenshotBackend final : public Backend {
  Q_OBJECT
  Q_PROPERTY(bool playing READ demoPlaying CONSTANT)
  Q_PROPERTY(bool loaded READ demoPlaying CONSTANT)
  Q_PROPERTY(double position READ demoPosition CONSTANT)
  Q_PROPERTY(double duration READ demoDuration CONSTANT)
public:
  bool demoPlaying() const { return true; }
  double demoPosition() const { return 68; }
  double demoDuration() const { return 246; }
  Q_INVOKABLE int request(QString, QVariantMap = {}, QString = "weapi", bool = false) { return ++requestId; }
  Q_INVOKABLE void restoreAccount() {}
private:
  int requestId = 0;
};

int main(int argc, char **argv) {
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  QGuiApplication app(argc, argv);
  QCoreApplication::setOrganizationName("CloudlaneScreenshot");
  QCoreApplication::setApplicationName("cloudlane-screenshot");
  QTemporaryDir isolated;
  if (!isolated.isValid()) return 1;
  QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, isolated.path());
  qputenv("XDG_DATA_HOME", isolated.path().toUtf8());
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Basic");
  qmlRegisterType<VideoItem>("Cloudlane", 1, 0, "VideoSurface");
  ScreenshotBackend backend;
  QQmlApplicationEngine engine;
  QStringList warnings;
  QObject::connect(&engine, &QQmlEngine::warnings, &app, [&](const QList<QQmlError> &list) {
    for (const auto &warning : list) warnings << warning.toString();
  });
  engine.rootContext()->setContextProperty("Backend", &backend);
  engine.rootContext()->setContextProperty("DemoArtRoot", QUrl::fromLocalFile(QDir::currentPath() + "/tests/fixtures/art/").toString());
  engine.load(QUrl("qrc:/qml/Main.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
  auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
  if (!window || !QTest::qWaitForWindowExposed(window)) return 1;
  if (qEnvironmentVariableIsSet("HYPRLAND_INSTANCE_SIGNATURE"))
    QProcess::execute("hyprctl", {"eval", "hl.dispatch(hl.dsp.window.float({action=\"on\",window=\"pid:" + QString::number(app.applicationPid()) + "\"}))"});
  const QSize size(1440, 900);
  window->setMinimumSize(size); window->setMaximumSize(size); window->resize(size);
  QQmlExpression fixture(qmlContext(window), window, R"(
    Actions.pending = {}; window.viewGeneration++;
    window.nav = 'Home'; window.page = 'Home'; window.category = '';
    window.profile = {userId:'demo-listener',nickname:'Demo listener'};
    window.loading = ''; window.error = ''; window.staleText = '';
    var names = ['Quiet mornings','After hours','Open roads','Deep focus','Weekend sounds','New horizons'];
    window.items = names.map(function(name, i) {
      return {id:'demo-'+i,kind:'playlist',name:name,artist:'Cloudlane selections',cover:DemoArtRoot+i+'.svg'};
    });
    window.libraryPlaylists = window.items.slice(0,4);
    window.currentTrack = {id:'demo-track',entryId:'demo-entry',kind:'song',name:'A little further',artist:'The Daydreams',album:'Open roads',cover:DemoArtRoot+'2.svg'};
    window.queue = [window.currentTrack, Object.assign({},window.currentTrack,{id:'demo-next',entryId:'demo-next'})];
    window.queueIndex = 0; window.likedIds = ['demo-track'];
    window.actualQuality = 'Lossless'; window.panel = 'Now Playing';
  )");
  fixture.evaluate();
  if (fixture.hasError()) warnings << fixture.error().toString();
  QTest::qWait(1200);
  const QString path = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "build/cloudlane-preview.png";
  const bool saved = window->size() == size && window->grabWindow().save(path);
  for (const auto &warning : warnings) qWarning().noquote() << warning;
  return saved && warnings.isEmpty() ? 0 : 1;
}
#include "screenshot.moc"
