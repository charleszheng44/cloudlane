#include "backend.h"
#include "mpris.h"
#include "videoitem.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QGuiApplication>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
class NativeTest : public QObject {
  Q_OBJECT
private slots:
  void playerAndViews() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       directory.path());
    qputenv("XDG_DATA_HOME", directory.path().toUtf8());
    Backend backend;
    QQmlApplicationEngine engine;
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, this,
            [&](const QList<QQmlError> &list) {
              for (auto e : list)
                warnings << e.toString();
            });
    engine.rootContext()->setContextProperty("Backend", &backend);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window);
    auto evaluate = [&](QString code) {
      QQmlExpression expression(qmlContext(window), window, code);
      auto value = expression.evaluate();
      if (expression.hasError())
        warnings << expression.error().toString();
      return value;
    };
    auto control = [&](const char *name) {
      // View delegates belong to the visual tree, which can differ from
      // QObject ownership. Find the control actually rendered in the window.
      std::function<QQuickItem *(QQuickItem *)> find =
          [&](QQuickItem *item) -> QQuickItem * {
        if (item->objectName() == QString::fromLatin1(name))
          return item;
        for (auto child : item->childItems())
          if (auto result = find(child))
            return result;
        return nullptr;
      };
      return find(window->contentItem());
    };
    auto click = [&](QQuickItem *item, qreal fraction = .5) {
      QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                        item->mapToScene(QPointF(item->width() * fraction,
                                                 item->height() / 2))
                            .toPoint());
      QTest::qWait(50);
    };
    auto resize = [&](QSize size) {
      window->setMaximumSize(QSize(16777215, 16777215));
      window->setMinimumSize(size);
      window->setMaximumSize(size);
      window->resize(size);
    };
    QVERIFY(QTest::qWaitForWindowExposed(window));
    if (qEnvironmentVariableIsSet("HYPRLAND_INSTANCE_SIGNATURE")) {
      QProcess::execute(
          "hyprctl",
          {"eval",
           "hl.dispatch(hl.dsp.window.float({action=\"on\",window=\"pid:" +
               QString::number(QCoreApplication::applicationPid()) + "\"}))"});
      QTest::qWait(300);
      resize(QSize(1100, 760));
    }
    QTest::qWait(500);
    QTRY_COMPARE(window->width(), 1100);
    QTRY_COMPARE(window->height(), 760);
    QVERIFY(!backend.playing());
    evaluate("Actions.navigate('我的音乐','本地音乐')");
    QCOMPARE(window->property("viewKind").toString(), QString("songs"));
    const QString media = qEnvironmentVariable("YUNJIAN_TEST_MEDIA");
    QVERIFY2(!media.isEmpty(),
             "Set YUNJIAN_TEST_MEDIA to the generated local video fixture");
    window->setProperty(
        "currentTrack",
        QVariantMap{{"id", "fixture"},
                    {"entryId", "test123"},
                    {"kind", "local"},
                    {"name", "Native playback test"},
                    {"artist", "Local audio fixture"},
                    {"url", QUrl::fromLocalFile(media).toString()}});
    evaluate(
        "window.currentTrack=JSON.parse(JSON.stringify(window.currentTrack)); "
        "window.queue=[window.currentTrack,Object.assign({},window."
        "currentTrack,{id:'second',entryId:'test456'})];window.queueIndex=0");
    auto playButton = control("playPauseButton");
    auto previousButton = control("previousButton");
    auto nextButton = control("nextButton");
    auto muteButton = control("muteButton");
    auto volumeSlider = control("volumeSlider");
    auto seekSlider = control("seekSlider");
    QVERIFY(playButton && previousButton && nextButton && muteButton &&
            volumeSlider && seekSlider);
    backend.setVolume(0);
    backend.setMetadata({{"id", "fixture"},
                         {"entryId", "test123"},
                         {"name", "Native playback test"},
                         {"duration", 12000}});
    window->setProperty("videoVisible", true);
    QTest::qWait(300);
    QSignalSpy loaded(&backend, &Backend::mediaLoaded);
    backend.load(QUrl::fromLocalFile(media).toString());
    QVERIFY(loaded.wait(5000));
    QTRY_VERIFY_WITH_TIMEOUT(backend.playing(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(backend.duration() > 5, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(backend.position() > .3, 4000);
    QCOMPARE(playButton->property("symbol").toString(), QString("pause"));
    QVERIFY(window->grabWindow().save("build/native-player-playing.png"));
    click(playButton);
    QTRY_VERIFY_WITH_TIMEOUT(!backend.playing(), 2000);
    QCOMPARE(playButton->property("symbol").toString(), QString("play"));
    click(seekSlider, .25);
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(backend.position() - 3) < .5, 2000);
    backend.setVolume(40);
    click(muteButton);
    QTRY_COMPARE(backend.volume(), 0);
    QCOMPARE(muteButton->property("symbol").toString(), QString("mute"));
    click(muteButton);
    QTRY_COMPARE(backend.volume(), 40);
    click(volumeSlider, .75);
    QTRY_VERIFY2(
        backend.volume() > 60 && backend.volume() < 85,
        qPrintable(QString("Volume %1, slider %2")
                       .arg(backend.volume())
                       .arg(volumeSlider->property("value").toDouble())));
    backend.setVolume(0);
    click(nextButton);
    QTRY_COMPARE(window->property("queueIndex").toInt(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(backend.playing(), 3000);
    click(previousButton);
    QTRY_COMPARE(window->property("queueIndex").toInt(), 0);
    QTRY_VERIFY2_WITH_TIMEOUT(
        backend.playing(),
        qPrintable(window->property("error").toString() + " / " +
                   window->property("playerStatus").toString() + " / " +
                   warnings.join('\n')),
        3000);
    click(playButton);
    QTRY_VERIFY(!backend.playing());
    QVERIFY(window->grabWindow().save("build/native-video-test.png"));
    backend.stop();
    QVERIFY(!backend.playing());
    if (qEnvironmentVariableIsSet("YUNJIAN_LIVE_TESTS")) {
      QVERIFY(QMetaObject::invokeMethod(window, "login"));
      QTRY_VERIFY_WITH_TIMEOUT(!backend.loginQr().isEmpty(), 25000);
      QCOMPARE(backend.loginPhase(), QString("waiting"));
      QCOMPARE(window->property("qr").toString(), backend.loginQr());
      QVERIFY(QMetaObject::invokeMethod(window, "closeLogin"));
      QTRY_COMPARE(backend.loginPhase(), QString("idle"));
      evaluate(
          "window.videoVisible=false; window.viewGeneration++; "
          "window.loading=''; window.error=''; Actions.search('海阔天空',0)");
      QTRY_VERIFY_WITH_TIMEOUT(window->property("loading").toString().isEmpty(),
                               25000);
      qInfo() << "Public search:" << window->property("error").toString();
      QCOMPARE(window->property("error").toString(), QString());
      QTest::qWait(200);
      QVERIFY(window->grabWindow().save("build/native-search.png"));
      evaluate("Actions.navigate('首页')");
      QTRY_VERIFY_WITH_TIMEOUT(window->property("loading").toString().isEmpty(),
                               25000);
      QCOMPARE(window->property("error").toString(), QString());
      QTRY_VERIFY_WITH_TIMEOUT(
          control("browseCover") &&
              control("browseCover")->property("status").toInt() == 1,
          10000);
    }
    resize(QSize(1400, 850));
    QTRY_COMPARE(window->width(), 1400);
    evaluate("window.panel='正在播放'");
    QTest::qWait(400);
    QVERIFY(window->grabWindow().save("build/native-spotify-layout.png"));
    evaluate("window.panel=''");
    resize(QSize(640, 480));
    QTRY_COMPARE(window->width(), 640);
    QTRY_COMPARE(window->height(), 480);
    QTest::qWait(400);
    for (auto item : {playButton, previousButton, nextButton, muteButton,
                      volumeSlider, seekSlider}) {
      QVERIFY(item->isVisible());
      const auto bounds = item->mapRectToScene(item->boundingRect());
      QVERIFY(bounds.left() >= 0 && bounds.right() <= window->width());
      QVERIFY(bounds.top() >= 0 && bounds.bottom() <= window->height());
    }
    click(volumeSlider, .5);
    QTRY_VERIFY(backend.volume() > 40 && backend.volume() < 60);
    volumeSlider->forceActiveFocus();
    const double keyboardVolume = backend.volume();
    QTest::keyClick(window, Qt::Key_Right);
    QTRY_VERIFY(backend.volume() > keyboardVolume);
    evaluate("window.panel='队列'");
    QTest::qWait(300);
    evaluate("window.panel=''");
    QTest::qWait(400);
    QVERIFY(window->grabWindow().save("build/native-narrow.png"));
    for (const auto &warning : warnings)
      qWarning().noquote() << warning;
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
    window->close();
  }
};
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setOrganizationName("YunjianTests");
  QCoreApplication::setApplicationName("native-test");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Basic");
  qmlRegisterType<VideoItem>("Yunjian", 1, 0, "VideoSurface");
  NativeTest test;
  return QTest::qExec(&test, argc, argv);
}
#include "native_test.moc"
