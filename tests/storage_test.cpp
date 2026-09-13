#include "storage.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>
class StorageTest : public QObject {
  Q_OBJECT
private slots:
  void cacheAndLocal() {
    QTemporaryDir dir;
    Storage store(nullptr, dir.path() + "/data", dir.path() + "/music");
    store.initialize();
    store.cache("account-a/songs", {{"code", 200}, {"title", "private-a"}});
    QVERIFY(store.cached("account-b/songs").isEmpty());
    auto cached = store.cached("account-a/songs");
    QVERIFY(cached.value("_stale").toBool());
    QCOMPARE(cached.value("title").toString(), QString("private-a"));
    store.storeLocal({QVariantMap{
        {"id", "file:///测试.flac"}, {"name", "测试"}, {"kind", "local"}}});
    Storage reopened(nullptr, dir.path() + "/data", dir.path() + "/music");
    QSignalSpy loaded(&reopened, &Storage::localLoaded);
    reopened.initialize();
    QCOMPARE(loaded.count(), 1);
    QCOMPARE(loaded.first().first().toList().size(), 1);
  }
  void downloads() {
    QTemporaryDir dir;
    QFile audio(qEnvironmentVariable("YUNJIAN_TEST_AUDIO"));
    QVERIFY(audio.open(QIODevice::ReadOnly));
    const auto bytes = audio.readAll();
    QVERIFY(bytes.size() > 1000);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    bool supportRange = true;
    connect(&server, &QTcpServer::newConnection, this, [&] {
      auto socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
        const auto request = socket->readAll();
        if (!request.contains("\r\n\r\n"))
          return;
        qint64 offset = 0;
        auto match =
            QRegularExpression("Range: bytes=(\\d+)-",
                               QRegularExpression::CaseInsensitiveOption)
                .match(QString::fromLatin1(request));
        if (match.hasMatch() && supportRange)
          offset = match.captured(1).toLongLong();
        auto payload = bytes.mid(offset);
        QByteArray header =
            offset ? "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes " +
                         QByteArray::number(offset) + "-" +
                         QByteArray::number(bytes.size() - 1) + "/" +
                         QByteArray::number(bytes.size()) + "\r\n"
                   : "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: audio/wav\r\nContent-Length: " +
                  QByteArray::number(payload.size()) +
                  "\r\nConnection: close\r\n\r\n";
        socket->write(header + payload);
        socket->disconnectFromHost();
      });
    });
    const auto url =
        "http://127.0.0.1:" + QString::number(server.serverPort()) +
        "/audio.wav?temporary-grant=secret";
    QVariantMap grant{
        {"url", url},
        {"type", "wav"},
        {"size", bytes.size()},
        {"md5", QString::fromLatin1(
                    QCryptographicHash::hash(bytes, QCryptographicHash::Md5)
                        .toHex())}};
    Storage store(nullptr, dir.path() + "/data", dir.path() + "/music");
    connect(&store, &Storage::message, this,
            [](QString message) { qInfo() << message; });
    store.initialize();
    QVariantList tasks;
    connect(&store, &Storage::downloadsChanged, this, [&](QVariantList data) {
      tasks = data;
    });
    store.account("fixture-account");
    store.startDownload(
        {{"id", "1"}, {"name", "Download test"}, {"kind", "song"}}, grant);
    QTRY_VERIFY_WITH_TIMEOUT(
        !tasks.isEmpty() && tasks.first().toMap().value("state") == "complete",
        5000);
    auto task = tasks.first().toMap();
    QFile output(QUrl(task.value("fileUrl").toString()).toLocalFile());
    QVERIFY(output.open(QIODevice::ReadOnly));
    QCOMPARE(output.readAll(), bytes);
    output.close();
    // Account isolation, and persistence without copying the signed URL into
    // the DB.
    store.account("other-account");
    QVERIFY(tasks.isEmpty());
    store.account("fixture-account");
    QCOMPARE(tasks.size(), 1);
    QFile database(dir.path() + "/data/library.sqlite3");
    QVERIFY(database.open(QIODevice::ReadOnly));
    QVERIFY(!database.readAll().contains("temporary-grant"));
    // A mismatched integrity value fails rather than exposing a completed file.
    auto bad = grant;
    bad["md5"] = QString(32, '0');
    store.startDownload(
        {{"id", "2"}, {"name", "Bad checksum"}, {"kind", "song"}}, bad);
    QTRY_VERIFY_WITH_TIMEOUT(
        tasks.size() == 2 && tasks.first().toMap().value("state") == "failed",
        5000);
    QVERIFY(!QFileInfo::exists(
        QUrl(tasks.first().toMap().value("fileUrl").toString()).toLocalFile()));
    // Retry from a partial file while the server ignores Range: restart
    // cleanly.
    const auto retry = tasks.first().toMap().value("taskId").toString();
    auto connection = QSqlDatabase::addDatabase("QSQLITE", "fixture-editor");
    connection.setDatabaseName(dir.path() + "/data/library.sqlite3");
    QVERIFY(connection.open());
    {
      QSqlQuery q(connection);
      q.prepare("SELECT temp FROM downloads WHERE id=?");
      q.addBindValue(retry);
      QVERIFY(q.exec());
      QVERIFY(q.next());
      QFile partial(q.value(0).toString());
      QVERIFY(partial.open(QIODevice::WriteOnly | QIODevice::Truncate));
      partial.write(bytes.left(500));
    }
    connection.close();
    connection = QSqlDatabase();
    QSqlDatabase::removeDatabase("fixture-editor");
    supportRange = false;
    store.startDownload(
        {{"id", "2"}, {"name", "Bad checksum"}, {"kind", "song"}}, grant,
        retry);
    QTRY_VERIFY_WITH_TIMEOUT(tasks.first().toMap().value("state") == "complete",
                             5000);
    auto local =
        QUrl(tasks.first().toMap().value("fileUrl").toString()).toLocalFile();
    QFile resumed(local);
    QVERIFY(resumed.open(QIODevice::ReadOnly));
    QCOMPARE(resumed.readAll(), bytes);
  }
};
QTEST_GUILESS_MAIN(StorageTest)
#include "storage_test.moc"
