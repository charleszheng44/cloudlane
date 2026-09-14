#include "theme.h"
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <cmath>

namespace {
QString read(const QString &path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}
QString mix(const QColor &a, const QColor &b, double amount) {
  return QColor::fromRgbF(a.redF() * (1 - amount) + b.redF() * amount,
                         a.greenF() * (1 - amount) + b.greenF() * amount,
                         a.blueF() * (1 - amount) + b.blueF() * amount).name();
}
double luminance(const QColor &c) {
  auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
  return .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
}
}

Theme::Theme(QString stateHome, QString configHome, QObject *parent)
    : QObject(parent),
      stateRoot(stateHome.isEmpty() ? qEnvironmentVariable("XDG_STATE_HOME", QDir::homePath() + "/.local/state") : stateHome),
      configRoot(configHome.isEmpty() ? qEnvironmentVariable("XDG_CONFIG_HOME", QDir::homePath() + "/.config") : configHome) {
  debounce.setSingleShot(true);
  debounce.setInterval(100);
  connect(&watcher, &QFileSystemWatcher::fileChanged, &debounce, qOverload<>(&QTimer::start));
  connect(&watcher, &QFileSystemWatcher::directoryChanged, &debounce, qOverload<>(&QTimer::start));
  connect(&debounce, &QTimer::timeout, this, &Theme::reload);
  reload();
}

void Theme::reload() {
  const auto current = stateRoot + "/omarchy/current/theme";
  const auto legacy = configRoot + "/omarchy/current/theme";
  const auto dir = QFileInfo::exists(current + "/colors.toml") ? current : legacy;
  QVariantMap source;
  const QRegularExpression colorLine(
      R"re(^\s*([a-z_]+)\s*=\s*["'](#[0-9a-fA-F]{6})["'])re",
      QRegularExpression::MultilineOption);
  auto matches = colorLine.globalMatch(read(dir + "/colors.toml"));
  while (matches.hasNext()) {
    const auto match = matches.next();
    source[match.captured(1)] = match.captured(2).toLower();
  }
  const QColor bg(source.value("background", "#1a1b26").toString());
  const bool light = luminance(bg) > .45;
  const QColor fg(source.value("foreground", light ? "#202020" : "#a9b1d6").toString());
  const QColor accent(source.value("accent", source.value("blue", light ? "#315cb7" : "#7aa2f7")).toString());
  QVariantMap next{
      {"background", bg.name()}, {"foreground", fg.name()}, {"accent", accent.name()},
      {"dark_background", mix(bg, light ? QColor(Qt::white) : QColor(Qt::black), .22)},
      {"light_foreground", fg.name()}, {"selection", mix(bg, fg, .12)},
      {"muted", mix(bg, fg, .46)}, {"red", light ? "#a72537" : "#f7768e"}};
  for (auto it = source.cbegin(); it != source.cend(); ++it)
    next[it.key()] = it.value();
  next["hover"] = mix(bg, fg, .08);
  next["border"] = mix(bg, fg, .42);
  next["accent_foreground"] = luminance(accent) > .179 ? "#000000" : "#ffffff";

  // User [font] values override the theme. Start from the shell default on
  // every reload so switching to a theme without an override resets it.
  int nextSize = 12;
  for (const auto &path : {dir + "/shell.toml", configRoot + "/omarchy/shell.toml"}) {
    QString section;
    for (auto line : read(path).split('\n')) {
      line = line.section('#', 0, 0).trimmed();
      if (line.startsWith('[')) section = line;
      if (section != "[font]") continue;
      const auto match = QRegularExpression(R"(^base-size\s*=\s*(\d+)\s*$)").match(line);
      if (match.hasMatch()) nextSize = qBound(10, match.captured(1).toInt(), 32);
    }
  }

  // Atomic file writes and symlink swaps invalidate inode watches. Re-arm
  // both candidate paths plus existing ancestors (including absent configs).
  if (!watcher.files().isEmpty()) watcher.removePaths(watcher.files());
  if (!watcher.directories().isEmpty()) watcher.removePaths(watcher.directories());
  QSet<QString> paths;
  for (auto path : {current + "/colors.toml", current + "/shell.toml",
                    legacy + "/colors.toml", legacy + "/shell.toml",
                    configRoot + "/omarchy/shell.toml"}) {
    while (path != "/" && !path.isEmpty()) {
      if (QFileInfo::exists(path)) paths.insert(path);
      if (path == stateRoot || path == configRoot) break;
      path = QFileInfo(path).absolutePath();
    }
  }
  watcher.addPaths(paths.values());
  if (next != palette || size != nextSize) {
    palette = next;
    size = nextSize;
    emit changed();
  }
}
