#pragma once
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <memory>
class Storage final : public QObject {
  Q_OBJECT
public:
  explicit Storage(QObject *parent = nullptr, QString dataRoot = {},
                   QString musicRoot = {});
  ~Storage();
  QJsonObject cached(const QString &key);
  void cache(const QString &key, const QJsonObject &data);
public slots:
  void initialize();
  void account(QString id);
  void startDownload(QVariantMap track, QVariantMap grant,
                     QString retryId = {});
  void pauseDownload(QString id);
  void storeLocal(QVariantList tracks);
signals:
  void downloadsChanged(QVariantList tasks);
  void localLoaded(QVariantList tracks);
  void message(QString text);

private:
  struct Transfer;
  void publish();
  void updateTask(const std::shared_ptr<Transfer> &task, QString state,
                  QString error = {});
  QSqlDatabase database;
  QString accountId, dataRoot, musicRoot;
  QNetworkAccessManager *network = nullptr;
  QHash<QString, std::shared_ptr<Transfer>> active;
};
