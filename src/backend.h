#pragma once
#include "session.h"
#include <QFileSystemWatcher>
#include <QThread>
#include <QVariantMap>
#include <mpv/client.h>
class Backend : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
  Q_PROPERTY(int fontSize READ fontSize NOTIFY themeChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playerChanged)
  Q_PROPERTY(double position READ position NOTIFY playerChanged)
  Q_PROPERTY(double duration READ duration NOTIFY playerChanged)
  Q_PROPERTY(bool loaded READ hasMedia NOTIFY playerChanged)
  Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)
  Q_PROPERTY(double volume READ volume NOTIFY playerChanged)
public:
  explicit Backend(QObject *parent = nullptr);
  ~Backend();
  QVariantMap theme() const { return colors; }
  int fontSize() const { return baseSize; }
  bool playing() const { return loaded && !paused; }
  bool hasMedia() const { return loaded; }
  mpv_handle *playerHandle() const { return mpv; }
  QVariantMap metadata() const { return trackMetadata; }
  double rate() const { return playbackRate; }
  double position() const { return time; }
  double duration() const { return length; }
  double volume() const { return gain; }
  Q_INVOKABLE int request(QString path, QVariantMap args = {},
                          QString mode = "weapi");
  Q_INVOKABLE void saveAccount(QString id);
  Q_INVOKABLE void logout();
  Q_INVOKABLE QString qrImage(QString text);
  Q_INVOKABLE void load(QString url, double previewEnd = 0,
                        double previewStart = 0, bool startPaused = false);
  Q_INVOKABLE void toggle();
  Q_INVOKABLE void setPaused(bool value);
  Q_INVOKABLE void setMetadata(QVariantMap value);
  Q_INVOKABLE QString newId() const;
  Q_INVOKABLE void scanLocal(QVariantList urls);
  Q_INVOKABLE void stop();
  Q_INVOKABLE void seek(double seconds);
  Q_INVOKABLE void setVolume(double value);
  Q_INVOKABLE void setSpeed(double value);
  static QVariantList localFiles(QVariantList urls);
  Q_INVOKABLE QString state(QString key) const;
  Q_INVOKABLE void setState(QString key, QString value);
  Q_INVOKABLE void copyText(QString text);
signals:
  void response(int id, QVariantMap data, QString error);
  void message(QString text);
  void themeChanged();
  void playerChanged();
  void playbackEnded();
  void mediaLoaded();
  void metadataChanged();
  void localReady(QVariantList tracks);
  void desktopAction(QString action);
  void seeked(qint64 position);

private:
  void readTheme();
  void drainPlayer();
  QThread networkThread;
  Session *session;
  int nextId = 0;
  mpv_handle *mpv = nullptr;
  bool paused = true, loaded = false;
  QVariantMap trackMetadata;
  double playbackRate = 1;
  double time = 0, length = 0, gain = 65;
  QVariantMap colors;
  int baseSize = 14;
  QFileSystemWatcher watcher;
};
