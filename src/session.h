#pragma once
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QObject>
#include <QPointer>
#include <functional>
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
  explicit Session(QObject *parent = nullptr);
public slots:
  void initialize();
  void submit(int id, QString path, QJsonObject data, QString mode = "weapi");
  void reset(bool clearSecret = true);
  void save(QString account);
signals:
  void finished(int id, QJsonObject data, QString error);
  void persistenceStatus(QString message);

private:
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
