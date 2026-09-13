#include "storage.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <taglib/fileref.h>
struct Storage::Transfer {
  QString id, path, temp, owner, checksum;
  QVariantMap track;
  QFile file;
  QNetworkReply *reply = nullptr;
  qint64 received = 0, total = 0, resumeOffset = 0;
  bool headersChecked = false, paused = false;
  QString error;
  QElapsedTimer report;
};
Storage::Storage(QObject *parent, QString data, QString music)
    : QObject(parent), dataRoot(data), musicRoot(music) {}
Storage::~Storage() {
  for (const auto &task : active) {
    task->paused = true;
    task->reply->disconnect(this);
    task->reply->abort();
    task->file.close();
  }
  auto name = database.connectionName();
  database.close();
  database = QSqlDatabase();
  QSqlDatabase::removeDatabase(name);
}
void Storage::initialize() {
  auto dir =
      dataRoot.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
          : dataRoot;
  QDir().mkpath(dir);
  QFile::setPermissions(dir,
                        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
  database =
      QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
  database.setDatabaseName(dir + "/library.sqlite3");
  if (!database.open()) {
    emit message("音乐库数据库无法打开：" + database.lastError().text());
    return;
  }
  QSqlQuery q(database);
  q.exec("PRAGMA journal_mode=WAL");
  q.exec("PRAGMA busy_timeout=3000");
  q.exec("CREATE TABLE IF NOT EXISTS cache (key TEXT PRIMARY KEY, time INTEGER "
         "NOT NULL, data BLOB NOT NULL)");
  q.exec("CREATE TABLE IF NOT EXISTS local_tracks (id TEXT PRIMARY KEY, data "
         "BLOB NOT NULL)");
  q.exec("CREATE TABLE IF NOT EXISTS downloads (id TEXT PRIMARY KEY, account "
         "TEXT NOT NULL, track BLOB NOT NULL, path TEXT NOT NULL, temp TEXT "
         "NOT NULL, state TEXT NOT NULL, received INTEGER NOT NULL, total "
         "INTEGER NOT NULL, error TEXT NOT NULL, time INTEGER NOT NULL)");
  q.exec("UPDATE downloads SET state='paused',error='重新获取下载权限后可继续' "
         "WHERE state='running'");
  QVariantList tracks;
  if (q.exec("SELECT data FROM local_tracks ORDER BY id"))
    while (q.next())
      tracks << QJsonDocument::fromJson(q.value(0).toByteArray())
                    .object()
                    .toVariantMap();
  emit localLoaded(tracks);
}
QJsonObject Storage::cached(const QString &key) {
  QSqlQuery q(database);
  q.prepare("SELECT time,data FROM cache WHERE key=?");
  q.addBindValue(key);
  if (!q.exec() || !q.next())
    return {};
  auto result = QJsonDocument::fromJson(q.value(1).toByteArray()).object();
  result["_stale"] = true;
  result["_cacheTimestamp"] = q.value(0).toLongLong();
  return result;
}
void Storage::cache(const QString &key, const QJsonObject &data) {
  const auto bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
  if (bytes.size() > 2 * 1024 * 1024)
    return;
  QSqlQuery q(database);
  q.prepare("INSERT OR REPLACE INTO cache VALUES (?,?,?)");
  q.addBindValue(key);
  q.addBindValue(QDateTime::currentSecsSinceEpoch());
  q.addBindValue(bytes);
  q.exec();
  q.exec("DELETE FROM cache WHERE key IN (SELECT key FROM cache ORDER BY time "
         "DESC LIMIT -1 OFFSET 64)");
}
void Storage::storeLocal(QVariantList tracks) {
  if (!database.transaction())
    return;
  QSqlQuery q(database);
  q.prepare("INSERT OR REPLACE INTO local_tracks VALUES (?,?)");
  for (const auto &item : tracks) {
    auto track = item.toMap();
    q.bindValue(0, track.value("id"));
    q.bindValue(
        1, QJsonDocument::fromVariant(track).toJson(QJsonDocument::Compact));
    if (!q.exec()) {
      database.rollback();
      emit message("本地音乐库保存失败");
      return;
    }
  }
  if (!database.commit())
    emit message("本地音乐库保存失败");
}
void Storage::account(QString id) {
  if (accountId == id) {
    publish();
    return;
  }
  const auto tasks = active.values();
  for (const auto &task : tasks)
    pauseDownload(task->id);
  accountId = id;
  publish();
}
void Storage::publish() {
  QVariantList list;
  QSqlQuery q(database);
  q.prepare("SELECT id,track,path,state,received,total,error FROM downloads "
            "WHERE account=? ORDER BY time DESC, rowid DESC");
  q.addBindValue(accountId);
  if (q.exec())
    while (q.next()) {
      auto row = QJsonDocument::fromJson(q.value(1).toByteArray())
                     .object()
                     .toVariantMap();
      row["taskId"] = q.value(0);
      row["fileUrl"] = QUrl::fromLocalFile(q.value(2).toString()).toString();
      row["state"] = q.value(3);
      row["received"] = q.value(4);
      row["total"] = q.value(5);
      row["error"] = q.value(6);
      list << row;
    }
  emit downloadsChanged(list);
}
void Storage::updateTask(const std::shared_ptr<Transfer> &t, QString state,
                         QString error) {
  QSqlQuery q(database);
  q.prepare(
      "UPDATE downloads SET state=?,received=?,total=?,error=? WHERE id=?");
  q.addBindValue(state);
  q.addBindValue(t->received);
  q.addBindValue(t->total);
  q.addBindValue(error.isNull() ? QStringLiteral("") : error);
  q.addBindValue(t->id);
  if (!q.exec())
    t->error = "下载状态无法保存：" + q.lastError().text();
  publish();
}
void Storage::pauseDownload(QString id) {
  auto t = active.value(id);
  if (!t)
    return;
  t->paused = true;
  t->reply->abort();
}
void Storage::startDownload(QVariantMap track, QVariantMap grant,
                            QString retryId) {
  if (accountId.isEmpty()) {
    emit message("请先登录再下载");
    return;
  }
  if (active.size() >= 3) {
    emit message("最多同时下载 3 首，请稍后重试");
    return;
  }
  for (const auto &t : active)
    if (t->owner == accountId && t->track.value("id") == track.value("id")) {
      emit message("此歌曲已在下载");
      return;
    }
  QUrl url(grant.value("url").toString());
  const auto type = grant.value("type").toString().toLower();
  if ((url.scheme() != "https" && url.scheme() != "http") ||
      !QStringList{"mp3", "flac", "m4a", "aac", "ogg", "opus", "wav"}.contains(
          type) ||
      !grant.value("freeTrialInfo").isNull()) {
    emit message("服务未提供普通音频文件的下载权限");
    return;
  }
  auto t = std::make_shared<Transfer>();
  t->owner = accountId;
  t->track = track;
  t->checksum = grant.value("md5").toString();
  t->total = grant.value("size").toLongLong();
  if (t->total > 2LL * 1024 * 1024 * 1024) {
    emit message("文件超过当前下载大小限制");
    return;
  }
  if (!retryId.isEmpty()) {
    QSqlQuery q(database);
    q.prepare("SELECT path,temp FROM downloads WHERE id=? AND account=? AND "
              "state!='complete'");
    q.addBindValue(retryId);
    q.addBindValue(accountId);
    if (!q.exec() || !q.next()) {
      emit message("下载任务不存在");
      return;
    }
    t->id = retryId;
    t->path = q.value(0).toString();
    t->temp = q.value(1).toString();
  } else {
    t->id = QUuid::createUuid().toString(QUuid::Id128);
    auto dir =
        musicRoot.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
                  "/云间"
            : musicRoot;
    if (!QDir().mkpath(dir)) {
      emit message("下载目录无法创建");
      return;
    }
    auto name = track.value("name").toString().left(80);
    name.replace(QRegularExpression("[\\x00-\\x1f/\\\\:*?\"<>|]"), "_");
    t->path = dir + "/" + name + "-" + t->id.left(8) + "." + type;
    t->temp = dir + "/." + name + "-" + t->id.left(8) + ".part." + type;
    QSqlQuery q(database);
    q.prepare("INSERT INTO downloads VALUES (?,?,?,?,?,'running',0,?,'',?)");
    q.addBindValue(t->id);
    q.addBindValue(accountId);
    q.addBindValue(
        QJsonDocument::fromVariant(track).toJson(QJsonDocument::Compact));
    q.addBindValue(t->path);
    q.addBindValue(t->temp);
    q.addBindValue(t->total);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
      emit message("无法保存下载任务");
      return;
    }
  }
  t->file.setFileName(t->temp);
  if (!t->file.open(
          QIODevice::ReadWrite |
          (retryId.isEmpty() ? QIODevice::NewOnly : QIODevice::OpenMode{}))) {
    updateTask(t, "failed", t->file.errorString());
    return;
  }
  QFile::setPermissions(t->temp, QFile::ReadOwner | QFile::WriteOwner);
  t->resumeOffset = t->received = t->file.size();
  t->file.seek(t->received);
  if (!network)
    network = new QNetworkAccessManager(this);
  QNetworkRequest req(url);
  req.setTransferTimeout(30000);
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);
  if (t->received > 0)
    req.setRawHeader("Range", "bytes=" + QByteArray::number(t->received) + "-");
  t->reply = network->get(req);
  t->reply->setReadBufferSize(256 * 1024);
  active[t->id] = t;
  t->report.start();
  updateTask(t, "running");
  connect(t->reply, &QIODevice::readyRead, this, [this, t] {
    if (!t->headersChecked) {
      const int status =
          t->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      if (status != 200 && status != 206)
        return;
      t->headersChecked = true;
      if (status == 200 && t->resumeOffset) {
        if (!t->file.resize(0) || !t->file.seek(0))
          t->error = "无法重置下载文件";
        t->received = 0;
      }
      if (status == 206 &&
          !t->reply->rawHeader("Content-Range")
               .startsWith("bytes " + QByteArray::number(t->resumeOffset) +
                           "-"))
        t->error = "下载续传范围不匹配";
    }
    while (t->reply->bytesAvailable() > 0 && t->error.isEmpty()) {
      auto bytes = t->reply->read(256 * 1024);
      if (t->received + bytes.size() > 2LL * 1024 * 1024 * 1024 ||
          (t->total > 0 && t->received + bytes.size() > t->total)) {
        t->error = "文件大小与下载许可不符";
        break;
      }
      if (t->file.write(bytes) != bytes.size()) {
        t->error = t->file.errorString();
        break;
      }
      t->received += bytes.size();
    }
    if (!t->error.isEmpty())
      t->reply->abort();
    else if (t->report.elapsed() > 500) {
      t->report.restart();
      updateTask(t, "running");
    }
  });
  connect(t->reply, &QNetworkReply::finished, this, [this, t] {
    if (!t->file.flush() && t->error.isEmpty())
      t->error = t->file.errorString();
    t->file.close();
    QString state = "complete", error = t->error;
    if (t->paused) {
      state = "paused";
      error = "重新获取下载权限后可继续";
    } else if (error.isEmpty() && t->reply->error() != QNetworkReply::NoError)
      error = t->reply->errorString();
    if (state == "complete" && error.isEmpty()) {
      if (!t->headersChecked || t->received <= 0 ||
          (t->total > 0 && t->received != t->total))
        error = "下载文件不完整";
      else if (!t->checksum.isEmpty()) {
        QFile file(t->temp);
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file) ||
            hash.result().toHex() != t->checksum.toLatin1().toLower())
          error = "下载文件校验失败";
      }
      if (error.isEmpty()) {
        TagLib::FileRef file(t->temp.toUtf8().constData());
        if (file.isNull() || !file.audioProperties())
          error = "此下载不是可验证的普通音频文件";
      }
      if (error.isEmpty() && !QFile::rename(t->temp, t->path))
        error = "下载完成，但无法保存最终文件";
    }
    if (state != "paused" && !error.isEmpty())
      state = "failed";
    active.remove(t->id);
    updateTask(t, state, error);
    t->reply->deleteLater();
  });
}
