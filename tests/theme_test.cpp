#include "theme.h"
#include <QColor>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cstdio>

class ThemeTest : public QObject {
  Q_OBJECT
  static void write(const QString &path, const QByteArray &content) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QSaveFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.commit());
  }
private slots:
  void atomicPaletteAndFontChanges() {
    QTemporaryDir temp;
    const auto state = temp.path() + "/state", config = temp.path() + "/config";
    QDir().mkpath(state); QDir().mkpath(config);
    Theme theme(state, config);
    QCOMPARE(theme.fontSize(), 12);
    const auto dir = state + "/omarchy/current/theme";
    write(dir + "/colors.toml", "background = '#ffffff'\nforeground = '#222222'\naccent = '#315cb7'\n");
    QTRY_COMPARE(theme.colors()["background"].toString(), "#ffffff");
    QVERIFY(QColor(theme.colors()["selection"].toString()).lightness() > 200);
    QCOMPARE(theme.colors()["accent_foreground"].toString(), "#ffffff");
    write(dir + "/shell.toml", "[font]\nbase-size = 18\n");
    QTRY_COMPARE(theme.fontSize(), 18);
    write(config + "/omarchy/shell.toml", "[font]\nbase-size = 16\n[bar]\nbase-size = 28\n");
    QTRY_COMPARE(theme.fontSize(), 16);
    write(dir + "/colors.toml", "background = '#112233'\naccent = '#99ccff'\n");
    QTRY_COMPARE(theme.colors()["background"].toString(), "#112233");
    QCOMPARE(theme.colors()["accent_foreground"].toString(), "#000000");
    write(dir + "/colors.toml", "background = '#223344'\naccent = 'invalid'\n");
    QTRY_COMPARE(theme.colors()["background"].toString(), "#223344");
    QVERIFY(QColor(theme.colors()["accent"].toString()).isValid());
    QVERIFY(QFile::remove(config + "/omarchy/shell.toml"));
    QTRY_COMPARE(theme.fontSize(), 18);
    QVERIFY(QFile::remove(dir + "/shell.toml"));
    QTRY_COMPARE(theme.fontSize(), 12);
  }
  void followsSwappedThemeSymlink() {
    QTemporaryDir temp;
    const auto state = temp.path() + "/state", config = temp.path() + "/config";
    const auto dark = temp.path() + "/dark", light = temp.path() + "/light";
    write(dark + "/colors.toml", "background = '#102030'\n");
    write(light + "/colors.toml", "background = '#fafafa'\nforeground = '#252525'\n");
    QDir().mkpath(state + "/omarchy/current"); QDir().mkpath(config);
    const auto link = state + "/omarchy/current/theme";
    QVERIFY(QFile::link(dark, link));
    Theme theme(state, config);
    QCOMPARE(theme.colors()["background"].toString(), "#102030");
    QVERIFY(QFile::link(light, link + ".next"));
    QCOMPARE(std::rename(qPrintable(link + ".next"), qPrintable(link)), 0);
    QTRY_COMPARE(theme.colors()["background"].toString(), "#fafafa");
    write(light + "/colors.toml", "background = '#eeeeee'\n");
    QTRY_COMPARE(theme.colors()["background"].toString(), "#eeeeee");
  }
  void supportsLegacyOmarchyPath() {
    QTemporaryDir temp;
    const auto config = temp.path() + "/config";
    write(config + "/omarchy/current/theme/colors.toml", "background = '#123456'\n");
    Theme theme(temp.path() + "/state", config);
    QCOMPARE(theme.colors()["background"].toString(), "#123456");
  }
};
QTEST_GUILESS_MAIN(ThemeTest)
#include "theme_test.moc"
