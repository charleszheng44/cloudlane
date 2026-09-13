#include "session.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  app.setOrganizationName("YunjianTests");
  app.setApplicationName("probe");
  QTemporaryDir state;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, state.path());
  Session session;
  session.initialize();
  QJsonArray results;
  QObject::connect(
      &session, &Session::finished, &app,
      [&](int id, QJsonObject data, QString error) {
        QJsonObject result{
            {"id", id}, {"code", data.value("code")}, {"error", error}};
        if (id == 1)
          result["songs"] =
              data.value("result").toObject().value("songs").toArray().size();
        if (id == 2)
          result["hasQrKey"] = !data.value("unikey").toString().isEmpty() ||
                               !data.value("data")
                                    .toObject()
                                    .value("unikey")
                                    .toString()
                                    .isEmpty();
        if (id == 3) {
          auto a = data.value("data").toArray();
          result["tracks"] = a.size();
          result["hasStream"] =
              !a.isEmpty() &&
              !a.first().toObject().value("url").toString().isEmpty();
        }
        results.append(result);
        printf(
            "%s\n",
            QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
        fflush(stdout);
        if (results.size() == 3)
          app.quit();
      });
  session.submit(1, "/api/cloudsearch/pc",
                 {{"s", "海阔天空"}, {"type", 1}, {"limit", 3}, {"offset", 0}},
                 "eapi");
  session.submit(2, "/api/login/qrcode/unikey", {{"type", 3}}, "eapi");
  session.submit(
      3, "/api/song/enhance/player/url/v1",
      {{"ids", "[347230]"}, {"level", "standard"}, {"encodeType", "flac"}},
      "xeapi");
  QTimer::singleShot(50000, &app, [&] { app.exit(2); });
  return app.exec();
}
