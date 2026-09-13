#include "backend.h"
#include "mpris.h"
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QGuiApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
class MprisTest : public QObject {
  Q_OBJECT
private slots:
  void dbusContract() {
    QTemporaryDir dir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       dir.path());
    qputenv("XDG_DATA_HOME", dir.path().toUtf8());
    Backend backend;
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());
    const QString name = "org.mpris.MediaPlayer2.yunjian_test";
    QVERIFY(bus.registerService(name));
    new MprisRoot(&backend);
    new MprisPlayer(&backend);
    QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &backend,
                               QDBusConnection::ExportAdaptors));
    backend.setMetadata({{"id", "music1"},
                         {"entryId", "entry1"},
                         {"name", "fixture"},
                         {"duration", 10000},
                         {"canNext", true}});
    auto get = QDBusMessage::createMethodCall(name, "/org/mpris/MediaPlayer2",
                                              "org.freedesktop.DBus.Properties",
                                              "GetAll");
    get << QString("org.mpris.MediaPlayer2.Player");
    auto call = bus.asyncCall(get);
    QTRY_VERIFY(call.isFinished());
    QVERIFY2(call.reply().type() != QDBusMessage::ErrorMessage,
             qPrintable(call.reply().errorMessage()));
    auto properties = qdbus_cast<QVariantMap>(call.reply().arguments().first());
    QCOMPARE(properties.value("PlaybackStatus").toString(), QString("Stopped"));
    QVERIFY(properties.value("CanPlay").toBool());
    QVERIFY(!properties.value("CanSeek").toBool());
    auto metadata = qdbus_cast<QVariantMap>(properties.value("Metadata"));
    QCOMPARE(metadata.value("xesam:title").toString(), QString("fixture"));
    QCOMPARE(metadata.value("mpris:length").toLongLong(), 10000000LL);
    QSignalSpy action(&backend, &Backend::desktopAction);
    auto next =
        QDBusMessage::createMethodCall(name, "/org/mpris/MediaPlayer2",
                                       "org.mpris.MediaPlayer2.Player", "Next");
    auto nextCall = bus.asyncCall(next);
    QTRY_VERIFY(nextCall.isFinished());
    QCOMPARE(action.count(), 1);
    QCOMPARE(action.first().first().toString(), QString("next"));
    auto set = QDBusMessage::createMethodCall(name, "/org/mpris/MediaPlayer2",
                                              "org.freedesktop.DBus.Properties",
                                              "Set");
    set << QString("org.mpris.MediaPlayer2.Player") << QString("Volume")
        << QVariant::fromValue(QDBusVariant(.25));
    auto setCall = bus.asyncCall(set);
    QTRY_VERIFY(setCall.isFinished());
    QCOMPARE(backend.volume(), 25.0);
    bus.unregisterObject("/org/mpris/MediaPlayer2");
    bus.unregisterService(name);
  }
};
QTEST_MAIN(MprisTest)
#include "mpris_test.moc"
