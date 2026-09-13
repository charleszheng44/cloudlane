#pragma once
#include "backend.h"
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

class MprisRoot final : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
  Q_PROPERTY(bool CanQuit READ yes CONSTANT)
  Q_PROPERTY(bool CanRaise READ yes CONSTANT)
  Q_PROPERTY(bool HasTrackList READ no CONSTANT)
  Q_PROPERTY(QString Identity READ identity CONSTANT)
  Q_PROPERTY(QString DesktopEntry READ desktopEntry CONSTANT)
  Q_PROPERTY(QStringList SupportedUriSchemes READ schemes CONSTANT)
  Q_PROPERTY(QStringList SupportedMimeTypes READ mimeTypes CONSTANT)
public:
  explicit MprisRoot(Backend *backend)
      : QDBusAbstractAdaptor(backend), owner(backend) {}
  bool yes() const { return true; }
  bool no() const { return false; }
  QString identity() const { return QStringLiteral("云间"); }
  QString desktopEntry() const { return "io.github.charleszheng44.Yunjian"; }
  QStringList schemes() const { return {}; }
  QStringList mimeTypes() const { return {}; }
public slots:
  void Raise() { emit owner->desktopAction("raise"); }
  void Quit() { emit owner->desktopAction("quit"); }

private:
  Backend *owner;
};

class MprisPlayer final : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
  Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
  Q_PROPERTY(QVariantMap Metadata READ metadata)
  Q_PROPERTY(double Volume READ volume WRITE setVolume)
  Q_PROPERTY(qlonglong Position READ position)
  Q_PROPERTY(double Rate READ rate WRITE setRate)
  Q_PROPERTY(double MinimumRate READ minimumRate CONSTANT)
  Q_PROPERTY(double MaximumRate READ maximumRate CONSTANT)
  Q_PROPERTY(bool CanGoNext READ canNext)
  Q_PROPERTY(bool CanGoPrevious READ canPrevious)
  Q_PROPERTY(bool CanPlay READ canPlay)
  Q_PROPERTY(bool CanPause READ canPause)
  Q_PROPERTY(bool CanSeek READ canSeek)
  Q_PROPERTY(bool CanControl READ yes CONSTANT)
public:
  explicit MprisPlayer(Backend *backend);
  QString playbackStatus() const;
  QVariantMap metadata() const;
  double volume() const { return owner->volume() / 100.0; }
  void setVolume(double value) { owner->setVolume(value * 100); }
  qlonglong position() const { return qRound64(owner->position() * 1000000); }
  double rate() const { return owner->rate(); }
  void setRate(double value) {
    if (value >= .5 && value <= 2)
      owner->setSpeed(value);
  }
  double minimumRate() const { return .5; }
  double maximumRate() const { return 2; }
  bool canPlay() const {
    return !owner->metadata().value("id").toString().isEmpty();
  }
  bool canPause() const { return owner->hasMedia(); }
  bool canSeek() const { return owner->hasMedia() && owner->duration() > 0; }
  bool canNext() const { return owner->metadata().value("canNext").toBool(); }
  bool canPrevious() const {
    return owner->metadata().value("canPrevious").toBool();
  }
  bool yes() const { return true; }
public slots:
  void Next() {
    if (canNext())
      emit owner->desktopAction("next");
  }
  void Previous() {
    if (canPrevious())
      emit owner->desktopAction("previous");
  }
  void Pause() {
    if (canPause())
      owner->setPaused(true);
  }
  void PlayPause() {
    if (owner->hasMedia())
      owner->toggle();
    else
      Play();
  }
  void Stop() { emit owner->desktopAction("stop"); }
  void Play() {
    if (owner->hasMedia())
      owner->setPaused(false);
    else if (canPlay())
      emit owner->desktopAction("play");
  }
  void Seek(qlonglong offset);
  void SetPosition(const QDBusObjectPath &trackId, qlonglong value);
  void OpenUri(const QString &) {} // No advertised URI schemes in this preview.
signals:
  void Seeked(qlonglong position);

private:
  void changed();
  Backend *owner;
  QVariantMap lastProperties;
};
