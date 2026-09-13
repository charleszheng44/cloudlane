#include "session.h"
#include "crypto.h"
#include "storage.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QUuid>
#include <condition_variable>
#pragma push_macro("signals")
#undef signals
#include <libsecret/secret.h>
#pragma pop_macro("signals")
#include <memory>
#include <mutex>
#include <thread>
namespace {
// Secret Service may wait for a locked keyring. Bound that wait, including
// shutdown.
class SecretDeadline {
public:
  GCancellable *cancel = g_cancellable_new();
  SecretDeadline()
      : timer([this] {
          std::unique_lock lock(mutex);
          if (!ready.wait_for(lock, std::chrono::seconds(8),
                              [this] { return done; }))
            g_cancellable_cancel(cancel);
        }) {}
  ~SecretDeadline() {
    {
      std::lock_guard lock(mutex);
      done = true;
    }
    ready.notify_one();
    timer.join();
    g_object_unref(cancel);
  }

private:
  std::mutex mutex;
  std::condition_variable ready;
  bool done = false;
  std::thread timer;
};
const SecretSchema schema = {"io.github.charleszheng44.Yunjian",
                             SECRET_SCHEMA_NONE,
                             {{"account", SECRET_SCHEMA_ATTRIBUTE_STRING},
                              {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}},
                             0,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr};
const QByteArray desktopUa =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/124.0.0.0 "
    "Safari/537.36";
const QByteArray mobileUa =
    "NeteaseMusic/9.5.61.260802021928(9005061);Dalvik/2.1.0 (Linux; U; Android "
    "12; HBN-AL00 Build/cd737a2.0)";
} // namespace
QByteArray CookieJar::serialize() const {
  QJsonArray a;
  for (const auto &c : allCookies())
    a.append(QString::fromLatin1(c.toRawForm().toBase64()));
  return QJsonDocument(a).toJson(QJsonDocument::Compact);
}
void CookieJar::restore(const QByteArray &data) {
  QList<QNetworkCookie> cookies;
  for (const auto &v : QJsonDocument::fromJson(data).array())
    cookies += QNetworkCookie::parseCookies(
        QByteArray::fromBase64(v.toString().toLatin1()));
  setAllCookies(cookies);
}
QByteArray CookieJar::value(const QByteArray &name) const {
  for (const auto &cookie : allCookies())
    if (cookie.name() == name &&
        (cookie.isSessionCookie() ||
         cookie.expirationDate() > QDateTime::currentDateTimeUtc()) &&
        (cookie.domain() == ".music.163.com" ||
         cookie.domain() == "music.163.com" ||
         cookie.domain().endsWith(".music.163.com")))
      return cookie.value();
  return {};
}
Session::Session(QObject *p, Storage *store) : QObject(p), storage(store) {}
void Session::initialize() {
  QSettings settings;
  device = settings.value("deviceId").toByteArray();
  if (device.isEmpty()) {
    device = TransportCrypto::randomBytes(16).toHex();
    settings.setValue("deviceId", device);
  }
  reset(false);
  accountId = settings.value("accountId").toString();
  if (storage)
    storage->account(accountId);
  if (!accountId.isEmpty()) {
    SecretDeadline deadline;
    GError *error = nullptr;
    char *secret =
        secret_password_lookup_sync(&schema, deadline.cancel, &error, "account",
                                    accountId.toUtf8().constData(), nullptr);
    if (secret) {
      jar->restore(QByteArray::fromBase64(secret));
      secret_password_free(secret);
    }
    if (error) {
      emit persistenceStatus("系统钥匙环暂不可用，可使用本次会话登录。");
      g_error_free(error);
    }
  }
}
void Session::reset(bool clearSecret) {
  ++generation;
  cacheKeys.clear();
  auto retired = network;
  network = new QNetworkAccessManager(this);
  jar = new CookieJar(network);
  network->setCookieJar(jar);
  network->setRedirectPolicy(QNetworkRequest::ManualRedirectPolicy);
  keyState = {};
  sessionKey.clear();
  sessionId.clear();
  bootstrapping = false;
  auto waiters = std::move(bootstrapWaiters);
  bootstrapWaiters.clear();
  if (retired) {
    for (auto *reply : retired->findChildren<QNetworkReply *>())
      reply->abort();
    retired->deleteLater();
  }
  for (auto &cb : waiters)
    cb("登录会话已更改");
  if (clearSecret && !accountId.isEmpty()) {
    SecretDeadline deadline;
    GError *error = nullptr;
    secret_password_clear_sync(&schema, deadline.cancel, &error, "account",
                               accountId.toUtf8().constData(), nullptr);
    if (error)
      g_error_free(error);
    QSettings().remove("accountId");
    accountId.clear();
    if (storage)
      storage->account({});
  }
}
void Session::save(QString account) {
  if (account != accountId) {
    const auto freshCookies = jar->serialize();
    reset(false);
    jar->restore(freshCookies);
  }
  accountId = account;
  if (storage)
    storage->account(accountId);
  QSettings().setValue("accountId", account);
  SecretDeadline deadline;
  GError *error = nullptr;
  const auto bytes = jar->serialize().toBase64();
  bool stored = secret_password_store_sync(
      &schema, SECRET_COLLECTION_DEFAULT, "云间 · 网易云音乐账户",
      bytes.constData(), deadline.cancel, &error, "account",
      account.toUtf8().constData(), nullptr);
  if (!stored)
    emit persistenceStatus("系统钥匙环未保存登录；当前为会话登录。");
  if (error)
    g_error_free(error);
}
void Session::post(
    const QUrl &url, const QJsonObject &form,
    const QList<QPair<QByteArray, QByteArray>> &headers,
    std::function<void(QByteArray, QNetworkReply *, QString)> done) {
  QNetworkRequest request(url);
  request.setTransferTimeout(20000);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded;charset=utf-8");
  for (auto pair : headers)
    request.setRawHeader(pair.first, pair.second);
  auto reply = network->post(request, TransportCrypto::form(form));
  auto body = std::make_shared<QByteArray>();
  const auto gen = generation;
  connect(reply, &QIODevice::readyRead, reply, [reply, body] {
    body->append(reply->readAll());
    if (body->size() > 20 * 1024 * 1024)
      reply->abort();
  });
  connect(reply, &QNetworkReply::finished, this,
          [this, reply, body, gen, done = std::move(done)] {
            body->append(reply->readAll());
            QString error;
            if (gen != generation)
              error = "登录会话已更改";
            else if (body->size() > 20 * 1024 * 1024)
              error = "服务响应过大";
            else if (reply->error() != QNetworkReply::NoError)
              error = reply->errorString();
            else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                             .toInt() /
                         100 ==
                     3)
              error = "服务返回了未支持的重定向";
            done(*body, reply, error);
            reply->deleteLater();
          });
}
void Session::bootstrap(std::function<void(QString)> done) {
  if (!keyState.isEmpty()) {
    done({});
    return;
  }
  bootstrapWaiters.append(std::move(done));
  if (bootstrapping)
    return;
  bootstrapping = true;
  auto timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
  auto nonce = QString::fromLatin1(TransportCrypto::randomBytes(8).toHex());
  QJsonObject data{
      {"appVersion", "9.5.61"},
      {"currentKeyVersion", ""},
      {"deviceId", QString::fromLatin1(device)},
      {"nonce", nonce},
      {"os", "android"},
      {"requestType", "active"},
      {"signature",
       QString::fromLatin1(TransportCrypto::keySignature(timestamp, nonce))},
      {"t1", ""},
      {"t2", ""},
      {"timestamp", timestamp},
      {"uid", ""}};
  const auto gen = generation;
  post(QUrl("https://interface.music.163.com/api/gorilla/anti/crawler/security/"
            "key/get"),
       data, {{"User-Agent", mobileUa}, {"Cookie", "deviceId=" + device}},
       [this, gen, nonce](QByteArray body, QNetworkReply *, QString error) {
         if (gen != generation)
           return;
         try {
           if (error.isEmpty()) {
             auto root = QJsonDocument::fromJson(body).object();
             auto d = root.value("data").toObject();
             auto signature = TransportCrypto::keySignature(
                 d.value("timestamp").toVariant().toString(), nonce);
             if (root.value("code").toInt() != 200 ||
                 signature != d.value("signature").toString().toLatin1())
               error = "无法验证播放协议密钥";
             else
               keyState = TransportCrypto::decodePublicKey(
                   d.value("encryptedData").toString());
           }
         } catch (const std::exception &e) {
           error = QString::fromUtf8(e.what());
         }
         bootstrapping = false;
         auto pending = std::move(bootstrapWaiters);
         bootstrapWaiters.clear();
         for (auto &cb : pending)
           cb(error);
       });
}
void Session::submit(int id, QString path, QJsonObject data, QString mode,
                     bool cacheRead) {
  static const QSet<QString> readable{"/api/personalized/playlist",
                                      "/api/playlist/list",
                                      "/api/toplist",
                                      "/api/v1/discovery/new/songs",
                                      "/api/album/new",
                                      "/api/djradio/recommend/v1",
                                      "/api/mv/all",
                                      "/api/user/playlist",
                                      "/api/album/sublist",
                                      "/api/artist/sublist",
                                      "/api/djradio/get/subed",
                                      "/api/cloudvideo/allvideo/sublist",
                                      "/api/play-record/song/list",
                                      "/api/digitalAlbum/purchased",
                                      "/api/v1/cloud/get",
                                      "/api/cloudsearch/pc",
                                      "/api/v1/artist/songs",
                                      "/api/dj/program/byradio",
                                      "/api/v3/discovery/recommend/songs",
                                      "/api/v1/event/get",
                                      "/api/msg/notices",
                                      "/api/msg/private/users",
                                      "/api/msg/private/history"};
  if (cacheRead && storage && readable.contains(path)) {
    auto key = accountId.toUtf8() + "/" + mode.toUtf8() + "/" + path.toUtf8() +
               "/" + QJsonDocument(data).toJson(QJsonDocument::Compact);
    cacheKeys.insert(
        id,
        QString::fromLatin1(
            QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex()));
  }
  if (!network)
    initialize();
  if (!path.startsWith("/api/") || path.contains("..") || path.contains('?')) {
    complete(id, {}, "无效服务请求");
    return;
  }
  if (mode == "xeapi") {
    const auto gen = generation;
    bootstrap([this, id, path, data, mode, gen](QString error) {
      if (gen != generation) {
        complete(id, {}, "登录会话已更改");
        return;
      }
      if (!error.isEmpty())
        complete(id, {}, error);
      else
        send(id, path, data, mode);
    });
  } else
    send(id, path, data, mode);
}
void Session::send(int id, const QString &path, QJsonObject data,
                   const QString &mode) {
  try {
    const auto csrf = jar->value("__csrf");
    QJsonObject cookie{
        {"os", "pc"},
        {"appver", "3.1.17.204416"},
        {"osver", "Microsoft-Windows-10-Professional-build-19045-64bit"},
        {"channel", "netease"},
        {"deviceId", QString::fromLatin1(device)},
        {"__remember_me", "true"},
        {"_ntes_nuid", QString::fromLatin1(device)}};
    for (const auto &name : {QByteArray("MUSIC_U"), QByteArray("MUSIC_A"),
                             QByteArray("__csrf"), QByteArray("NMTID")})
      if (!jar->value(name).isEmpty())
        cookie[QString::fromLatin1(name)] =
            QString::fromLatin1(jar->value(name));
    QList<QPair<QByteArray, QByteArray>> headers{
        {"Referer", "https://music.163.com"}};
    QJsonObject encrypted;
    QString url;
    data["e_r"] = false;
    if (mode == "weapi") {
      data["csrf_token"] = QString::fromLatin1(csrf);
      encrypted = TransportCrypto::weapi(
          QJsonDocument(data).toJson(QJsonDocument::Compact));
      url = "https://music.163.com/weapi/" + path.mid(5);
      headers.append({"User-Agent", desktopUa});
    } else if (mode == "eapi") {
      QJsonObject header = cookie;
      header["requestId"] =
          QString::number(QDateTime::currentMSecsSinceEpoch()) + "_0123";
      header["buildver"] = QString::number(QDateTime::currentSecsSinceEpoch());
      data["header"] = header;
      encrypted = TransportCrypto::eapi(
          path, QJsonDocument(data).toJson(QJsonDocument::Compact));
      url = "https://interfacepc.music.163.com/eapi/" + path.mid(5);
      headers.append({"User-Agent", desktopUa});
    } else if (mode == "xeapi") {
      cookie["os"] = "android";
      cookie["osver"] = "16";
      cookie["appver"] = "9.5.61";
      cookie["sDeviceId"] = QString::fromLatin1(device);
      auto build = QByteArray::number(QDateTime::currentSecsSinceEpoch());
      headers += {{"User-Agent", mobileUa}, {"X-Client-Enc-State", "ENCRYPTED"},
                  {"x-aeapi", "true"},      {"x-deviceid", device},
                  {"x-sdeviceid", device},  {"x-os", "android"},
                  {"x-osver", "16"},        {"x-appver", "9.5.61"},
                  {"x-buildver", build}};
      if (!jar->value("MUSIC_U").isEmpty())
        headers.append({"x-music-u", jar->value("MUSIC_U")});
      encrypted =
          TransportCrypto::xeapi(path, data, keyState, sessionKey, sessionId);
      url = "https://interface3.music.163.com/xeapi/" + path.mid(5);
    } else {
      complete(id, {}, "未支持的服务协议");
      return;
    }
    QByteArray cookieHeader;
    for (auto it = cookie.begin(); it != cookie.end(); ++it) {
      if (!cookieHeader.isEmpty())
        cookieHeader += "; ";
      cookieHeader +=
          it.key().toLatin1() + "=" + it.value().toString().toLatin1();
    }
    headers.append({"Cookie", cookieHeader});
    const auto gen = generation;
    post(QUrl(url), encrypted, headers,
         [this, id, gen, mode](QByteArray body, QNetworkReply *reply,
                               QString error) {
           if (gen != generation) {
             complete(id, {}, "登录会话已更改");
             return;
           }
           QJsonObject root;
           try {
             if (error.isEmpty()) {
               QJsonParseError parse;
               root = QJsonDocument::fromJson(
                          TransportCrypto::decodeResponse(body), &parse)
                          .object();
               if (parse.error != QJsonParseError::NoError || root.isEmpty())
                 error = "服务响应格式无法识别";
               if (mode == "xeapi" && reply->hasRawHeader("x-encr-ssid") &&
                   reply->hasRawHeader("x-encr-sskey")) {
                 auto key = reply->rawHeader("x-encr-sskey");
                 if (key.size() == 16 || key.size() == 32) {
                   sessionId =
                       QString::fromLatin1(reply->rawHeader("x-encr-ssid"));
                   sessionKey = key;
                 }
               }
             }
           } catch (const std::exception &e) {
             error = QString::fromUtf8(e.what());
           }
           complete(id, root, error);
         });
  } catch (const std::exception &e) {
    complete(id, {}, QString::fromUtf8(e.what()));
  }
}

void Session::complete(int id, QJsonObject data, QString error) {
  const auto key = cacheKeys.take(id);
  if (storage && !key.isEmpty()) {
    if (error.isEmpty() && data.value("code").toInt() == 200)
      storage->cache(key, data);
    else if (!error.isEmpty() || data.value("code").toInt() >= 500 ||
             data.value("code").toInt() == 429) {
      auto saved = storage->cached(key);
      if (!saved.isEmpty()) {
        data = saved;
        error.clear();
      }
    }
  }
  emit finished(id, data, error);
}
