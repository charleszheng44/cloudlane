#pragma once
#include "session.h"
#include "storage.h"
#include <QFileSystemWatcher>
#include <QThread>
#include <QVariantMap>
#include <mpv/client.h>
class Backend : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString loginPhase READ loginPhase NOTIFY loginChanged)
  Q_PROPERTY(QString loginStatus READ loginStatus NOTIFY loginChanged)
  Q_PROPERTY(QString loginQr READ loginQr NOTIFY loginChanged)
  Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
  Q_PROPERTY(int fontSize READ fontSize NOTIFY themeChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playerChanged)
  Q_PROPERTY(double position READ position NOTIFY playerChanged)
  Q_PROPERTY(double seekMinimum READ seekMinimum NOTIFY playerChanged)
  Q_PROPERTY(double duration READ duration NOTIFY playerChanged)
  Q_PROPERTY(bool loaded READ hasMedia NOTIFY playerChanged)
  Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)
  Q_PROPERTY(QVariantList downloads READ downloads NOTIFY downloadsChanged)
  Q_PROPERTY(double volume READ volume NOTIFY playerChanged)
public:
  explicit Backend(QObject *parent = nullptr);
  ~Backend();
  QString loginPhase() const { return phase; }
  QString loginStatus() const { return loginMessage; }
  QString loginQr() const { return qr; }
  Q_INVOKABLE void startLogin();
  Q_INVOKABLE void cancelLogin();
  Q_INVOKABLE void restoreAccount();
  Q_INVOKABLE void retryLogin();
  QVariantMap theme() const { return colors; }
  int fontSize() const { return baseSize; }
  bool playing() const { return loaded && !paused; }
  bool hasMedia() const { return loaded; }
  mpv_handle *playerHandle() const { return mpv; }
  QVariantMap metadata() const { return trackMetadata; }
  QVariantList downloads() const { return downloadTasks; }
  double rate() const { return playbackRate; }
  double position() const { return time; }
  double duration() const {
    return rangeEnd > 0 ? qMin(length, rangeEnd) : length;
  }
  double seekMinimum() const { return rangeStart; }
  double volume() const { return gain; }
  Q_INVOKABLE int request(QString path, QVariantMap args = {},
                          QString mode = "weapi", bool cacheRead = false);
  Q_INVOKABLE void saveAccount(QString id);
  Q_INVOKABLE void logout();
  Q_INVOKABLE QString qrImage(QString text);
  Q_INVOKABLE void load(QString url, double previewEnd = 0,
                        double previewStart = 0, bool startPaused = false);
  Q_INVOKABLE void toggle();
  Q_INVOKABLE void setPaused(bool value);
  Q_INVOKABLE void setMetadata(QVariantMap value);
  Q_INVOKABLE QString newId() const;
  Q_INVOKABLE QVariantMap parseLink(QString text) const;
  Q_INVOKABLE void scanLocal(QVariantList urls);
  Q_INVOKABLE void storeLocal(QVariantList tracks);
  Q_INVOKABLE void download(QVariantMap track, QVariantMap grant,
                            QString retryId = {});
  Q_INVOKABLE void pauseDownload(QString id);
  Q_INVOKABLE void stop();
  Q_INVOKABLE void seek(double seconds);
  Q_INVOKABLE void setVolume(double value);
  Q_INVOKABLE void setSpeed(double value);
  static QVariantList localFiles(QVariantList urls);
  Q_INVOKABLE QString state(QString key) const;
  Q_INVOKABLE void setState(QString key, QString value);
  Q_INVOKABLE void copyText(QString text);
signals:
  void loginChanged();
  void accountReady(QVariantMap profile, bool afterLogin);
  void response(int id, QVariantMap data, QString error);
  void message(QString text);
  void themeChanged();
  void videoUpdate();
  void playerChanged();
  void playbackEnded();
  void mediaLoaded();
  void metadataChanged();
  void localReady(QVariantList tracks);
  void localLoaded(QVariantList tracks);
  void downloadsChanged();
  void desktopAction(QString action);
  void openRequested(QString uri);
  void seeked(qint64 position);

private:
  void readTheme();
  void drainPlayer();
  QThread networkThread;
  Storage *storage;
  Session *session;
  QVariantList downloadTasks;
  int nextId = 0;
  QString phase = "idle", loginMessage, qr;
  mpv_handle *mpv = nullptr;
  bool paused = true, loaded = false, scanInProgress = false;
  QVariantMap trackMetadata;
  double playbackRate = 1, rangeStart = 0, rangeEnd = 0;
  double time = 0, length = 0, gain = 65;
  QVariantMap colors;
  int baseSize = 14;
  QFileSystemWatcher watcher;
};
