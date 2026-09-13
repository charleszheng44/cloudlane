#include "backend.h"
#include "videoitem.h"
#include "mpris.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QSettings>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QtTest>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
class NativeTest : public QObject {
  Q_OBJECT
private slots:
  void playerAndViews() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,directory.path());
    Backend backend;
    QQmlApplicationEngine engine;
    QStringList warnings;
    connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError>&list){for(auto e:list)warnings<<e.toString();});
    engine.rootContext()->setContextProperty("Backend",&backend);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
    auto evaluate=[&](QString code){QQmlExpression expression(qmlContext(window),window,code);auto value=expression.evaluate();if(expression.hasError())warnings<<expression.error().toString();return value;};
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(500);
    QVERIFY(!backend.playing());
    evaluate("Actions.navigate('我的音乐','本地音乐')");
    QCOMPARE(window->property("viewKind").toString(),QString("songs"));
    const QString media=qEnvironmentVariable("YUNJIAN_TEST_MEDIA");QVERIFY2(!media.isEmpty(),"Set YUNJIAN_TEST_MEDIA to the generated local video fixture");
    backend.setMetadata({{"id","fixture"},{"entryId","test123"},{"name","Native playback test"},{"duration",12000}});
    window->setProperty("videoVisible",true);
    QTest::qWait(300);
    QSignalSpy loaded(&backend,&Backend::mediaLoaded);
    backend.load(QUrl::fromLocalFile(media).toString());
    QVERIFY(loaded.wait(5000));
    QTRY_VERIFY_WITH_TIMEOUT(backend.playing(),3000);
    QTRY_VERIFY_WITH_TIMEOUT(backend.duration()>5,3000);
    QTRY_VERIFY_WITH_TIMEOUT(backend.position()>.3,4000);
    backend.setPaused(true);
    QTRY_VERIFY_WITH_TIMEOUT(!backend.playing(),2000);
    backend.seek(3);
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(backend.position()-3)<.5,2000);
    QVERIFY(window->grabWindow().save("build/native-video-test.png"));
    backend.stop();QVERIFY(!backend.playing());
    evaluate("app.videoVisible=false; app.viewGeneration++; app.loading=''; app.error=''; Actions.search('海阔天空',0)");
    QTRY_VERIFY_WITH_TIMEOUT(window->property("loading").toString().isEmpty(),25000);
    // Public-network availability is recorded; local media/UI assertions stay deterministic.
    qInfo()<<"Public search:"<<window->property("error").toString();
    QTest::qWait(200);
    QVERIFY(window->grabWindow().save("build/native-search.png"));
    window->resize(640,480);QTest::qWait(400);
    evaluate("app.panel='队列'");QTest::qWait(300);evaluate("app.panel=''");
    QVERIFY(window->grabWindow().save("build/native-narrow.png"));
    for(const auto &warning:warnings)qWarning().noquote()<<warning;
    QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
    window->close();
  }
};
int main(int argc,char **argv){
  QGuiApplication app(argc,argv);QCoreApplication::setOrganizationName("YunjianTests");QCoreApplication::setApplicationName("native-test");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);QQuickStyle::setStyle("Basic");
  qmlRegisterType<VideoItem>("Yunjian",1,0,"VideoSurface");NativeTest test;return QTest::qExec(&test,argc,argv);
}
#include "native_test.moc"
