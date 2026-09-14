#pragma once
#include "loginflow.h"
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QObject>
#include <QPointer>
#include <functional>
class Storage;
class CookieJar : public QNetworkCookieJar {
public:
  using QNetworkCookieJar::QNetworkCookieJar;
  QByteArray serialize() const;
  void restore(const QByteArray &data);
  QByteArray value(const QByteArray &name) const;
};
class Session : public QObject {
  Q_OBJECT
public:
  explicit Session(QObject *parent = nullptr, Storage *storage = nullptr);
public slots:
  void initialize();
  void submit(int id, QString path, QJsonObject data, QString mode = "weapi",
              bool cacheRead = false);
  void reset(bool clearSecret = true);
  void save(QString account);
  void startLogin();
  void cancelLogin();
  void restoreAccount();
  void retryLogin();
signals:
  void finished(int id, QJsonObject data, QString error);
  void persistenceStatus(QString message);
  void loginChanged(QString phase, QString message);
  void loginChallenge(QString url);
  void accountReady(QJsonObject profile, bool afterLogin);

private:
  void complete(int id, QJsonObject data, QString error);
  Storage *storage = nullptr;
  LoginFlow *login = nullptr;
  int nextInternalId = 0;
  QHash<int, LoginFlow::Callback> internalRequests;
  QHash<int, QString> cacheKeys;
  void bootstrap(std::function<void(QString)> completion);
  void send(int id, const QString &path, QJsonObject data, const QString &mode);
  void
  post(const QUrl &url, const QJsonObject &form,
       const QList<QPair<QByteArray, QByteArray>> &headers,
       std::function<void(QByteArray, QNetworkReply *, QString)> completion);
  QNetworkAccessManager *network = nullptr;
  CookieJar *jar = nullptr;
  QJsonObject keyState;
  QByteArray sessionKey, device;
  QString sessionId, accountId;
  quint64 generation = 0;
  bool bootstrapping = false;
  QList<std::function<void(QString)>> bootstrapWaiters;
};
