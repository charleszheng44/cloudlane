#include "backend.h"
#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QImage>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent>
#include <qrencode.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
Backend::Backend(QObject *p) : QObject(p), session(new Session) {
  session->moveToThread(&networkThread);
  connect(&networkThread, &QThread::started, session, &Session::initialize);
  connect(&networkThread, &QThread::finished, session, &QObject::deleteLater);
  connect(session, &Session::finished, this,
          [this](int id, QJsonObject data, QString error) {
            emit response(id, data.toVariantMap(), error);
          });
  connect(session, &Session::persistenceStatus, this, &Backend::message);
  networkThread.start();
  readTheme();
  connect(&watcher, &QFileSystemWatcher::fileChanged, this,
          [this] { QTimer::singleShot(150, this, &Backend::readTheme); });
  connect(&watcher, &QFileSystemWatcher::directoryChanged, this,
          [this] { QTimer::singleShot(150, this, &Backend::readTheme); });
  mpv = mpv_create();
  if (mpv) {
    mpv_set_option_string(mpv, "config", "no");
    mpv_set_option_string(mpv, "load-scripts", "no");
    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "vo",
                          qEnvironmentVariable("QT_QUICK_BACKEND") == "software"
                              ? "null"
                              : "libmpv");
    mpv_set_option_string(mpv, "hwdec", "auto-safe");
    mpv_set_option_string(mpv, "audio-display", "no");
    mpv_set_option_string(mpv, "volume", "65");
    if (mpv_initialize(mpv) < 0) {
      mpv_terminate_destroy(mpv);
      mpv = nullptr;
    } else {
      for (const auto &name : {"time-pos", "duration", "volume"})
        mpv_observe_property(mpv, 0, name, MPV_FORMAT_DOUBLE);
      mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
      mpv_set_wakeup_callback(
          mpv,
          [](void *ctx) {
            QMetaObject::invokeMethod(
                static_cast<Backend *>(ctx),
                [ctx] { static_cast<Backend *>(ctx)->drainPlayer(); },
                Qt::QueuedConnection);
          },
          this);
    }
  }
}
Backend::~Backend() {
  if (mpv) {
    mpv_set_wakeup_callback(mpv, nullptr, nullptr);
    mpv_terminate_destroy(mpv);
    mpv = nullptr;
  }
  networkThread.quit();
  networkThread.wait();
}
int Backend::request(QString path, QVariantMap args, QString mode) {
  int id = ++nextId;
  QMetaObject::invokeMethod(
      session,
      [this, id, path, args, mode] {
        session->submit(id, path, QJsonObject::fromVariantMap(args), mode);
      },
      Qt::QueuedConnection);
  return id;
}
void Backend::saveAccount(QString id) {
  QMetaObject::invokeMethod(
      session, [this, id] { session->save(id); }, Qt::QueuedConnection);
}
void Backend::logout() {
  stop();
  QMetaObject::invokeMethod(
      session, [this] { session->reset(); }, Qt::QueuedConnection);
}
QString Backend::qrImage(QString text) {
  auto qr = QRcode_encodeString(text.toUtf8().constData(), 0, QR_ECLEVEL_M,
                                QR_MODE_8, 1);
  if (!qr)
    return {};
  const int border = 4, scale = 6;
  QImage image((qr->width + border * 2) * scale,
               (qr->width + border * 2) * scale, QImage::Format_RGB32);
  image.fill(Qt::white);
  for (int y = 0; y < qr->width; ++y)
    for (int x = 0; x < qr->width; ++x)
      if (qr->data[y * qr->width + x] & 1)
        for (int dy = 0; dy < scale; ++dy)
          for (int dx = 0; dx < scale; ++dx)
            image.setPixelColor((x + border) * scale + dx,
                                (y + border) * scale + dy, Qt::black);
  QRcode_free(qr);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return "data:image/png;base64," + QString::fromLatin1(bytes.toBase64());
}
void Backend::load(QString url, double previewEnd, double previewStart,
                   bool startPaused) {
  if (!mpv) {
    emit message("系统播放器初始化失败");
    return;
  }
  QUrl uri(url);
  if (uri.scheme() != "https" && uri.scheme() != "http" &&
      uri.scheme() != "file") {
    emit message("不支持的媒体地址");
    return;
  }
  auto path = uri.isLocalFile() ? uri.toLocalFile().toUtf8() : uri.toEncoded();
  auto options = QByteArray("start=") +
                 QByteArray::number(qMax(0.0, previewStart)) +
                 (previewEnd > 0 ? ",end=" + QByteArray::number(previewEnd)
                                 : QByteArray());
  const char *args[] = {"loadfile", path.constData(),    "replace",
                        "-1",       options.constData(), nullptr};
  loaded = false;
  time = length = 0;
  mpv_command_async(mpv, 0, args);
  int pause = startPaused ? 1 : 0;
  mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &pause);
  emit playerChanged();
}
void Backend::toggle() {
  if (mpv && loaded) {
    int value = paused ? 0 : 1;
    mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &value);
  }
}
void Backend::stop() {
  if (mpv) {
    const char *args[] = {"stop", nullptr};
    mpv_command_async(mpv, 0, args);
  }
  paused = true;
  loaded = false;
  time = length = 0;
  emit playerChanged();
}
void Backend::seek(double seconds) {
  if (mpv && loaded && seconds >= 0 && seconds <= length) {
    auto value = QByteArray::number(seconds);
    const char *args[] = {"seek", value.constData(), "absolute+exact", nullptr};
    mpv_command_async(mpv, 0, args);
    emit seeked(qRound64(seconds * 1000000));
  }
}
void Backend::setVolume(double value) {
  gain = qBound(0.0, value, 100.0);
  if (mpv)
    mpv_set_property_async(mpv, 0, "volume", MPV_FORMAT_DOUBLE, &gain);
  emit playerChanged();
}
void Backend::setSpeed(double value) {
  value = qBound(.5, value, 2.0);
  playbackRate = value;
  if (mpv)
    mpv_set_property_async(mpv, 0, "speed", MPV_FORMAT_DOUBLE, &value);
}
void Backend::drainPlayer() {
  if (!mpv)
    return;
  while (auto event = mpv_wait_event(mpv, 0)) {
    if (event->event_id == MPV_EVENT_NONE)
      break;
    if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
      auto property = static_cast<mpv_event_property *>(event->data);
      if (!property->data)
        continue;
      auto name = QByteArray(property->name);
      if (name == "pause")
        paused = *static_cast<int *>(property->data);
      else if (name == "time-pos")
        time = *static_cast<double *>(property->data);
      else if (name == "duration")
        length = *static_cast<double *>(property->data);
      else if (name == "volume")
        gain = *static_cast<double *>(property->data);
      emit playerChanged();
    } else if (event->event_id == MPV_EVENT_FILE_LOADED) {
      loaded = true;
      emit mediaLoaded();
      emit playerChanged();
    } else if (event->event_id == MPV_EVENT_IDLE) {
      loaded = false;
      emit playerChanged();
    } else if (event->event_id == MPV_EVENT_END_FILE) {
      auto end = static_cast<mpv_event_end_file *>(event->data);
      if (end->reason == MPV_END_FILE_REASON_EOF)
        emit playbackEnded();
      else if (end->reason == MPV_END_FILE_REASON_ERROR) {
        paused = true;
        emit playerChanged();
        emit message(QString("播放失败：") + mpv_error_string(end->error));
      }
    }
  }
}
QVariantList Backend::localFiles(QVariantList urls) {
  QVariantList files;
  for (const auto &value : urls) {
    QUrl uri(value.toString());
    auto path = uri.toLocalFile();
    if (path.isEmpty() || !QFileInfo(path).isFile())
      continue;
    TagLib::FileRef ref(path.toUtf8().constData());
    QVariantMap item{{"id", uri.toString()},
                     {"url", uri.toString()},
                     {"name", QFileInfo(path).completeBaseName()},
                     {"artist", "本地音乐"},
                     {"album", ""},
                     {"duration", 0},
                     {"kind", "local"}};
    if (!ref.isNull()) {
      if (auto tag = ref.tag()) {
        auto title = QString::fromStdString(tag->title().to8Bit(true));
        if (!title.isEmpty())
          item["name"] = title;
        item["artist"] = QString::fromStdString(tag->artist().to8Bit(true));
        item["album"] = QString::fromStdString(tag->album().to8Bit(true));
      }
      if (ref.audioProperties())
        item["duration"] = ref.audioProperties()->lengthInMilliseconds();
    }
    files.append(item);
  }
  return files;
}
QString Backend::state(QString key) const {
  return QSettings().value("ui/" + key).toString();
}
void Backend::setState(QString key, QString value) {
  if (value.size() < 1024 * 1024)
    QSettings().setValue("ui/" + key, value);
}
void Backend::copyText(QString text) {
  QGuiApplication::clipboard()->setText(text);
}
void Backend::readTheme() {
  const auto state = qEnvironmentVariable("XDG_STATE_HOME",
                                          QDir::homePath() + "/.local/state");
  const auto config =
      qEnvironmentVariable("XDG_CONFIG_HOME", QDir::homePath() + "/.config");
  const auto dir = state + "/omarchy/current/theme";
  QVariantMap next{{"background", "#1a1b26"}, {"dark_background", "#13141c"},
                   {"foreground", "#a9b1d6"}, {"light_foreground", "#b4bee6"},
                   {"accent", "#7aa2f7"},     {"selection", "#292e42"},
                   {"muted", "#414868"},      {"red", "#f7768e"}};
  QFile palette(dir + "/colors.toml");
  if (palette.open(QIODevice::ReadOnly)) {
    QRegularExpression re("^([a-z_]+)\\s*=\\s*\"(#[0-9a-fA-F]{6})\"",
                          QRegularExpression::MultilineOption);
    auto matches = re.globalMatch(QString::fromUtf8(palette.readAll()));
    while (matches.hasNext()) {
      auto m = matches.next();
      next[m.captured(1)] = m.captured(2);
    }
  }
  for (const auto &path :
       {dir + "/shell.toml", config + "/omarchy/shell.toml"}) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
      QRegularExpression re("base-size\\s*=\\s*(\\d+)");
      auto m = re.match(QString::fromUtf8(f.readAll()));
      if (m.hasMatch())
        baseSize = qBound(10, m.captured(1).toInt(), 32);
    }
    if (QFileInfo::exists(path) && !watcher.files().contains(path))
      watcher.addPath(path);
  }
  for (const auto &path :
       {dir, state + "/omarchy/current", config + "/omarchy"})
    if (QFileInfo::exists(path) && !watcher.directories().contains(path))
      watcher.addPath(path);
  if (QFileInfo::exists(palette.fileName()) &&
      !watcher.files().contains(palette.fileName()))
    watcher.addPath(palette.fileName());
  colors = next;
  emit themeChanged();
}

void Backend::setPaused(bool value) {
  if (!mpv)
    return;
  int flag = value ? 1 : 0;
  mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &flag);
}
void Backend::setMetadata(QVariantMap value) {
  trackMetadata = value;
  emit metadataChanged();
}
QString Backend::newId() const {
  return QUuid::createUuid().toString(QUuid::Id128);
}
void Backend::scanLocal(QVariantList urls) {
  auto watcher = new QFutureWatcher<QVariantList>(this);
  connect(watcher, &QFutureWatcher<QVariantList>::finished, this,
          [this, watcher] {
            emit localReady(watcher->result());
            watcher->deleteLater();
          });
  watcher->setFuture(
      QtConcurrent::run([urls] { return Backend::localFiles(urls); }));
}
