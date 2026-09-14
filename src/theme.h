#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

// Reads Omarchy's public palette/config files without modifying them.
class Theme final : public QObject {
  Q_OBJECT
public:
  explicit Theme(QString stateHome = {}, QString configHome = {},
                 QObject *parent = nullptr);
  QVariantMap colors() const { return palette; }
  int fontSize() const { return size; }
signals:
  void changed();
private:
  void reload();
  QString stateRoot, configRoot;
  QVariantMap palette;
  int size = 12;
  QFileSystemWatcher watcher;
  QTimer debounce;
};
