#include "mpris.h"
#include <QDBusConnection>
#include <QDBusMessage>
MprisPlayer::MprisPlayer(Backend *backend)
    : QDBusAbstractAdaptor(backend), owner(backend) {
  connect(owner, &Backend::playerChanged, this, &MprisPlayer::changed);
  connect(owner, &Backend::metadataChanged, this, &MprisPlayer::changed);
  connect(owner, &Backend::seeked, this, &MprisPlayer::Seeked);
}
QString MprisPlayer::playbackStatus() const {
  return !owner->hasMedia() ? "Stopped"
         : owner->playing() ? "Playing"
                            : "Paused";
}
QVariantMap MprisPlayer::metadata() const {
  const auto m = owner->metadata();
  auto entry = m.value("entryId").toString();
  if (entry.isEmpty())
    return {};
  return {{"mpris:trackid",
           QVariant::fromValue(QDBusObjectPath(
               "/io/github/charleszheng44/Yunjian/track/" + entry))},
          {"mpris:length", qlonglong(m.value("duration").toLongLong() * 1000)},
          {"xesam:title", m.value("name")},
          {"xesam:artist", QStringList{m.value("artist").toString()}},
          {"xesam:album", m.value("album")},
          {"mpris:artUrl", m.value("cover")}};
}
void MprisPlayer::Seek(qlonglong offset) {
  if (!canSeek())
    return;
  const auto value = owner->position() + offset / 1000000.0;
  if (value > owner->duration())
    Next();
  else
    owner->seek(qMax(0.0, value));
}
void MprisPlayer::SetPosition(const QDBusObjectPath &id, qlonglong value) {
  const auto current =
      metadata().value("mpris:trackid").value<QDBusObjectPath>();
  if (canSeek() && id.path() == current.path() && value >= 0 &&
      value <= qRound64(owner->duration() * 1000000))
    owner->seek(value / 1000000.0);
}
void MprisPlayer::changed() {
  QVariantMap current{{"PlaybackStatus", playbackStatus()},
                      {"Metadata", metadata()},
                      {"Volume", volume()},
                      {"Rate", rate()},
                      {"CanGoNext", canNext()},
                      {"CanGoPrevious", canPrevious()},
                      {"CanPlay", canPlay()},
                      {"CanPause", canPause()},
                      {"CanSeek", canSeek()}};
  QVariantMap delta;
  for (auto it = current.cbegin(); it != current.cend(); ++it)
    if (lastProperties.value(it.key()) != it.value())
      delta.insert(it.key(), it.value());
  lastProperties = current;
  if (delta.isEmpty())
    return;
  auto event = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged");
  event << QString("org.mpris.MediaPlayer2.Player") << delta << QStringList{};
  QDBusConnection::sessionBus().send(event);
}
